# Mod Organizer

Mod Organizer (MO) is a tool for managing mod collections of arbitrary 
size. It is specifically designed for people who like to experiment
with mods and thus need an easy and reliable way to install and
uninstall them.

## Alternative Download Location

* [ModOrganizer/modorganizer/releases](https://github.com/ModOrganizer/modorganizer/releases)

## Building

* Run CMake, setting PREFIX_PATH to a directory in the structure of

PREFIX_PATH  
....x86  
........bin  
........include  
........lib  
........share  
....x86  
........bin  
........include  
........lib  
........share  

Required DLLs should go in bin
Header files in include
static libraries in lib

QT 5.9 is expected to be installed at C:\Qt, and the 2017 64bit and
2015 32bit versions are expected to be installed as well.

If Qt is installed somewhere else, edit the CMake.

The 32bit versions of are only required if you're building as 32bit for some reason.

## Other Repositories

MO consists of multiple repositories on github. The Umbrella project will download them automatically as required. They should however also be buildable individually.
Here is a complete list:
* https://github.com/TanninOne/modorganizer
* https://github.com/TanninOne/modorganizer-uibase
* https://github.com/TanninOne/modorganizer-hookdll
* https://github.com/TanninOne/modorganizer-bsatk
* https://github.com/TanninOne/modorganizer-esptk
* https://github.com/TanninOne/modorganizer-archive
* https://github.com/TanninOne/modorganizer-nxmhandler
* https://github.com/TanninOne/modorganizer-lootcli
* https://github.com/TanninOne/modorganizer-helper
* https://github.com/TanninOne/modorganizer-tool_nmmimport
* https://github.com/TanninOne/modorganizer-tool_configurator
* https://github.com/TanninOne/modorganizer-tool_inieditor
* https://github.com/TanninOne/modorganizer-preview_base
* https://github.com/TanninOne/modorganizer-diagnose_basic
* https://github.com/TanninOne/modorganizer-installer_quick
* https://github.com/TanninOne/modorganizer-installer_manual
* https://github.com/TanninOne/modorganizer-installer_fomod
* https://github.com/TanninOne/modorganizer-installer_bundle
* https://github.com/TanninOne/modorganizer-installer_bain
* https://github.com/TanninOne/modorganizer-game_skyrim
* https://github.com/TanninOne/modorganizer-game_oblivion
* https://github.com/TanninOne/modorganizer-game_fallout3
* https://github.com/TanninOne/modorganizer-game_fallout4
* https://github.com/TanninOne/modorganizer-game_falloutnv
* https://github.com/TanninOne/modorganizer-game_gamebryo
* https://github.com/TanninOne/modorganizer-game_features
* https://github.com/TanninOne/modorganizer-check_fnis
* https://github.com/TanninOne/modorganizer-plugin_python
* https://github.com/TanninOne/modorganizer-bsa_extractor
* https://github.com/TanninOne/usvfs
