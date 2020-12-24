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

    struct GmicOutputWriter
    {
        GmicOutputWriter(IWICBitmapSource* source) : image(source)
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

    private:

        OSErr WritePixels(
            const FileHandle* file,
            int32 imageWidth,
            int32 imageHeight,
            int32 numberOfChannels,
            int32 bitsPerChannel)
        {
            UINT wicStride;

            if (!TryCalculateWICStride(imageWidth, numberOfChannels, bitsPerChannel, wicStride))
            {
                return memFullErr;
            }

            int32 chunkHeight;
            UINT copyBufferSize;

            if (!TryCalculateCopyBufferSize(imageHeight, wicStride, chunkHeight, copyBufferSize))
            {
                return memFullErr;
            }

            std::unique_ptr<BYTE[]> buffer(new (std::nothrow) BYTE[static_cast<size_t>(copyBufferSize)]);

            if (buffer == nullptr)
            {
                return memFullErr;
            }

            const int32 bytesPerChannel = bitsPerChannel / 8;
            const size_t outputStride = static_cast<size_t>(imageWidth) * numberOfChannels * bytesPerChannel;

            WICRect copyRect{};
            copyRect.X = 0;
            copyRect.Width = imageWidth;

            BYTE* bufferStart = buffer.get();

            for (int32 y = 0; y < imageHeight; y += chunkHeight)
            {
                copyRect.Y = y;
                copyRect.Height = std::min(chunkHeight, imageHeight - y);

                if (FAILED(image->CopyPixels(&copyRect, wicStride, copyBufferSize, bufferStart)))
                {
                    return ioErr;
                }

                INT rowCount = copyRect.Height;

                if (wicStride == outputStride)
                {
                    // If the WIC image stride matches the output image stride
                    // we can write the buffer directly.

                    const size_t imageDataLength = static_cast<size_t>(rowCount) * wicStride;

                    OSErr err = WriteFile(file, bufferStart, imageDataLength);

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

            return noErr;
        }

        bool TryCalculateCopyBufferSize(
            const int32& imageHeight,
            const UINT& wicStride,
            int32& chunkHeight,
            UINT& copyBufferSize)
        {
            chunkHeight = std::min(imageHeight, 256);
            uint64 bufferSize = static_cast<uint64>(chunkHeight) * wicStride;

            // WIC can only copy up to 4 GB at a time.
            while (bufferSize > std::numeric_limits<UINT>::max())
            {
                chunkHeight /= 2;
                bufferSize = static_cast<uint64>(chunkHeight) * wicStride;
            }

            if (bufferSize > 0 && bufferSize <= std::numeric_limits<UINT>::max())
            {
                copyBufferSize = static_cast<UINT>(bufferSize);
                return true;
            }
            else
            {
                return false;
            }
        }

        bool TryCalculateWICStride(
            const int32& imageWidth,
            const int32& numberOfChannels,
            const int32& bitsPerChannel,
            UINT& wicStride)
        {
            const uint64 bitsPerPixel = static_cast<uint64>(numberOfChannels) * bitsPerChannel;
            const uint64 imageStride = ((static_cast<int64>(imageWidth) * bitsPerPixel) + 7) / 8;

            if (imageStride <= std::numeric_limits<UINT>::max())
            {
                wicStride = static_cast<UINT>(imageStride);
                return true;
            }
            else
            {
                return false;
            }
        }

        IWICBitmapSource* image;
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

        WICPixelFormatGUID targetFormat = GetTargetFormat(format, bitsPerChannel, numberOfChannels);

        if (IsEqualGUID(format, targetFormat))
        {
            GmicOutputWriter writer(decoderFrame.get());

            OSErrException::ThrowIfError(WritePixelsFromCallback(
                static_cast<int32>(uiWidth),
                static_cast<int32>(uiHeight),
                numberOfChannels,
                bitsPerChannel,
                &GmicOutputWriter::WriteCallback,
                &writer,
                output));
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

            GmicOutputWriter writer(formatConverter.get());

            OSErrException::ThrowIfError(WritePixelsFromCallback(
                static_cast<int32>(uiWidth),
                static_cast<int32>(uiHeight),
                numberOfChannels,
                bitsPerChannel,
                &GmicOutputWriter::WriteCallback,
                &writer,
                output));
        }
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
