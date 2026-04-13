# Package Project example

This directory contains a script for packaging an Unreal Engine project for Windows using an [Installed Build](https://dev.epicgames.com/documentation/en-us/unreal-engine/installed-build-reference-guide-for-unreal-engine) inside an [AutoSDK container image](../autosdk/) under Linux. The script supports the use of Installed Build files stored on the host filesystem, or [wrapped in a container image](../wrap-installed-build/). It supports Unreal Engine 5.6.0 and newer.

> [!IMPORTANT]
> Unlike most scripts in this repository, this script only supports being run under native Linux. It cannot be run under Windows or macOS with Docker Desktop, due to interactions between Wine's executable loader and the systems used to mount host filesystem directories into Linux containers under these platforms.


## Contents

- [Prerequisites](#prerequisites)
- [Building the Wine and AutoSDK base images](#building-the-wine-and-autosdk-base-images)
- [Packaging the project](#packaging-the-project)
    - [Using Installed Build files from the host filesystem](#using-installed-build-files-from-the-host-filesystem)
    - [Using Installed Build files from a container image](#using-installed-build-files-from-a-container-image)


## Prerequisites

- Installed Build of Unreal Engine version 5.6.0 or newer, either stored on the host filesystem or [wrapped in a container image](../wrap-installed-build/)
- Unreal Engine project compatible with the version of the engine
- [Docker Engine](https://docs.docker.com/engine/install/) version 23.0.0 or newer
- [Python](https://www.python.org/) 3.7 or newer


## Building the Wine and AutoSDK base images

The script in this directory will automatically build the Wine and AutoSDK base images if they do not exist. If you wish to build them manually then simply follow the instructions in the relevant README files:

- The README in the [repository's top-level **build** directory](../../../build/README.md) provides instructions for building a base image containing Epic's patched version of Wine.
- The README in the [**autosdk** directory](../autosdk/README.md) provides instructions for building an image that contains the AutoSDK components required by your version of Unreal Engine.


## Packaging the project

### Using Installed Build files from the host filesystem

To perform packaging using Installed Build files located on the host filesystem, run the wrapper script with the path of both the `.uproject` file you wish to package and the path to the Installed Build files:

```bash
./package.sh --project </path/to/.uproject> --engine </path/to/UE/source>
```

The wrapper script will run the Python build script itself. The Python build script will start a container using the appropriate AutoSDK base image and bind-mount both the project source tree and the Installed Build files from the host filesystem into the container. The script will then package the project for Windows.

Once packaging completes, the packaged files will be available on the host filesystem at `</path/to/.uproject>/dist`.

### Using Installed Build files from a container image

To perform packaging using Installed Build files that have been [wrapped in a container image](../wrap-installed-build/), run the wrapper script with the path of the `.uproject` file you wish to package and the container image tag:

```bash
./package.sh --project </path/to/.uproject> --image epicgames/unreal-engine:dev-wine-<VERSION>
```

The wrapper script will run the Python build script itself. The Python build script will start a container using the specified Installed Build container image and bind-mount the project source tree from the host filesystem into the container. The script will then package the project for Windows.

Once packaging completes, the packaged files will be available on the host filesystem at `</path/to/.uproject>/dist`.
