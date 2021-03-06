// Copyright (c) 2016 ucclkp <ucclkp@gmail.com>.
// This file is part of ukive project.
//
// This program is licensed under GPLv3 license that can be
// found in the LICENSE file.

#ifndef SHELL_SHADOW_SHADOW_WINDOW_H_
#define SHELL_SHADOW_SHADOW_WINDOW_H_

#include "ukive/window/window.h"
#include "ukive/animation/animator.h"


namespace ukive {
    class Button;
    class Canvas;
    class ShadowEffect;
    class ImageFrame;
}

namespace shell {

    class ShadowWindow :
        public ukive::Window, public ukive::AnimationListener {
    public:
        ShadowWindow();
        ~ShadowWindow();

        void onCreated() override;
        void onPreDrawCanvas(ukive::Canvas* canvas) override;
        void onDestroy() override;
        bool onInputEvent(ukive::InputEvent* e) override;

        void onContextChanged(const ukive::Context& context) override;

        // ukive::AnimationListener
        void onAnimationProgress(ukive::Animator* animator) override;

    private:
        void createImages();

        ukive::Button* ce_button_;
        ukive::Animator animator_;

        ukive::ImageFrame* shadow_img_ = nullptr;
        std::shared_ptr<ukive::ImageFrame> content_img_;
        std::unique_ptr<ukive::ShadowEffect> shadow_effect_;
    };

}

#endif  // SHELL_SHADOW_SHADOW_WINDOW_H_