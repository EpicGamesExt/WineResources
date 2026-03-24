# AutoSDK enabled image example

This directory contains a [Dockerfile](./context/Dockerfile) and accompanying script for building a container image that extends the [base image containing Epic's patched version of Wine](../../../build/) and adds the Windows compiler toolchain files for use with the engine's [AutoSDK](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-the-autosdk-system-in-unreal-engine) system. The resulting image can be used for building Unreal Engine itself from source, and as the basis for packaging Unreal Engine projects that contain C++ code. The script supports Unreal Engine 5.6.0 and newer.


## Contents

- [Prerequisites](#prerequisites)
- [Building the Wine base image](#building-the-wine-base-image)
- [Building the AutoSDK container image](#building-the-autosdk-container-image)
- [Using the AutoSDK container image](#using-the-autosdk-container-image)


## Prerequisites

> [!IMPORTANT]
> Different releases of Unreal Engine require different versions of the Windows toolchain components, so it is necessary to build an AutoSDK container image for the specific engine version that you are targeting. The Python script in this directory parses the SDK JSON file `Engine/Config/Windows/Windows_SDK.json` from the Unreal Engine source tree to determine which toolchain components to install. The format of this file became compatible with Python's JSON parser in [CL 39200598](https://github.com/EpicGames/UnrealEngine/commit/6f85c6f96fb7cbe7417514125ae64f15f6f2c40c), which is present in Unreal Engine 5.6.0 and newer. Older versions of Unreal Engine are not supported.

- Unreal Engine source code (from either [GitHub](https://www.unrealengine.com/en-US/ue-on-github) or [Perforce](https://dev.epicgames.com/documentation/en-us/unreal-engine/accessing-unreal-engine-with-perforce)), version 5.6.0 or newer
- [Docker Engine](https://docs.docker.com/engine/install/) version 23.0.0 or newer
- [Python](https://www.python.org/) 3.7 or newer


## Building the Wine base image

The script in this directory will automatically build a base image containing Epic's patched version of Wine if it does not exist. If you wish to build the base image manually then simply follow the instructions in the [README for the repository's top-level **build** directory](../../../build/README.md).


## Building the AutoSDK container image

Run the appropriate wrapper script depending on the operating system, passing in the path to the root of the Unreal Engine source tree (the directory which contains the `Engine` subdirectory):

- Under Linux and macOS, run `./assemble.sh </path/to/UE/source>`
- Under Windows, run `.\assemble.bat <path\to\UE\source>`

The wrapper script will run the Python build script itself, using the appropriate commands for the operating system. The Python build script will automatically determine which SDK components are required for the provided version of the engine. The script will then build the AutoSDK container image.

Once the build completes, the container image will be available with the tag `epicgames/autosdk-wine:<VERSION>`, where `<VERSION>` is the version of Unreal Engine that the container image was built for.


## Using the AutoSDK container image

The functionality supported by the built AutoSDK container image depends on the version of Unreal Engine:

- For Unreal Engine 5.6.0 and newer, the AutoSDK image can be used to package Unreal Engine projects that contain C++ code. For details, see the [**package-project**](../package-project/) directory.

- For Unreal Engine 5.7.0 and newer, the AutoSDK image can also be used to build Unreal Engine itself from source. For details, see the [**create-installed-build**](../create-installed-build/) directory.
