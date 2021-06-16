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

#include "GmicQtParameters.h"
#include "GmicPluginTerminology.h"
#include "FileUtil.h"
#include "ScopedBufferSuite.h"
#include "Utilities.h"
#include <boost/predef.h>
#include <codecvt>

namespace
{
    struct GmicQtParametersHeader
    {
        GmicQtParametersHeader()
        {
            // G8FP = GMIC 8BF filter parameters
            signature[0] = 'G';
            signature[1] = '8';
            signature[2] = 'F';
            signature[3] = 'P';
#if BOOST_ENDIAN_BIG_BYTE
            endian[0] = 'B';
            endian[1] = 'E';
            endian[2] = 'D';
            endian[3] = 'N';
#elif BOOST_ENDIAN_LITTLE_BYTE
            endian[0] = 'L';
            endian[1] = 'E';
            endian[2] = 'D';
            endian[3] = 'N';
#else
#error "Unknown endianness on this platform."
#endif
            version = 1;
        }

        GmicQtParametersHeader(const FileHandle* fileHandle) : version()
        {
            ReadFile(fileHandle, this, sizeof(*this));

            if (strncmp(signature, "G8FP", 4) != 0)
            {
                throw std::runtime_error("The GmicQt parameters file has an invalid file signature.");
            }

#if BOOST_ENDIAN_BIG_BYTE
            const char* const platformEndian = "BEDN";
#elif BOOST_ENDIAN_LITTLE_BYTE
            const char* const platformEndian = "LEDN";
#else
#error "Unknown endianness on this platform."
#endif

            if (strncmp(endian, platformEndian, 4) != 0)
            {
                throw std::runtime_error("The GmicQt parameters file endianess does not match the current platform.");
            }
        }

        char signature[4];
        char endian[4];
        int32_t version;
    };

    void ReadUtf8String(const FileHandle* fileHandle, std::string& value)
    {
        int32_t stringLength = 0;

        ReadFile(fileHandle, &stringLength, sizeof(stringLength));

        if (stringLength == 0)
        {
            value = std::string();
        }
        else
        {
            std::vector<char> stringChars(stringLength);

            ReadFile(fileHandle, &stringChars[0], stringLength);

            value.assign(stringChars.begin(), stringChars.end());
        }
    }

    void WriteUtf8String(const FileHandle* fileHandle, const std::string& value)
    {
        // Check that the required byte buffer size can fit in a uint32_t.
        if (value.size() > static_cast<size_t>(std::numeric_limits<int32_t>::max()))
        {
            throw std::runtime_error("The string cannot be written to the file because it is too long.");
        }

        int32_t stringLength = static_cast<int32_t>(value.size());

        WriteFile(fileHandle, &stringLength, sizeof(stringLength));

        if (value.size() > 0)
        {
            WriteFile(fileHandle, value.c_str(), stringLength);
        }
    }

    OSErr ConvertUtf8StringToASUnicode(const std::string& utf8Str, std::vector<ASUnicode>& unicodeChars)
    {
        OSErr err = noErr;

        try
        {
            std::wstring_convert<std::codecvt<ASUnicode, char, std::mbstate_t>, ASUnicode> convert;

            auto unicodeStr = convert.from_bytes(utf8Str);

            unicodeChars = std::vector<ASUnicode>(unicodeStr.size());
            size_t unicodeCharsLengthInBytes = unicodeChars.size() * sizeof(ASUnicode);

            std::memcpy(unicodeChars.data(), unicodeStr.c_str(), unicodeCharsLengthInBytes);
        }
        catch (const std::bad_alloc&)
        {
            err = memFullErr;
        }
        catch (...)
        {
            err = paramErr;
        }

        return err;
    }

    OSErr PutDescriptorString(
        ASZStringSuite* zstringSuite,
        PSActionDescriptorProcs* descriptorProcs,
        PIActionDescriptor descriptor,
        DescriptorKeyID key,
        const std::string& utf8Str)
    {
        std::vector<ASUnicode> unicodeChars;

        OSErr err = ConvertUtf8StringToASUnicode(utf8Str, unicodeChars);

        if (err == noErr)
        {
            ASZString zstr;

            ASErr zstringErr = zstringSuite->MakeFromUnicode(unicodeChars.data(), unicodeChars.size(), &zstr);

            if (zstringErr == kASNoError)
            {
                err = descriptorProcs->PutZString(descriptor, key, zstr);

                zstringSuite->Release(zstr);
            }
            else
            {
                switch (zstringErr)
                {
                case kSPOutOfMemoryError:
                    err = memFullErr;
                    break;
                default:
                    err = paramErr;
                    break;
                }
            }
        }

        return err;
    }

    bool ActionDescriptiorSuiteSupported(const FilterRecordPtr filterRecord)
    {
        bool result = false;

        if (SPBasicSuiteIsAvailable(filterRecord))
        {
            PSActionDescriptorProcs* suite = nullptr;

            SPErr err = filterRecord->sSPBasic->AcquireSuite(
                kPSActionDescriptorSuite,
                2,
                const_cast<const void**>(reinterpret_cast<void**>(&suite)));

            if (err == kSPNoError)
            {
                // Ensure that the structure function pointer are not null.
                if (suite != nullptr &&
                    suite->Make != nullptr &&
                    suite->Free != nullptr &&
                    suite->HandleToDescriptor != nullptr &&
                    suite->AsHandle != nullptr &&
                    suite->GetZString != nullptr &&
                    suite->PutZString != nullptr &&
                    suite->GetDataLength != nullptr &&
                    suite->GetData != nullptr &&
                    suite->PutData != nullptr)
                {
                    result = true;
                }

                filterRecord->sSPBasic->ReleaseSuite(kPSActionDescriptorSuite, 2);
            }
        }

        return result;
    }

    bool ASZStringSuiteSupported(const FilterRecordPtr filterRecord)
    {
        bool result = false;

        if (SPBasicSuiteIsAvailable(filterRecord))
        {
            ASZStringSuite* suite = nullptr;

            SPErr err = filterRecord->sSPBasic->AcquireSuite(
                kASZStringSuite,
                1,
                const_cast<const void**>(reinterpret_cast<void**>(&suite)));

            if (err == kSPNoError)
            {
                // Ensure that the structure function pointer are not null.
                if (suite != nullptr &&
                    suite->MakeFromUnicode != nullptr &&
                    suite->Release != nullptr &&
                    suite->LengthAsCString != nullptr &&
                    suite->AsCString != nullptr)
                {
                    result = true;
                }

                filterRecord->sSPBasic->ReleaseSuite(kASZStringSuite, 1);
            }
        }

        return result;
    }

    bool HostSupportsActionDescriptiorSuites(const FilterRecordPtr filterRecord)
    {
        bool result = false;

        if (SPBasicSuiteIsAvailable(filterRecord))
        {
            result = ActionDescriptiorSuiteSupported(filterRecord) && ASZStringSuiteSupported(filterRecord);
        }

        return result;
    }

    bool ActionDescriptorSuitesAreAvailable(const FilterRecordPtr filterRecord)
    {
        static bool suitesAvailable = HostSupportsActionDescriptiorSuites(filterRecord);

        return suitesAvailable;
    }

    struct FilterOpaqueDataHeader
    {
        int32_t commandLength;
        int32_t menuPathLength;
    };
}

GmicQtParameters::GmicQtParameters(const FilterRecordPtr filterRecord)
    : command(), filterMenuPath(), inputMode("Active Layer"), filterName()
{
    if (filterRecord->descriptorParameters != nullptr &&
        filterRecord->descriptorParameters->descriptor != nullptr &&
        ActionDescriptorSuitesAreAvailable(filterRecord))
    {
        PSActionDescriptorProcs* descriptorProcs = nullptr;

        SPErr err = filterRecord->sSPBasic->AcquireSuite(
            kPSActionDescriptorSuite,
            2,
            const_cast<const void**>(reinterpret_cast<void**>(&descriptorProcs)));

        if (err == kSPNoError)
        {
            ASZStringSuite* zstringSuite = nullptr;

            err = filterRecord->sSPBasic->AcquireSuite(
                kASZStringSuite,
                1,
                const_cast<const void**>(reinterpret_cast<void**>(&zstringSuite)));

            if (err == kSPNoError)
            {
                PIActionDescriptor descriptor;

                if (descriptorProcs->HandleToDescriptor(filterRecord->descriptorParameters->descriptor, &descriptor) == noErr)
                {
                    Boolean hasKey;

                    if (descriptorProcs->HasKey(descriptor, keyFilterInputMode, &hasKey) == noErr && hasKey)
                    {
                        ReadFilterInputMode(zstringSuite, descriptorProcs, descriptor);
                    }

                    if (descriptorProcs->HasKey(descriptor, keyFilterOpaqueData, &hasKey) == noErr && hasKey)
                    {
                        ReadFilterOpaqueData(filterRecord, descriptorProcs, descriptor);
                    }

                    // G'MIC-Qt does not need the filter name.

                    descriptorProcs->Free(descriptor);
                }

                filterRecord->sSPBasic->ReleaseSuite(kASZStringSuite, 1);
            }

            filterRecord->sSPBasic->ReleaseSuite(kPSActionDescriptorSuite, 2);
        }
    }
}

GmicQtParameters::GmicQtParameters(const boost::filesystem::path& path)
{
    std::unique_ptr<FileHandle> file = OpenFile(path, FileOpenMode::Read);
    GmicQtParametersHeader header(file.get());

    ReadUtf8String(file.get(), command);
    ReadUtf8String(file.get(), filterMenuPath);
    ReadUtf8String(file.get(), inputMode);
    ReadUtf8String(file.get(), filterName);
}

GmicQtParameters::~GmicQtParameters()
{
}

bool GmicQtParameters::IsValid() const
{
    return !command.empty();
}

void GmicQtParameters::SaveToDescriptor(FilterRecordPtr filterRecord) const
{
    if (filterRecord->descriptorParameters != nullptr &&
        ActionDescriptorSuitesAreAvailable(filterRecord))
    {
        PSActionDescriptorProcs* descriptorProcs = nullptr;

        SPErr err = filterRecord->sSPBasic->AcquireSuite(
            kPSActionDescriptorSuite,
            2,
            const_cast<const void**>(reinterpret_cast<void**>(&descriptorProcs)));

        if (err == kSPNoError)
        {
            ASZStringSuite* zstringSuite = nullptr;

            err = filterRecord->sSPBasic->AcquireSuite(
                kASZStringSuite,
                1,
                const_cast<const void**>(reinterpret_cast<void**>(&zstringSuite)));

            if (err == kSPNoError)
            {
                PIActionDescriptor descriptor;

                if (descriptorProcs->Make(&descriptor) == noErr)
                {
                    if (PutDescriptorString(zstringSuite, descriptorProcs, descriptor, keyFilterName, filterName) == noErr &&
                        PutDescriptorString(zstringSuite, descriptorProcs, descriptor, keyFilterInputMode, inputMode) == noErr &&
                        WriteFilterOpaqueData(filterRecord, descriptorProcs, descriptor) == noErr)
                    {
                        if (filterRecord->descriptorParameters->descriptor != nullptr)
                        {
                            filterRecord->handleProcs->disposeProc(filterRecord->descriptorParameters->descriptor);
                        }

                        descriptorProcs->AsHandle(descriptor, &filterRecord->descriptorParameters->descriptor);
                    }

                    descriptorProcs->Free(descriptor);
                }

                filterRecord->sSPBasic->ReleaseSuite(kASZStringSuite, 1);
            }

            filterRecord->sSPBasic->ReleaseSuite(kPSActionDescriptorSuite, 2);
        }
    }
}

void GmicQtParameters::SaveToFile(const boost::filesystem::path& path) const
{
    GmicQtParametersHeader header;

    std::unique_ptr<FileHandle> file = OpenFile(path, FileOpenMode::Write);

    WriteFile(file.get(), &header, sizeof(header));
    WriteUtf8String(file.get(), command);
    WriteUtf8String(file.get(), filterMenuPath);
    WriteUtf8String(file.get(), inputMode);
    // G'MIC-Qt does not need the filter name, so write an empty string.
    WriteUtf8String(file.get(), std::string());
}

OSErr GmicQtParameters::ReadFilterInputMode(
    ASZStringSuite* zstringSuite,
    PSActionDescriptorProcs* descriptorProcs,
    PIActionDescriptor descriptor)
{
    OSErr err = noErr;

    try
    {
        ASZString zstr;

        OSErrException::ThrowIfError(descriptorProcs->GetZString(descriptor, keyFilterInputMode, &zstr));

        const ASUInt32 stringLength = zstringSuite->LengthAsCString(zstr);

        if (stringLength == 0)
        {
            // This should never happen, but if it does we just release the allocated ZString.
            // The class constructor already set the input mode to its default value.
            zstringSuite->Release(zstr);
        }
        else
        {
            std::unique_ptr<char[]> buffer(new (std::nothrow) char[stringLength]);

            if (buffer)
            {
                if (zstringSuite->AsCString(zstr, buffer.get(), stringLength, true) == kASNoError)
                {
                    zstringSuite->Release(zstr);

                    ASUInt32 lengthWithoutTerminator = stringLength;

                    // Remove the NUL-terminator from the end of the string.
                    if (buffer.get()[stringLength - 1] == 0)
                    {
                        lengthWithoutTerminator -= 1;
                    }

                    inputMode = std::string(buffer.get(), buffer.get() + lengthWithoutTerminator);
                }
                else
                {
                    zstringSuite->Release(zstr);
                    err = paramErr;
                }
            }
            else
            {
                zstringSuite->Release(zstr);
                err = memFullErr;
            }
        }
    }
    catch (const std::bad_alloc&)
    {
        err = memFullErr;
    }
    catch (const OSErrException& e)
    {
        err = e.GetErrorCode();
    }
    catch (...)
    {
        err = paramErr;
    }

    return err;
}

OSErr GmicQtParameters::ReadFilterOpaqueData(
    const FilterRecordPtr filterRecord,
    PSActionDescriptorProcs* suite,
    PIActionDescriptor descriptor)
{
    OSErr err = noErr;

    try
    {
        int32 dataSize;

        OSErrException::ThrowIfError(suite->GetDataLength(descriptor, keyFilterOpaqueData, &dataSize));

        unique_buffer_suite_buffer scopedBuffer(filterRecord, dataSize);

        char* data = static_cast<char*>(scopedBuffer.Lock());

        OSErrException::ThrowIfError(suite->GetData(descriptor, keyFilterOpaqueData, data));

        FilterOpaqueDataHeader* header = (FilterOpaqueDataHeader*)data;

        command = std::string(data + sizeof(FilterOpaqueDataHeader), header->commandLength);
        filterMenuPath = std::string(data + sizeof(FilterOpaqueDataHeader) + command.size(), header->menuPathLength);
    }
    catch (const std::bad_alloc&)
    {
        err = memFullErr;
    }
    catch (const OSErrException& e)
    {
        err = e.GetErrorCode();
    }
    catch (...)
    {
        err = paramErr;
    }

    return err;
}

OSErr GmicQtParameters::WriteFilterOpaqueData(
    const FilterRecordPtr filterRecord,
    PSActionDescriptorProcs* suite,
    PIActionDescriptor descriptor) const
{
    uint64 dataSize = static_cast<uint64>(sizeof(FilterOpaqueDataHeader)) + command.size() + filterMenuPath.size();

    if (dataSize > static_cast<uint64>(std::numeric_limits<int32>::max()))
    {
        return memFullErr;
    }

    OSErr err = noErr;

    try
    {
        unique_buffer_suite_buffer scopedBuffer(filterRecord, static_cast<int32>(dataSize));

        uint8* data = static_cast<uint8*>(scopedBuffer.Lock());

        FilterOpaqueDataHeader* header = (FilterOpaqueDataHeader*)data;
        header->commandLength = static_cast<int32>(command.size());
        header->menuPathLength = static_cast<int32>(filterMenuPath.size());

        memcpy(data + sizeof(FilterOpaqueDataHeader), command.data(), command.size());
        memcpy(data + sizeof(FilterOpaqueDataHeader) + command.size(), filterMenuPath.data(), filterMenuPath.size());

        OSErrException::ThrowIfError(suite->PutData(descriptor, keyFilterOpaqueData, static_cast<int32>(dataSize), data));
    }
    catch (const OSErrException& e)
    {
        err = e.GetErrorCode();
    }
    catch (...)
    {
        err = paramErr;
    }

    return err;
}
