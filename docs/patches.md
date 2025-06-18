# Wine Patches

This page documents the custom patches that are applied when [building Wine for use with Unreal Engine](../README.md#building-a-patched-version-of-wine).


## Contents

- [ntdll.dll: Update NtQueryDirectoryFile to align with current Windows behaviour](#ntdlldll-update-ntquerydirectoryfile-to-align-with-current-windows-behaviour)
- [Memory patches to report cgroup usage and limit values inside containers and add support for overcommit prevention](#memory-patches-to-report-cgroup-usage-and-limit-values-inside-containers-and-add-support-for-overcommit-prevention)
- [GlobalMemoryStatusEx cache control patch](#globalmemorystatusex-cache-control-patch)
- [Selective smaps_rollup patch](#selective-smaps_rollup-patch)
- [Historical Wine Patches](#historical-wine-patches)
    - [wineserver: Report non-zero exit code for abnormal process termination](#wineserver-report-non-zero-exit-code-for-abnormal-process-termination)
    - [Minidump backport patches](#minidump-backport-patches)


## Memory patches to report cgroup usage and limit values inside containers and add support for overcommit prevention

**Upstream merge request:** None

**Status:** Unsuitable for submission upstream

**Patch file:** [wine-overcommit-prevention-support.patch](../patches/wine-overcommit-prevention-support.patch)

This combined patchset addresses two issues:

- By default, Wine reads system memory statistics from the file `/proc/meminfo` under Linux, which reports values for the entire host system even when accessed from inside a container. This results in incorrect values being reported to Unreal Engine when it is running under Wine inside a Linux container. This patch adds support for querying the memory statistics of the container's [cgroup](https://www.kernel.org/doc/Documentation/cgroup-v2.txt) instead, ensuring Unreal Engine receives values that accurately reflect the container environment.

- Unreal Engine includes functionality for detecting Out Of Memory (OOM) conditions under Windows and reporting them via the crash reporter. This functionality relies on the fact that Windows will refuse to overcommit memory and that memory allocation requests will fail in an OOM scenario. By contrast, the Linux kernel permits overcommitting memory by default and will invoke the [Out Of Memory (OOM) killer](https://www.kernel.org/doc/gorman/html/understand/understand016.html) to free up memory when an OOM condition is detected. This prevents Unreal Engine's OOM handling logic from triggering correctly under Wine. Although it is possible to [configure the Linux kernel to disable overcommit](https://www.kernel.org/doc/Documentation/vm/overcommit-accounting), this setting applies at a system-wide level and cannot be controlled for individual Linux containers. Disabling overcommit at a system level may result in erratic or unexpected behaviour when running native Linux applications that were not designed to gracefully handle memory allocation failures. This patch adds support to Wine for preventing memory overcommit by Windows applications and reporting memory allocation failures in the same manner as Windows itself. This ensures Unreal Engine's OOM handling logic is triggered correctly, whilst avoiding adverse side effects for native Linux applications running on the same system.

The code in this patchset depends on two shared libraries that are stored outside of the Wine source tree: the [memory shim](../memory-shim/) (which is injected into all processes to intercept memory allocation functions) and [libmemory-patches](../libmemory-patches/) (which provides shared code directly used by the code in the patchset).


## GlobalMemoryStatusEx cache control patch

**Upstream merge request:** None

**Status:** Unsuitable for submission upstream

**Patch file:** [globalmemorystatusex-cache-window.patch](../patches/globalmemorystatusex-cache-window.patch)

The Wine implementation of the [GlobalMemoryStatusEx](https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-globalmemorystatusex) caches values from previous calls, returning the cached value if a new call is made within a specific timeframe. By default, the cache window is 1 second, but this patch makes the cache window configurable (and provides the option to disable caching altogether).


## Selective smaps_rollup patch

**Upstream merge request:** None

**Status:** Unsuitable for submission upstream

**Patch file:** [selective-smaps-rollup.patch](../patches/selective-smaps-rollup.patch)

By default, Wine reads process memory statistics from the file `/proc/PID/status` under Linux, which may report approximate values for performance reasons. This patch modifies the code to instead read the more accurate (but more expensive) values from the file `/proc/PID/smaps_rollup`, but only for the specific Unreal Engine thread named "ShaderCompilingThread", since this is the only thread that requires these more accurate values.


# Historical Wine Patches

This section describes historical patches that are no longer maintained out-of-tree because they have been merged upstream and are present in the version of Wine that we target.


## wineserver: Report non-zero exit code for abnormal process termination

**Upstream merge request:** <https://gitlab.winehq.org/wine/wine/-/merge_requests/3908>

**Status:** Merged upstream in commit [2dfeb87f](https://gitlab.winehq.org/wine/wine/-/commit/2dfeb87f410a49dcf0a40d9f81315122b529fa06), present in Wine 9.11 and newer

By default, Wine reports an exit code of zero for Windows processes that terminate due to their corresponding Linux process receiving a signal such as `SIGKILL` (e.g. due to a user manually killing the process, or the [Linux Out Of Memory (OOM) killer](https://www.kernel.org/doc/gorman/html/understand/understand016.html) targeting the process due to memory pressure). This makes it impossible to accurately detect failures when running child processes that are terminated in OOM scenarios, which is problematic when compiling code or shaders. This patch ensures that a non-zero exit code is reported in these scenarios so that they can be correctly detected.


## Minidump backport patches

This file backported the minidump code changes from Wine 9.13 to the stable Wine 9.0 release, and was kindly provided by Eric Pouech from [CodeWeavers](https://www.codeweavers.com/).


## ntdll.dll: Update NtQueryDirectoryFile to align with current Windows behaviour

**Upstream merge request:** <https://gitlab.winehq.org/wine/wine/-/merge_requests/6904>

**Status:** Merged upstream in commit [a44f628c](https://gitlab.winehq.org/wine/wine/-/commit/a44f628c0a604bc25c032a962e23916656baa724), present in Wine 10.3 and newer

**Patch file:** [ntquerydirectoryfile-reset-mask.patch](../patches/ntquerydirectoryfile-reset-mask.patch)

Wine's implementation of the [NtQueryDirectoryFile](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntquerydirectoryfile) function behaves in a manner consistent with the description in Microsoft's documentation. However, Microsoft changed the behaviour of this function in Windows 8 without updating the corresponding documentation, and some applications such as MSVC have been designed to rely on the new behaviour. This patch implements the new behaviour under Wine, which fixes errors that can occur when compiling Unreal Engine's C++ code with the Visual Studio compiler toolchain.
