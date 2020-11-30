# gmic-8bf

A filter plug-in for Adobe® Photoshop®* and other compatible software that interacts with [G'MIC-Qt](https://github.com/c-koi/gmic-qt).

The latest version can be downloaded from the [Releases](https://github.com/0xC0000054/gmic-8bf/releases) tab.

## Installation

1. Close your host application.
2. Place GmicPlugin.8bf and gmic folder in the folder that your host application searches for filter plug-ins.
3. Restart your host application.
4. The plug-in will now be available as the G'MIC-Qt menu item in the GMIC category.

## Usage

To start the plug-in select the G'MIC-Qt item in the filter list of your host application.
The following image shows the menu location in Adobe Photoshop.

![Menu Location](images/MenuLocation.png)

If a filter produces multiple output images the plug-in will prompt the user for a folder to copy them into
after the G'MIC-Qt dialog has been closed.

Filters that require multiple layers can only be used if the host supports providing layers to the plug-in.
This feature is supported in Adobe Photoshop CS (8.0) and later (and possibly other Adobe applications), but 3rd-party
hosts will most likely not support it due to the license restrictions added to the Photoshop SDK after version 6.0.

## License

This project is licensed under the terms of the MIT License.   
See [License.txt](License.txt) for more information.

# Source code

## Prerequisites

* Visual Studio 2019
* The boost::filesystem, boost::endian and LibPNG packages from [VCPkg](https://github.com/microsoft/vcpkg).
* The Adobe Photoshop CS6 SDK, see the read-me in the ext folder for more details.

## Building the plugin

* Open the solution
* Update the post build events to copy the build output to the filters folder of your host application
* Build the solution

```
* Adobe and Photoshop are either registered trademarks or trademarks of Adobe Systems Incorporated in the United States and/or other countries.
```

