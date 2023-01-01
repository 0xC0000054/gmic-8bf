////////////////////////////////////////////////////////////////////////
//
// This file is part of gmic-8bf, a filter plug-in module that
// interfaces with G'MIC-Qt.
//
// Copyright (c) 2020, 2021, 2022, 2023 Nicholas Hayes
//
// This file is licensed under the MIT License.
// See LICENSE.txt for complete licensing and attribution information.
//
////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#if _WIN32_WINNT == _WIN32_WINNT_WIN7
#define _WIN7_PLATFORM_UPDATE 1
#endif // _WIN32_WINNT == _WIN32_WINNT_WIN7

#include "ImageConversionWin.h"
#include "FileUtil.h"
#include "FileIO.h"
#include "Gmic8bfImageWriter.h"
#include "ReadOnlyMemoryStream.h"
#include <boost/filesystem.hpp>
#include <wincodec.h>
#include <wil/com.h>
#include <wil/resource.h>
#include <new>

namespace
{
    WICPixelFormatGUID GetTargetFormat(
        const WICPixelFormatGUID& format,
        int& bitsPerChannel,
        int& numberOfChannels)
    {
        WICPixelFormatGUID targetFormat = GUID_WICPixelFormat32bppRGBA;
        bitsPerChannel = 8;
        numberOfChannels = 4;

        if (IsEqualGUID(format, GUID_WICPixelFormat24bppBGR) ||
            IsEqualGUID(format, GUID_WICPixelFormat24bppRGB) ||
            IsEqualGUID(format, GUID_WICPixelFormat32bppBGR) ||
            IsEqualGUID(format, GUID_WICPixelFormat16bppBGR555) ||
            IsEqualGUID(format, GUID_WICPixelFormat16bppBGR565) ||
            IsEqualGUID(format, GUID_WICPixelFormat48bppRGB) ||
            IsEqualGUID(format, GUID_WICPixelFormat48bppBGR) ||
            IsEqualGUID(format, GUID_WICPixelFormat48bppBGRFixedPoint) ||
            IsEqualGUID(format, GUID_WICPixelFormat48bppRGBFixedPoint) ||
            IsEqualGUID(format, GUID_WICPixelFormat48bppRGBHalf) ||
            IsEqualGUID(format, GUID_WICPixelFormat64bppRGB) ||
            IsEqualGUID(format, GUID_WICPixelFormat64bppRGBFixedPoint) ||
            IsEqualGUID(format, GUID_WICPixelFormat64bppRGBHalf) ||
            IsEqualGUID(format, GUID_WICPixelFormat128bppRGBFloat) ||
            IsEqualGUID(format, GUID_WICPixelFormat128bppRGBFixedPoint))
        {
            targetFormat = GUID_WICPixelFormat24bppRGB;
            bitsPerChannel = 8;
            numberOfChannels = 3;
        }
        else if (IsEqualGUID(format, GUID_WICPixelFormat8bppGray) ||
                 IsEqualGUID(format, GUID_WICPixelFormatBlackWhite) ||
                 IsEqualGUID(format, GUID_WICPixelFormat2bppGray) ||
                 IsEqualGUID(format, GUID_WICPixelFormat4bppGray) ||
                 IsEqualGUID(format, GUID_WICPixelFormat8bppAlpha) ||
                 IsEqualGUID(format, GUID_WICPixelFormat16bppGray) ||
                 IsEqualGUID(format, GUID_WICPixelFormat16bppGrayHalf) ||
                 IsEqualGUID(format, GUID_WICPixelFormat16bppGrayFixedPoint) ||
                 IsEqualGUID(format, GUID_WICPixelFormat32bppGrayFloat) ||
                 IsEqualGUID(format, GUID_WICPixelFormat32bppGrayFixedPoint))
        {
            targetFormat = GUID_WICPixelFormat8bppGray;
            bitsPerChannel = 8;
            numberOfChannels = 1;
        }

        return targetFormat;
    }

    struct GmicOutputWriter
    {
        GmicOutputWriter(
            IWICBitmap* source,
            int32 width,
            int32 height)
            : image(source),
              tileWidth(::std::min(width, 1024)),
              tileHeight(::std::min(height, 1024))
        {
        }

        static void WriteCallback(
            FileHandle* file,
            int32 imageWidth,
            int32 imageHeight,
            int32 numberOfChannels,
            int32 bitsPerChannel,
            void* callbackState)
        {
            if (file == nullptr || callbackState == nullptr)
            {
                throw ::std::runtime_error("A required pointer was null.");
            }

            GmicOutputWriter* instance = static_cast<GmicOutputWriter*>(callbackState);

            instance->WritePixels(file, imageWidth, imageHeight, numberOfChannels, bitsPerChannel);
        }

        int32 GetTileHeight() const
        {
            return tileHeight;
        }

        int32 GetTileWidth() const
        {
            return tileWidth;
        }

    private:

        void WritePixels(
            FileHandle* file,
            int32 imageWidth,
            int32 imageHeight,
            int32 numberOfChannels,
            int32 bitsPerChannel)
        {
            const int32 bytesPerChannel = bitsPerChannel / 8;

            WICRect lockRect{};

            for (int32 y = 0; y < imageHeight; y += tileHeight)
            {
                lockRect.Y = y;
                lockRect.Height = ::std::min(tileHeight, imageHeight - y);

                INT rowCount = lockRect.Height;

                for (int32 x = 0; x < imageWidth; x += tileWidth)
                {
                    lockRect.X = x;
                    lockRect.Width = ::std::min(tileWidth, imageWidth - x);

                    wil::com_ptr<IWICBitmapLock> bitmapLock;

                    THROW_IF_FAILED(image->Lock(&lockRect, WICBitmapLockRead, &bitmapLock));

                    UINT wicStride;
                    THROW_IF_FAILED(bitmapLock->GetStride(&wicStride));

                    UINT wicBufferSize;
                    BYTE* bufferStart;

                    THROW_IF_FAILED(bitmapLock->GetDataPointer(&wicBufferSize, &bufferStart));

                    INT columnCount = lockRect.Width;

                    const size_t outputStride = static_cast<size_t>(columnCount) * numberOfChannels * bytesPerChannel;

                    if (wicStride == outputStride)
                    {
                        // If the WIC image stride matches the output image stride
                        // we can write the buffer directly.

                        WriteFile(file, bufferStart, static_cast<size_t>(wicBufferSize));
                    }
                    else
                    {
                        for (INT i = 0; i < rowCount; i++)
                        {
                            BYTE* row = bufferStart + (static_cast<size_t>(i) * wicStride);

                            WriteFile(file, row, outputStride);
                        }
                    }
                }
            }
        }

        IWICBitmap* image;
        const int32 tileHeight;
        const int32 tileWidth;
    };

    void DoGmicInputFormatConversion(
        ::std::unique_ptr<InputLayerInfo>& output,
        IWICImagingFactory* factory,
        IWICBitmapDecoder* decoder)
    {
        wil::com_ptr<IWICBitmapFrameDecode> decoderFrame;

        THROW_IF_FAILED(decoder->GetFrame(0, &decoderFrame));

        UINT uiWidth;
        UINT uiHeight;

        THROW_IF_FAILED(decoderFrame->GetSize(&uiWidth, &uiHeight));

        if (uiWidth > static_cast<UINT>(::std::numeric_limits<int32>::max()) ||
            uiHeight > static_cast<UINT>(::std::numeric_limits<int32>::max()))
        {
            throw wil::ResultException(HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW));
        }

        WICPixelFormatGUID format;

        THROW_IF_FAILED(decoderFrame->GetPixelFormat(&format));

        int bitsPerChannel;
        int numberOfChannels;

        WICPixelFormatGUID targetFormat = GetTargetFormat(
                                            format,
                                            bitsPerChannel,
                                            numberOfChannels);
        wil::com_ptr<IWICBitmap> bitmap;

        if (IsEqualGUID(format, targetFormat))
        {
            THROW_IF_FAILED(factory->CreateBitmapFromSource(decoderFrame.get(), WICBitmapCacheOnLoad, &bitmap));
        }
        else
        {
            wil::com_ptr<IWICFormatConverter> formatConverter;

            THROW_IF_FAILED(factory->CreateFormatConverter(&formatConverter));
            THROW_IF_FAILED(formatConverter->Initialize(
                decoderFrame.get(),
                targetFormat,
                WICBitmapDitherTypeNone,
                nullptr,
                0.f,
                WICBitmapPaletteTypeCustom));
            THROW_IF_FAILED(factory->CreateBitmapFromSource(formatConverter.get(), WICBitmapCacheOnLoad, &bitmap));
        }

        GmicOutputWriter writer(
            bitmap.get(),
            static_cast<int32>(uiWidth),
            static_cast<int32>(uiHeight));

        const boost::filesystem::path path = GetTemporaryFileName(GetInputDirectory(), ".g8i");

        WritePixelsFromCallback(
            static_cast<int32>(uiWidth),
            static_cast<int32>(uiHeight),
            numberOfChannels,
            bitsPerChannel,
            /* planar */ false,
            writer.GetTileWidth(),
            writer.GetTileHeight(),
            &GmicOutputWriter::WriteCallback,
            &writer,
            path);

        output.reset(new InputLayerInfo(
            path,
            static_cast<int32>(uiWidth),
            static_cast<int32>(uiHeight),
            true,
            "2nd Layer"));
    }

    wil::com_ptr_t<IWICImagingFactory> GetWICImagingFactory()
    {
        wil::com_ptr_t<IWICImagingFactory> factory;

        if (FAILED(::CoCreateInstance(CLSID_WICImagingFactory2, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory))))
        {
            // Use CLSID_WICImagingFactory1 if we could not create a CLSID_WICImagingFactory2 instance.
            // This should only occur on Windows 7 without the Platform Update.
            THROW_IF_FAILED(::CoCreateInstance(CLSID_WICImagingFactory1, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory)));
        }

        return factory;
    }
}

void ConvertImageToGmicInputFormatNative(
    const boost::filesystem::path& input,
    ::std::unique_ptr<InputLayerInfo>& output)
{
    try
    {
        auto comCleanup = wil::CoInitializeEx(COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

        auto factory = GetWICImagingFactory();
        wil::com_ptr<IWICBitmapDecoder> decoder;

        const GUID* vendor = &GUID_VendorMicrosoftBuiltIn;

        THROW_IF_FAILED(factory->CreateDecoderFromFilename(
            input.c_str(),
            vendor,
            GENERIC_READ,
            WICDecodeMetadataCacheOnDemand,
            &decoder));

        DoGmicInputFormatConversion(output, factory.get(), decoder.get());
    }
    catch (const wil::ResultException& e)
    {
        if (e.GetErrorCode() == E_OUTOFMEMORY)
        {
            throw ::std::bad_alloc();
        }
        else
        {
            throw ::std::runtime_error(e.what());
        }
    }
}

void ConvertImageToGmicInputFormatNative(
    const void* input,
    size_t inputLength,
    ::std::unique_ptr<InputLayerInfo>& output)
{
    try
    {
        auto comCleanup = wil::CoInitializeEx(COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

        auto factory = GetWICImagingFactory();
        wil::com_ptr<IWICBitmapDecoder> decoder;

        wil::com_ptr<ReadOnlyMemoryStream> stream(new ReadOnlyMemoryStream(input, inputLength));

        const GUID* vendor = &GUID_VendorMicrosoftBuiltIn;

        THROW_IF_FAILED(factory->CreateDecoderFromStream(
            stream.get(),
            vendor,
            WICDecodeMetadataCacheOnDemand,
            &decoder));

        DoGmicInputFormatConversion(output, factory.get(), decoder.get());
    }
    catch (const wil::ResultException& e)
    {
        if (e.GetErrorCode() == E_OUTOFMEMORY)
        {
            throw ::std::bad_alloc();
        }
        else
        {
            throw ::std::runtime_error(e.what());
        }
    }
}
