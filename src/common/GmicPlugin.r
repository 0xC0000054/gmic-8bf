#include "PIDefines.h"

#ifdef __PIMac__
	#include <Carbon.r>
	#include "PIGeneral.r"
	#include "PIUtilities.r"
#elif defined(__PIWin__)
	#define Rez
	#include "PIGeneral.h"

#endif

#include "PIActions.h"

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