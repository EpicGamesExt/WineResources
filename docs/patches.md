# Wine Patches

This page documents the custom patches that are applied when [building Wine for use with Unreal Engine](../README.md#building-a-patched-version-of-wine).


## Contents

- [cmd.exe: Fix crashes when for_ctrl->set is empty](#cmdexe-fix-crashes-when-for_ctrl-set-is-empty)
- [cryptnet: Close leaking file handle in find_cached_revocation_status](#cryptnet-close-leaking-file-handle-in-find_cached_revocation_status)
- [Fix start of word position matching in findstr](#fix-start-of-word-position-matching-in-findstr)
- [Add case for SO_CONDITIONAL_ACCEPT in setsockopt()](#add-case-for-so_conditional_accept-in-setsockopt)
- [Fix GetModuleFileName string termination](#fix-getmodulefilename-string-termination)
- [GlobalMemoryStatusEx cache control patch](#globalmemorystatusex-cache-control-patch)
- [Add missing sections to ImageGetDigestStream](#add-missing-sections-to-imagegetdigeststream)
- [Add the minidump API set to Wine's API set schema](#add-the-minidump-api-set-to-wines-api-set-schema)
- [Selective smaps_rollup patch](#selective-smaps_rollup-patch)
- [Memory patches to report cgroup usage and limit values inside containers and add support for overcommit prevention](#memory-patches-to-report-cgroup-usage-and-limit-values-inside-containers-and-add-support-for-overcommit-prevention)
- [Historical Wine Patches](#historical-wine-patches)
    - [wineserver: Report non-zero exit code for abnormal process termination](#wineserver-report-non-zero-exit-code-for-abnormal-process-termination)
    - [Minidump backport patches](#minidump-backport-patches)
    - [ntdll.dll: Update NtQueryDirectoryFile to align with current Windows behaviour](#ntdlldll-update-ntquerydirectoryfile-to-align-with-current-windows-behaviour)


## cmd.exe: Fix crashes when for_ctrl->set is empty

**Upstream merge request:** https://gitlab.winehq.org/wine/wine/-/merge_requests/10293

**Status:** Merged upstream in commit [5d6905a1](https://gitlab.winehq.org/wine/wine/-/commit/5d6905a100d4632f8ab9512064d10c8a35ebabbd), present in Wine 11.5 and newer

**Patch file:** [cmd-fixes.patch](../patches/cmd-fixes.patch)

This change fixes an issue with `cmd.exe`, where attempting to loop over a null set using a `for` loop triggers a segfault. For example, this trivial example breaks under Wine (but not under Windows):

```bat
for %%i in () do echo "no"
```


## cryptnet: Close leaking file handle in find_cached_revocation_status

**Upstream merge request:** https://gitlab.winehq.org/wine/wine/-/merge_requests/9894

**Status:** Merged upstream in commit [df706cc9](https://gitlab.winehq.org/wine/wine/-/commit/df706cc9a409ad21da98e87a1bbdd043fbd9213b), present in Wine 11.1 and newer

**Patch file:** [cryptnet-file-stream-leak-fix.patch](../patches/cryptnet-file-stream-leak-fix.patch)

Fixes an issue where `find_cached_revocation_status` only closes file handles in failure cases. This causes a leak of stdio stream handles. This leads to .NET MSBuild failing to build sufficiently large projects once all stream I/O indexes are used up and `_wfsopen` fails.


## Fix start of word position matching in findstr

**Upstream merge request:** https://gitlab.winehq.org/wine/wine/-/merge_requests/10294

**Status:** Merged upstream in commit [dc2fa8cc](https://gitlab.winehq.org/wine/wine/-/commit/dc2fa8cc2c438e6e4a48e53837d7bd1e4f01df47), present in Wine 11.5 and newer

**Patch file:** [findstr-fixes.patch](../patches/findstr-fixes.patch)

Add support for the `\<` (start of word position matching) pattern in `findstr`. Currently, only `^` (start of line) is implemented.


## Add case for SO_CONDITIONAL_ACCEPT in setsockopt()

**Upstream merge request:** None

**Status:** Unsuitable for submission upstream

**Patch file:** [gdk_so_conditional_accept.patch](../patches/gdk_so_conditional_accept.patch)

This patch adds a no-op case for `SO_CONDITIONAL_ACCEPT` in `dlls/ws2_32/socket.c` for `setsockopt()`.


## Fix GetModuleFileName string termination

**Upstream merge request:** https://gitlab.winehq.org/wine/wine/-/merge_requests/10291

**Status:** Under review

**Patch file:** [getmodulefilename.patch](../patches/getmodulefilename.patch)

After Windows XP the string termination behaviour of `GetModuleFileName` was changed to always terminate the path returned, even if the buffer is insufficient to contain the null terminator. This patch addresses that change in behaviour to ensure strings are always null terminated, even if the buffer is too small.


## GlobalMemoryStatusEx cache control patch

**Upstream merge request:** None

**Status:** Unsuitable for submission upstream

**Patch file:** [globalmemorystatusex-cache-window.patch](../patches/globalmemorystatusex-cache-window.patch)

The Wine implementation of the [GlobalMemoryStatusEx](https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-globalmemorystatusex) caches values from previous calls, returning the cached value if a new call is made within a specific timeframe. By default, the cache window is 1 second, but this patch makes the cache window configurable (and provides the option to disable caching altogether).


## Add missing sections to ImageGetDigestStream

**Upstream merge request:** https://gitlab.winehq.org/wine/wine/-/merge_requests/10295

**Status:** Under review

**Patch file:** [imagegetdigeststream.patch](../patches/imagegetdigeststream.patch)

Unreal Engine uses the Windows API [ImageGetDigestStream()](https://learn.microsoft.com/en-us/windows/win32/api/imagehlp/nf-imagehlp-imagegetdigeststream) with the `DigestLevel` values of `CERT_PE_IMAGE_DIGEST_RESOURCES | CERT_PE_IMAGE_DIGEST_ALL_IMPORT_INFO` to [generate hash values](https://github.com/EpicGames/UnrealEngine/blob/0b917fe1ab67ca45e1233a866c92e791fc451ef8/Engine/Source/Developer/ShaderCompilerCommon/Private/DXCWrapper.cpp#L53-L57) using a [callback function](https://github.com/EpicGames/UnrealEngine/blob/0b917fe1ab67ca45e1233a866c92e791fc451ef8/Engine/Source/Developer/ShaderCompilerCommon/Private/DXCWrapper.cpp#L20-L25) to read the given byte data and generate a hash values for each DLL listed below.

```
dxil.dll
dxcompiler.dll
ShaderConductor.dll
```

The combined DLL hash value is subsequently used with the [ShaderModelHash](https://github.com/EpicGames/UnrealEngine/blob/0b917fe1ab67ca45e1233a866c92e791fc451ef8/Engine/Source/Developer/Windows/ShaderFormatD3D/Private/ShaderFormatD3D.cpp#L54-L82), to generated a version hash for the given shader formats:

```
PCD3D_SM6
PCD3D_SM5
```

A DDC key is generated when processing the results of the compilation of a [ShaderMap](https://github.com/EpicGames/UnrealEngine/blob/0b917fe1ab67ca45e1233a866c92e791fc451ef8/Engine/Source/Runtime/Engine/Private/ShaderCompiler/ShaderCompilerEditor.cpp#L578-L649). This ShaderMap is [saved](https://github.com/EpicGames/UnrealEngine/blob/0b917fe1ab67ca45e1233a866c92e791fc451ef8/Engine/Source/Runtime/Engine/Private/ShaderCompiler/ShaderCompilerEditor.cpp#L543-L576) to the DDC and the key is used to identify it within the DDC. This key includes the generated version hash described above for the shader type that is being compiled, either `PCD3D_SM6` or `PCD3D_SM5`.

Wine's implementation of [ImageGetDigestStream()](https://gitlab.winehq.org/wine/wine/-/blob/91b081763ce78a7323d886ad3441f0b9e1fd7909/dlls/imagehlp/integrity.c) is currently incomplete and does not include the code to process a number of PE file sections. Because of this, the byte data returned is different causing the hash values to differ for the same DLL on Wine and Windows.

To enable parity between the Windows and Wine behaviour for the Digest level `CERT_PE_IMAGE_DIGEST_RESOURCES | CERT_PE_IMAGE_DIGEST_ALL_IMPORT_INFO` the following changes needed to be applied:
- The order of the returned sections `.data` and `.rdata` need to be changed.
- The removal of the zeroing out of the data in the OptionalHeader for the fields `SizeOfInitializedData` and `SizeOfImage` to match the data returned by Windows.
- The addition on the following PE file sections:

```
.pdata
.didat
.tls
.00cfg
_RDATA
.reloc
```
- For signed DLL's such as `dxil.dll`, the byte data returned for `Size` and `VirtualAddress` in `OptionalHeader[IMAGE_FILE_SECURITY_DIRECTORY]` are required to be set to zero to match the [Windows behaviour](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#certificate-data), as the certificate data is not used to generate hash values.


## Add the minidump API set to Wine's API set schema

**Upstream merge request:** None

**Status:** Unsuitable for submission upstream

**Patch file:** [minidump-apiset.patch](../patches/minidump-apiset.patch)

This patch adds the `api-ms-win-core-debug-minidump` API set to `dlls/apisetschema/apisetschema.spec`. This is required by some third-party SDKs.


## Selective smaps_rollup patch

**Upstream merge request:** None

**Status:** Unsuitable for submission upstream

**Patch file:** [selective-smaps-rollup.patch](../patches/selective-smaps-rollup.patch)

By default, Wine reads process memory statistics from the file `/proc/PID/status` under Linux, which may report approximate values for performance reasons. This patch modifies the code to instead read the more accurate (but more expensive) values from the file `/proc/PID/smaps_rollup`, but only for the specific Unreal Engine thread named "ShaderCompilingThread", since this is the only thread that requires these more accurate values.


## Memory patches to report cgroup usage and limit values inside containers and add support for overcommit prevention

**Upstream merge request:** None

**Status:** Unsuitable for submission upstream

**Patch file:** [wine-overcommit-prevention-support.patch](../patches/wine-overcommit-prevention-support.patch)

This combined patchset addresses two issues:

- By default, Wine reads system memory statistics from the file `/proc/meminfo` under Linux, which reports values for the entire host system even when accessed from inside a container. This results in incorrect values being reported to Unreal Engine when it is running under Wine inside a Linux container. This patch adds support for querying the memory statistics of the container's [cgroup](https://www.kernel.org/doc/Documentation/cgroup-v2.txt) instead, ensuring Unreal Engine receives values that accurately reflect the container environment.

- Unreal Engine includes functionality for detecting Out Of Memory (OOM) conditions under Windows and reporting them via the crash reporter. This functionality relies on the fact that Windows will refuse to overcommit memory and that memory allocation requests will fail in an OOM scenario. By contrast, the Linux kernel permits overcommitting memory by default and will invoke the [Out Of Memory (OOM) killer](https://www.kernel.org/doc/gorman/html/understand/understand016.html) to free up memory when an OOM condition is detected. This prevents Unreal Engine's OOM handling logic from triggering correctly under Wine. Although it is possible to [configure the Linux kernel to disable overcommit](https://www.kernel.org/doc/Documentation/vm/overcommit-accounting), this setting applies at a system-wide level and cannot be controlled for individual Linux containers. Disabling overcommit at a system level may result in erratic or unexpected behaviour when running native Linux applications that were not designed to gracefully handle memory allocation failures. This patch adds support to Wine for preventing memory overcommit by Windows applications and reporting memory allocation failures in the same manner as Windows itself. This ensures Unreal Engine's OOM handling logic is triggered correctly, whilst avoiding adverse side effects for native Linux applications running on the same system.

The code in this patchset depends on two shared libraries that are stored outside of the Wine source tree: the [memory shim](../memory-shim/) (which is injected into all processes to intercept memory allocation functions) and [libmemory-patches](../libmemory-patches/) (which provides shared code directly used by the code in the patchset).


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
