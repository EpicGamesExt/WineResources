# Wine container image build script

This directory contains a [Dockerfile template](./template/) for building a container image that encapsulates Epic's patched version of Wine, along with a [Python script](./build.py) for rendering the template and building a container image from the rendered Dockerfile. The template supports a variety of options that allow you to customise the container image and include only the components that you need.


## Contents

- [Prerequisites](#prerequisites)
- [Running the build script](#running-the-build-script)
- [Rendering the template without building an image](#rendering-the-template-without-building-an-image)
- [Customising the container image](#customising-the-container-image)
    - [Optional components](#optional-components)
    - [Configurable settings](#configurable-settings)


## Prerequisites

- The build script requires [Python](https://www.python.org/) 3.7 or newer.
- If you are building a container image then you will need [Docker Engine](https://docs.docker.com/engine/install/) version 23.0.0 or newer. (This is not required if you are just rendering the Dockerfile template for use on a different machine.)


## Running the build script

Run the appropriate wrapper script depending on the operating system:

- Under Linux and macOS, run `./build.sh`
- Under Windows, run `.\build.bat`

This will automatically install the Python packages that the build script depends upon, and then run the Python build script itself.

The Python build script will render the Dockerfile template with the default options and build the container image from the rendered Dockerfile. Once the build completes, the container image will be available with the tag `epicgames/wine-patched:10.1`.


## Rendering the template without building an image

The build script supports a flag called `--layout` that renders the Dockerfile template without building a container image from the rendered Dockerfile (replace `./build.sh` with `.\build.bat` under Windows):

```bash
./build.sh --layout
```

This will render the Dockerfile template with the default options and place the rendered output in a subdirectory named `context`. The contents of this subdirectory can then be used as the Docker build context when building an image on another machine.


## Customising the container image

The Dockerfile template is designed to be customisable, and these customisations can be applied both when rendering a Dockerfile for use on another machine and when building a container image on the current machine.

### Optional components

By default, the build script will include all supported components when rendering the Dockerfile template. Any of the optional components can be excluded by specifying the appropriate flag:

- `--no-32bit`: disables support for running 32-bit Windows applications. This reduces the size of the container image, but the built Wine binaries will only be able to run 64-bit Windows applications.

- `--no-sudo`: disables [sudo](https://en.wikipedia.org/wiki/Sudo) support for the container's non-root user. This increases the security of the container, but it will not be possible to run commands that require admin privileges from inside the container.

- `--no-mitigations`: excludes the [mitigations for known issues that can occur when building and running Unreal Engine under Wine](../docs/mitigations.md). This reduces the size of the container image, but the container environment will not be capable of building Unreal Engine from source.

### Configurable settings

There are also a number of settings that can be tweaked:

- `--base-image=<IMAGE>`: Sets the default base image that the Dockerfile will use in its `FROM` directives. Although this can be customised, the specified image must be based on Ubuntu 22.04 LTS or the rendered Dockerfile will not function correctly. The default value for this option is `ubuntu:22.04`.

- `--user-id=<UID>`: Sets the user ID for the container's non-root user. The default value for this option is `1000`.
