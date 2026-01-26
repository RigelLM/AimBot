// capture/DxgiDesktopDuplicationSource.h
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
    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<IDXGIOutputDuplication> dup_;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> staging_;
    UINT width_= 0;
    UINT height_ = 0;

    cv::Mat bgra_;
    cv::Mat bgr_;

    bool initialized_ = false;
};
