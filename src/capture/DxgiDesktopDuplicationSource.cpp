// DxgiDesktopDuplicationSource.cpp
// Desktop capture implementation using DXGI Desktop Duplication (IDXGIOutputDuplication).
//
// High-level flow:
//   - Create a D3D11 device + immediate context
//   - Obtain IDXGIOutput1 for monitor 0 and call DuplicateOutput()
//   - For each grab():
//       AcquireNextFrame(0ms timeout) -> ID3D11Texture2D (GPU)
//       CopyResource -> staging texture (CPU-readable)
//       Map staging -> copy into cv::Mat (BGRA)
//       Convert BGRA -> BGR and return as cv::Mat
//
// Robustness:
//   - If AcquireNextFrame fails (device reset / display mode changes), re-init.

#include "aimbot/capture/DxgiDesktopDuplicationSource.h"

#include <stdexcept>
#include <cstring> // memcpy
#include <functional>

using Microsoft::WRL::ComPtr;

namespace {
// Simple RAII helper: run a function when leaving scope.
// Used to guarantee ReleaseFrame() / Unmap() even when returning early.
struct ScopeExit {
    explicit ScopeExit(std::function<void()> f) : f_(std::move(f)) {}
    ~ScopeExit() { if (f_) f_(); }
    std::function<void()> f_;
};
} // namespace

DxgiDesktopDuplicationSource::DxgiDesktopDuplicationSource() {
    init();
}

DxgiDesktopDuplicationSource::~DxgiDesktopDuplicationSource() {
    shutdown();
}

void DxgiDesktopDuplicationSource::init() {
    // Always start from a clean state.
    shutdown();

    // 1) Create D3D11 device/context.
    //    - Prefer hardware device.
    //    - In debug builds, enable the debug layer (if installed).
    D3D_FEATURE_LEVEL fl{};
    UINT flags = 0;
#ifdef _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    HRESULT hr = D3D11CreateDevice(
        nullptr,                    // default adapter
        D3D_DRIVER_TYPE_HARDWARE,   // hardware device
        nullptr,                    // no software rasterizer module
        flags,                      // creation flags
        nullptr, 0,                 // default feature levels
        D3D11_SDK_VERSION,
        device_.GetAddressOf(),
        &fl,
        context_.GetAddressOf()
    );

    // If hardware device creation fails, fall back to WARP (software rasterizer).
    // This can help on some VMs / remote sessions / restricted environments.
    if (FAILED(hr)) {
        hr = D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_WARP,
            nullptr,
            flags,
            nullptr, 0,
            D3D11_SDK_VERSION,
            device_.GetAddressOf(),
            &fl,
            context_.GetAddressOf()
        );
    }

    // If device/context creation still failed, keep initialized_ = false.
    if (FAILED(hr) || !device_ || !context_) {
        return;
    }

    // 2) Get IDXGIOutput1 (monitor 0) and create the duplication object.
    ComPtr<IDXGIDevice> dxgiDevice;
    hr = device_.As(&dxgiDevice); // ID3D11Device -> IDXGIDevice
    if (FAILED(hr) || !dxgiDevice) return;

    ComPtr<IDXGIAdapter> adapter;
    hr = dxgiDevice->GetAdapter(adapter.GetAddressOf()); // get GPU adapter
    if (FAILED(hr) || !adapter) return;

    ComPtr<IDXGIOutput> output;
    hr = adapter->EnumOutputs(0, output.GetAddressOf()); // get output #0 (primary monitor)
    if (FAILED(hr) || !output) return;

    ComPtr<IDXGIOutput1> output1;
    hr = output.As(&output1); // IDXGIOutpu -> IDXGIOutput1
    if (FAILED(hr) || !output1) return;

    // Create the desktop duplication interface.
    hr = output1->DuplicateOutput(device_.Get(), dup_.GetAddressOf());
    if (FAILED(hr) || !dup_) return;

    initialized_ = true;
}

void DxgiDesktopDuplicationSource::shutdown() {
    // Release resources (order not critical; ComPtr handles Release()).
    staging_.Reset();
    dup_.Reset();
    context_.Reset();
    device_.Reset();

    // Release OpenCV buffers.
    bgra_.release();
    bgr_.release();
    width_ = height_ = 0;

    initialized_ = false;
}

bool DxgiDesktopDuplicationSource::grab(cv::Mat& outBgr) {
    // Clear output by default.
    outBgr.release();

    // Must be initialized and have valid duplication + D3D objects.
    if (!initialized_ || !dup_ || !device_ || !context_) {
        return false;
    }

    DXGI_OUTDUPL_FRAME_INFO frameInfo{};
    ComPtr<IDXGIResource> desktopResource;

    // Acquire next frame with 0ms timeout:
    // - If no new frame is available, return quickly (non-blocking main loop).
    HRESULT hr = dup_->AcquireNextFrame(0, &frameInfo, desktopResource.GetAddressOf());
    if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
        return false;
    }
    if (FAILED(hr) || !desktopResource) {
        // Duplication can become invalid (display mode switch, device reset, etc.).
        // Attempt to recreate duplication objects and try again next loop.
        init();
        return false;
    }

    // Ensure ReleaseFrame() is always called.
    ScopeExit releaseFrame([&]() {
        dup_->ReleaseFrame();
    });

    // Convert acquired resource to ID3D11Texture2D.
    ComPtr<ID3D11Texture2D> desktopTexture;
    hr = desktopResource.As(&desktopTexture);
    if (FAILED(hr) || !desktopTexture) {
        return false;
    }

    // Read texture description to allocate staging + cv::Mat if needed.
    D3D11_TEXTURE2D_DESC desc{};
    desktopTexture->GetDesc(&desc);

    if (desc.Width == 0 || desc.Height == 0) {
        return false;
    }

    // If resolution changed or we haven't created staging yet, recreate everything.
    if (width_ != desc.Width || height_ != desc.Height || staging_ == nullptr) {
        width_  = desc.Width;
        height_ = desc.Height;

        // Allocate OpenCV buffers:
        // - bgra_: raw mapped texture data (BGRA)
        // - bgr_:  converted output (BGR)
        bgra_ = cv::Mat(static_cast<int>(height_), static_cast<int>(width_), CV_8UC4);
        bgr_  = cv::Mat(static_cast<int>(height_), static_cast<int>(width_), CV_8UC3);

        // Create CPU-readable staging texture.
        D3D11_TEXTURE2D_DESC stagingDesc = desc;
        stagingDesc.Usage = D3D11_USAGE_STAGING;
        stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        stagingDesc.BindFlags = 0;
        stagingDesc.MiscFlags = 0;

        staging_.Reset();
        hr = device_->CreateTexture2D(&stagingDesc, nullptr, staging_.GetAddressOf());
        if (FAILED(hr) || !staging_) {
            return false;
        }
    }

    // Copy GPU texture to CPU-readable staging texture, then map it for reading.
    context_->CopyResource(staging_.Get(), desktopTexture.Get());

    D3D11_MAPPED_SUBRESOURCE mapped{};
    hr = context_->Map(staging_.Get(), 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(hr) || !mapped.pData) {
        return false;
    }

    // Ensure Unmap() is always called.
    ScopeExit unmap([&]() {
        context_->Unmap(staging_.Get(), 0);
    });

    // Copy row by row because mapped.RowPitch can be larger than width * bytesPerPixel.
    const int rowBytes = static_cast<int>(width_ * 4);  // BGRA => 4 bytes/pixel
    for (UINT y = 0; y < height_; ++y) {
        std::memcpy(
            bgra_.ptr(static_cast<int>(y)),
            static_cast<const unsigned char*>(mapped.pData) + y * mapped.RowPitch,
            rowBytes
        );
    }

    // Convert BGRA -> BGR for the rest of the OpenCV pipeline.
    cv::cvtColor(bgra_, bgr_, cv::COLOR_BGRA2BGR);

    // Return the BGR buffer (shallow copy; shares memory with bgr_ inside this object).
    // Caller should use it immediately or clone() if it needs persistent ownership.
    outBgr = bgr_;
    return true;
}
