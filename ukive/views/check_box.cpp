// Copyright (c) 2016 ucclkp <ucclkp@gmail.com>.
// This file is part of ukive project.
//
// This program is licensed under GPLv3 license that can be
// found in the LICENSE file.

#include "ukive/views/check_box.h"

#include <cmath>

#include "ukive/animation/interpolator.h"
#include "ukive/event/input_event.h"
#include "ukive/graphics/canvas.h"
#include "ukive/views/check_listener.h"
#include "ukive/window/window.h"


namespace ukive {

    CheckBox::CheckBox(Context c)
        : CheckBox(c, {}) {}

    CheckBox::CheckBox(Context c, AttrsRef attrs)
        : super(c, attrs),
          checked_(false)
    {
        anim_.setDuration(200);
        anim_.setInterpolator(new LinearInterpolator(1));

        Rect space;
        space.left = c.dp2pxi(24);
        setSpace(space);

        setClickable(true);
        setFocusable(true);
    }

    void CheckBox::setChecked(bool checked) {
        if (checked == checked_) {
            return;
        }
        checked_ = checked;

        if (listener_) {
            listener_->onCheckChanged(this, checked);
        }

        if (checked_) {
            anim_.reset();
            anim_.start();
        } else {
            anim_.stop();
        }
        requestDraw();
    }

    void CheckBox::setOnCheckListener(OnCheckListener* l) {
        listener_ = l;
    }

    void CheckBox::onDraw(Canvas* canvas) {
        super::onDraw(canvas);

        anim_.update();

        auto bounds = getContentBounds();
        int length = std::min(getContext().dp2pxi(20), bounds.height());

        float hp_offset = 0.f;
        int stroke_width = getContext().dp2pxi(2.f);
        if (stroke_width % 2) {
            hp_offset = 0.5f;
        }

        RectF check_bounds(
            hp_offset + stroke_width,
            (bounds.height() - length) / 2.f + hp_offset + stroke_width,
            float(length - stroke_width * 2), float(length - stroke_width * 2));

        canvas->drawRoundRect(
            check_bounds, float(stroke_width), getContext().dp2px(1), Color::Blue400);

        if (checked_) {
            int inner_length = int(check_bounds.width());
            auto value = float(anim_.getCurValue());

            auto line1s = PointF(
                check_bounds.left + stroke_width, check_bounds.top + inner_length / 2.f);
            auto line1e = PointF(
                check_bounds.left + inner_length / 2.f, check_bounds.top + inner_length - stroke_width);
            auto line2s = PointF(
                check_bounds.left + inner_length / 2.f, check_bounds.top + inner_length - stroke_width);
            auto line2e = PointF(
                check_bounds.left + inner_length - stroke_width, check_bounds.top + stroke_width);

            auto vec1 = line1e - line1s;
            auto vec2 = line2e - line2s;

            canvas->drawLine(
                line1s, line1s + vec1 * (std::min(value, 0.5f) * 2),
                std::floor(getContext().dp2px(2.5f)), Color::Blue800);
            canvas->drawLine(
                line2s, line2s + vec2 * (std::max(value - 0.5f, 0.f) * 2),
                std::floor(getContext().dp2px(2.5f)), Color::Blue800);
        }

        if (anim_.isRunning()) {
            requestDraw();
        }
    }

    bool CheckBox::onInputEvent(InputEvent* e) {
        switch (e->getEvent()) {
        case InputEvent::EVM_UP:
        case InputEvent::EVT_UP:
            if (isLocalPointerInThisVisible(e) &&
                e->getMouseKey() == InputEvent::MK_LEFT &&
                isPressed())
            {
                setChecked(!checked_);
            }
            break;

        default:
            break;
        }

        return super::onInputEvent(e);
    }
}
