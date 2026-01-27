// capture/DxgiDesktopDuplicationSource.h
// Frame source implementation based on DXGI Desktop Duplication.
// Captures the current desktop image using D3D11 + IDXGIOutputDuplication,
// then outputs a CPU-accessible BGR cv::Mat for downstream OpenCV processing.
#pragma once

#include "IFrameSource.h"

#include <wrl/client.h>
#include <d3d11.h>
#include <dxgi1_2.h>

class DxgiDesktopDuplicationSource : public IFrameSource {
public:
    DxgiDesktopDuplicationSource();
    ~DxgiDesktopDuplicationSource() override;

    bool grab(cv::Mat& outBgr) override;

private:
    void init();
    void shutdown();

private:
    // D3D11 device and immediate context used to copy frames.
    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    // DXGI duplication interface to acquire desktop frames.
    Microsoft::WRL::ComPtr<IDXGIOutputDuplication> dup_;

    // CPU-readable staging texture (D3D11_USAGE_STAGING) used to map frame pixels.
    Microsoft::WRL::ComPtr<ID3D11Texture2D> staging_;
    // Captured frame dimensions.
    UINT width_= 0;
    UINT height_ = 0;

    // Internal buffers:
    // bgra_: CV_8UC4 buffer for the mapped BGRA desktop image.
    // bgr_:  CV_8UC3 buffer after BGRA -> BGR conversion for OpenCV.
    cv::Mat bgra_;
    cv::Mat bgr_;

    bool initialized_ = false;
};
