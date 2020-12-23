////////////////////////////////////////////////////////////////////////
//
// This file is part of gmic-8bf, a filter plug-in module that
// interfaces with G'MIC-Qt.
//
// Copyright (c) 2020 Nicholas Hayes
//
// This file is licensed under the MIT License.
// See LICENSE.txt for complete licensing and attribution information.
//
////////////////////////////////////////////////////////////////////////

#include "ImageConversionWin.h"
#include "GmicInputWriter.h"
#include "ReadOnlyMemoryStream.h"
#include <boost/filesystem.hpp>
#include <wincodec.h>
#include <wil/com.h>
#include <wil/resource.h>
#include <stdexcept>

namespace
{
    class OSErrException : public std::exception
    {
    public:
        OSErrException(OSErr err) : error(err)
        {
        }

        OSErr GetErrorCode() const
        {
            return error;
        }

        static void ThrowIfError(OSErr err)
        {
            if (err != noErr)
            {
                throw OSErrException(err);
            }
        }

    private:
        OSErr error;
    };

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
            IsEqualGUID(format, GUID_WICPixelFormat16bppBGR565))
        {
            targetFormat = GUID_WICPixelFormat24bppRGB;
            bitsPerChannel = 8;
            numberOfChannels = 3;
        }
        else if (IsEqualGUID(format, GUID_WICPixelFormat8bppGray) ||
                 IsEqualGUID(format, GUID_WICPixelFormatBlackWhite) ||
                 IsEqualGUID(format, GUID_WICPixelFormat2bppGray) ||
                 IsEqualGUID(format, GUID_WICPixelFormat4bppGray) ||
                 IsEqualGUID(format, GUID_WICPixelFormat8bppAlpha))
        {
            targetFormat = GUID_WICPixelFormat8bppGray;
            bitsPerChannel = 8;
            numberOfChannels = 1;
        }
        else if (IsEqualGUID(format, GUID_WICPixelFormat16bppGray) ||
                 IsEqualGUID(format, GUID_WICPixelFormat16bppGrayHalf) ||
                 IsEqualGUID(format, GUID_WICPixelFormat32bppGrayFloat))
        {
            targetFormat = GUID_WICPixelFormat16bppGray;
            bitsPerChannel = 16;
            numberOfChannels = 1;
        }
        else if (IsEqualGUID(format, GUID_WICPixelFormat48bppRGB) ||
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
            targetFormat = GUID_WICPixelFormat48bppRGB;
            bitsPerChannel = 16;
            numberOfChannels = 3;
        }
        else if (IsEqualGUID(format, GUID_WICPixelFormat64bppRGBA) ||
                 IsEqualGUID(format, GUID_WICPixelFormat64bppBGRA) ||
                 IsEqualGUID(format, GUID_WICPixelFormat64bppBGRAFixedPoint) ||
                 IsEqualGUID(format, GUID_WICPixelFormat64bppPBGRA) ||
                 IsEqualGUID(format, GUID_WICPixelFormat64bppRGBAFixedPoint) ||
                 IsEqualGUID(format, GUID_WICPixelFormat64bppPRGBA) ||
                 IsEqualGUID(format, GUID_WICPixelFormat64bppRGBAHalf) ||
                 IsEqualGUID(format, GUID_WICPixelFormat128bppRGBAFloat) ||
                 IsEqualGUID(format, GUID_WICPixelFormat128bppRGBAFixedPoint))
        {
            targetFormat = GUID_WICPixelFormat64bppRGBA;
            bitsPerChannel = 16;
            numberOfChannels = 4;
        }

        return targetFormat;
    }

    void DoGmicInputFormatConversion(
        const boost::filesystem::path& output,
        IWICImagingFactory* factory,
        IWICBitmapDecoder* decoder)
    {
        wil::com_ptr<IWICBitmapFrameDecode> decoderFrame;

        THROW_IF_FAILED(decoder->GetFrame(0, &decoderFrame));

        UINT uiWidth;
        UINT uiHeight;

        THROW_IF_FAILED(decoderFrame->GetSize(&uiWidth, &uiHeight));

        WICPixelFormatGUID format;

        THROW_IF_FAILED(decoderFrame->GetPixelFormat(&format));

        wil::com_ptr<IWICBitmap> bitmap;

        int bitsPerChannel;
        int numberOfChannels;

        WICPixelFormatGUID targetFormat = GetTargetFormat(format, bitsPerChannel, numberOfChannels);

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

        wil::com_ptr<IWICBitmapLock> bitmapLock;

        WICRect rcLock = { 0, 0, static_cast<INT>(uiWidth), static_cast<INT>(uiHeight) };

        THROW_IF_FAILED(bitmap->Lock(&rcLock, WICBitmapLockRead, &bitmapLock));

        UINT cbStride;
        UINT cbScan0;
        WICInProcPointer scan0 = nullptr;

        THROW_IF_FAILED(bitmapLock->GetStride(&cbStride));
        THROW_IF_FAILED(bitmapLock->GetDataPointer(&cbScan0, &scan0));

        OSErrException::ThrowIfError(CopyFromPixelBuffer(
            static_cast<int32>(uiWidth),
            static_cast<int32>(uiHeight),
            numberOfChannels,
            bitsPerChannel,
            scan0,
            static_cast<size_t>(cbStride),
            output));
    }
}

OSErr ConvertImageToGmicInputFormatNative(
    const FilterRecordPtr filterRecord,
    const boost::filesystem::path& input,
    const boost::filesystem::path& output)
{
    OSErr err = noErr;

    try
    {
        auto comCleanup = wil::CoInitializeEx(COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

        auto factory = wil::CoCreateInstance<IWICImagingFactory>(CLSID_WICImagingFactory1);
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
    catch (const std::bad_alloc&)
    {
        err = memFullErr;
    }
    catch (const OSErrException& e)
    {
        err = e.GetErrorCode();
    }
    catch (const wil::ResultException& e)
    {
        if (e.GetErrorCode() == E_OUTOFMEMORY)
        {
            err = memFullErr;
        }
        else
        {
            err = ShowErrorMessage(e.what(), filterRecord, ioErr);
        }
    }
    catch (...)
    {
        err = ioErr;
    }

    return err;
}

OSErr ConvertImageToGmicInputFormatNative(
    const FilterRecordPtr filterRecord,
    const void* input,
    size_t inputLength,
    const boost::filesystem::path& output)
{
    if (inputLength > std::numeric_limits<UINT>::max())
    {
        // The IWICBitmapLock interface can only handle up to 4 GB of memory.
        return memFullErr;
    }

    OSErr err = noErr;

    try
    {
        auto comCleanup = wil::CoInitializeEx(COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

        auto factory = wil::CoCreateInstance<IWICImagingFactory>(CLSID_WICImagingFactory1);
        wil::com_ptr<IWICBitmapDecoder> decoder;

        wil::com_ptr<ReadOnlyMemoryStream> stream(new ReadOnlyMemoryStream(input, static_cast<UINT>(inputLength)));

        const GUID* vendor = &GUID_VendorMicrosoftBuiltIn;

        THROW_IF_FAILED(factory->CreateDecoderFromStream(
            stream.get(),
            vendor,
            WICDecodeMetadataCacheOnDemand,
            &decoder));

        DoGmicInputFormatConversion(output, factory.get(), decoder.get());
    }
    catch (const std::bad_alloc&)
    {
        err = memFullErr;
    }
    catch (const OSErrException& e)
    {
        err = e.GetErrorCode();
    }
    catch (const wil::ResultException& e)
    {
        if (e.GetErrorCode() == E_OUTOFMEMORY)
        {
            err = memFullErr;
        }
        else
        {
            err = ShowErrorMessage(e.what(), filterRecord, ioErr);
        }
    }
    catch (...)
    {
        err = ioErr;
    }

    return err;
}
