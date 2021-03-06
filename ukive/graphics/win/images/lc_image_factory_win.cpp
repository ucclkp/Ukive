// Copyright (c) 2016 ucclkp <ucclkp@gmail.com>.
// This file is part of ukive project.
//
// This program is licensed under GPLv3 license that can be
// found in the LICENSE file.

#include "ukive/graphics/win/images/lc_image_factory_win.h"

#include <ShlObj.h>
#include <VersionHelpers.h>

#include "utils/log.h"
#include "utils/convert.h"
#include "utils/number.hpp"

#include "ukive/graphics/images/lc_image.h"
#include "ukive/graphics/win/images/lc_image_frame_win.h"
#include "ukive/window/window_dpi_utils.h"


namespace ukive {

    WICPixelFormatGUID mapPixelFormat(const ImageOptions& options) {
        WICPixelFormatGUID format;
        switch (options.pixel_format) {
        case ImagePixelFormat::HDR:
            DCHECK(false);
            format = GUID_WICPixelFormat32bppPBGRA;
            break;

        case ImagePixelFormat::RAW:
        case ImagePixelFormat::B8G8R8A8_UNORM:
        default:
            switch (options.alpha_mode) {
            case ImageAlphaMode::STRAIGHT:
                format = GUID_WICPixelFormat32bppBGRA;
                break;
            case ImageAlphaMode::IGNORED:
                format = GUID_WICPixelFormat32bppBGR;
                break;
            case ImageAlphaMode::PREMULTIPLIED:
            default:
                format = GUID_WICPixelFormat32bppPBGRA;
                break;
            }
            break;
        }

        return format;
    }

}

namespace ukive {

    LcImageFactoryWin::LcImageFactoryWin()
    {}

    bool LcImageFactoryWin::initialization() {
        static bool is_win8 = ::IsWindows8OrGreater();

        HRESULT hr = CoCreateInstance(
            is_win8 ? CLSID_WICImagingFactory : CLSID_WICImagingFactory1,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&wic_factory_));
        if (FAILED(hr)) {
            LOG(Log::ERR) << "Failed to create WIC factory: " << hr;
            return false;
        }
        return true;
    }

    void LcImageFactoryWin::destroy() {
        wic_factory_.reset();
    }

    LcImageFrame* LcImageFactoryWin::create(
        int width, int height, const ImageOptions& options)
    {
        if (width <= 0 || height <= 0) {
            DCHECK(false);
            return nullptr;
        }

        auto format = mapPixelFormat(options);

        ComPtr<IWICBitmap> bmp;
        HRESULT hr = wic_factory_->CreateBitmap(
            width, height, format,
            WICBitmapCacheOnDemand, &bmp);
        if (FAILED(hr)) {
            DCHECK(false);
            return nullptr;
        }

        switch (options.dpi_type) {
        case ImageDPIType::SPECIFIED:
            bmp->SetResolution(options.dpi_x, options.dpi_y);
            break;

        case ImageDPIType::DEFAULT:
        default:
            bmp->SetResolution(kDefaultDpi, kDefaultDpi);
            break;
        }

        return new LcImageFrameWin(bmp);
    }

    LcImageFrame* LcImageFactoryWin::create(
        int width, int height,
        uint8_t* pixel_data, size_t size, size_t stride,
        const ImageOptions& options)
    {
        DCHECK(size <= std::numeric_limits<UINT>::max());
        DCHECK(stride <= std::numeric_limits<UINT>::max());

        if (width <= 0 || height <= 0) {
            DCHECK(false);
            return nullptr;
        }

        auto format = mapPixelFormat(options);

        ComPtr<IWICBitmap> bmp;
        HRESULT hr = wic_factory_->CreateBitmapFromMemory(
            width, height, format,
            UINT(stride), UINT(size), pixel_data,
            &bmp);
        if (FAILED(hr)) {
            DCHECK(false);
            return nullptr;
        }

        switch (options.dpi_type) {
        case ImageDPIType::SPECIFIED:
            bmp->SetResolution(options.dpi_x, options.dpi_y);
            break;

        case ImageDPIType::DEFAULT:
        default:
            bmp->SetResolution(kDefaultDpi, kDefaultDpi);
            break;
        }

        return new LcImageFrameWin(bmp);
    }

    LcImage LcImageFactoryWin::decodeFile(
        const std::u16string& file_name, const ImageOptions& options)
    {
        auto decoder = createDecoder(file_name);
        if (!decoder) {
            return {};
        }
        return processDecoder(decoder.get(), options);
    }

    LcImage LcImageFactoryWin::decodeMemory(
        uint8_t* buffer, size_t size, const ImageOptions& options)
    {
        auto decoder = createDecoder(buffer, size);
        if (!decoder) {
            return {};
        }
        return processDecoder(decoder.get(), options);
    }

    bool LcImageFactoryWin::getThumbnail(
        const std::u16string& file_name,
        int frame_width, int frame_height,
        std::string* out, int* real_w, int* real_h, ImageOptions* options)
    {
        if (!out || !real_w || !real_h || !options) {
            return false;
        }

        bool succeeded = false;
        ComPtr<IShellItemImageFactory> factory;
        HRESULT hr = ::SHCreateItemFromParsingName(
            reinterpret_cast<PCWSTR>(file_name.c_str()), nullptr, IID_PPV_ARGS(&factory));
        if (SUCCEEDED(hr)) {
            HBITMAP hbmp;
            hr = factory->GetImage({ frame_width, frame_height }, SIIGBF_RESIZETOFIT, &hbmp);
            if (SUCCEEDED(hr)) {
                BITMAP bmp;
                if (::GetObjectW(hbmp, sizeof(BITMAP), &bmp) && bmp.bmBitsPixel == 32) {
                    BITMAPINFO info;
                    std::memset(&info, 0, sizeof(BITMAPINFO));
                    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                    info.bmiHeader.biWidth = bmp.bmWidth;
                    info.bmiHeader.biHeight = -bmp.bmHeight;
                    info.bmiHeader.biPlanes = bmp.bmPlanes;
                    info.bmiHeader.biBitCount = bmp.bmBitsPixel;
                    info.bmiHeader.biCompression = BI_RGB;

                    out->resize(bmp.bmWidth * bmp.bmHeight * 4);

                    HDC hdc = ::GetDC(nullptr);
                    if (::GetDIBits(
                        hdc, hbmp, 0, bmp.bmHeight,
                        static_cast<void*>(&*out->begin()), &info, DIB_RGB_COLORS))
                    {
                        *real_w = bmp.bmWidth;
                        *real_h = bmp.bmHeight;
                        options->dpi_type = ImageDPIType::DEFAULT;
                        options->pixel_format = ImagePixelFormat::B8G8R8A8_UNORM;
                        options->alpha_mode = ImageAlphaMode::IGNORED;
                        succeeded = true;
                    }

                    ::ReleaseDC(nullptr, hdc);
                }

                ::DeleteObject(hbmp);
            }
        }
        return succeeded;
    }

    bool LcImageFactoryWin::saveToPNGFile(
        int width, int height,
        uint8_t* data, size_t byte_count, size_t stride,
        const ImageOptions& options,
        const std::u16string& file_name)
    {
        if (width <= 0 || height <= 0 ||
            !data || byte_count == 0 || stride == 0 ||
            byte_count > std::numeric_limits<UINT>::max() ||
            stride > std::numeric_limits<UINT>::max())
        {
            DCHECK(false);
            return false;
        }

        ComPtr<IWICStream> stream;
        HRESULT hr = wic_factory_->CreateStream(&stream);
        if (FAILED(hr)) {
            return false;
        }

        hr = stream->InitializeFromFilename(reinterpret_cast<LPCWSTR>(file_name.c_str()), GENERIC_WRITE);
        if (FAILED(hr)) {
            return false;
        }

        ComPtr<IWICBitmapEncoder> encoder;
        hr = wic_factory_->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder);
        if (FAILED(hr)) {
            return false;
        }

        hr = encoder->Initialize(stream.get(), WICBitmapEncoderNoCache);
        if (FAILED(hr)) {
            return false;
        }

        ComPtr<IWICBitmapFrameEncode> frame;
        hr = encoder->CreateNewFrame(&frame, nullptr);
        if (FAILED(hr)) {
            return false;
        }

        hr = frame->Initialize(nullptr);
        if (FAILED(hr)) {
            return false;
        }

        hr = frame->SetSize(width, height);
        if (FAILED(hr)) {
            return false;
        }

        auto target_format = mapPixelFormat(options);
        auto cur_format = target_format;

        hr = frame->SetPixelFormat(&cur_format);
        if (FAILED(hr)) {
            return false;
        }

        hr = IsEqualGUID(cur_format, target_format) ? S_OK : E_FAIL;
        if (FAILED(hr)) {
            return false;
        }

        hr = frame->WritePixels(
            height, UINT(stride), UINT(byte_count), data);
        if (FAILED(hr)) {
            return false;
        }

        hr = frame->Commit();
        if (FAILED(hr)) {
            return false;
        }

        hr = encoder->Commit();
        if (FAILED(hr)) {
            return false;
        }

        return true;
    }

    void LcImageFactoryWin::getGlobalMetadata(IWICBitmapDecoder* decoder, GifImageData* data) {
        ComPtr<IWICMetadataQueryReader> reader;
        HRESULT hr = decoder->GetMetadataQueryReader(&reader);
        if (SUCCEEDED(hr)) {
            PROPVARIANT prop_var;
            PropVariantInit(&prop_var);
            hr = reader->GetMetadataByName(L"/logscrdesc/Width", &prop_var);
            if (SUCCEEDED(hr) && prop_var.vt == VT_UI2) {
                data->width = prop_var.uiVal;
            }

            PropVariantClear(&prop_var);
            hr = reader->GetMetadataByName(L"/logscrdesc/Height", &prop_var);
            if (SUCCEEDED(hr) && prop_var.vt == VT_UI2) {
                data->height = prop_var.uiVal;
            }

            PropVariantClear(&prop_var);
            hr = reader->GetMetadataByName(L"/logscrdesc/GlobalColorTableFlag", &prop_var);
            if (SUCCEEDED(hr) && prop_var.vt == VT_BOOL && prop_var.boolVal) {
                PropVariantClear(&prop_var);
                hr = reader->GetMetadataByName(L"/logscrdesc/BackgroundColorIndex", &prop_var);
                if (SUCCEEDED(hr) && prop_var.vt == VT_UI1) {
                    UINT bg_index = prop_var.bVal;
                    ComPtr<IWICPalette> palette;
                    hr = wic_factory_->CreatePalette(&palette);
                    if (SUCCEEDED(hr)) {
                        hr = decoder->CopyPalette(palette.get());
                    }

                    if (SUCCEEDED(hr)) {
                        UINT color_count = 0;
                        hr = palette->GetColorCount(&color_count);
                        if (SUCCEEDED(hr) && color_count > 0) {
                            UINT actual_count = 0;
                            WICColor* color_table = new WICColor[color_count];
                            hr = palette->GetColors(color_count, color_table, &actual_count);
                            if (SUCCEEDED(hr) && actual_count > 0 && bg_index < actual_count) {
                                data->bg_color = Color::ofARGB(color_table[bg_index]);
                            }
                            delete[] color_table;
                        }
                    }
                }
            }

            PropVariantClear(&prop_var);
            hr = reader->GetMetadataByName(L"/logscrdesc/PixelAspectRatio", &prop_var);
            if (SUCCEEDED(hr) && prop_var.vt == VT_UI1) {
                UINT ratio = prop_var.bVal;
                if (ratio != 0) {
                    float pixel_ratio = (ratio + 15.f) / 64.f;
                    if (pixel_ratio > 1.f) {
                        data->height = static_cast<int>(data->height / pixel_ratio);
                    } else {
                        data->width = static_cast<int>(data->width * pixel_ratio);
                    }
                }
            }

            PropVariantClear(&prop_var);
            hr = reader->GetMetadataByName(L"/appext/application", &prop_var);
            if (SUCCEEDED(hr)
                && prop_var.vt == (VT_UI1 | VT_VECTOR)
                && prop_var.caub.cElems == 11
                && (!memcmp(prop_var.caub.pElems, "NETSCAPE2.0", prop_var.caub.cElems) ||
                    !memcmp(prop_var.caub.pElems, "ANIMEXTS1.0", prop_var.caub.cElems))) {

                PropVariantClear(&prop_var);
                hr = reader->GetMetadataByName(L"/appext/data", &prop_var);
                if (SUCCEEDED(hr)
                    && (prop_var.vt == (VT_UI1 | VT_VECTOR)
                        && prop_var.caub.cElems >= 4
                        && prop_var.caub.pElems[0] > 0
                        && prop_var.caub.pElems[1] == 1)) {
                    data->loop_count = MAKEWORD(prop_var.caub.pElems[2],
                        prop_var.caub.pElems[3]);
                }
            }
            PropVariantClear(&prop_var);
        }
    }

    void LcImageFactoryWin::getFrameMetadata(IWICBitmapFrameDecode* decoder, GifImageFrData* data) {
        ComPtr<IWICMetadataQueryReader> reader;
        HRESULT hr = decoder->GetMetadataQueryReader(&reader);
        if (SUCCEEDED(hr)) {
            PROPVARIANT prop_var;
            PropVariantInit(&prop_var);
            hr = reader->GetMetadataByName(L"/grctlext/Disposal", &prop_var);
            if (SUCCEEDED(hr) && prop_var.vt == VT_UI1) {
                data->disposal = prop_var.bVal;
            }

            PropVariantClear(&prop_var);
            hr = reader->GetMetadataByName(L"/grctlext/Delay", &prop_var);
            if (SUCCEEDED(hr) && prop_var.vt == VT_UI2) {
                data->frame_interval = prop_var.uiVal * 10;
            }

            PropVariantClear(&prop_var);
            hr = reader->GetMetadataByName(L"/imgdesc/Left", &prop_var);
            if (SUCCEEDED(hr) && prop_var.vt == VT_UI2) {
                data->left = prop_var.uiVal;
            }

            PropVariantClear(&prop_var);
            hr = reader->GetMetadataByName(L"/imgdesc/Top", &prop_var);
            if (SUCCEEDED(hr) && prop_var.vt == VT_UI2) {
                data->top = prop_var.uiVal;
            }

            PropVariantClear(&prop_var);
            hr = reader->GetMetadataByName(L"/imgdesc/Width", &prop_var);
            if (SUCCEEDED(hr) && prop_var.vt == VT_UI2) {
                data->width = prop_var.uiVal;
            }

            PropVariantClear(&prop_var);
            hr = reader->GetMetadataByName(L"/imgdesc/Height", &prop_var);
            if (SUCCEEDED(hr) && prop_var.vt == VT_UI2) {
                data->height = prop_var.uiVal;
            }

            PropVariantClear(&prop_var);
            hr = reader->GetMetadataByName(L"/imgdesc/InterlaceFlag", &prop_var);
            if (SUCCEEDED(hr) && prop_var.vt == VT_BOOL) {
                auto v_interlace = prop_var.boolVal;
                if (v_interlace) {
                    data->interlace = true;
                } else {
                    data->interlace = false;
                }
            }

            PropVariantClear(&prop_var);
            hr = reader->GetMetadataByName(L"/imgdesc/SortFlag", &prop_var);
            if (SUCCEEDED(hr) && prop_var.vt == VT_BOOL) {
                auto v_sort = prop_var.boolVal;
                if (v_sort) {
                    data->sort = true;
                } else {
                    data->sort = false;
                }
            }
            PropVariantClear(&prop_var);
        }
    }

    ComPtr<IWICBitmapDecoder> LcImageFactoryWin::createDecoder(const std::u16string& file_name) {
        /*
         * 这里，某些图片使用 WICDecodeMetadataCacheOnLoad 解码时会报错，
         * 可能是 WIC 对属性的要求较为严格所致，使用 WICDecodeMetadataCacheOnDemand 即可。
         */
        ComPtr<IWICBitmapDecoder> decoder;
        HRESULT hr = wic_factory_->CreateDecoderFromFilename(
            reinterpret_cast<LPCWSTR>(file_name.c_str()),
            nullptr,
            GENERIC_READ,
            WICDecodeMetadataCacheOnDemand,
            &decoder);
        if (FAILED(hr)) {
            DLOG(Log::WARNING) << "Failed to decode file: " << utl::UTF16ToUTF8(file_name) << " " << std::hex << hr;
            return {};
        }
        return decoder;
    }

    ComPtr<IWICBitmapDecoder> LcImageFactoryWin::createDecoder(uint8_t* buffer, size_t size) {
        ComPtr<IWICStream> stream;
        HRESULT hr = wic_factory_->CreateStream(&stream);
        if (FAILED(hr)) {
            DCHECK(false);
            return {};
        }

        hr = stream->InitializeFromMemory(buffer, utl::num_cast<DWORD>(size));
        if (FAILED(hr)) {
            DCHECK(false);
            return {};
        }

        ComPtr<IWICBitmapDecoder> decoder;
        hr = wic_factory_->CreateDecoderFromStream(
            stream.get(), nullptr, WICDecodeMetadataCacheOnDemand, &decoder);
        if (FAILED(hr)) {
            DCHECK(false);
            return {};
        }
        return decoder;
    }

    ComPtr<IWICBitmap> LcImageFactoryWin::convertPixelFormat(
        IWICBitmapSource* frame, const ImageOptions& options)
    {
        // Format convert the frame to 32bppPBGRA
        ComPtr<IWICFormatConverter> converter;
        HRESULT hr = wic_factory_->CreateFormatConverter(&converter);
        if (FAILED(hr)) {
            DCHECK(false);
            return {};
        }

        auto format = mapPixelFormat(options);

        hr = converter->Initialize(
            frame,                          // Input bitmap to convert
            format,                         // Destination pixel format
            WICBitmapDitherTypeNone,        // Specified dither pattern
            nullptr,                        // Specify a particular palette
            0.f,                            // Alpha threshold
            WICBitmapPaletteTypeCustom      // Palette translation type
        );

        if (FAILED(hr)) {
            DCHECK(false);
            return {};
        }

        ComPtr<IWICBitmap> wic_bitmap;
        hr = wic_factory_->CreateBitmapFromSource(
            converter.get(), WICBitmapCacheOnDemand, &wic_bitmap);
        if (FAILED(hr)) {
            DCHECK(false);
            return {};
        }

        return wic_bitmap;
    }

    LcImage LcImageFactoryWin::processDecoder(
        IWICBitmapDecoder* decoder, const ImageOptions& options)
    {
        UINT frame_count = 0;
        HRESULT hr = decoder->GetFrameCount(&frame_count);
        if (FAILED(hr) || frame_count < 1) {
            DCHECK(false);
            return {};
        }

        GUID img_format;
        hr = decoder->GetContainerFormat(&img_format);
        if (FAILED(hr)) {
            DCHECK(false);
            return {};
        }

        LcImage image;
        if (img_format == GUID_ContainerFormatGif) {
            auto data = std::make_shared<GifImageData>();
            getGlobalMetadata(decoder, data.get());
            image.setData(data);
        }

        for (UINT i = 0; i < frame_count; ++i) {
            ComPtr<IWICBitmapFrameDecode> frame_decoder;
            hr = decoder->GetFrame(i, &frame_decoder);
            if (FAILED(hr)) {
                DCHECK(false);
                return {};
            }

            ComPtr<IWICBitmap> wic_bitmap;
            if (options.pixel_format == ImagePixelFormat::RAW) {
                hr = wic_factory_->CreateBitmapFromSource(
                    frame_decoder.get(), WICBitmapCacheOnDemand, &wic_bitmap);
                if (FAILED(hr)) {
                    DCHECK(false);
                    return {};
                }
            } else {
                wic_bitmap = convertPixelFormat(frame_decoder.get(), options);
            }

            if (!wic_bitmap) {
                return {};
            }

            switch (options.dpi_type) {
            case ImageDPIType::SPECIFIED:
                wic_bitmap->SetResolution(options.dpi_x, options.dpi_y);
                break;

            case ImageDPIType::DEFAULT:
            default:
                wic_bitmap->SetResolution(kDefaultDpi, kDefaultDpi);
                break;
            }

            LcImageFrame* frame = new LcImageFrameWin(wic_bitmap);

            if (img_format == GUID_ContainerFormatGif) {
                auto fr_data = std::make_shared<GifImageFrData>();
                getFrameMetadata(frame_decoder.get(), fr_data.get());
                frame->setData(fr_data);
            }
            image.addFrame(frame);
        }

        return image;
    }

}
