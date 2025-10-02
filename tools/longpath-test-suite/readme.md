This is a suite of tests for longpath support in Wine.

According to the [MSDN documenation](https://learn.microsoft.com/en-us/windows/win32/fileio/maximum-file-path-limitation?tabs=registry#functions-without-max_path-restrictions) the following functions are able to opt-in to long path behavior without needing to prepend with the UNC prefix `\\?\` so long as the appropriate registry key has been set.

CreateDirectoryW, CreateDirectoryExW, GetCurrentDirectoryW, RemoveDirectoryW, SetCurrentDirectoryW.
CopyFileW, CopyFile2, CopyFileExW, CreateFileW, CreateFile2, CreateHardLinkW, CreateSymbolicLinkW, DeleteFileW, FindFirstFileW, FindFirstFileExW, FindNextFileW, GetFileAttributesW, GetFileAttributesExW, SetFileAttributesW, GetFullPathNameW, GetLongPathNameW, MoveFileW, MoveFileExW, MoveFileWithProgressW, ReplaceFileW, SearchPathW, FindFirstFileNameW, FindNextFileNameW, FindFirstStreamW, FindNextStreamW, GetCompressedFileSizeW, GetFinalPathNameByHandleW.

The standard for Wine however is that all API functions that accept paths should simply support long-paths all the time and ignore the UNC prefix where it is present.

COMPILING
---------

As of writing, the test suite only supports being compilation with MinGW-w64. Simply run the script `build.sh` to build in a `build/` subdirectory with output to an additional `bin/` directory. If other toolchains or options are required, cmake can be invoked directly.

RUNNING
-------

The executable should not be run directly, required file-system set up is done in the `run.sh` script. To support multiple versions of Wine, the `run.sh` script requires passing in the path of the Wine binary to be used as a parameter. The test suite assumes that `WINEPREFIX` has been set to the prefix in which it is to be run (creating one in the user folder as standard if not set).

At the moment, all tests run in sequence reporting their results to stdout with a summary report at the end.
