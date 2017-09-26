# Mod Organizer

Mod Organizer (MO) is a tool for managing mod collections of arbitrary 
size. It is specifically designed for people who like to experiment
with mods and thus need an easy and reliable way to install and
uninstall them.

## About This Branch
This branch is a WIP and is not ready to be used or built by others. Right now mostly refactoring and backend improvements/replacements.

Use at your own risk.

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
