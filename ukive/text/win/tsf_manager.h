// Copyright (c) 2016 ucclkp <ucclkp@gmail.com>.
// This file is part of ukive project.
//
// This program is licensed under GPLv3 license that can be
// found in the LICENSE file.

#ifndef UKIVE_TEXT_WIN_TSF_MANAGER_H_
#define UKIVE_TEXT_WIN_TSF_MANAGER_H_

#include <msctf.h>

#include "ukive/system/win/com_ptr.hpp"
#include "ukive/text/input_method_manager.h"


namespace ukive {

    class TsfSink;
    class TsfEditor;
    class TsfInputConnection;

    class TsfManager : public InputMethodManager {
    public:
        TsfManager();
        ~TsfManager();

        bool initialization() override;
        void destroy() override;

        bool updateIMEStatus() override;

        TfClientId getClientId() const;
        ComPtr<ITfThreadMgr> getThreadManager() const;

        HRESULT setupSinks();
        HRESULT releaseSinks();

    private:
        // https://docs.microsoft.com/zh-cn/windows/win32/tsf/predefined-compartments
        HRESULT setupCompartmentSinks(
            const ComPtr<ITfCompartment>& open_mode,
            const ComPtr<ITfCompartment>& conv_mode);
        HRESULT releaseCompartmentSinks();
        HRESULT getCompartments(
            ComPtr<ITfCompartmentMgr>& cm,
            ComPtr<ITfCompartment>& open_mode,
            ComPtr<ITfCompartment>& conv_mode);

        TfClientId client_id_;
        ComPtr<TsfSink> sink_;
        ComPtr<ITfThreadMgr> thread_mgr_;

        DWORD alpn_sink_cookie_;
        DWORD open_mode_sink_cookie_;
        DWORD conv_mode_sink_cookie_;
    };


    class TsfSink : public ITfInputProcessorProfileActivationSink, public ITfCompartmentEventSink {
    public:
        explicit TsfSink(TsfManager* mgr);
        virtual ~TsfSink();

        // ITfInputProcessorProfileActivationSink
        // Notification for keyboard input locale change
        STDMETHODIMP OnActivated(
            DWORD dwProfileType, LANGID langid, REFCLSID clsid, REFGUID catid, REFGUID guidProfile,
            HKL hkl, DWORD dwFlags) override;

        // ITfCompartmentEventSink
        // Notification for open mode (toggle state) change
        STDMETHODIMP OnChange(REFGUID rguid) override;

        STDMETHODIMP_(ULONG) AddRef() override;
        STDMETHODIMP_(ULONG) Release() override;
        STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj) override;

    private:
        ULONG ref_count_;
        TsfManager* tsf_mgr_;
        ITfCompositionView* composition_view_;
    };

}

#endif  // UKIVE_TEXT_WIN_TSF_MANAGER_H_