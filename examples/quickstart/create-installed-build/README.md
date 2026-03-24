# Create Installed Build example

This directory contains a script for building Unreal Engine from source for Windows inside an [AutoSDK container image](../autosdk/). It supports Unreal Engine 5.7.0 and newer.

> [!IMPORTANT]
> Unlike most scripts in this repository, this script only supports being run under native Linux. It cannot be run under Windows or macOS with Docker Desktop, due to interactions between Wine's executable loader and the systems used to mount host filesystem directories into Linux containers under these platforms.


## Contents

- [Prerequisites](#prerequisites)
- [Building the Wine and AutoSDK base images](#building-the-wine-and-autosdk-base-images)
- [Building Unreal Engine from source](#building-unreal-engine-from-source)
    - [Wrapping the Installed Build in a container image](#wrapping-the-installed-build-in-a-container-image)


## Prerequisites

- Unreal Engine source code (from either [GitHub](https://www.unrealengine.com/en-US/ue-on-github) or [Perforce](https://dev.epicgames.com/documentation/en-us/unreal-engine/accessing-unreal-engine-with-perforce)), version 5.7.0 or newer
- [Docker Engine](https://docs.docker.com/engine/install/) version 23.0.0 or newer
- [Python](https://www.python.org/) 3.7 or newer


## Building the Wine and AutoSDK base images

The script in this directory will automatically build the Wine and AutoSDK base images if they do not exist. If you wish to build them manually then simply follow the instructions in the relevant README files:

- The README in the [repository's top-level **build** directory](../../../build/README.md) provides instructions for building a base image containing Epic's patched version of Wine.
- The README in the [**autosdk** directory](../autosdk/README.md) provides instructions for building an image that contains the AutoSDK components required by your version of Unreal Engine.


## Building Unreal Engine from source

Run the wrapper script, passing in the path to the root of the Unreal Engine source tree (the directory which contains the `Engine` subdirectory):

```bash
./compile.sh </path/to/UE/source>
```

The wrapper script will run the Python build script itself. The Python build script will start a container using the appropriate AutoSDK base image and bind-mount the Unreal Engine source tree from the host filesystem into the container. The script will then build the engine from source, producing an [Installed Build](https://dev.epicgames.com/documentation/en-us/unreal-engine/installed-build-reference-guide-for-unreal-engine) which can be used under Windows or Wine.

Once the build completes, the Installed Build will be available on the host filesystem at `</path/to/UE/source>/LocalBuilds/Engine/Windows`.

### Wrapping the Installed Build in a container image

By default, the Installed Build produced by the script only exists on the host filesystem. If you want to create a container image that includes the Installed Build files then you can specify the optional `--wrap` flag when running the script:

```bash
./compile.sh </path/to/UE/source> --wrap
```

This will automatically copy the the Installed Build files from the host filesystem into a container image. When this process completes, the container image will be available with the tag `epicgames/unreal-engine:dev-wine-<VERSION>`, where `<VERSION>` is the version of Unreal Engine that the container image was built for.

Alternatively, you can create a container image from existing Installed Build files at any time by manually running the script in the [**wrap-installed-build**](../wrap-installed-build/) directory.
