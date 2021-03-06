// Copyright (c) 2016 ucclkp <ucclkp@gmail.com>.
// This file is part of ukive project.
//
// This program is licensed under GPLv3 license that can be
// found in the LICENSE file.

#include "image_element.h"

#include "ukive/graphics/canvas.h"
#include "ukive/window/context.h"


namespace ukive {

    ImageElement::ImageElement(const std::shared_ptr<ImageFrame>& img)
        : image_(img) {}


    void ImageElement::setOpacity(float opt) {
        opacity_ = opt;
    }

    void ImageElement::setExtendMode(ExtendMode mode) {
        mode_ = mode;
    }

    void ImageElement::draw(Canvas* canvas) {
        if (!image_) {
            return;
        }

        if (mode_ == Wrap) {
            canvas->fillImageRepeat(RectF(getBounds()), image_.get());
        } else {
            canvas->drawImage(RectF(getBounds()), opacity_, image_.get());
        }
    }

    Element::Opacity ImageElement::getOpacity() const {
        if (opacity_ >= 1.0f) {
            return OPA_OPAQUE;
        }
        if (opacity_ == 0) {
            return OPA_TRANSPARENT;
        }
        return OPA_SEMILUCENT;
    }

    int ImageElement::getContentWidth() const {
        if (!image_) {
            return 0;
        }
        return int(std::ceil(image_->getSize().width));
    }

    int ImageElement::getContentHeight() const {
        if (!image_) {
            return 0;
        }
        return int(std::ceil(image_->getSize().height));
    }

    std::shared_ptr<ImageFrame> ImageElement::getImage() const {
        return image_;
    }

    void ImageElement::onContextChanged(const Context& context) {
        if (context.getChanged() == Context::DEV_LOST) {
            image_.reset();
        }
    }

}