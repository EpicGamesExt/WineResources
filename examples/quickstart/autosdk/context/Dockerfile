ARG WINE_VERSION='invalid'
FROM epicgames/wine-patched:${WINE_VERSION} AS structure

# The list of Visual Studio components and workloads to install
ARG COMPONENTS_AND_WORKLOADS=''

# The .NET Framework SDK to install (if any)
ARG DOTNET_FRAMEWORK_SDK=''

# The AutoSDK identifier for the version of Visual Studio (VS2019 / VS2022 / VS2026)
ARG VISUAL_STUDIO_IDENTIFIER=''

# The major version number for the version of Visual Studio (VS2019 == 16, VS2022 == 17, VS2026 == 18)
ARG VISUAL_STUDIO_VERSION=''

# Disable Wine debug logging while we assemble the AutoSDK directory structure
ENV WINEDEBUG=-all

# Install our required system packages
USER root
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked --mount=type=cache,target=/var/lib/apt,sharing=locked \
	apt-get update && \
	apt-get install -y --no-install-recommends \
		git \
		msitools \
		python3

# Clone the msvc-wine repository
USER nonroot
RUN git clone --depth=1 'https://github.com/mstorsjo/msvc-wine.git' /tmp/msvc-wine
WORKDIR /tmp/msvc-wine

# Install the requested Visual Studio components and workloads
RUN python3 ./vsdownload.py \
	--accept-license \
	--skip-patch \
	--cache ./cache \
	--dest ./downloaded \
	--major "$VISUAL_STUDIO_VERSION" \
	$COMPONENTS_AND_WORKLOADS

# Create the AutoSDK directory structure
ENV AUTOSDK_DIR=/tmp/AutoSDK/HostWin64/Win64
RUN mkdir -p "$AUTOSDK_DIR"
RUN mkdir -p "$AUTOSDK_DIR/DIA SDK"
RUN mkdir -p "$AUTOSDK_DIR/LLVM"

# Move the files for the DIA SDK, MSVC and the Windows SDK into the AutoSDK structure
RUN mv '/tmp/msvc-wine/downloaded/DIA SDK'       "$AUTOSDK_DIR/DIA SDK/$VISUAL_STUDIO_IDENTIFIER" && \
	mv '/tmp/msvc-wine/downloaded/VC/Tools/MSVC' "$AUTOSDK_DIR/$VISUAL_STUDIO_IDENTIFIER" && \
	mv '/tmp/msvc-wine/downloaded/Windows Kits'  "$AUTOSDK_DIR/Windows Kits"

# Query the LLVM version and move the files for LLVM into the AutoSDK structure
# (Note that we create a copy of the LLVM files before running clang, in case any first-run logic modifies the on-disk files)
RUN cp -R '/tmp/msvc-wine/downloaded/VC/Tools/Llvm' /tmp/llvm && \
	LLVM_VERSION=`wine /tmp/llvm/bin/clang-cl.exe --version | grep 'clang version' | grep -o -E '[0-9\.]+'` && \
	mv '/tmp/msvc-wine/downloaded/VC/Tools/Llvm' "$AUTOSDK_DIR/LLVM/$LLVM_VERSION"

# Extract the files for the .NET Framework SDK (if required) and move them into the AutoSDK structure
RUN if [ ! -z "$DOTNET_FRAMEWORK_SDK" ]; then \
		NETFXSDK_DIRECTORY=`find ./cache/ -iname "Microsoft.Net.$DOTNET_FRAMEWORK_SDK.SDK-4.*" -type d` && \
		cd "$NETFXSDK_DIRECTORY" && \
		msiextract ./sdk_tools*.msi && \
		mv './Program Files/Windows Kits/NETFXSDK' "$AUTOSDK_DIR/Windows Kits/NETFXSDK"; \
	fi

# Locate the Windows SDK installer executable
RUN WINSDK_INSTALLER=`find ./cache/ -iname winsdksetup.exe -type f` && \
	cp "$WINSDK_INSTALLER" /tmp/winsdksetup.exe

# Extract the Debugging Tools for Windows installer MSI from the Windows SDK installer executable
RUN cd /tmp && \
	wine winsdksetup.exe \
		/layout ./winsdk_extracted \
		/quiet \
		/ceip off \
		/features 'OptionID.WindowsDesktopDebuggers' && \
	mv /tmp/winsdk_extracted/Installers /tmp/DebugTools

# Extract the files for the Debugging Tools for Windows and copy them into the AutoSDK structure
# (Note that we copy here rather than move, so we can merge the directory trees)
RUN cd /tmp/DebugTools && \
	msiextract './X64 Debuggers And Tools-x64_en-us.msi' && \
	cd './Windows Kits' && \
	cp -R * "$AUTOSDK_DIR/Windows Kits/"

# Copy the assembled AutoSDK files into a clean version of our Wine base image
FROM epicgames/wine-patched:${WINE_VERSION}
COPY --from=structure --chown=nonroot:nonroot /tmp/AutoSDK ${WINEPREFIX}/drive_c/AutoSDK

# Set the environment variable to configure Unreal Engine to use our AutoSDK files
ENV UE_SDKS_ROOT=C:/AutoSDK

# Disable Wine debug logging by default
ENV WINEDEBUG=-all
