////////////////////////////////////////////////////////////////////////
//
// This file is part of gmic-8bf, a filter plug-in module that
// interfaces with G'MIC-Qt.
//
// Copyright (c) 2020-2026 Nicholas Hayes
//
// This file is licensed under the MIT License.
// See LICENSE.txt for complete licensing and attribution information.
//
////////////////////////////////////////////////////////////////////////

#include "Utilities.h"
#include "ScopedHandleSuite.h"
#include <SafeInt.hpp>
#include <codecvt>
#include <locale>
#include <string>

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

            // For layer transparency, only the first channel needs to be checked.
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

    bool HostSPBasicSuiteAvailable(SPBasicSuite* proc)
    {
        bool available = true;

        if (proc == nullptr)
        {
            available = false;
        }
        else if (proc->AcquireSuite == nullptr ||
                 proc->ReleaseSuite == nullptr ||
                 proc->IsEqual == nullptr ||
                 proc->AllocateBlock == nullptr ||
                 proc->FreeBlock == nullptr ||
                 proc->ReallocateBlock == nullptr ||
                 proc->Undefined == nullptr)
        {
            available = false;
        }

        return available;
    }
}

bool HandleSuiteIsAvailable(const FilterRecord* filterRecord)
{
    static bool handleProcsAvailable = HostHandleProcsAvailable(filterRecord);

    return handleProcsAvailable;
}

bool SPBasicSuiteIsAvailable(const FilterRecord* filterRecord)
{
    static bool spBasicSuiteAvailable = HostSPBasicSuiteAvailable(filterRecord->sSPBasic);

    return spBasicSuiteAvailable;
}

bool TryGetActiveLayerNameAsUTF8String(const FilterRecord* filterRecord, ::std::string& utf8LayerName)
{
    bool result = false;

#if PSSDK_HAS_LAYER_SUPPORT
    int32 targetLayerIndex;
    try
    {
        if (HandleSuiteIsAvailable(filterRecord) && TryGetTargetLayerIndex(filterRecord, targetLayerIndex))
        {
            ScopedHandleSuiteHandle complexProperty(filterRecord->handleProcs);

            if (filterRecord->propertyProcs->getPropertyProc(
                kPhotoshopSignature,
                propUnicodeLayerName,
                targetLayerIndex,
                nullptr,
                complexProperty.put()) == noErr)
            {
                if (complexProperty)
                {
                    ScopedHandleSuiteLock lock = complexProperty.lock();

                    utf8LayerName = ConvertLayerNameToUTF8(reinterpret_cast<uint16*>(lock.data()));
                    result = !utf8LayerName.empty();
                }
            }
        }
    }
    catch (...)
    {
        // Ignore any exceptions that are thrown when retrieving the layer name or converting it to UTF-8.
        result = false;
    }
#endif // PSSDK_HAS_LAYER_SUPPORT

    return result;
}

bool HostMeetsRequirements(const FilterRecord* filterRecord) noexcept
{
    return filterRecord->advanceState != nullptr && HostBufferProcsAvailable(filterRecord);
}

int32 GetImageDepth(const FilterRecord* filterRecord) noexcept
{
    int32 depth = filterRecord->depth;

    if (depth == 0)
    {
        // The host is not Photoshop 5.0 compatible, try to guess the image depth from the image mode.
        switch (filterRecord->imageMode)
        {
        case plugInModeGrayScale:
        case plugInModeRGBColor:
            depth = 8;
            break;
        case plugInModeGray16:
        case plugInModeRGB48:
            depth = 16;
            break;
        case plugInModeGray32:
        case plugInModeRGB96:
            depth = 32;
            break;
        }
    }

    return depth;
}

int32 GetImagePlaneCount(int16 imageMode, int32 layerPlanes, int32 transparencyPlanes)
{
    int32 imagePlanes;

    switch (imageMode)
    {
    case plugInModeGrayScale:
    case plugInModeGray16:
    case plugInModeGray32:
        imagePlanes = 1;
        break;
    case plugInModeRGBColor:
    case plugInModeRGB48:
    case plugInModeRGB96:
        imagePlanes = 3;
        break;
    default:
        return 0;
    }

    if (layerPlanes == imagePlanes && transparencyPlanes > 0)
    {
        imagePlanes++;
    }

    return imagePlanes;
}

VPoint GetImageSize(const FilterRecordPtr filterRecord)
{
    VPoint imageSize;

    if (filterRecord->bigDocumentData != nullptr && filterRecord->bigDocumentData->PluginUsing32BitCoordinates)
    {
        imageSize = filterRecord->bigDocumentData->imageSize32;
    }
    else
    {
        imageSize.h = filterRecord->imageSize.h;
        imageSize.v = filterRecord->imageSize.v;
    }

    return imageSize;
}

int32 GetTileHeight(int16 suggestedTileHeight)
{
    // Some hosts may use an unsigned value for the tile height
    // so we have to check if it is a positive number.

    return suggestedTileHeight > 0 ? suggestedTileHeight : 1024;
}

int32 GetTileWidth(int16 suggestedTileWidth)
{
    // Some hosts may use an unsigned value for the tile width
    // so we have to check if it is a positive number.

    return suggestedTileWidth > 0 ? suggestedTileWidth : 1024;
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

bool TryMultiplyInt32(int32 a, int32 x, int32& result)
{
    return SafeMultiply(a, x, result);
}

#if PSSDK_HAS_LAYER_SUPPORT
::std::string ConvertLayerNameToUTF8(const uint16* layerName)
{
    ::std::wstring_convert<::std::codecvt_utf8_utf16<uint16>, uint16> convert;
    return convert.to_bytes(layerName);
}

bool HostSupportsReadingFromMultipleLayers(const FilterRecord* filterRecord)
{
    return HostChannelPortAvailable(filterRecord) && ReadImageDocumentSupportsLayers(filterRecord);
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
