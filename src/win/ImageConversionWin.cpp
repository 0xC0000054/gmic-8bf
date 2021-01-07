////////////////////////////////////////////////////////////////////////
//
// This file is part of gmic-8bf, a filter plug-in module that
// interfaces with G'MIC-Qt.
//
// Copyright (c) 2020, 2021 Nicholas Hayes
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
#include <new>
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
              tileWidth(std::min(width, 1024)),
              tileHeight(std::min(height, 1024))
        {
        }

        static OSErr WriteCallback(
            const FileHandle* file,
            int32 imageWidth,
            int32 imageHeight,
            int32 numberOfChannels,
            int32 bitsPerChannel,
            void* callbackState)
        {
            if (file == nullptr || callbackState == nullptr)
            {
                return nilHandleErr;
            }

            GmicOutputWriter* instance = static_cast<GmicOutputWriter*>(callbackState);

            return instance->WritePixels(file, imageWidth, imageHeight, numberOfChannels, bitsPerChannel);
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

        OSErr WritePixels(
            const FileHandle* file,
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
                lockRect.Height = std::min(tileHeight, imageHeight - y);

                INT rowCount = lockRect.Height;

                for (int32 x = 0; x < imageWidth; x += tileWidth)
                {
                    lockRect.X = x;
                    lockRect.Width = std::min(tileWidth, imageWidth - x);

                    wil::com_ptr<IWICBitmapLock> bitmapLock;

                    HRESULT hr = image->Lock(&lockRect, WICBitmapLockRead, &bitmapLock);
                    if (FAILED(hr))
                    {
                        return ioErr;
                    }

                    UINT wicStride;
                    hr = bitmapLock->GetStride(&wicStride);
                    if (FAILED(hr))
                    {
                        return ioErr;
                    }

                    UINT wicBufferSize;
                    BYTE* bufferStart;

                    hr = bitmapLock->GetDataPointer(&wicBufferSize, &bufferStart);
                    if (FAILED(hr))
                    {
                        return ioErr;
                    }

                    INT columnCount = lockRect.Width;

                    const size_t outputStride = static_cast<size_t>(columnCount) * numberOfChannels * bytesPerChannel;

                    if (wicStride == outputStride)
                    {
                        // If the WIC image stride matches the output image stride
                        // we can write the buffer directly.

                        OSErr err = WriteFile(file, bufferStart, static_cast<size_t>(wicBufferSize));

                        if (err != noErr)
                        {
                            return err;
                        }
                    }
                    else
                    {
                        for (INT i = 0; i < rowCount; i++)
                        {
                            BYTE* row = bufferStart + (static_cast<size_t>(i) * wicStride);

                            OSErr err = WriteFile(file, row, outputStride);

                            if (err != noErr)
                            {
                                return err;
                            }
                        }
                    }
                }
            }

            return noErr;
        }

        IWICBitmap* image;
        const int32 tileHeight;
        const int32 tileWidth;
    };

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

        OSErrException::ThrowIfError(WritePixelsFromCallback(
            static_cast<int32>(uiWidth),
            static_cast<int32>(uiHeight),
            numberOfChannels,
            bitsPerChannel,
            /* planar */ false,
            writer.GetTileWidth(),
            writer.GetTileHeight(),
            &GmicOutputWriter::WriteCallback,
            &writer,
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
    OSErr err = noErr;

    try
    {
        auto comCleanup = wil::CoInitializeEx(COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

        auto factory = wil::CoCreateInstance<IWICImagingFactory>(CLSID_WICImagingFactory1);
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
