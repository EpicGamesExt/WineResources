# Wrapped Installed Build example

This directory contains a [Dockerfile](./context/Dockerfile) and accompanying script for building a container image that extends an [AutoSDK base image](../autosdk/) and adds the files for an [Installed Build](https://dev.epicgames.com/documentation/en-us/unreal-engine/installed-build-reference-guide-for-unreal-engine) of Unreal Engine. It supports Unreal Engine 5.6.0 and newer.


## Contents

- [Prerequisites](#prerequisites)
- [Building the Wine and AutoSDK base images](#building-the-wine-and-autosdk-base-images)
- [Building the container image](#building-the-container-image)
    - [Copying the Installed Build files](#copying-the-installed-build-files)
    - [Running the build script](#running-the-build-script)


## Prerequisites

- Installed Build of Unreal Engine, either copied from a Windows machine (version 5.6.0 or newer) or [built from source under Wine](../create-installed-build/) (version 5.7.0 or newer)
- [Docker Engine](https://docs.docker.com/engine/install/) version 23.0.0 or newer
- [Python](https://www.python.org/) 3.7 or newer


## Building the Wine and AutoSDK base images

The script in this directory will automatically build the Wine and AutoSDK base images if they do not exist. If you wish to build them manually then simply follow the instructions in the relevant README files:

- The README in the [repository's top-level **build** directory](../../../build/README.md) provides instructions for building a base image containing Epic's patched version of Wine.
- The README in the [**autosdk** directory](../autosdk/README.md) provides instructions for building an image that contains the AutoSDK components required by your version of Unreal Engine.


## Building the container image

### Copying the Installed Build files

This example does not build Unreal Engine from source, but rather wraps an existing Installed Build that needs to be supplied by the user. You will either need a Windows machine to download or build an Installed Build of Unreal Engine 5.6.0 or newer, or for Unreal Engine 5.7.0 and newer you can [build the engine from source under Wine](../create-installed-build/) on a Linux machine. Either way, once the files have been obtained then the container image itself can be built under Linux, macOS or Windows.

The recommended way to obtain an Installed Build under Windows is to install Unreal Engine via the Epic Games Launcher, by following these instructions: <https://dev.epicgames.com/documentation/en-us/unreal-engine/installing-unreal-engine>

Alternatively, you can create an Installed Build by building Unreal Engine from source:

- For Unreal Engine 5.6.0 or newer, you can build the engine from source under Windows by following these instructions: <https://dev.epicgames.com/documentation/en-us/unreal-engine/create-an-installed-build-of-unreal-engine>

- For Unreal Engine 5.7.0 or newer, you can build the engine from source under Wine by following the instructions in the [**create-installed-build**](../create-installed-build/) directory.

Once you have obtained the Installed Build files, copy them to the [**context/UnrealEngine**](./context/UnrealEngine/) subdirectory. If the files have been copied correctly then the Unreal Editor executable should exist at the path:

```
context/UnrealEngine/Engine/Binaries/Win64/UnrealEditor.exe
```

> [!WARNING]
> When copying Installed Build files under Windows, you may encounter an error if any of the destination file paths exceed the [maximum path length limit](https://learn.microsoft.com/en-us/windows/win32/fileio/maximum-file-path-limitation). To avoid this, either ensure the destination directory has a short path (e.g. by storing the local copy of this repository in the root of a drive, such as `C:\WineResources`), or follow Microsoft's [instructions to enable long path support](https://learn.microsoft.com/en-us/windows/win32/fileio/maximum-file-path-limitation?#enable-long-paths-in-windows-10-version-1607-and-later).

### Running the build script

Run the appropriate wrapper script depending on the operating system:

- Under Linux and macOS, run `./build.sh`
- Under Windows, run `.\build.bat`

The wrapper script will run the Python build script itself, using the appropriate commands for the operating system. The Python build script will verify that the Installed Build files have been copied to the correct location, and will then automatically detect the version of Unreal Engine that the files represent. The script will then build the container image.

Once the build completes, the container image will be available with the tag `epicgames/unreal-engine:dev-wine-<VERSION>`, where `<VERSION>` is the version of Unreal Engine that the container image was built for. The image can be used to package Unreal Engine projects, either manually or by using the script in the [**package-project**](../package-project/) directory.

> [!IMPORTANT]
> The Dockerfile step that copies the Installed Build files into the container image may take a long time to complete (i.e. over an hour on many systems, and potentially multiple hours under Windows when using Docker Desktop with WSL2). There is no output to indicate the progress of the copy operation, so it may appear to have frozen, but this is almost certainly not the case and you will simply need to wait for it to complete.
