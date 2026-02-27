# Wine Resources

This repository contains resources related to building and running Unreal Engine under the [Wine](https://www.winehq.org/) compatibility layer. Note that these resources primarily focus on **development and cloud deployment workflows** for Unreal Engine, rather than building and publishing games for the Valve Steam Deck. For resources related to the latter use case, see the [Steam Deck section of the Unreal Engine documentation](https://docs.unrealengine.com/en-US/steam-deck-in-unreal-engine/).

Pull requests that fix bugs are welcomed. **However, support is not provided via this GitHub repository.** Premium support is available for Unreal Engine licensees via [Epic Pro Support](https://www.unrealengine.com/en-US/support).

> [!IMPORTANT]
> Wine support for Unreal Engine is still under active development and is considered experimental. See the [*Workload Status*](./docs/status.md) page for details of which workloads are currently known to function under Wine.


## Contents

- [Building a patched version of Wine](#building-a-patched-version-of-wine)
- [Documentation](#documentation)
- [Legal](#legal)


## Building a patched version of Wine

Epic Games provides a [set of patches](./docs/patches.md) for Wine that make it more suitable for advanced Unreal Engine use cases. The Python script in the [**build**](./build/) subdirectory can be used to build a container image that encapsulates the patched version of Wine, along with a pre-populated Wine prefix and a [set of mitigations](./docs/mitigations.md) for known issues that can occur when building and running Unreal Engine under Wine. This base image can then be extended when creating other container images for use in cloud deployment scenarios.

Although it is of course possible to apply the patches manually to the Wine source code and build it, this workflow is not recommended since there is no means of automatically incorporating the mitigations that are included in the container image environment.


## Documentation

The [**docs**](./docs) subdirectory contains documentation related to the resources in this repository and their use:

- The [*Licensing Considerations*](./docs/licenses.md) page examines the licenses of proprietary software that is commonly used to build and run Unreal Engine under Windows, and identifies any restrictions that are relevant when running the software under Wine rather than a real Windows environment.

- The [*Mitigations for Known Issues*](./docs/mitigations.md) page lists known issues that can occur when building and running Unreal Engine under Wine, along with the mitigations for these issues that are automatically applied when [building a container image](#building-a-patched-version-of-wine) for Epic's patched version of Wine.

- The [*Related Resources*](./docs/resources.md) page presents a curated list of additional resources from external websites that complement the resources provided in this repository.

- The [*Wine Patches*](./docs/patches.md) page lists the patches that Epic provides for making Wine better suited to advanced Unreal Engine use cases.

- The [*Workload Status*](./docs/status.md) page outlines which Unreal Engine workloads are currently known to function under Wine, and to what extent each workload functions correctly.


## Legal

&copy; 2024 - 2026, Epic Games, Inc. The code in this repository is licensed under the MIT License, see the file [LICENSE](./LICENSE) for details.

Unreal and its logo are Epic's trademarks or registered trademarks in the US and elsewhere.
