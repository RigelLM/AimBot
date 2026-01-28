#pragma once

#include <wrl/client.h>
#include <d3d11.h>

#include <opencv2/core.hpp>

struct ImVec2; // forward decl (avoid including imgui.h here)

namespace aimbot::gui {

    class Dx11MatTexture {
    public:
        Dx11MatTexture() = default;

        // Update GPU texture with a cv::Mat.
        // Accepts CV_8UC3 (BGR) or CV_8UC4 (BGRA).
        // Returns true if updated successfully.
        bool update(ID3D11Device* device, ID3D11DeviceContext* ctx, const cv::Mat& mat);

        // DX11 backend expects ImTextureID = ID3D11ShaderResourceView*
        void* imTextureID() const { return srv_.Get(); }

        int width()  const { return w_; }
        int height() const { return h_; }
        bool valid() const { return srv_ != nullptr; }

    private:
        void ensureTexture(ID3D11Device* device, int w, int h);
        void uploadBGRA(ID3D11DeviceContext* ctx, const cv::Mat& bgra);

    private:
        Microsoft::WRL::ComPtr<ID3D11Texture2D> tex_;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv_;
        int w_ = 0;
        int h_ = 0;

        // temp buffer for BGR->BGRA conversion
        cv::Mat bgra_;
    };

} // namespace aimbot::gui
