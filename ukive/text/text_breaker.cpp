// Copyright (c) 2016 ucclkp <ucclkp@gmail.com>.
// This file is part of ukive project.
//
// This program is licensed under GPLv3 license that can be
// found in the LICENSE file.

#include "text_breaker.h"

#include "utils/unicode.h"


namespace ukive {

    CharacterBreaker::CharacterBreaker(const std::u16string* text)
        : cur_(0), prev_(0), text_(text) {
    }

    void CharacterBreaker::setCur(size_t cur_pos) {
        if (cur_pos > text_->length()) {
            cur_pos = text_->length();
        }
        cur_ = cur_pos;
        prev_ = cur_;
    }

    bool CharacterBreaker::next() {
        if (cur_ >= text_->length()) {
            return false;
        }

        prev_ = cur_;
        char16_t ch = text_->at(cur_);
        if (IS_HIGH_SURROGATES(ch)) {
            ++cur_;
            if (cur_ >= text_->length()) {
                return true;
            }

            ch = text_->at(cur_);
            if (IS_LOW_SURROGATES(ch)) {
                ++cur_;
                return true;
            }
            return true;
        }

        if (ch == u'\r') {
            ++cur_;
            if (cur_ >= text_->length()) {
                return true;
            }

            ch = text_->at(cur_);
            if (ch == u'\n') {
                ++cur_;
                return true;
            }
            return true;
        }

        ++cur_;
        return true;
    }

    bool CharacterBreaker::prev() {
        if (cur_ == 0) {
            return false;
        }

        prev_ = cur_;
        --cur_;

        char16_t ch = text_->at(cur_);
        if (IS_LOW_SURROGATES(ch)) {
            if (cur_ == 0) {
                return true;
            }

            ch = text_->at(cur_ - 1);
            if (IS_HIGH_SURROGATES(ch)) {
                --cur_;
                return true;
            }
            return true;
        }

        if (ch == u'\n') {
            if (cur_ == 0) {
                return true;
            }

            ch = text_->at(cur_ - 1);
            if (ch == u'\r') {
                --cur_;
                return true;
            }
            return true;
        }

        return true;
    }

}
