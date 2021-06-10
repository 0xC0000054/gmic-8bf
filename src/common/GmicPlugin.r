#include "PIDefines.h"

#ifdef __PIMac__
	#include <Carbon.r>
	#include "PIGeneral.r"
	#include "PIUtilities.r"
#elif defined(__PIWin__)
	#define Rez
	#include "PIGeneral.h"

#endif

#include "GmicPluginTerminology.h"

resource 'PiPL' ( 16000, "GmicPlugin", purgeable )
{
	{
		Kind { Filter },
		Name { "G'MIC-Qt..." },
		Category { "GMIC" },
		Version { (latestFilterVersion << 16 ) | latestFilterSubVersion },

		#ifdef __PIMac__
			#if (defined(__i386__))
				CodeMacIntel32 { "Gmic_Entry_Point" },
			#endif
			#if (defined(__ppc__))
				CodeMachOPowerPC { 0, 0, "Gmic_Entry_Point" },
			#endif
		#else
			#if defined(_WIN64)
				CodeWin64X86 { "Gmic_Entry_Point" },
			#else
				CodeWin32X86 { "Gmic_Entry_Point" },
			#endif
		#endif

		HasTerminology
		{
			plugInClassID,				// Class ID
			plugInEventID,				// Event ID
			16000,						// AETE ID
			// Unique string or empty for AppleScript compliant:
			"94310818-5BEB-4DBF-A9D5-6C0FD0B809FA"
		},

		SupportedModes
		{
			noBitmap, doesSupportGrayScale,
			noIndexedColor, doesSupportRGBColor,
			noCMYKColor, noHSLColor,
			noHSBColor, noMultichannel,
			noDuotone, noLABColor
		},

		EnableInfo { "in (PSHOP_ImageMode, RGBMode, GrayScaleMode, RGB48Mode, Gray16Mode)" },

		/* Limit large documents to 100,000 x 100,000 pixels. */
		PlugInMaxSize { 100000, 100000 },

		FilterLayerSupport { doesSupportFilterLayers },

		FilterCaseInfo
		{
			{
				/* Flat data, no selection */
				inStraightData, outStraightData,
				doNotWriteOutsideSelection,
				filtersLayerMasks, worksWithBlankData,
				copySourceToDestination,

				/* Flat data with selection */
				inStraightData, outStraightData,
				writeOutsideSelection,
				filtersLayerMasks, worksWithBlankData,
				copySourceToDestination,

				/* Floating selection */
				inStraightData, outStraightData,
				writeOutsideSelection,
				filtersLayerMasks, worksWithBlankData,
				copySourceToDestination,

				/* Editable transparency, no selection */
				inStraightData, outStraightData,
				doNotWriteOutsideSelection,
				filtersLayerMasks, worksWithBlankData,
				copySourceToDestination,

				/* Editable transparency, with selection */
				inStraightData, outStraightData,
				writeOutsideSelection,
				filtersLayerMasks, worksWithBlankData,
				copySourceToDestination,

				/* Preserved transparency, no selection */
				inStraightData, outStraightData,
				doNotWriteOutsideSelection,
				filtersLayerMasks, worksWithBlankData,
				copySourceToDestination,

				/* Preserved transparency, with selection */
				inStraightData, outStraightData,
				writeOutsideSelection,
				filtersLayerMasks, worksWithBlankData,
				copySourceToDestination
			}
		}
	}
};

resource 'PiPL' ( 16001, "GmicOutputSettingsPlugin", purgeable )
{
	{
		Kind { Filter },
		Name { "Input/Output Settings for G'MIC-Qt..." },
		Category { "GMIC" },
		Version { (latestFilterVersion << 16 ) | latestFilterSubVersion },

		#ifdef __PIMac__
			#if (defined(__i386__))
				CodeMacIntel32 { "Gmic_IO_Settings_Entry_Point" },
			#endif
			#if (defined(__ppc__))
				CodeMachOPowerPC { 0, 0, "Gmic_IO_Settings_Entry_Point" },
			#endif
		#else
			#if defined(_WIN64)
				CodeWin64X86 { "Gmic_IO_Settings_Entry_Point" },
			#else
				CodeWin32X86 { "Gmic_IO_Settings_Entry_Point" },
			#endif
		#endif

		SupportedModes
		{
			noBitmap, doesSupportGrayScale,
			noIndexedColor, doesSupportRGBColor,
			noCMYKColor, noHSLColor,
			noHSBColor, noMultichannel,
			noDuotone, noLABColor
		},

		EnableInfo { "in (PSHOP_ImageMode, RGBMode, GrayScaleMode, RGB48Mode, Gray16Mode)" },

		/* Limit large documents to 100,000 x 100,000 pixels. */
		PlugInMaxSize { 100000, 100000 },

		FilterLayerSupport { doesSupportFilterLayers },

		FilterCaseInfo
		{
			{
				/* Flat data, no selection */
				inStraightData, outStraightData,
				doNotWriteOutsideSelection,
				filtersLayerMasks, worksWithBlankData,
				copySourceToDestination,

				/* Flat data with selection */
				inStraightData, outStraightData,
				writeOutsideSelection,
				filtersLayerMasks, worksWithBlankData,
				copySourceToDestination,

				/* Floating selection */
				inStraightData, outStraightData,
				writeOutsideSelection,
				filtersLayerMasks, worksWithBlankData,
				copySourceToDestination,

				/* Editable transparency, no selection */
				inStraightData, outStraightData,
				doNotWriteOutsideSelection,
				filtersLayerMasks, worksWithBlankData,
				copySourceToDestination,

				/* Editable transparency, with selection */
				inStraightData, outStraightData,
				writeOutsideSelection,
				filtersLayerMasks, worksWithBlankData,
				copySourceToDestination,

				/* Preserved transparency, no selection */
				inStraightData, outStraightData,
				doNotWriteOutsideSelection,
				filtersLayerMasks, worksWithBlankData,
				copySourceToDestination,

				/* Preserved transparency, with selection */
				inStraightData, outStraightData,
				writeOutsideSelection,
				filtersLayerMasks, worksWithBlankData,
				copySourceToDestination
			}
		}
	}
};

//-------------------------------------------------------------------------------
//	Dictionary (scripting) resource
//-------------------------------------------------------------------------------

resource 'aete' (16000, plugInName "GmicPlugin dictionary", purgeable)
{
	1, 0, english, roman,									/* aete version and language specifiers */
	{
		"GmicPlugin developers",							/* vendor suite name */
		"",													/* optional description */
		plugInSuiteID,										/* suite ID */
		1,													/* suite code, must be 1 */
		1,													/* suite level, must be 1 */
		{													/* structure for filters */
			"G'MIC-Qt filter",										/* unique filter name */
			"",												/* optional description */
			plugInClassID,									/* class ID, must be unique or Suite ID */
			plugInEventID,									/* event ID, must be unique to class ID */

			NO_REPLY,										/* never a reply */
			IMAGE_DIRECT_PARAMETER,							/* direct parameter, used by Photoshop */
			{												/* parameters here, if any */
				"Filter Name",								/* parameter name */
				keyFilterName,								/* parameter key ID */
				typeChar,									/* parameter type ID */
				"",											/* optional description */
				flagsSingleParameter,						/* parameter flags */

				"Input Mode",								// second parameter
				keyFilterInputMode,							// parameter key ID
				typeChar,									// parameter type ID
				"",											// optional description
				flagsSingleParameter,						// parameter flags

				"G'MIC-Qt Data",							// third parameter
				keyFilterOpaqueData,						// parameter key ID
				typeRawData,								// parameter type ID
				"",											// optional description
				flagsSingleParameter,						// parameter flags
			}
		},
		{													/* non-filter plug-in class here */
		},
		{													/* comparison ops (not supported) */
		},
		{													/* any enumerations */
		}
	}
};