#include "aimbot/capture/DxgiDesktopDuplicationSource.h"

#include <stdexcept>
#include <cstring> // memcpy
#include <functional>

using Microsoft::WRL::ComPtr;

namespace {
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
    shutdown();

    // 1) Create D3D11 device/context
    D3D_FEATURE_LEVEL fl{};
    UINT flags = 0;
#ifdef _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    HRESULT hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        flags,
        nullptr, 0,
        D3D11_SDK_VERSION,
        device_.GetAddressOf(),
        &fl,
        context_.GetAddressOf()
    );

    // 硬件失败就尝试 WARP（某些虚拟机/远程环境）
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

    if (FAILED(hr) || !device_ || !context_) {
        return; // initialized_ 仍为 false
    }

    // 2) Get IDXGIOutput1 (monitor 0) and DuplicateOutput
    ComPtr<IDXGIDevice> dxgiDevice;
    hr = device_.As(&dxgiDevice); // ID3D11Device -> IDXGIDevice
    if (FAILED(hr) || !dxgiDevice) return;

    ComPtr<IDXGIAdapter> adapter;
    hr = dxgiDevice->GetAdapter(adapter.GetAddressOf()); // 找到显卡适配器
    if (FAILED(hr) || !adapter) return;

    ComPtr<IDXGIOutput> output;
    hr = adapter->EnumOutputs(0, output.GetAddressOf()); // 找到显示输出（第0块屏幕）
    if (FAILED(hr) || !output) return;

    ComPtr<IDXGIOutput1> output1;
    hr = output.As(&output1); // IDXGIOutpu -> IDXGIOutput1
    if (FAILED(hr) || !output1) return;

    hr = output1->DuplicateOutput(device_.Get(), dup_.GetAddressOf()); // 创建桌面复制对象
    if (FAILED(hr) || !dup_) return;

    initialized_ = true;
}

void DxgiDesktopDuplicationSource::shutdown() {
    // 顺序不重要，ComPtr 会自动 Release
    staging_.Reset();
    dup_.Reset();
    context_.Reset();
    device_.Reset();

    bgra_.release();
    bgr_.release();
    width_ = height_ = 0;

    initialized_ = false;
}

bool DxgiDesktopDuplicationSource::grab(cv::Mat& outBgr) {
    outBgr.release();

    if (!initialized_ || !dup_ || !device_ || !context_) {
        return false;
    }

    DXGI_OUTDUPL_FRAME_INFO frameInfo{};
    ComPtr<IDXGIResource> desktopResource;

    // 0ms timeout：没新帧就快速返回 false（主循环可继续/节流/非阻塞）
    HRESULT hr = dup_->AcquireNextFrame(0, &frameInfo, desktopResource.GetAddressOf());
    if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
        return false;
    }
    if (FAILED(hr) || !desktopResource) {
        // duplication 可能失效（比如显示模式切换/设备重置），尝试重建
        init();
        return false;
    }

    // 确保 ReleaseFrame 一定被调用
    ScopeExit releaseFrame([&]() {
        dup_->ReleaseFrame();
    });

    ComPtr<ID3D11Texture2D> desktopTexture;
    hr = desktopResource.As(&desktopTexture);
    if (FAILED(hr) || !desktopTexture) {
        return false;
    }

    // 检测尺寸变化并且准备staging texture + Mat
    D3D11_TEXTURE2D_DESC desc{};
    desktopTexture->GetDesc(&desc);

    // 尺寸变化：重建 Mat 和 staging texture
    if (desc.Width == 0 || desc.Height == 0) {
        return false;
    }

    if (width_ != desc.Width || height_ != desc.Height || staging_ == nullptr) {
        width_  = desc.Width;
        height_ = desc.Height;

        bgra_ = cv::Mat(static_cast<int>(height_), static_cast<int>(width_), CV_8UC4);
        bgr_  = cv::Mat(static_cast<int>(height_), static_cast<int>(width_), CV_8UC3);

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

    // 拷贝到 staging 并 Map 读取
    context_->CopyResource(staging_.Get(), desktopTexture.Get());

    D3D11_MAPPED_SUBRESOURCE mapped{};
    hr = context_->Map(staging_.Get(), 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(hr) || !mapped.pData) {
        return false;
    }

    // 确保 Unmap
    ScopeExit unmap([&]() {
        context_->Unmap(staging_.Get(), 0);
    });

    const int rowBytes = static_cast<int>(width_ * 4);
    for (UINT y = 0; y < height_; ++y) {
        std::memcpy(
            bgra_.ptr(static_cast<int>(y)),
            static_cast<const unsigned char*>(mapped.pData) + y * mapped.RowPitch,
            rowBytes
        );
    }

    cv::cvtColor(bgra_, bgr_, cv::COLOR_BGRA2BGR);
    outBgr = bgr_; // shallow copy（共享数据），足够快
    return true;
}
