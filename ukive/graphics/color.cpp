// Copyright (c) 2016 ucclkp <ucclkp@gmail.com>.
// This file is part of ukive project.
//
// This program is licensed under GPLv3 license that can be
// found in the LICENSE file.

#include "color.h"

#include "utils/convert.h"
#include "utils/log.h"


namespace ukive {

    Color::Color()
        : a(0.f), r(0.f), g(0.f), b(0.f) {}

    Color::Color(float r, float g, float b, float a)
        : a(a), r(r), g(g), b(b) {}

    Color Color::parse(const std::string_view& color) {
        if (color.empty() || color.at(0) != '#') {
            LOG(Log::ERR) << "Unknown color: " << color;
            return Red500;
        }

        if (color.length() == 7) {
            int r, g, b;
            if (!utl::hexStringToNumber(color.substr(1, 2), &r) ||
                !utl::hexStringToNumber(color.substr(3, 2), &g) ||
                !utl::hexStringToNumber(color.substr(5, 2), &b))
            {
                LOG(Log::ERR) << "Unknown color: " << color;
                return Red500;
            }

            return ofInt(r, g, b);
        }

        if (color.length() == 9) {
            int a, r, g, b;
            if (!utl::hexStringToNumber(color.substr(1, 2), &a) ||
                !utl::hexStringToNumber(color.substr(3, 2), &r) ||
                !utl::hexStringToNumber(color.substr(5, 2), &g) ||
                !utl::hexStringToNumber(color.substr(7, 2), &b))
            {
                LOG(Log::ERR) << "Unknown color: " << color;
                return Red500;
            }

            return ofInt(r, g, b, a);
        }

        LOG(Log::ERR) << "Unknown color: " << color;
        return Red500;
    }

    Color Color::ofInt(int r, int g, int b, int a) {
        return Color(r / 255.f, g / 255.f, b / 255.f, a / 255.f);
    }

    Color Color::ofRGB(uint32_t rgb, float a) {
        return Color(
            ((rgb & red_mask) >> red_shift) / 255.f,
            ((rgb & green_mask) >> green_shift) / 255.f,
            ((rgb & blue_mask) >> blue_shift) / 255.f,
            a);
    }

    Color Color::ofARGB(uint32_t argb) {
        return Color(
            ((argb & red_mask) >> red_shift) / 255.f,
            ((argb & green_mask) >> green_shift) / 255.f,
            ((argb & blue_mask) >> blue_shift) / 255.f,
            ((argb & alpha_mask) >> alpha_shift) / 255.f);
    }

    int Color::getA(uint32_t argb) {
        return (argb & alpha_mask) >> alpha_shift;
    }

    int Color::getR(uint32_t argb) {
        return (argb & red_mask) >> red_shift;
    }

    int Color::getG(uint32_t argb) {
        return (argb & green_mask) >> green_shift;
    }

    int Color::getB(uint32_t argb) {
        return (argb & blue_mask) >> blue_shift;
    }


    // Color White.
    const Color Color::White = Color::parse("#FFFFFF");
    // Color Black.
    const Color Color::Black = Color::parse("#000000");
    // Color Transparent.
    const Color Color::Transparent = Color::parse("#00000000");

    // Material Color Red.
    Color Color::Red50 = Color::parse("#FFEBEE");
    Color Color::Red100 = Color::parse("#FFCDD2");
    Color Color::Red200 = Color::parse("#EF9A9A");
    Color Color::Red300 = Color::parse("#E57373");
    Color Color::Red400 = Color::parse("#EF5350");
    Color Color::Red500 = Color::parse("#F44336");
    Color Color::Red600 = Color::parse("#E53935");
    Color Color::Red700 = Color::parse("#D32F2F");
    Color Color::Red800 = Color::parse("#C62828");
    Color Color::Red900 = Color::parse("#B71C1C");

    // Material Color Orange.
    Color Color::Orange50 = Color::parse("#FFF3E0");
    Color Color::Orange100 = Color::parse("#FFE0B2");
    Color Color::Orange200 = Color::parse("#FFCC80");
    Color Color::Orange300 = Color::parse("#FFB74D");
    Color Color::Orange400 = Color::parse("#FFA726");
    Color Color::Orange500 = Color::parse("#FF9800");
    Color Color::Orange600 = Color::parse("#FB8C00");
    Color Color::Orange700 = Color::parse("#F57C00");
    Color Color::Orange800 = Color::parse("#EF6C00");
    Color Color::Orange900 = Color::parse("#E65100");

    // Material Color Yellow.
    Color Color::Yellow50 = Color::parse("#FFFDE7");
    Color Color::Yellow100 = Color::parse("#FFF9C4");
    Color Color::Yellow200 = Color::parse("#FFF59D");
    Color Color::Yellow300 = Color::parse("#FFF176");
    Color Color::Yellow400 = Color::parse("#FFEE58");
    Color Color::Yellow500 = Color::parse("#FFEB3B");
    Color Color::Yellow600 = Color::parse("#FDD835");
    Color Color::Yellow700 = Color::parse("#FBC02D");
    Color Color::Yellow800 = Color::parse("#F9A825");
    Color Color::Yellow900 = Color::parse("#F57F17");

    // Material Color Pink.
    Color Color::Pink50 = Color::parse("#FCE4EC");
    Color Color::Pink100 = Color::parse("#F8BBD0");
    Color Color::Pink200 = Color::parse("#F48FB1");
    Color Color::Pink300 = Color::parse("#F06292");
    Color Color::Pink400 = Color::parse("#EC407A");
    Color Color::Pink500 = Color::parse("#E91E63");
    Color Color::Pink600 = Color::parse("#D81B60");
    Color Color::Pink700 = Color::parse("#C2185B");
    Color Color::Pink800 = Color::parse("#AD1457");
    Color Color::Pink900 = Color::parse("#880E4F");

    // Material Color Green.
    Color Color::Green50 = Color::parse("#E8F5E9");
    Color Color::Green100 = Color::parse("#C8E6C9");
    Color Color::Green200 = Color::parse("#A5D6A7");
    Color Color::Green300 = Color::parse("#81C784");
    Color Color::Green400 = Color::parse("#66BB6A");
    Color Color::Green500 = Color::parse("#4CAF50");
    Color Color::Green600 = Color::parse("#43A047");
    Color Color::Green700 = Color::parse("#388E3C");
    Color Color::Green800 = Color::parse("#2E7D32");
    Color Color::Green900 = Color::parse("#1B5E20");

    // Material Color Blue.
    Color Color::Blue50 = Color::parse("#E3F2FD");
    Color Color::Blue100 = Color::parse("#BBDEFB");
    Color Color::Blue200 = Color::parse("#90CAF9");
    Color Color::Blue300 = Color::parse("#64B5F6");
    Color Color::Blue400 = Color::parse("#42A5F5");
    Color Color::Blue500 = Color::parse("#2196F3");
    Color Color::Blue600 = Color::parse("#1E88E5");
    Color Color::Blue700 = Color::parse("#1976D2");
    Color Color::Blue800 = Color::parse("#1565C0");
    Color Color::Blue900 = Color::parse("#0D47A1");

    // Material Color Grey.
    Color Color::Grey50 = Color::parse("#FAFAFA");
    Color Color::Grey100 = Color::parse("#F5F5F5");
    Color Color::Grey200 = Color::parse("#EEEEEE");
    Color Color::Grey300 = Color::parse("#E0E0E0");
    Color Color::Grey400 = Color::parse("#BDBDBD");
    Color Color::Grey500 = Color::parse("#9E9E9E");
    Color Color::Grey600 = Color::parse("#757575");
    Color Color::Grey700 = Color::parse("#616161");
    Color Color::Grey800 = Color::parse("#424242");
    Color Color::Grey900 = Color::parse("#212121");

}