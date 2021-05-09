// Copyright (c) 2016 ucclkp <ucclkp@gmail.com>.
// This file is part of ukive project.
//
// This program is licensed under GPLv3 license that can be
// found in the LICENSE file.

#ifndef UKIVE_GRAPHICS_VSYNC_PROVIDER_H_
#define UKIVE_GRAPHICS_VSYNC_PROVIDER_H_

#include <vector>


namespace ukive {

    class VSyncCallback {
    public:
        virtual ~VSyncCallback() = default;

        virtual void onVSync(
            uint64_t start_time, uint32_t display_freq, uint32_t real_interval) = 0;
    };

    class VSyncProvider {
    public:
        static VSyncProvider* create();

        virtual ~VSyncProvider() = default;

        void addCallback(VSyncCallback* cb);
        void removeCallback(VSyncCallback* cb);

        virtual void requestVSync() = 0;

    protected:
        void notifyCallbacks(
            uint64_t start_time, uint32_t display_freq, uint32_t real_interval);

    private:
        std::vector<VSyncCallback*> callbacks_;
    };

}

#endif  // UKIVE_GRAPHICS_VSYNC_PROVIDER_H_