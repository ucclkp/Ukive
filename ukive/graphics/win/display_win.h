// Copyright (c) 2016 ucclkp <ucclkp@gmail.com>.
// This file is part of ukive project.
//
// This program is licensed under GPLv3 license that can be
// found in the LICENSE file.

#ifndef UKIVE_GRAPHICS_WIN_DISPLAY_WIN_H_
#define UKIVE_GRAPHICS_WIN_DISPLAY_WIN_H_

#include "ukive/graphics/display.h"
#include "ukive/system/win/com_ptr.hpp"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <dxgi.h>
#include <dxgi1_6.h>


namespace ukive {

    class WindowImplWin;

    class DisplayWin : public Display {
    public:
        DisplayWin();

        bool makePrimary() override;
        bool makeFromPoint(const Point& p) override;
        bool makeFromRect(const Rect& r) override;
        bool makeFromWindow(Window* w) override;

        bool isInHDRMode() const override;

        void getName(std::u16string* name) override;
        void getAdapterName(std::u16string* name) override;
        Rect getBounds() const override;
        Rect getWorkArea() const override;
        Rect getPixelBounds() const override;
        Rect getPixelWorkArea() const override;
        void getUserScale(float* sx, float* sy) const override;
        uint32_t getRefreshRate() const override;

        bool makeFromWindowImpl(WindowImplWin* win);
        bool waitForVSync();

    private:
        bool queryMonitorInfo(HMONITOR monitor);
        bool queryDXGIInfo(HMONITOR monitor);

        bool is_empty_ = true;
        HMONITOR monitor_;
        DEVMODEW monitor_mode_;
        MONITORINFOEXW monitor_info_;
        ComPtr<IDXGIOutput> cur_output_;
        ComPtr<IDXGIOutput6> cur_output6_;
        ComPtr<IDXGIAdapter1> cur_adapter_;
    };

}

#endif  // UKIVE_GRAPHICS_WIN_DISPLAY_WIN_H_