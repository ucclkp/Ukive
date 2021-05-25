// Copyright (c) 2016 ucclkp <ucclkp@gmail.com>.
// This file is part of ukive project.
//
// This program is licensed under GPLv3 license that can be
// found in the LICENSE file.

#ifndef UKIVE_VIEWS_SCROLL_VIEW_H_
#define UKIVE_VIEWS_SCROLL_VIEW_H_

#include "ukive/animation/scroller.h"
#include "ukive/event/velocity_calculator.h"
#include "ukive/graphics/vsync_provider.h"
#include "ukive/views/layout/layout_view.h"


namespace ukive {

    class Scroller;
    class InputEvent;

    class ScrollView : public LayoutView, public VSyncCallback
    {
    public:
        explicit ScrollView(Context c);
        ScrollView(Context c, AttrsRef attrs);
        ~ScrollView();

        // View
        Size onDetermineSize(const SizeInfo& info) override;
        void onLayout(
            const Rect& new_bounds, const Rect& old_bounds) override;
        void onBoundsChanged(const Rect& new_bounds, const Rect& old_bounds) override;
        void onScrollChanged(
            int scroll_x, int scroll_y, int old_scroll_x, int old_scroll_y) override;

        bool onHookInputEvent(InputEvent* e) override;
        bool onInputEvent(InputEvent* e) override;

        void onPreDraw() override;

        // VSyncCallback
        void onVSync(
            uint64_t start_time, uint32_t display_freq, uint32_t real_interval) override;

    private:
        bool canScroll() const;
        bool canScroll(bool top) const;
        int computeScrollRange() const;
        int computeScrollExtend() const;

        bool processVerticalScroll(int dy);

        int mouse_x_cache_;
        int mouse_y_cache_;
        int saved_pointer_type_;

        int prev_touch_x_ = 0;
        int prev_touch_y_ = 0;
        int start_touch_x_ = 0;
        int start_touch_y_ = 0;
        bool is_touch_down_ = false;

        Scroller scroller_;
        VelocityCalculator velocity_calculator_;
    };

}

#endif  // UKIVE_VIEWS_SCROLL_VIEW_H_