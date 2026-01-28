#include <opencv2/imgproc.hpp>
#include <stdexcept>
#include <cstring> // memcpy

#include "aimbot/gui/Dx11MatTexture.h"

namespace aimbot::gui {

    bool Dx11MatTexture::update(ID3D11Device* device, ID3D11DeviceContext* ctx, const cv::Mat& mat)
    {
        if (!device || !ctx) return false;
        if (mat.empty()) return false;

        if (mat.type() != CV_8UC1 && mat.type() != CV_8UC3 && mat.type() != CV_8UC4) {
            return false; // unsupported
        }

        const int w = mat.cols;
        const int h = mat.rows;

        ensureTexture(device, w, h);

        // Convert to BGRA if needed
        if (mat.type() == CV_8UC4) {
            // assume BGRA already
            uploadBGRA(ctx, mat);
        }
        else if (mat.type() == CV_8UC3) {
            // BGR -> BGRA
            bgra_.create(h, w, CV_8UC4);
            cv::cvtColor(mat, bgra_, cv::COLOR_BGR2BGRA);
            uploadBGRA(ctx, bgra_);
        }
        else {
            // GRAY -> BGRA (mask)
            bgra_.create(h, w, CV_8UC4);
            cv::cvtColor(mat, bgra_, cv::COLOR_GRAY2BGRA);
            uploadBGRA(ctx, bgra_);
        }

        return true;
    }

    void Dx11MatTexture::ensureTexture(ID3D11Device* device, int w, int h)
    {
        if (tex_ && srv_ && w_ == w && h_ == h) return;

        w_ = w;
        h_ = h;
        tex_.Reset();
        srv_.Reset();

        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = (UINT)w;
        desc.Height = (UINT)h;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // matches BGRA
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        HRESULT hr = device->CreateTexture2D(&desc, nullptr, tex_.GetAddressOf());
        if (FAILED(hr)) {
            throw std::runtime_error("CreateTexture2D failed");
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = desc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;

        hr = device->CreateShaderResourceView(tex_.Get(), &srvDesc, srv_.GetAddressOf());
        if (FAILED(hr)) {
            throw std::runtime_error("CreateShaderResourceView failed");
        }
    }

    void Dx11MatTexture::uploadBGRA(ID3D11DeviceContext* ctx, const cv::Mat& bgra)
    {
        D3D11_MAPPED_SUBRESOURCE mapped{};
        HRESULT hr = ctx->Map(tex_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (FAILED(hr)) return;

        const int w = bgra.cols;
        const int h = bgra.rows;
        const int bytesPerRow = w * 4;

        const unsigned char* src = bgra.data;
        unsigned char* dst = reinterpret_cast<unsigned char*>(mapped.pData);

        for (int y = 0; y < h; ++y) {
            std::memcpy(dst + y * mapped.RowPitch, src + y * bgra.step, bytesPerRow);
        }

        ctx->Unmap(tex_.Get(), 0);
    }

} // namespace aimbot::gui
