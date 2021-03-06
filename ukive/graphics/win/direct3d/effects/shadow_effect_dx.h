// Copyright (c) 2016 ucclkp <ucclkp@gmail.com>.
// This file is part of ukive project.
//
// This program is licensed under GPLv3 license that can be
// found in the LICENSE file.

#ifndef UKIVE_GRAPHICS_WIN_DIRECT3D_EFFECTS_SHADOW_EFFECT_DX_H_
#define UKIVE_GRAPHICS_WIN_DIRECT3D_EFFECTS_SHADOW_EFFECT_DX_H_

#include <memory>

#include "ukive/graphics/effects/shadow_effect.h"
#include "ukive/graphics/win/directx_manager.h"
#include "ukive/graphics/3d/matrix.hpp"
#include "ukive/window/context.h"


namespace ukive {

    class Canvas;
    class OffscreenBufferWin;

    class ShadowEffectDX : public ShadowEffect {
    public:
        explicit ShadowEffectDX(Context context);
        ~ShadowEffectDX();

        bool initialize() override;
        void destroy() override;

        bool generate(Canvas* c) override;
        bool draw(Canvas* c) override;
        bool setContent(OffscreenBuffer* content) override;
        ImageFrame* getOutput() const override;

        void resetCache() override;
        bool hasCache() const override;

        bool setRadius(int radius) override;
        int getRadius() const override;

    private:
        struct ConstBuffer {
            ukv3d::Matrix4x4F wvo;
        };

        struct VertexData {
            ukv3d::Point3F position;

            VertexData() {}
            VertexData(const ukv3d::Point3F& pos)
                : position(pos) {}
        };

        bool createTexture(
            ComPtr<ID3D11Texture2D>& tex,
            ComPtr<ID3D11RenderTargetView>& rtv,
            ComPtr<ID3D11ShaderResourceView>& srv);
        bool createKernelTexture();
        bool setSize(int width, int height, bool hdr);
        void render();

        int width_;
        int height_;
        int view_width_;
        int view_height_;
        int radius_;
        float semi_radius_;
        bool is_initialized_ = false;
        bool is_hdr_enabled_ = false;

        ukv3d::Matrix4x4F wvo_matrix_;
        ukv3d::Matrix4x4F world_matrix_;
        ukv3d::Matrix4x4F view_matrix_;
        ukv3d::Matrix4x4F ortho_matrix_;

        ComPtr<ID3D11RenderTargetView> bg_rtv_;
        ComPtr<ID3D11ShaderResourceView> bg_srv_;

        ComPtr<ID3D11Texture2D> kernel_tex2d_;
        ComPtr<ID3D11ShaderResourceView> kernel_srv_;

        ComPtr<ID3D11Texture2D> shadow1_tex2d_;
        ComPtr<ID3D11RenderTargetView> shadow1_rtv_;
        ComPtr<ID3D11ShaderResourceView> shadow1_srv_;

        ComPtr<ID3D11Texture2D> shadow2_tex2d_;
        ComPtr<ID3D11RenderTargetView> shadow2_rtv_;
        ComPtr<ID3D11ShaderResourceView> shadow2_srv_;

        ComPtr<ID3D11Buffer> vert_buffer_;
        ComPtr<ID3D11Buffer> index_buffer_;
        ComPtr<ID3D11Buffer> const_buffer_;
        ComPtr<ID3D11InputLayout> input_layout_;
        ComPtr<ID3D11PixelShader> vert_ps_;
        ComPtr<ID3D11PixelShader> hori_ps_;
        ComPtr<ID3D11VertexShader> vs_;

        D3D11_VIEWPORT viewport_;
        ComPtr<ID3D11RasterizerState> rasterizer_state_;

        Context context_;
        std::shared_ptr<ImageFrame> cache_;
    };

}

#endif  // UKIVE_GRAPHICS_WIN_DIRECT3D_EFFECTS_SHADOW_EFFECT_DX_H_