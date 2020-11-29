# gmic-8bf

A filter plug-in for Adobe® Photoshop®* and other compatible software that interacts with [G'MIC-Qt](https://github.com/c-koi/gmic-qt).

## Installation

1. Close your host application.
2. Place GmicPlugin.8bf and gmic folder in the folder that your host application searches for filter plug-ins.
3. Restart your host application.
4. The plug-in will now be available as the G'MIC-Qt menu item in the GMIC category.

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

