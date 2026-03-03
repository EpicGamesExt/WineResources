# Extend the base image containing the patched version of Wine
ARG WINE_VERSION
FROM epicgames/wine-patched:${WINE_VERSION}

# Disable Wine debug logging
ENV WINEDEBUG=-all

# Copy the Installed Build of Unreal Engine to C:\UnrealEngine under our Wine prefix
COPY --chown=nonroot:nonroot ./UnrealEngine ${WINEPREFIX}/drive_c/UnrealEngine
