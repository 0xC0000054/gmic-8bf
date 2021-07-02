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
#include <array>
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

    template<class T> class ScopedPICASuite
    {
    public:
        ScopedPICASuite(SPBasicSuite* basicSuite, const char* name, int32 version) noexcept
            : basicSuite(basicSuite), suite(nullptr), name(name), version(version), suiteValid(false)
        {
            SPErr err = basicSuite->AcquireSuite(
                name,
                version,
                const_cast<const void**>(reinterpret_cast<void**>(&suite)));
            suiteValid = err == kSPNoError && suite != nullptr;
        }

        ~ScopedPICASuite()
        {
            if (suiteValid)
            {
                basicSuite->ReleaseSuite(name, version);
                suiteValid = false;
            }
        }

        T* get() const noexcept
        {
            return suite;
        }

        const T* operator->() const noexcept
        {
            return suite;
        }

        explicit operator bool() const noexcept
        {
            return suiteValid;
        }

    private:

        SPBasicSuite* basicSuite;
        T* suite;
        const char* name;
        int32 version;
        bool suiteValid;
    };

    class ScopedActionDescriptor
    {
    public:
        explicit ScopedActionDescriptor(PSActionDescriptorProcs* descriptorProcs)
            : descriptorProcs(descriptorProcs), descriptor(), descriptorValid(false)
        {
           descriptorValid = descriptorProcs->Make(&descriptor) == noErr;
        }

        ScopedActionDescriptor(PSActionDescriptorProcs* descriptorProcs, PIDescriptorHandle handle)
            : descriptorProcs(descriptorProcs), descriptor(), descriptorValid(false)
        {
            descriptorValid = descriptorProcs->HandleToDescriptor(handle, &descriptor) == noErr;
        }

        ~ScopedActionDescriptor()
        {
            if (descriptorValid)
            {
                descriptorProcs->Free(descriptor);
                descriptorValid = false;
            }
        }

        PIActionDescriptor get() const noexcept
        {
            return descriptor;
        }

        explicit operator bool() const noexcept
        {
            return descriptorValid;
        }

    private:

        PSActionDescriptorProcs* descriptorProcs;
        PIActionDescriptor descriptor;
        bool descriptorValid;
    };

    void ThrowIfASError(ASErr err)
    {
        if (err != kASNoError)
        {
            switch (err)
            {
            case kSPOutOfMemoryError:
                throw OSErrException(memFullErr);
            case kSPUnimplementedError:
                throw OSErrException(errPlugInHostInsufficient);
            default:
                throw OSErrException(paramErr);
            }
        }
    }

    class ScopedASZString
    {
    public:

        ScopedASZString(ASZStringSuite* suite, std::vector<ASUnicode> unicodeChars)
            : zstringSuite(suite), zstr(nullptr), zstringValid(false)
        {
            ThrowIfASError(suite->MakeFromUnicode(unicodeChars.data(), unicodeChars.size(), &zstr));
            zstringValid = true;
        }

        ScopedASZString(
            ASZStringSuite* zstringSuite,
            PSActionDescriptorProcs* descriptorProcs,
            PIActionDescriptor descriptor,
            DescriptorKeyID key)
            : zstringSuite(zstringSuite), zstr(nullptr), zstringValid(false)
        {
            OSErrException::ThrowIfError(descriptorProcs->GetZString(descriptor, key, &zstr));
            zstringValid = true;
        }

        ~ScopedASZString()
        {
            if (zstringValid)
            {
                zstringSuite->Release(zstr);
                zstr = nullptr;
                zstringValid = false;
            }
        }

        ASZString get() const
        {
            return zstr;
        }

    private:

        ASZStringSuite* zstringSuite;
        ASZString zstr;
        bool zstringValid;
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

    std::vector<ASUnicode> ConvertUtf8StringToASUnicode(const std::string& utf8Str)
    {
        std::codecvt_utf8_utf16<ASUnicode> converter;
        std::codecvt_utf8_utf16<ASUnicode>::state_type state = std::codecvt_utf8_utf16<ASUnicode>::state_type();

        const char* first = utf8Str.data();
        const char* last = first + utf8Str.length();

        const int utf16Length = converter.length(state, first, last, utf8Str.length());
        std::vector<ASUnicode> unicodeChars = std::vector<ASUnicode>(utf16Length);

        ASUnicode* dest = unicodeChars.data();
        ASUnicode* destEnd = dest + unicodeChars.size();
        ASUnicode* outputLocation;

        auto result = converter.in(state, first, last, first, dest, destEnd, outputLocation);

        switch (result)
        {
        case std::codecvt_base::ok:
        case std::codecvt_base::partial:
            if (outputLocation != destEnd)
            {
                throw std::runtime_error("Bad UTF-8 to UTF-16 conversion.");
            }
            break;
        case std::codecvt_base::noconv:
            for (size_t i = 0, length = last - first; i < length; i++, first++)
            {
                unicodeChars[i] = static_cast<ASUnicode>(static_cast<unsigned char>(*first));
            }
            break;
        default:
            throw std::runtime_error("Bad UTF-8 to UTF-16 conversion.");
        }

        return unicodeChars;
    }

    void PutDescriptorString(
        ASZStringSuite* zstringSuite,
        PSActionDescriptorProcs* descriptorProcs,
        PIActionDescriptor descriptor,
        DescriptorKeyID key,
        const std::string& utf8Str)
    {
        std::vector<ASUnicode> unicodeChars = ConvertUtf8StringToASUnicode(utf8Str);

        ScopedASZString zstr(zstringSuite, unicodeChars);

        OSErrException::ThrowIfError(descriptorProcs->PutZString(descriptor, key, zstr.get()));
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

        if (HandleSuiteIsAvailable(filterRecord) && SPBasicSuiteIsAvailable(filterRecord))
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
        SPBasicSuite* basicSuite = filterRecord->sSPBasic;

        ScopedPICASuite<PSActionDescriptorProcs> descriptorProcs(basicSuite, kPSActionDescriptorSuite, 2);

        if (descriptorProcs)
        {
            ScopedPICASuite<ASZStringSuite> zstringSuite(basicSuite, kASZStringSuite, 1);

            if (zstringSuite)
            {
                ScopedActionDescriptor descriptor(descriptorProcs.get(), filterRecord->descriptorParameters->descriptor);

                if (descriptor)
                {
                    Boolean hasKey;

                    if (descriptorProcs->HasKey(descriptor.get(), keyFilterInputMode, &hasKey) == noErr && hasKey)
                    {
                        ReadFilterInputMode(zstringSuite.get(), descriptorProcs.get(), descriptor.get());
                    }

                    if (descriptorProcs->HasKey(descriptor.get(), keyFilterOpaqueData, &hasKey) == noErr && hasKey)
                    {
                        ReadFilterOpaqueData(filterRecord, descriptorProcs.get(), descriptor.get());
                    }

                    // G'MIC-Qt does not need the filter name.
                }
            }
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
        SPBasicSuite* basicSuite = filterRecord->sSPBasic;

        ScopedPICASuite<PSActionDescriptorProcs> descriptorProcs(basicSuite, kPSActionDescriptorSuite, 2);

        if (descriptorProcs)
        {
            ScopedPICASuite<ASZStringSuite> zstringSuite(basicSuite, kASZStringSuite, 1);

            if (zstringSuite)
            {
                ScopedActionDescriptor descriptor(descriptorProcs.get());

                if (descriptor)
                {
                    PutDescriptorString(zstringSuite.get(), descriptorProcs.get(), descriptor.get(), keyFilterName, filterName);
                    PutDescriptorString(zstringSuite.get(), descriptorProcs.get(), descriptor.get(), keyFilterInputMode, inputMode);
                    WriteFilterOpaqueData(filterRecord, descriptorProcs.get(), descriptor.get());

                    if (filterRecord->descriptorParameters->descriptor != nullptr)
                    {
                        filterRecord->handleProcs->disposeProc(filterRecord->descriptorParameters->descriptor);
                    }

                    OSErrException::ThrowIfError(descriptorProcs->AsHandle(descriptor.get(), &filterRecord->descriptorParameters->descriptor));
                }
            }
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
        ScopedASZString zstr(zstringSuite, descriptorProcs, descriptor, keyFilterInputMode);

        const ASUInt32 stringLength = zstringSuite->LengthAsCString(zstr.get());

        if (stringLength > 0)
        {
            std::unique_ptr<char[]> buffer = std::make_unique<char[]>(stringLength);

            ThrowIfASError(zstringSuite->AsCString(zstr.get(), buffer.get(), stringLength, true));

            ASUInt32 lengthWithoutTerminator = stringLength;

            // Remove the NUL-terminator from the end of the string.
            if (buffer[stringLength - 1] == 0)
            {
                lengthWithoutTerminator -= 1;
            }

            inputMode = std::string(buffer.get(), buffer.get() + lengthWithoutTerminator);
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

void GmicQtParameters::WriteFilterOpaqueData(
    const FilterRecordPtr filterRecord,
    PSActionDescriptorProcs* suite,
    PIActionDescriptor descriptor) const
{
    uint64 dataSize = static_cast<uint64>(sizeof(FilterOpaqueDataHeader)) + command.size() + filterMenuPath.size();

    if (dataSize > static_cast<uint64>(std::numeric_limits<int32>::max()))
    {
        throw std::runtime_error("The G'MIC-Qt data is larger than 2 GB.");
    }

    unique_buffer_suite_buffer scopedBuffer(filterRecord, static_cast<int32>(dataSize));

    uint8* data = static_cast<uint8*>(scopedBuffer.Lock());

    FilterOpaqueDataHeader* header = (FilterOpaqueDataHeader*)data;
    header->commandLength = static_cast<int32>(command.size());
    header->menuPathLength = static_cast<int32>(filterMenuPath.size());

    memcpy(data + sizeof(FilterOpaqueDataHeader), command.data(), command.size());
    memcpy(data + sizeof(FilterOpaqueDataHeader) + command.size(), filterMenuPath.data(), filterMenuPath.size());

    OSErrException::ThrowIfError(suite->PutData(descriptor, keyFilterOpaqueData, static_cast<int32>(dataSize), data));
}
