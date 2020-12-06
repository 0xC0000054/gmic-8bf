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

#include "GmicPlugin.h"
#include <codecvt>
#include <locale>
#include <string>
#include <boost/predef.h>

namespace
{
    // Define the required suite versions and minimum callback routine counts here.
    // This allows the plug-in to work in 3rd party hosts that do not have access to the post-6.0 Photoshop SDKs.

    constexpr int16 RequiredBufferProcsVersion = 2;
    constexpr int16 RequiredBufferProcsCount = 5;

    constexpr int16 RequiredChannelPortsProcsVersion = 1;
    constexpr int16 RequiredChannelPortsProcsCount = 3;

    constexpr int16 RequiredHandleProcsVersion = 1;
    constexpr int16 RequiredHandleProcsCount = 6;

    constexpr int16 RequiredPropertyProcsVersion = 1;
    constexpr int16 RequiredPropertyProcsCount = 2;

    constexpr int16 RequiredReadChannelDescVersion = 0;

    constexpr int16 ReadLayerDescFirstVersion = 0;

    constexpr int16 ReadImageDocumentDescFirstVersion = 0;
    constexpr int16 RequiredReadImageDocumentDescVersion = 2;


    bool IsAffinityPhoto(const FilterRecord* filterRecord) noexcept
    {
        return (filterRecord->hostSig == 'AffP' || filterRecord->hostSig == 'PffA');
    }

    // Adapted from PIUtilities.cpp in the Photoshop 6 SDK
    //-------------------------------------------------------------------------------
    //
    //	HostBufferProcsAvailable
    //
    //	Determines whether the BufferProcs callback is available.
    //
    //	Inputs:
    //		FilterRecord* filterRecord	Pointer to FilterRecord structure.
    //
    //	Outputs:
    //
    //		returns TRUE				If the BufferProcs callback is available.
    //		returns FALSE				If the BufferProcs callback is absent.
    //
    //-------------------------------------------------------------------------------

    bool HostBufferProcsAvailable(const FilterRecord* filterRecord) noexcept
    {
        const BufferProcs* procs = filterRecord->bufferProcs;
#if DEBUG_BUILD
        if (procs != nullptr)
        {
            DebugOut("bufferProcsVersion=%d numBufferProcs=%d allocateProc=%p lockProc=%p unlockProc=%p freeProc=%p spaceProc=%p",
                procs->bufferProcsVersion,
                procs->numBufferProcs,
                procs->allocateProc,
                procs->freeProc,
                procs->lockProc,
                procs->unlockProc,
                procs->spaceProc);
        }
        else
        {
            DebugOut("BufferProcs == nullptr");
        }
#endif // DEBUG_BUILD


        bool available = true;		// assume docInfo are available

        // We want to check for this stuff in a logical order, going from things
        // that should always be present to things that "may" be present.  It's
        // always a danger checking things that "may" be present because some
        // hosts may not leave them NULL if unavailable, instead pointing to
        // other structures to save space.  So first we'll check the main
        // proc pointer, then the version, the number of routines, then some
        // of the actual routines:

        // Affinity Photo populates all the callback pointers, but sets the bufferProcsVersion and numBufferProcs fields to zero.
        const bool isAffinityPhoto = IsAffinityPhoto(filterRecord);

        if (procs == nullptr)
        {
            available = false;
        }
        else if (!isAffinityPhoto && procs->bufferProcsVersion != RequiredBufferProcsVersion)
        {
            available = false;
        }
        else if (!isAffinityPhoto && procs->numBufferProcs < RequiredBufferProcsCount)
        {
            available = false;
        }
        else if (procs->allocateProc == nullptr ||
            procs->lockProc == nullptr ||
            procs->unlockProc == nullptr ||
            procs->freeProc == nullptr ||
            procs->spaceProc == nullptr)
        {
            available = false;
        }

        return available;

    } // end HostBufferProcsAvailable

    //-------------------------------------------------------------------------------
    //
    //	HostHandleProcsAvailable
    //
    //	Determines whether the HandleProcs callback is available.
    //
    //	The HandleProcs are cross-platform master pointers that point to
    //	pointers that point to data that is allocated in the Photoshop
    //	virtual memory structure.  They're reference counted and
    //	managed more efficiently than the operating system calls.
    //
    //	WARNING:  Do not mix operating system handle creation, deletion,
    //			  and sizing routines with these callback routines.  They
    //			  operate differently, allocate memory differently, and,
    //			  while you won't crash, you can cause memory to be
    //			  allocated on the global heap and never deallocated.
    //
    //	Inputs:
    //		FilterRecord* filterRecord	Pointer to FilterRecord structure.
    //
    //	Outputs:
    //
    //		returns TRUE				If the HandleProcs callback is available.
    //		returns FALSE				If the HandleProcs callback is absent.
    //
    //-------------------------------------------------------------------------------

    bool HostHandleProcsAvailable(const FilterRecord* filterRecord) noexcept
    {
        const HandleProcs* procs = filterRecord->handleProcs;
#if DEBUG_BUILD
        if (procs != nullptr)
        {
            DebugOut("handleProcsVersion=%d numHandleProcs=%d newProc=%p disposeProc=%p getSizeProc=%p setSizeProc=%p lockProc=%p unlockProc=%p",
                procs->handleProcsVersion,
                procs->numHandleProcs,
                procs->newProc,
                procs->disposeProc,
                procs->getSizeProc,
                procs->setSizeProc,
                procs->lockProc,
                procs->unlockProc);
        }
        else
        {
            DebugOut("HandleProcs == nullptr");
        }
#endif // DEBUG_BUILD

        Boolean available = true;		// assume docInfo are available

        // We want to check for this stuff in a logical order, going from things
        // that should always be present to things that "may" be present.  It's
        // always a danger checking things that "may" be present because some
        // hosts may not leave them NULL if unavailable, instead pointing to
        // other structures to save space.  So first we'll check the main
        // proc pointer, then the version, the number of routines, then some
        // of the actual routines:

        // Affinity Photo populates all the callback pointers, but sets the handleProcsVersion and numHandleProcs fields to zero.
        const bool isAffinityPhoto = IsAffinityPhoto(filterRecord);

        if (procs == nullptr)
        {
            available = false;
        }
        else if (!isAffinityPhoto && procs->handleProcsVersion != RequiredHandleProcsVersion)
        {
            available = false;
        }
        else if (!isAffinityPhoto && procs->numHandleProcs < RequiredHandleProcsCount)
        {
            available = false;
        }
        else if (procs->newProc == nullptr ||
            procs->disposeProc == nullptr ||
            procs->getSizeProc == nullptr ||
            procs->setSizeProc == nullptr ||
            procs->lockProc == nullptr ||
            procs->unlockProc == nullptr)
        {
            available = false;
        }

        return available;

    } // end HostHandleProcsAvailable

#ifdef PSSDK_HAS_LAYER_SUPPORT
    //-------------------------------------------------------------------------------
    //
    //	HostChannelPortAvailable
    //
    //	Determines whether the ChannelPortProcs callback is available.
    //
    //	The Channel Port Procs are callbacks designed to simplify
    //	merged image and target access, with built in scaling.
    //	They're used extensively by Selection modules.
    //
    //	Inputs:
    //		FilterRecord* filterRecord	Pointer to FilterRecord structure.
    //
    //	Outputs:
    //		returns TRUE				If ChannelPortProcs is available.
    //		returns FALSE				If ChannelPortProcs is absent.
    //
    //-------------------------------------------------------------------------------

    bool HostChannelPortAvailable(const FilterRecord* filterRecord)
    {
        ChannelPortProcs* procs = filterRecord->channelPortProcs;

#if DEBUG_BUILD
        if (procs != nullptr)
        {
            DebugOut("channelPortProcsVersion=%d numChannelPortProcs=%d readPixelsProc=%p readPortForWritePortProc=%p writeBasePixelsProc=%p",
                procs->channelPortProcsVersion,
                procs->numChannelPortProcs,
                procs->readPixelsProc,
                procs->readPortForWritePortProc,
                procs->writeBasePixelsProc);
        }
        else
        {
            DebugOut("ChannelPortProcs == nullptr");
        }
#endif // DEBUG_BUILD

        bool available = true;		// assume docInfo are available

        // We want to check for this stuff in a logical order, going from things
        // that should always be present to things that "may" be present.  It's
        // always a danger checking things that "may" be present because some
        // hosts may not leave them NULL if unavailable, instead pointing to
        // other structures to save space.  So first we'll check the main
        // proc pointer, then the version, the number of routines, then some
        // of the actual routines:

        if (procs == NULL)
        {
            available = false;
        }
        else if (procs->channelPortProcsVersion != RequiredChannelPortsProcsVersion)
        {
            available = false;
        }
        else if (procs->numChannelPortProcs != RequiredChannelPortsProcsCount)
        {
            available = false;
        }
        else if (procs->readPixelsProc == nullptr ||
            procs->writeBasePixelsProc == nullptr ||
            procs->readPortForWritePortProc == nullptr)
        {
            available = false;
        }

        return available;

    } // end HostChannelPortAvailable

    bool ReadChannelDescSupported(const ReadChannelDesc* readChannelDesc)
    {
        bool result = true;

        if (readChannelDesc == nullptr)
        {
            result = false;
        }
        else if (readChannelDesc->minVersion != RequiredReadChannelDescVersion)
        {
            result = false;
        }

        return result;
    }

    bool ReadLayerDescSupported(const ReadLayerDesc* layersDescriptor)
    {
#if DEBUG_BUILD
        if (layersDescriptor != nullptr)
        {
            DebugOut("ReadLayerDesc: minVersion=%d maxVersion=%d",
                layersDescriptor->minVersion,
                layersDescriptor->maxVersion);
        }
        else
        {
            DebugOut("ReadLayerDesc == nullptr");
        }
#endif // DEBUG_BUILD

        bool result = true;

        if (layersDescriptor == nullptr)
        {
            result = false;
        }
        else if (layersDescriptor->minVersion != ReadLayerDescFirstVersion)
        {
            result = false;
        }
        else if (layersDescriptor->compositeChannelsList == nullptr)
        {
            result = false;
        }
        else
        {
            // Walk the compositeChannelsList and ensure the ReadChannelDesc
            // structures are supported.
            ReadChannelDesc* compositeChannel = layersDescriptor->compositeChannelsList;

            while (compositeChannel != nullptr && result)
            {
                result &= ReadChannelDescSupported(compositeChannel);

                compositeChannel = compositeChannel->next;
            }

            // For layer transparency, only the first channel need to be checked.
            if (result && layersDescriptor->transparency != nullptr)
            {
                result &= ReadChannelDescSupported(layersDescriptor->transparency);
            }
        }

        return result;
    }

    bool ReadImageDocumentSupportsLayers(const FilterRecord* filterRecord)
    {
        const ReadImageDocumentDesc* docInfo = filterRecord->documentInfo;
#if DEBUG_BUILD
        if (docInfo != nullptr)
        {
            DebugOut("ReadImageDocumentDesc: minVersion=%d maxVersion=%d",
                docInfo->minVersion,
                docInfo->maxVersion);
        }
        else
        {
            DebugOut("ReadImageDocumentDesc == nullptr");
        }
#endif // DEBUG_BUILD

        bool result = true;

        if (docInfo == nullptr)
        {
            result = false;
        }
        else if (docInfo->minVersion != ReadImageDocumentDescFirstVersion)
        {
            result = false;
        }
        else if (docInfo->maxVersion < RequiredReadImageDocumentDescVersion)
        {
            result = false;
        }
        else
        {
            result = ReadLayerDescSupported(docInfo->layersDescriptor);
        }

        return result;
    }
#endif // PSSDK_HAS_LAYER_SUPPORT

    //-------------------------------------------------------------------------------
    //
    //	HostPropertyProcsAvailable
    //
    //	Determines whether the Property suite of callbacks is available.
    //
    //	The Property suite callbacks are two callbacks, GetProperty and
    //	SetProperty, that manage a list of different data elements.  See
    //	PIProperties.h.
    //
    //	Inputs:
    //		FilterRecord* filterRecord	Pointer to FilterRecord structure.
    //
    //	Outputs:
    //
    //		returns TRUE				If the Property suite is available.
    //		returns FALSE				If the Property suite is absent.
    //
    //-------------------------------------------------------------------------------

    bool HostPropertyProcsAvailable(const FilterRecord* filterRecord) noexcept
    {
        const PropertyProcs* procs = filterRecord->propertyProcs;
#if DEBUG_BUILD
        if (procs != nullptr)
        {
            DebugOut("propertyProcsVersion=%d numPropertyProcs=%d getPropertyProc=%p setPropertyProc=%p",
                procs->propertyProcsVersion,
                procs->numPropertyProcs,
                procs->getPropertyProc,
                procs->setPropertyProc);
        }
        else
        {
            DebugOut("PropertyProcs == nullptr");
        }
#endif // DEBUG_BUILD

        // Affinity Photo populates all the callback pointers, but sets the propertyProcsVersion and numPropertyProcs fields to zero.
        const bool isAffinityPhoto = IsAffinityPhoto(filterRecord);

        bool available = true;

        if (procs == nullptr)
        {
            available = false;
        }
        else if (!isAffinityPhoto && procs->propertyProcsVersion != RequiredPropertyProcsVersion)
        {
            available = false;
        }
        else if (!isAffinityPhoto && procs->numPropertyProcs < RequiredPropertyProcsCount)
        {
            available = false;
        }
        else if (procs->getPropertyProc == nullptr ||
            procs->setPropertyProc == nullptr)
        {
            available = false;
        }

        return available;
    }

    bool PropertySuiteIsAvailable(const FilterRecord* filterRecord) noexcept
    {
        static bool propertySuiteAvailable = HostPropertyProcsAvailable(filterRecord);

        return propertySuiteAvailable;
    }
}


#if DEBUG_BUILD
void DebugOut(const char* fmt, ...) noexcept
{
#if __PIWin__
    va_list argp;
    char dbg_out[4096] = {};

    va_start(argp, fmt);
    vsprintf_s(dbg_out, fmt, argp);
    va_end(argp);

    OutputDebugStringA(dbg_out);
    OutputDebugStringA("\n");
#else
#error "Debug output has not been configured for this platform."
#endif // __PIWin__
}

std::string FourCCToString(const uint32 fourCC)
{
    std::string value(4, '\0');

    const char* sigPtr = reinterpret_cast<const char*>(&fourCC);

#if BOOST_ENDIAN_BIG_BYTE
    value.insert(0, 1, sigPtr[0]);
    value.insert(1, 1, sigPtr[1]);
    value.insert(2, 1, sigPtr[2]);
    value.insert(3, 1, sigPtr[3]);
#elif BOOST_ENDIAN_LITTLE_BYTE
    value.insert(0, 1, sigPtr[3]);
    value.insert(1, 1, sigPtr[2]);
    value.insert(2, 1, sigPtr[1]);
    value.insert(3, 1, sigPtr[0]);
#else
#error "Unknown endianness on this platform."
#endif

    return value;
}
#endif // DEBUG_BUILD

bool TryGetLayerNameAsUTF8String(const FilterRecord* filterRecord, std::string& utf8LayerName)
{
    bool result = false;

#if PSSDK_HAS_LAYER_SUPPORT
    int32 targetLayerIndex;

    if (TryGetTargetLayerIndex(filterRecord, targetLayerIndex))
    {
        Handle complexProperty = nullptr;

        if (filterRecord->propertyProcs->getPropertyProc(
            kPhotoshopSignature,
            propUnicodeLayerName,
            targetLayerIndex,
            nullptr,
            &complexProperty) == noErr)
        {
            if (complexProperty != nullptr)
            {
                Ptr data = filterRecord->handleProcs->lockProc(complexProperty, false);

                if (data != nullptr)
                {
                    utf8LayerName = ConvertLayerNameToUTF8(reinterpret_cast<uint16*>(data));
                    result = !utf8LayerName.empty();
                }

                filterRecord->handleProcs->unlockProc(complexProperty);
                filterRecord->handleProcs->disposeProc(complexProperty);
            }
        }
    }
#endif // PSSDK_HAS_LAYER_SUPPORT

    return result;
}

bool HostMeetsRequirements(const FilterRecord* filterRecord) noexcept
{
    if (filterRecord->advanceState != nullptr &&
        HostBufferProcsAvailable(filterRecord) &&
        HostHandleProcsAvailable(filterRecord))
    {
        return true;
    }

    return false;
}

Fixed int2fixed(int value)
{
    return value << 16;
}

VPoint GetImageSize(const FilterRecordPtr filterRecord)
{
    if (filterRecord->bigDocumentData != nullptr && filterRecord->bigDocumentData->PluginUsing32BitCoordinates)
    {
        return filterRecord->bigDocumentData->imageSize32;
    }
    else
    {
        VPoint point{};

        point.h = filterRecord->imageSize.h;
        point.v = filterRecord->imageSize.v;

        return point;
    }
}

void SetInputRect(FilterRecordPtr filterRecord, int32 top, int32 left, int32 bottom, int32 right)
{
    if (filterRecord->bigDocumentData != nullptr && filterRecord->bigDocumentData->PluginUsing32BitCoordinates)
    {
        filterRecord->bigDocumentData->inRect32.top = top;
        filterRecord->bigDocumentData->inRect32.left = left;
        filterRecord->bigDocumentData->inRect32.bottom = bottom;
        filterRecord->bigDocumentData->inRect32.right = right;
    }
    else
    {
        filterRecord->inRect.top = static_cast<int16>(top);
        filterRecord->inRect.left = static_cast<int16>(left);
        filterRecord->inRect.bottom = static_cast<int16>(bottom);
        filterRecord->inRect.right = static_cast<int16>(right);
    }
}

void SetOutputRect(FilterRecordPtr filterRecord, int32 top, int32 left, int32 bottom, int32 right)
{
    if (filterRecord->bigDocumentData != nullptr && filterRecord->bigDocumentData->PluginUsing32BitCoordinates)
    {
        filterRecord->bigDocumentData->outRect32.top = top;
        filterRecord->bigDocumentData->outRect32.left = left;
        filterRecord->bigDocumentData->outRect32.bottom = bottom;
        filterRecord->bigDocumentData->outRect32.right = right;
    }
    else
    {
        filterRecord->outRect.top = static_cast<int16>(top);
        filterRecord->outRect.left = static_cast<int16>(left);
        filterRecord->outRect.bottom = static_cast<int16>(bottom);
        filterRecord->outRect.right = static_cast<int16>(right);
    }
}

void SetMaskRect(FilterRecordPtr filterRecord, int32 top, int32 left, int32 bottom, int32 right)
{
    if (filterRecord->bigDocumentData != nullptr && filterRecord->bigDocumentData->PluginUsing32BitCoordinates)
    {
        filterRecord->bigDocumentData->maskRect32.top = top;
        filterRecord->bigDocumentData->maskRect32.left = left;
        filterRecord->bigDocumentData->maskRect32.bottom = bottom;
        filterRecord->bigDocumentData->maskRect32.right = right;
    }
    else
    {
        filterRecord->maskRect.top = static_cast<int16>(top);
        filterRecord->maskRect.left = static_cast<int16>(left);
        filterRecord->maskRect.bottom = static_cast<int16>(bottom);
        filterRecord->maskRect.right = static_cast<int16>(right);
    }
}

#if PSSDK_HAS_LAYER_SUPPORT
std::string ConvertLayerNameToUTF8(const uint16* layerName)
{
    std::wstring_convert<std::codecvt_utf8_utf16<uint16>, uint16> convert;
    return convert.to_bytes(layerName);
}

bool HostSupportsReadingFromMultipleLayers(const FilterRecord* filterRecord)
{
    if (HostChannelPortAvailable(filterRecord) && ReadImageDocumentSupportsLayers(filterRecord))
    {
        return true;
    }

    return false;
}

bool DocumentHasMultipleLayers(const FilterRecord* filterRecord)
{
    if (PropertySuiteIsAvailable(filterRecord))
    {
        intptr_t numberOfLayers;

        if (filterRecord->propertyProcs->getPropertyProc(kPhotoshopSignature, propNumberOfLayers, 0, &numberOfLayers, nullptr) == noErr)
        {
            // Photoshop considers a document with only a single "Background" layer as having zero layers.
            return numberOfLayers >= 2;
        }
    }

    return false;
}

bool TryGetTargetLayerIndex(const FilterRecord* filterRecord, int32& targetLayerIndex)
{
    if (PropertySuiteIsAvailable(filterRecord))
    {
        intptr_t layerIndex;

        if (filterRecord->propertyProcs->getPropertyProc(kPhotoshopSignature, propTargetLayerIndex, 0, &layerIndex, nullptr) == noErr)
        {
            targetLayerIndex = static_cast<int32>(layerIndex);
            return targetLayerIndex >= 0;
        }
    }

    targetLayerIndex = 0;
    return false;
}
#endif // PSSDK_HAS_LAYER_SUPPORT
