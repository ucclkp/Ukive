// Copyright (c) 2016 ucclkp <ucclkp@gmail.com>.
// This file is part of ukive project.
//
// This program is licensed under GPLv3 license that can be
// found in the LICENSE file.

#include "ukive/system/dialog/win/open_file_dialog_win.h"

#include <ShlObj.h>

#include "ukive/system/win/com_ptr.hpp"
#include "ukive/window/win/window_impl_win.h"
#include "ukive/window/window.h"


namespace ukive {

    OpenFileDialogWin::OpenFileDialogWin() {}

    int OpenFileDialogWin::show(Window* parent, uint32_t flags) {
        ComPtr<IFileOpenDialog> pfd;
        HRESULT hr = ::CoCreateInstance(
            CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
        if (FAILED(hr)) {
            return -1;
        }

        FILEOPENDIALOGOPTIONS options = FOS_FORCEFILESYSTEM;
        if (flags & OFD_OPEN_DIRECTORY) {
            options |= FOS_PICKFOLDERS;
        }
        if (flags & OFD_MULTI_SELECT) {
            options |= FOS_ALLOWMULTISELECT;
        }
        if (flags & OFD_SHOW_HIDDEN) {
            options |= FOS_FORCESHOWHIDDEN;
        }

        pfd->SetOptions(options);

        std::vector<COMDLG_FILTERSPEC> exts_raw;
        for (const auto& ext : exts_) {
            exts_raw.push_back(
                { reinterpret_cast<const wchar_t*>(ext.second.c_str()),
                reinterpret_cast<const wchar_t*>(ext.first.c_str()) });
        }
        if (!exts_.empty()) {
            pfd->SetFileTypes(UINT(exts_raw.size()), exts_raw.data());
        }

        auto hwnd = static_cast<WindowImplWin*>(parent->getImpl())->getHandle();
        hr = pfd->Show(hwnd);
        if (FAILED(hr)) {
            if (hr == ERROR_CANCELLED) {
                return 0;
            }
            return -1;
        }

        ComPtr<IShellItemArray> item_array;
        hr = pfd->GetResults(&item_array);
        if (FAILED(hr)) {
            return -1;
        }

        DWORD count;
        hr = item_array->GetCount(&count);
        if (FAILED(hr)) {
            return -1;
        }

        sel_files_.clear();

        for (DWORD i = 0; i < count; ++i) {
            ComPtr<IShellItem> item;
            if (SUCCEEDED(item_array->GetItemAt(i, &item))) {
                LPWSTR path;
                if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))) {
                    std::u16string file_path(reinterpret_cast<char16_t*>(path));
                    ::CoTaskMemFree(path);
                    sel_files_.push_back(std::move(file_path));
                }
            }
        }

        return 1;
    }

    void OpenFileDialogWin::addType(std::u16string types, std::u16string desc) {
        exts_.push_back({ std::move(types), std::move(desc) });
    }

    void OpenFileDialogWin::clearTypes() {
        exts_.clear();
    }

    const std::vector<std::u16string>& OpenFileDialogWin::getSelectedFiles() const {
        return sel_files_;
    }

}
