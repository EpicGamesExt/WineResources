# Extend the base image containing the patched version of Wine
FROM epicgames/wine-patched:10.1

# Disable Wine debug logging
ENV WINEDEBUG=-all

# Copy the Installed Build of Unreal Engine to C:\UnrealEngine under our Wine prefix
COPY --chown=nonroot:nonroot ./UnrealEngine ${WINEPREFIX}/drive_c/UnrealEngine
