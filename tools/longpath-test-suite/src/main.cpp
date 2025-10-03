#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT _WIN32_WINNT_WIN10
#include <windows.h>
#include <winternl.h>
#include <shlwapi.h>
#include <io.h>
#include <direct.h>

#include <stdio.h>
#include <string.h>

#if 1
#define LONG_FOLDER \
    L"this_right_here_is_a_noticeably_lengthy_name_for_the_topmost_parent_directory_that_sits_directly_under_the_c_drive\\" \
    L"this_right_here_is_a_noticeably_lengthy_name_for_the_nested_subdirectory_that_sits_directly_under_the_topmost_parent_directory\\" \
    L"this_right_here_is_a_noticeably_lengthy_name_for_yet_another_nested_subdirectory_sitting_further_under_the_parent_directory"
#else
#define LONG_FOLDER \
    L"this_right_here_is_a_noticeably_lengthy_name_for_the_topmost_parent_directory_that_sits_directly_under_the_c_drive\\" \
    L"this_right_here_is_a_noticeably_lengthy_name_for_the_nested_subdirectory_that_sits_directly_under_the_topmost_parent_directory"
#endif
#define SHORT_FOLDER L"short_folder"
#define RELATIVE_SUBFOLDER L"relative_subfolder"
#define LONG_FILE_NAME L"this_right_here_is_a_noticeably_lengthy_name_for_the_file_that_sits_directly_under_the_nested_subdirectory"
#define SHORT_FILE_NAME L"shorty"
#define FILE_EXT L".txt"

#define CREATE_FOLDER L"create"
#define CREATE_FOLDER_EX L"createEx"

#define REMOVE_FOLDER L"remove"

// Used for multiple Copy tests
#define ALT_SUFFIX L"Alt"

// Used to test CopyFileW
#define COPYW_SUFFIX L"CopyW"

// Used to test CopyFile2
#define COPY2_SUFFIX L"Copy2"

// Used to test CopyFileExW
#define COPYEXW_SUFFIX L"CopyExW"

// Used to test CreateFileW
#define CREATEW_SUFFIX L"NewW"

// Used to test CreateFile2
#define CREATE2_SUFFIX L"New2"

// Used to test CreateHardLinkW
#define HARDLINK_SUFFIX L"Hardlink"

// Used to test CreateSymbolicLinkW
#define SYMLINK_SUFFIX L"Symlink"

// Used to test DeleteFileW
#define DELETE_FILE_SUFFIX L"Delete"

// Used to test FindNextFileW
#define SEARCH_FILE_SUFFIX L"Search"

// Used to test MoveFileW
#define MOVE_FILE_SUFFIX L"Move"
#define MOVED_FILE_SUFFIX L"Moved"

// Used to test ReplaceFileW
#define BACKUP_FILE_SUFFIX L".temp"
#define FIRST_FILE_SUFFIX L"Rep"
#define SECOND_FILE_SUFFIX L"Rep2"

static const WCHAR longFile[] = L"C:\\" LONG_FOLDER L"\\" LONG_FILE_NAME FILE_EXT;
static const WCHAR shortFile[] = L"C:\\" SHORT_FILE_NAME FILE_EXT;
static size_t fileSize = 19; // bytes
static WCHAR shortBuffer[MAX_PATH + 1]; // Global buffer for test functions to use as they wish
static WCHAR longBuffer[MAX_PATH * 2];
static WCHAR longBufferAlt[MAX_PATH * 2];
static ULONGLONG JAN_1_2000 = 125911584000000000; // https://www.silisoftware.com/tools/date.php
static bool verbose = false;

enum class ReturnCode : int
{
    UnexpectedSuccess = 2,
    Warning = 1,        /* Passed with warnings */
    Success = 0,
    Failure = -1,       /* Failed a test case */
    Error = -2,         /* Could not run due to an error */
    ExpectedFailure = -3,
};

static DWORD
PrintWindowsError(DWORD errorCode)
{
    if (errorCode) {
        WCHAR buffer[256] = {};
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, errorCode, 0,
                      buffer, sizeof(buffer) / sizeof(WCHAR) - 1, 0);
        printf("\tWindows Error [%i]: %S\n", errorCode, buffer);
    }
    return errorCode;
}

/* ===============================================================================================
 *                                      TESTS
 * =============================================================================================== */

// CreateDirectoryW
static ReturnCode
TestCreateDirectoryW(const WCHAR* dir)
{
    BOOL success;
    HANDLE handle;

    SetLastError(0); // Throw away any lingering error
    printf("Testing: CreateDirectoryW. Directory name length %i\n", lstrlenW(dir));

    success = CreateDirectoryW(dir, NULL);

    if (!success) {
        printf("\tError: Failed call to CreateDirectoryW.\n");
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    handle = CreateFileW(dir, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        printf("\tFailed: Folder created with CreateDirectoryW not found after successful call.\n");
        PrintWindowsError(GetLastError());
        ReturnCode::Failure;
    }

    return ReturnCode::Success;
}

static ReturnCode
TestCreateDirectoryExW(const WCHAR* templateDir, const WCHAR* dir)
{
    BOOL success;
    HANDLE handle;

    SetLastError(0); // Throw away any lingering error
    printf("Testing: CreateDirectoryExW. Directory name length %i\n", lstrlenW(dir));

    success = CreateDirectoryExW(templateDir, dir, NULL);

    if (!success) {
        printf("\tError: Failed call to CreateDirectoryExW.\n");
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    handle = CreateFileW(dir, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        printf("\tFailed: Folder created with CreateDirectoryExW not found after successful call.\n");
        PrintWindowsError(GetLastError());
        ReturnCode::Failure;
    }

    return ReturnCode::Success;
}

static ReturnCode
TestRemoveDirectoryW(const WCHAR* dir)
{
    BOOL success;
    HANDLE handle;
    DWORD errorCode;

    SetLastError(0); // Throw away any lingering error
    printf("Testing: RemoveDirectoryW. Directory name length %i\n", lstrlenW(dir));

    success = RemoveDirectoryW(dir);
    if (!success) {
        printf("\tError: Failed call to RemoveDirectoryW.\n");
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    // Confirm folder is gone with CreateFileW
    handle = CreateFileW(dir, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS, NULL);

    errorCode = GetLastError();
    if (errorCode != ERROR_FILE_NOT_FOUND) {
        printf("\tError: Unexpected error code, expected ERROR_FILE_NOT_FOUND[%i]\n", ERROR_FILE_NOT_FOUND);
        if (errorCode) {
            PrintWindowsError(errorCode);
        } else {
            printf("\tNo Windows error code!\n");
        }
        return ReturnCode::Error;
    }

    if (handle != INVALID_HANDLE_VALUE) {
        printf("\tFailed: Directory still exists after calling RemoveDirectoryW.\n");
        PrintWindowsError(GetLastError());
        return ReturnCode::Failure;
    }

    return ReturnCode::Success;
}

static ReturnCode
TestGetAndSetCurrentDirectoryW(const WCHAR* dir)
{
    BOOL success;
    DWORD pwdSize;
    size_t dir2Len;
    int delta;

    SetLastError(0); // Throw away any lingering error
    printf("Testing: SetCurrentDirectoryW. Directory name length %i\n", lstrlenW(dir));

    success = SetCurrentDirectory(dir);
    if (!success) {
        printf("\tError: Failed call to SetCurrentDirectory with absolute path.\n");
        return ReturnCode::Error;
    }

    memset(longBuffer, 0, sizeof(longBuffer));
    pwdSize = GetCurrentDirectory(sizeof(longBuffer) / sizeof(*longBuffer) - 1, longBuffer);
    if (pwdSize < lstrlenW(dir)) {
        printf("\tFailed: GetCurrentDirectory reported unusually small output size [%i] for storing [%i] characters\n",
                pwdSize, lstrlenW(dir));
        PrintWindowsError(GetLastError());
        return ReturnCode::Failure;
    }

    delta = wcsncmp(longBuffer, dir, lstrlenW(dir));
    if (delta) {
        printf("\tFailed: Mismatch[%i] between set directory and comparison:\n\tDir: %S\n\tPWD: %s\n",
                delta, dir, longBuffer);
        PrintWindowsError(GetLastError());
        return ReturnCode::Failure;
    }

    // Try a relative move as well
    #if 1
    success = SetCurrentDirectory(RELATIVE_SUBFOLDER);
    if (!success) {
        printf("\tError: Failed call to SetCurrentDirectory with relative path.\n");
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    dir2Len = lstrlenW(dir) + 1 /*folder separator*/ + lstrlenW(RELATIVE_SUBFOLDER);
    memset(longBuffer, 0, sizeof(longBuffer));
    pwdSize = GetCurrentDirectory(sizeof(longBuffer) / sizeof(*longBuffer) - 1, longBuffer);
    if (pwdSize < dir2Len) {
        printf("\tFailed: GetCurrentDirectory reported unusually small output size [%i] for storing [%i] characters\n",
                pwdSize, dir2Len);
        PrintWindowsError(GetLastError());
        return ReturnCode::Failure;
    }

    memset(longBufferAlt, 0, sizeof(longBufferAlt));
    _snwprintf(longBufferAlt, sizeof(longBufferAlt) / sizeof(*longBufferAlt) - 1,
            L"%s\\%s", dir, RELATIVE_SUBFOLDER);
    delta = wcsncmp(longBuffer, longBufferAlt, dir2Len);

    if (delta) {
        printf("\tFailed: Mismatch[%i] between set relative directory and comparison:\n"
                "\tDir: %S\n\tPWD: %s\n", delta, longBufferAlt, longBuffer);
        return ReturnCode::Failure;
    }
    #endif

    return ReturnCode::Success;
}

static ReturnCode
TestCopyFileW(const WCHAR* source, const WCHAR* source2, const WCHAR* target)
{
    BOOL success;
    BOOL found;
    FILE* file;
    char word[33];

    SetLastError(0); // Throw away any lingering error
    printf("Testing: CopyFileW. Source path lengths: %i & %i, Target path length: %i\n",
            lstrlenW(source), lstrlenW(source2), lstrlenW(target));

    success = CopyFileW(source, target, TRUE);

    if (success == FALSE) {
        printf("\tError: Failed to copy file: %S->%S.\n", source, target);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    file = _wfopen(target, L"r");
    if (!file) {
        printf("\tFailed: Could not open copied file: %S.\n", target);
        PrintWindowsError(GetLastError());
        return ReturnCode::Failure;
    }
    fclose(file);

    // Try again but deliberately overwrite this time
    success = CopyFileW(source2, target, FALSE);

    if (success == FALSE) {
        printf("\tError: Failed to overwrite file: %S->%S.\n", source2, target);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    // Confirm the overwrite by checking the contents for the string "alternate"
    file = _wfopen(target, L"r");
    if (!file) {
        printf("\tFailed: Could not open copied file: %S.\n", target);
        PrintWindowsError(GetLastError());
        return ReturnCode::Failure;
    }

    found = FALSE;
    while (fscanf(file, "%32s", word) == 1) {
        if (strstr(word, "alternate")) {
            found = TRUE;
            break;
        }
    }
    fclose(file);

    if (!found) {
        printf("\tFailed: Overwritten file did not contain expected string \"alternate\": %S.\n", target);
        PrintWindowsError(GetLastError());
        return ReturnCode::Failure;
    }

    return ReturnCode::Success;
}

struct TestCopyFile2CallbackContext
{
    ReturnCode result;
    COPYFILE2_MESSAGE_ACTION callbackReturnValue;
};

static COPYFILE2_MESSAGE_ACTION TestCopyFile2ProgressRoutine(const COPYFILE2_MESSAGE* msg, PVOID context);

static ReturnCode
TestCopyFile2(const WCHAR* source, const WCHAR* source2, const WCHAR* target)
{
    TestCopyFile2CallbackContext context = {};
    COPYFILE2_EXTENDED_PARAMETERS params = {};
    BOOL cancelled = FALSE;
    HRESULT result;
    FILE* file;

    SetLastError(0); // Throw away any lingering error
    printf("Testing: CopyFile2. Source path lengths: %i & %i, Target path length: %i\n",
            lstrlenW(source), lstrlenW(source2), lstrlenW(target));

    context.result = ReturnCode::Success;
    context.callbackReturnValue = COPYFILE2_PROGRESS_CONTINUE;
    params.dwSize = sizeof(params);
    params.dwCopyFlags = COPY_FILE_FAIL_IF_EXISTS;
    params.pfCancel = &cancelled;
    params.pProgressRoutine = TestCopyFile2ProgressRoutine;
    params.pvCallbackContext = reinterpret_cast<void*>(&context);
    result = CopyFile2(source, target, &params);

    if (SUCCEEDED(result) < 0) {
        printf("\tError: CopyFile2 returned failure code: %i.\n", result);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    if (cancelled) {
        printf("\tError: CopyFile2 unexpectedly set cancel flag.\n");
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    if (context.result < ReturnCode::Success) {
        return context.result;
    }

    // Confirm copied file
    file = _wfopen(target, L"r");
    if (!file) {
        printf("\tFailed: Could not open copied file: %S.\n", target);
        PrintWindowsError(GetLastError());
        return ReturnCode::Failure;
    }
    fclose(file);

    // Attempt to pause and resume as part of overwrite (source2)
    context.callbackReturnValue = COPYFILE2_PROGRESS_PAUSE;
    memset(&params, 0, sizeof(params));
    params.dwSize = sizeof(params);
    params.dwCopyFlags = COPY_FILE_RESTARTABLE;
    params.pfCancel = &cancelled;
    params.pProgressRoutine = TestCopyFile2ProgressRoutine;
    params.pvCallbackContext = reinterpret_cast<void*>(&context);
    result = CopyFile2(source2, target, &params);

    if (cancelled) {
        printf("\tError: CopyFile2 unexpectedly set cancel flag.\n");
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    if (context.result < ReturnCode::Success) {
        return context.result;
    }

    if (SUCCEEDED(result) < 0) {
        if (result == HRESULT_FROM_WIN32(ERROR_REQUEST_PAUSED)) {
            context.callbackReturnValue = COPYFILE2_PROGRESS_QUIET;
            memset(&params, 0, sizeof(params));
            params.dwSize = sizeof(params);
            params.dwCopyFlags = COPY_FILE_RESUME_FROM_PAUSE;
            params.pfCancel = &cancelled;
            params.pProgressRoutine = TestCopyFile2ProgressRoutine;
            params.pvCallbackContext = reinterpret_cast<void*>(&context);
            result = CopyFile2(source2, target, &params);

            if (SUCCEEDED(result) < 0) {
                printf("\tError: CopyFile2 returned failure code: %i.\n", result);
                PrintWindowsError(GetLastError());
                return ReturnCode::Error;
            } else if (cancelled) {
                printf("\tError: CopyFile2 unexpectedly set cancel flag.\n");
                PrintWindowsError(GetLastError());
                return ReturnCode::Failure;
            } else if (context.result < ReturnCode::Success) {
                return context.result;
            }

        } else {
            printf("\tError: CopyFile2 returned failure code: %i.\n", result);
            PrintWindowsError(GetLastError());
            return ReturnCode::Error;
        }
    }

    file = _wfopen(target, L"r");
    if (!file) {
        printf("\tFailed: Could not open copied file: %S.\n", target);
        PrintWindowsError(GetLastError());
        return ReturnCode::Failure;
    }
    fclose(file);

    return context.result;;
}

static COPYFILE2_MESSAGE_ACTION
TestCopyFile2ProgressRoutine(const COPYFILE2_MESSAGE* msg, PVOID contextptr)
{
    TestCopyFile2CallbackContext* context = reinterpret_cast<TestCopyFile2CallbackContext*>(contextptr);

    printf("CopyFile2 Callback: ");
    switch (msg->Type) {
        case COPYFILE2_CALLBACK_CHUNK_STARTED: {
            printf("COPYFILE2_CALLBACK_CHUNK_STARTED\n");
        } break;

        case COPYFILE2_CALLBACK_CHUNK_FINISHED: {
            printf("COPYFILE2_CALLBACK_CHUNK_FINISHED\n");
        } break;

        case COPYFILE2_CALLBACK_STREAM_STARTED: {
            printf("COPYFILE2_CALLBACK_STREAM_STARTED\n");
        } break;

        case COPYFILE2_CALLBACK_STREAM_FINISHED: {
            printf("COPYFILE2_CALLBACK_STREAM_FINISHED\n");
        } break;

        case COPYFILE2_CALLBACK_POLL_CONTINUE: {
            printf("COPYFILE2_CALLBACK_POLL_CONTINUE\n");
        } break;

        case COPYFILE2_CALLBACK_ERROR: {
            printf("COPYFILE2_CALLBACK_ERROR\n");
            context->result = ReturnCode::Error;
            context->callbackReturnValue = COPYFILE2_PROGRESS_CANCEL;
        } break;

        default: {
            printf("Unknown Message Code: %i\n", msg->Type);
            context->result = ReturnCode::Error;
            context->callbackReturnValue = COPYFILE2_PROGRESS_CANCEL;
        } break;
    }

    return context->callbackReturnValue;
}

struct TestCopyFileExWCallbackContext
{
    ReturnCode result;
    DWORD callbackReturnValue;
};

static DWORD TestCopyFileExWProgressRoutine(
        LARGE_INTEGER TotalFileSize, LARGE_INTEGER TotalBytesTransferred,
        LARGE_INTEGER StreamSize, LARGE_INTEGER StreamBytesTransferred,
        DWORD dwStreamNumber, DWORD dwCallbackReason,
        HANDLE hSourceFile, HANDLE hDestinationFile, LPVOID lpData);

static ReturnCode
TestCopyFileExW(const WCHAR* source, const WCHAR* source2, const WCHAR* target)
{
    TestCopyFileExWCallbackContext context = {};
    BOOL cancelled = FALSE;
    BOOL success;
    FILE* file;

    SetLastError(0); // Throw away any lingering error
    printf("Testing: CopyFileExW. Source path lengths: %i & %i, Target path length: %i\n",
            lstrlenW(source), lstrlenW(source2), lstrlenW(target));

    context.result = ReturnCode::Success;
    context.callbackReturnValue = PROGRESS_CONTINUE;
    success = CopyFileExW(source, target, TestCopyFileExWProgressRoutine,
                            &context, &cancelled, COPY_FILE_FAIL_IF_EXISTS);

    if (!success) {
        printf("\tError: CopyFileExW call failed.\n");
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    if (cancelled) {
        printf("\tError: CopyFileExW unexpectedly set cancel flag.\n");
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    if (context.result < ReturnCode::Success) {
        return context.result;
    }

    // Confirm copied file
    file = _wfopen(target, L"r");
    if (!file) {
        printf("\tFailed: Could not open copied file: %S.\n", target);
        PrintWindowsError(GetLastError());
        return ReturnCode::Failure;
    }
    fclose(file);

    context.callbackReturnValue = PROGRESS_STOP;
    success = CopyFileExW(source, target, TestCopyFileExWProgressRoutine,
                            &context, &cancelled, COPY_FILE_RESTARTABLE | COPY_FILE_NO_BUFFERING);

    if (cancelled) {
        printf("\tError: CopyFileExW unexpectedly set cancel flag.\n");
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    if (context.result < ReturnCode::Success) {
        return context.result;
    }

    if (!success) {
        DWORD errorCode = GetLastError();
        if (errorCode == ERROR_REQUEST_ABORTED) {
            context.callbackReturnValue = PROGRESS_QUIET;
            success = CopyFileExW(source, target, TestCopyFileExWProgressRoutine,
                                    &context, &cancelled, COPY_FILE_RESTARTABLE | COPY_FILE_NO_BUFFERING);

            if (!success) {
                printf("\tError: CopyFileExW call failed.\n");
                PrintWindowsError(GetLastError());
                return ReturnCode::Error;
            } else if (cancelled) {
                printf("\tError: CopyFileExW unexpectedly set cancel flag.\n");
                PrintWindowsError(GetLastError());
                return ReturnCode::Failure;
            } else if (context.result < ReturnCode::Success) {
                return context.result;
            }
        } else {
            printf("\tError: CopyFileExW call failed.\n");
            PrintWindowsError(errorCode);
            return ReturnCode::Error;
        }
    }

    file = _wfopen(target, L"r");
    if (!file) {
        printf("\tFailed: Could not open copied file: %S.\n", target);
        PrintWindowsError(GetLastError());
        return ReturnCode::Failure;
    }
    fclose(file);

    return context.result;
}

static DWORD
TestCopyFileExWProgressRoutine(
        LARGE_INTEGER TotalFileSize, LARGE_INTEGER TotalBytesTransferred,
        LARGE_INTEGER StreamSize, LARGE_INTEGER StreamBytesTransferred,
        DWORD dwStreamNumber, DWORD dwCallbackReason,
        HANDLE hSourceFile, HANDLE hDestinationFile, LPVOID lpData)
{
    TestCopyFileExWCallbackContext* context = reinterpret_cast<TestCopyFileExWCallbackContext*>(lpData);
    double progress = (double)TotalBytesTransferred.QuadPart / (double)TotalFileSize.QuadPart;

    printf("Callback [%03i%%]: ", (int)progress * 100);
    switch (dwCallbackReason) {
        case CALLBACK_CHUNK_FINISHED: {
            printf("CALLBACK_CHUNK_FINISHED\n");
        } break;

        case CALLBACK_STREAM_SWITCH: {
            printf("CALLBACK_STREAM_SWITCH\n");
        } break;

        default: {
            printf("Error: Unknown Callback Reason Code: %i\n", dwCallbackReason);
            context->result = ReturnCode::Error;
            context->callbackReturnValue = PROGRESS_CANCEL;
        } break;
    }
    return context->callbackReturnValue;
}

static ReturnCode
TestCreateFileW(const WCHAR* oldFile, const WCHAR* newFile, DWORD flags)
{
    HANDLE handle;
    FILE* file;
    DWORD errorCode;

    SetLastError(0); // Throw away any lingering error
    printf("Testing: CreateFileW. Input path lengths: %i and %i\n", lstrlenW(oldFile), lstrlenW(newFile));

    // Attempt to open non-existent file

    handle = CreateFileW(newFile, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, flags, NULL);
    errorCode = GetLastError();

    if (handle != INVALID_HANDLE_VALUE) {
        printf("\tError: Failure opening non-existent file: Expected INVALID_HANDLE_VALUE(%i) but got %i\n",
                INVALID_HANDLE_VALUE, handle);
        PrintWindowsError(errorCode);
        return ReturnCode::Error;
    }

    if (errorCode != ERROR_FILE_NOT_FOUND) {
        printf("\tError: Unexpected error code, expected ERROR_FILE_NOT_FOUND[%i]\n", ERROR_FILE_NOT_FOUND);
        if (errorCode) {
            PrintWindowsError(errorCode);
        } else {
            printf("\tNo Windows error code!\n");
        }
        return ReturnCode::Error;
    }

    // Attempt to create new file when matching name already exists

    handle = CreateFileW(oldFile, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL,
                        CREATE_NEW, flags, NULL);
    errorCode = GetLastError();

    if (handle != INVALID_HANDLE_VALUE) {
        printf("\tError: Failure creating existing file as new: Expected INVALID_HANDLE_VALUE(%i) but got %i\n",
                INVALID_HANDLE_VALUE, handle);
        PrintWindowsError(errorCode);
        return ReturnCode::Error;
    }

    if (errorCode != ERROR_FILE_EXISTS) {
        printf("\tError: Unexpected error code, expected ERROR_FILE_EXISTS[%i]\n", ERROR_FILE_EXISTS);
        if (errorCode) {
            PrintWindowsError(errorCode);
        } else {
            printf("\tNo Windows error code!\n");
        }

        return ReturnCode::Error;
    }

    // Create new file

    handle = CreateFileW(newFile, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL,
                        CREATE_NEW, flags, NULL);

    if (handle == INVALID_HANDLE_VALUE) {
        printf("\tError: Failure creating new file %S: Expected INVALID_HANDLE_VALUE(%i) but got %i\n",
                newFile, INVALID_HANDLE_VALUE, handle);
        PrintWindowsError(GetLastError());

        return ReturnCode::Error;
    }

    CloseHandle(handle);

    file = _wfopen(newFile, L"w");
    if (!file) {
        printf("\tFailed: Could not open file opened with CREATE_NEW: %S.\n", newFile);
        PrintWindowsError(GetLastError());
        return ReturnCode::Failure;
    }
    fprintf(file, "If you're reading this, the test has failed");
    if (fclose(file)) {
        printf("\tError: Failed to close fd to file created with CREATE_NEW: %S.\n", newFile);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    // CREATE_ALWAYS

    handle = CreateFileW(newFile, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL,
                        CREATE_ALWAYS, flags, NULL);
    errorCode = GetLastError();

    if (handle == INVALID_HANDLE_VALUE) {
        printf("\tError: Failure creating new file %S: Expected INVALID_HANDLE_VALUE(%i) but got %i\n",
                newFile, INVALID_HANDLE_VALUE, handle);
        PrintWindowsError(errorCode);
        return ReturnCode::Error;
    }

    if (errorCode && errorCode != ERROR_ALREADY_EXISTS) {
        printf("\tError: Unexpected error code, expected ERROR_ALREADY_EXISTS[%i]\n",
                ERROR_ALREADY_EXISTS);
        PrintWindowsError(errorCode);
        CloseHandle(handle);
        return ReturnCode::Error;
    }

    CloseHandle(handle);

    file = _wfopen(newFile, L"r+");
    if (!file) {
        printf("\tFailed: Could not open newly created file: %S.\n", newFile);
        PrintWindowsError(GetLastError());
        return ReturnCode::Failure;
    }

    {// Confirm the file is empty now
        char buffer[4] = {};
        int scans;
        scans = fscanf_s(file, "%s", buffer, sizeof(buffer));
        if (scans > 0) {
            printf("\tFailed: File opened with CREATE_ALWAYS already has content: %S.\n", newFile);
            fclose(file);
            return ReturnCode::Failure;
        }
    }

    fprintf(file, "If you're reading this, the test has failed");
    if (fclose(file)) {
        printf("\tError: Failed to close fd to file created with CREATE_ALWAYS: %S.\n", newFile);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    // OPEN_ALWAYS

    handle = CreateFileW(newFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_ALWAYS, flags, NULL);
    errorCode = GetLastError();

    if (handle == INVALID_HANDLE_VALUE) {
        printf("\tError: Failure creating new file %S: Expected INVALID_HANDLE_VALUE(%i) but got %i\n",
                newFile, INVALID_HANDLE_VALUE, handle);
        PrintWindowsError(errorCode);
        return ReturnCode::Error;
    }

    if (errorCode && errorCode != ERROR_ALREADY_EXISTS) {
        printf("\tError: Unexpected error code, expected ERROR_ALREADY_EXISTS[%i]\n",
                ERROR_ALREADY_EXISTS);
        PrintWindowsError(errorCode);
        CloseHandle(handle);
        return ReturnCode::Error;
    }

    CloseHandle(handle);

    file = _wfopen(newFile, L"r");
    if (!file) {
        printf("\tFailed: Could not open existing file: %S.\n", newFile);
        PrintWindowsError(GetLastError());
        return ReturnCode::Failure;
    }

    {// Confirm the file contains something
        char buffer[8] = {};
        int scans;
        scans = fscanf_s(file, "%s", buffer, sizeof(buffer));
        if (scans < 1) {
            printf("\tFailed: File opened with OPEN_ALWAYS was empty: %S.\n", newFile);
            fclose(file);
            return ReturnCode::Failure;
        }
    }

    fclose(file);

    // OPEN_EXISTING

    handle = CreateFileW(newFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, flags, NULL);
    errorCode = GetLastError();

    if (handle == INVALID_HANDLE_VALUE) {
        printf("\tError creating new file %S: Expected INVALID_HANDLE_VALUE(%i) but got %i\n",
                newFile, INVALID_HANDLE_VALUE, handle);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    if (errorCode) {
        printf("\tError: Unexpected error code for opening existing file\n",
                ERROR_ALREADY_EXISTS);
        PrintWindowsError(GetLastError());
        CloseHandle(handle);
        return ReturnCode::Error;
    }

    CloseHandle(handle);

    file = _wfopen(newFile, L"r");
    if (!file) {
        printf("\tFailed: Could not open existing file: %S.\n", newFile);
        PrintWindowsError(GetLastError());
        return ReturnCode::Failure;
    }

    {// Confirm the file contains something
        char buffer[4] = {};
        int scans;
        scans = fscanf_s(file, "%s", buffer, sizeof(buffer));
        if (scans < 1) {
            printf("\tFailed: File opened with OPEN_EXISTING was empty: %S.\n", newFile);
            fclose(file);
            return ReturnCode::Failure;
        }
    }

    fclose(file);

    // TRUNCATE_EXISTING

    handle = CreateFileW(newFile, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL,
                        TRUNCATE_EXISTING, flags, NULL);
    errorCode = GetLastError();

    if (handle == INVALID_HANDLE_VALUE) {
        printf("\tError: Failure creating new file %S: Expected INVALID_HANDLE_VALUE(%i) but got %i\n",
                newFile, INVALID_HANDLE_VALUE, handle);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    if (errorCode) {
        printf("\tError: Unexpected error code for opening existing file\n",
                ERROR_ALREADY_EXISTS);
        PrintWindowsError(GetLastError());
        CloseHandle(handle);
        return ReturnCode::Error;
    }

    CloseHandle(handle);

    file = _wfopen(newFile, L"r");
    if (!file) {
        printf("\tFailed: Could not open existing file previously opened with TRUNCATE_ALWAYS: %S.\n", newFile);
        PrintWindowsError(GetLastError());
        return ReturnCode::Failure;
    }

    {// Confirm the file is empty now
        char buffer[4] = {};
        int scans;
        scans = fscanf_s(file, "%s", buffer, sizeof(buffer));
        if (scans > 0) {
            printf("\tFailed: File opened with TRUNCATE_ALWAYS already has content: %S.\n", newFile);
            fclose(file);
            return ReturnCode::Failure;
        }
    }

    fclose(file);

    return ReturnCode::Success;
}

static ReturnCode
TestCreateFile2(const WCHAR* oldFile, const WCHAR* newFile, DWORD attributes, DWORD flags)
{
    HANDLE handle;
    FILE* file;
    DWORD errorCode;
    CREATEFILE2_EXTENDED_PARAMETERS params = {};

    SetLastError(0); // Throw away any lingering error
    printf("Testing: CreateFile2. Input path lengths: %i and %i\n", lstrlenW(oldFile), lstrlenW(newFile));

    // This won't change through the test
    params.dwSize = sizeof(params);
    params.dwFileAttributes = attributes;
    params.dwFileFlags = flags;

    // Attempt to open non-existent file

    handle = CreateFile2(newFile, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, OPEN_EXISTING, &params);
    errorCode = GetLastError();

    if (handle != INVALID_HANDLE_VALUE) {
        printf("\tError: Failure opening non-existent file: Expected INVALID_HANDLE_VALUE(%i) but got %i\n",
                INVALID_HANDLE_VALUE, handle);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    if (errorCode != ERROR_FILE_NOT_FOUND) {
        printf("\tError: Unexpected error code, expected ERROR_FILE_NOT_FOUND[%i]\n", ERROR_FILE_NOT_FOUND);
        if (errorCode) {
            PrintWindowsError(GetLastError());
        } else {
            printf("\tNo Windows error code!\n");
        }

        return ReturnCode::Error;
    }

    // Attempt to create new file when matching name already exists

    handle = CreateFile2(oldFile, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, CREATE_NEW, &params);
    errorCode = GetLastError();

    if (handle != INVALID_HANDLE_VALUE) {
        printf("\tError: Failure creating existing file as new: Expected INVALID_HANDLE_VALUE(%i) but got %i\n",
                INVALID_HANDLE_VALUE, handle);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    if (errorCode != ERROR_FILE_EXISTS) {
        printf("\tError: Unexpected error code, expected ERROR_FILE_EXISTS[%i]\n", ERROR_FILE_EXISTS);
        if (errorCode) {
            PrintWindowsError(GetLastError());
        } else {
            printf("\tNo Windows error code!\n");
        }

        return ReturnCode::Error;
    }

    // Create new file

    handle = CreateFile2(newFile, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, CREATE_NEW, &params);

    if (handle == INVALID_HANDLE_VALUE) {
        printf("\tError: Failure creating new file %S: Expected INVALID_HANDLE_VALUE(%i) but got %i\n",
                newFile, INVALID_HANDLE_VALUE, handle);
        PrintWindowsError(GetLastError());

        return ReturnCode::Error;
    }

    CloseHandle(handle);

    file = _wfopen(newFile, L"w");
    if (!file) {
        printf("\tFailed: Could not open file opened with CREATE_NEW: %S.\n", newFile);
        PrintWindowsError(GetLastError());
        return ReturnCode::Failure;
    }
    fprintf(file, "If you're reading this, the test has failed");
    if (fclose(file)) {
        printf("\tError: Failed to close fd to file created with CREATE_NEW: %S.\n", newFile);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    // CREATE_ALWAYS

    handle = CreateFile2(newFile, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, CREATE_ALWAYS, &params);
    errorCode = GetLastError();

    if (handle == INVALID_HANDLE_VALUE) {
        printf("\tError: Failure creating new file %S: Expected INVALID_HANDLE_VALUE(%i) but got %i\n",
                newFile, INVALID_HANDLE_VALUE, handle);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    if (errorCode && errorCode != ERROR_ALREADY_EXISTS) {
        printf("\tError: Unexpected error code, expected ERROR_ALREADY_EXISTS[%i]\n",
                ERROR_ALREADY_EXISTS);
        PrintWindowsError(GetLastError());
        CloseHandle(handle);
        return ReturnCode::Error;
    }

    CloseHandle(handle);

    file = _wfopen(newFile, L"r+");
    if (!file) {
        printf("\tFailed: Could not open newly created file: %S.\n", newFile);
        PrintWindowsError(GetLastError());
        return ReturnCode::Failure;
    }

    {// Confirm the file is empty now
        char buffer[4] = {};
        int scans;
        scans = fscanf_s(file, "%s", buffer, sizeof(buffer));
        if (scans > 0) {
            printf("\tFailed: File opened with CREATE_ALWAYS already has content: %S.\n", newFile);
            fclose(file);
            return ReturnCode::Failure;
        }
    }

    fprintf(file, "If you're reading this, the test has failed");
    if (fclose(file)) {
        printf("\tError: Failed to close fd to file created with CREATE_ALWAYS: %S.\n", newFile);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    // OPEN_ALWAYS

    handle = CreateFile2(newFile, GENERIC_READ, FILE_SHARE_READ, OPEN_ALWAYS, &params);
    errorCode = GetLastError();

    if (handle == INVALID_HANDLE_VALUE) {
        printf("\tError: Failure creating new file %S: Expected INVALID_HANDLE_VALUE(%i) but got %i\n",
                newFile, INVALID_HANDLE_VALUE, handle);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    if (errorCode && errorCode != ERROR_ALREADY_EXISTS) {
        printf("\tError: Unexpected error code, expected ERROR_ALREADY_EXISTS[%i]\n",
                ERROR_ALREADY_EXISTS);
        PrintWindowsError(GetLastError());
        CloseHandle(handle);
        return ReturnCode::Error;
    }

    CloseHandle(handle);

    file = _wfopen(newFile, L"r");
    if (!file) {
        printf("\tFailed: Could not open existing file: %S.\n", newFile);
        PrintWindowsError(GetLastError());
        return ReturnCode::Failure;
    }

    {// Confirm the file contains something
        char buffer[8] = {};
        int scans;
        scans = fscanf_s(file, "%s", buffer, sizeof(buffer));
        if (scans < 1) {
            printf("\tFailed: File opened with OPEN_ALWAYS was empty: %S.\n", newFile);
            fclose(file);
            return ReturnCode::Failure;
        }
    }

    fclose(file);

    // OPEN_EXISTING

    handle = CreateFile2(newFile, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, &params);
    errorCode = GetLastError();

    if (handle == INVALID_HANDLE_VALUE) {
        printf("\tError: Failure creating new file %S: Expected INVALID_HANDLE_VALUE(%i) but got %i\n",
                newFile, INVALID_HANDLE_VALUE, handle);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    if (errorCode) {
        printf("\tError: Unexpected error code for opening existing file\n",
                ERROR_ALREADY_EXISTS);
        PrintWindowsError(GetLastError());
        CloseHandle(handle);
        return ReturnCode::Error;
    }

    CloseHandle(handle);

    file = _wfopen(newFile, L"r");
    if (!file) {
        printf("\tFailed: Could not open existing file: %S.\n", newFile);
        PrintWindowsError(GetLastError());
        return ReturnCode::Failure;
    }

    {// Confirm the file contains something
        char buffer[4] = {};
        int scans;
        scans = fscanf_s(file, "%s", buffer, sizeof(buffer));
        if (scans < 1) {
            printf("\tFailed: File opened with OPEN_EXISTING was empty: %S.\n", newFile);
            fclose(file);
            return ReturnCode::Failure;
        }
    }

    fclose(file);

    // TRUNCATE_EXISTING

    handle = CreateFile2(newFile, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, TRUNCATE_EXISTING, &params);
    errorCode = GetLastError();

    if (handle == INVALID_HANDLE_VALUE) {
        printf("\tError: Failure creating new file %S: Expected INVALID_HANDLE_VALUE(%i) but got %i\n",
                newFile, INVALID_HANDLE_VALUE, handle);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    if (errorCode) {
        printf("\tError: Unexpected error code for opening existing file\n",
                ERROR_ALREADY_EXISTS);
        PrintWindowsError(GetLastError());
        CloseHandle(handle);
        return ReturnCode::Error;
    }

    CloseHandle(handle);

    file = _wfopen(newFile, L"r");
    if (!file) {
        printf("\tFailed: Could not open existing file previously opened with TRUNCATE_ALWAYS: %S.\n", newFile);
        PrintWindowsError(GetLastError());
        return ReturnCode::Failure;
    }

    {// Confirm the file is empty now
        char buffer[4] = {};
        int scans;
        scans = fscanf_s(file, "%s", buffer, sizeof(buffer));
        if (scans > 0) {
            printf("\tFailed: File opened with TRUNCATE_ALWAYS already has content: %S.\n", newFile);
            fclose(file);
            return ReturnCode::Failure;
        }
    }

    fclose(file);

    return ReturnCode::Success;
}

static ReturnCode
TestCreateHardLinkW(const WCHAR* path, const WCHAR* linkname)
{
    HANDLE handle;

    SetLastError(0); // Throw away any lingering error
    printf("Testing: CreateHardLinkW. Input path length: %i\n", lstrlenW(path));

    if (!CreateHardLinkW(linkname, path, NULL)) {
        printf("\tError: Failed to create hard link: %S->%S.\n", linkname, path);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    handle = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        printf("\tError: Failed to get handle to hard link: %S->%S.\n", linkname, path);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    if (!CloseHandle(handle)) {
        printf("\tError: Failed to close handle to hard link: %S->%S.\n", linkname, path);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    if (!DeleteFileW(linkname)) {
        printf("\tError: Failed to delete hard link: %S->%S.\n", linkname, path);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    return ReturnCode::Success;
}

static ReturnCode
TestCreateSymbolicLinkToFileW(const WCHAR* path, const WCHAR* linkname)
{
    HANDLE handle;

    SetLastError(0); // Throw away any lingering error
    printf("Testing: CreateSymbolicLinkW on file. Input path length: %i\n", lstrlenW(path));

    if (!CreateSymbolicLinkW(linkname, path, 0)) {
        printf("\tError: Failed to create symbolic link: %S->%S.\n", linkname, path);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    handle = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        printf("\tError: Failed to get handle to symbolic link: %S->%S.\n", linkname, path);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    if (!CloseHandle(handle)) {
        printf("\tError: Failed to close handle to symbolic link: %S->%S.\n", linkname, path);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    if (!DeleteFileW(linkname)) {
        printf("\tError: Failed to delete symbolic link: %S->%S.\n", linkname, path);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    return ReturnCode::Success;
}

static ReturnCode
TestDeleteFileW(const WCHAR* path)
{
    HANDLE handle;

    SetLastError(0); // Throw away any lingering error
    printf("Testing: DeleteFileW. Input path length: %i\n", lstrlenW(path));

    if (!DeleteFileW(path)) {
        printf("\tError: Failed to delete file: %S.\n", path);
        return ReturnCode::Error;
    }

    handle = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (handle == INVALID_HANDLE_VALUE) {
        DWORD errorCode = GetLastError();
        if (errorCode == ERROR_ACCESS_DENIED || errorCode == ERROR_FILE_NOT_FOUND) {
            return ReturnCode::Success;
        } else {
            WCHAR buffer[256] = {0};
            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, errorCode, 0,
                        shortBuffer, sizeof(shortBuffer) / sizeof(WCHAR) - 1, 0);
            printf("Warning: Unexpected error code [%i] %S, expected either ERROR_ACCESS_DENIED"
                   " or ERROR_FILE_NOT_FOUND\n", errorCode, shortBuffer);
            return ReturnCode::Warning;
        }
    } else {
        printf("\tError: Unexpected handle [0x%X] returned after deleting, implying file still exists: %S.\n",
                handle, path);
        return ReturnCode::Error;
    }
}

static ReturnCode
TestFindFirstFileW(const WCHAR* path, const WCHAR* comparisonFileName)
{
    /* Note that regardless of backing implementation, the filename searched for cannot itself exceed
     * MAX_PATH or it won't fit inside WIN32_FIND_DATAW.cFileName anyway */
    WIN32_FIND_DATAW data;
    HANDLE handle;
    ReturnCode result = ReturnCode::Success;

    SetLastError(0); // Throw away any lingering error
    printf("Testing: FindFirstFileW. Input path length: %i\n", lstrlenW(path));

    if (lstrlenW(comparisonFileName) >= MAX_PATH) {
        printf("\tError: Comparison filename cannot exceed MAX_PATH[%i]-1 (is %i): %S.\n",
               MAX_PATH, lstrlenW(comparisonFileName), comparisonFileName);
        return ReturnCode::Error;
    }

    handle = FindFirstFileW(path, &data);
    if (handle == INVALID_HANDLE_VALUE) {
        printf("\tError: Failed to get handle to file %S.\n", path);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    if (StrCmpW(data.cFileName, comparisonFileName) != 0) {
        printf("\tWarning: Found filename didn't match comparison: %S vs %S\n",
                data.cFileName, comparisonFileName);
        result = ReturnCode::Warning;
    }

    return result;
}

static ReturnCode
TestFindFirstFileExW(const WCHAR* path, const WCHAR* comparisonFileName, DWORD flags)
{
    /* Note that regardless of backing implementation, the filename searched for cannot itself exceed
     * MAX_PATH or it won't fit inside WIN32_FIND_DATAW.cFileName anyway */
    WIN32_FIND_DATAW data;
    HANDLE handle;
    ReturnCode result = ReturnCode::Success;

    SetLastError(0); // Throw away any lingering error
    printf("Testing: FindFirstFileExW. Input path length: %i, flags: 0x%X\n", lstrlenW(path), flags);

    if (lstrlenW(comparisonFileName) >= MAX_PATH) {
        printf("\tError: Comparison filename cannot exceed MAX_PATH[%i]-1 (is %i): %S.\n",
               MAX_PATH, lstrlenW(comparisonFileName), comparisonFileName);
        return ReturnCode::Error;
    }

    handle = FindFirstFileExW(path, FindExInfoStandard, &data, FindExSearchNameMatch, NULL, flags);
    if (handle == INVALID_HANDLE_VALUE) {
        printf("\tError: Failed to get handle to file %S.\n", path);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    if (StrCmpW(data.cFileName, comparisonFileName) != 0) {
        printf("\tWarning: Found filename didn't match comparison: %S vs %S\n",
                data.cFileName, comparisonFileName);
        result = ReturnCode::Warning;
    }

    return result;
}

static ReturnCode
TestFindNextFileW(const WCHAR* directory, const WCHAR* filename, DWORD flags)
{
    /* Note that regardless of backing implementation, the filename searched for cannot itself exceed
     * MAX_PATH or it won't fit inside WIN32_FIND_DATAW.cFileName anyway. Due to this, this function isn't
     * strictly relevant to Longpath support because it doesn't take in a path argument in the first place. */
    WIN32_FIND_DATAW data;
    HANDLE handle;
    BOOL next = FALSE;
    DWORD errorCode;
    ReturnCode result;
    bool nextCalled;

    SetLastError(0); // Throw away any lingering error
    printf("Testing: FindNextFileW. Input directory, filename length: %i, flags: 0x%X\n",
            lstrlenW(directory), lstrlenW(filename), flags);

    if (lstrlenW(filename) >= MAX_PATH) {
        printf("\tError: Target filename cannot exceed MAX_PATH[%i]-1 (is %i): %S.\n",
               MAX_PATH, lstrlenW(filename), filename);
        return ReturnCode::Error;
    }

    handle = FindFirstFileExW(directory, FindExInfoBasic, &data, FindExSearchNameMatch, NULL, flags);
    if (handle == INVALID_HANDLE_VALUE) {
        printf("\tError: FindFirstFileExW failed to get handle to directory %S.\n", directory);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    result = ReturnCode::Failure;
    next = TRUE;
    nextCalled = false;
    while (next != FALSE) {
        if (StrCmpW(filename, data.cFileName) == 0) {
            result = ReturnCode::Success;
            nextCalled = true;
            break;
        } else {
            next = FindNextFileW(handle, &data);
            nextCalled = true;
        }
    }

    if (result == ReturnCode::Failure) {
        printf("\tFailed: Could not find file %S in directory %S.\n", filename, directory);
        return ReturnCode::Failure;
    }

    if (!nextCalled) {
        printf("\tWarning: Target file (%S) was first in the directory (%S) and so FindNextFileW has not been called.\n",
                filename, directory);
        result = ReturnCode::Warning;
    }

    return result;
}

static ReturnCode
TestFileAttributesW(const WCHAR* path)
{
    DWORD output;
    WIN32_FILE_ATTRIBUTE_DATA data;
    ULARGE_INTEGER large;
    ReturnCode result = ReturnCode::Success;

    SetLastError(0); // Throw away any lingering error
    printf("Testing: SetFileAttributesW, GetFileAttributesW, and GetFileAttributesExW. Input path length: %i\n", lstrlenW(path));

    if (!SetFileAttributesW(path, FILE_ATTRIBUTE_NORMAL)) {
        printf("\tError: Failed set attributes to FILE_ATTRIBUTE_NORMAL on file %s.\n", path);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    output = GetFileAttributesW(path);
    if (output == INVALID_FILE_ATTRIBUTES) {
        printf("\tError: Failed get attributes from file %S.\n", path);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    if (output != FILE_ATTRIBUTE_NORMAL) {
        printf("\tWarning: Unexpected attributes from file %S, expected 0x%X but got 0x%X.\n",
                path, FILE_ATTRIBUTE_NORMAL, output);
        PrintWindowsError(GetLastError());
        result = ReturnCode::Warning;
    }

    if (GetFileAttributesExW(path, GetFileExInfoStandard, &data) == FALSE) {
        printf("\tError: Failed to get extended attributes from file %S.\n", path);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    if (data.dwFileAttributes != FILE_ATTRIBUTE_NORMAL) {
        printf("\tWarning: Unexpected attributes from file %S, expected 0x%X but got 0x%X.\n",
                path, FILE_ATTRIBUTE_NORMAL, data.dwFileAttributes);
        result = ReturnCode::Warning;
    }

    // Sanity Checks
    large.LowPart = data.ftCreationTime.dwLowDateTime;
    large.HighPart = data.ftCreationTime.dwHighDateTime;
    if (large.QuadPart < JAN_1_2000) {
        printf("\tWarning: File creation time reported before Jan 1st, 2000: %S.\n", path);
        result = ReturnCode::Warning;
    }

    large.LowPart = data.nFileSizeLow;
    large.HighPart = data.nFileSizeHigh;
    if (large.QuadPart < 16) {
        printf("\tWarning: File size reported as less than 16 bytes: %S.\n", path);
        result = ReturnCode::Warning;
    }

    return result;
}

static ReturnCode
TestGetFullPathNameW(const WCHAR* path, const WCHAR* comparisonPath)
{
    WCHAR* buffer = 0;
    WCHAR* filePartPtr = 0;
    DWORD length;
    DWORD expectedLength;

    SetLastError(0); // Throw away any lingering error
    expectedLength = lstrlenW(comparisonPath); /* GetFullPathNameW's return value accounts for null terminator, but only if it fails */
    printf("Testing: GetFullPathNameW. Input path length: %i\n", lstrlenW(path));

    memset(shortBuffer, 0, sizeof(shortBuffer));
    length = GetFullPathNameW(path, sizeof(shortBuffer) / sizeof(*shortBuffer), shortBuffer, &filePartPtr);
    if (length < expectedLength) {
        printf("\tError: Failed to retrieve required path length from %S, expected %i got %i.\n",
                path, expectedLength, length);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    if (length > MAX_PATH) {
        buffer = longBuffer;
        memset(longBuffer, 0, sizeof(longBuffer));
        length = GetFullPathNameW(path, sizeof(longBuffer) / sizeof(*longBuffer), buffer, &filePartPtr);
        if (length < expectedLength) {
            printf("\tError: Failed to retrieve required path length from %S, expected %i got %i.\n",
                    path, expectedLength, length);
            PrintWindowsError(GetLastError());
            return ReturnCode::Error;
        }
    } else {
        buffer = shortBuffer;
    }

    { // Do checks
        int delta;
        WCHAR* check = buffer;
        DWORD checkLength = length;

        /* UNC prefix check.
         * Wine's policy is to invisibly act as if longpath support is always enabled so simply ignore the
         * prefix if it shows up. */
        if (wcsncmp(buffer, L"\\\\?\\", 4) == 0) {
            if (verbose)
                printf("\tNote: GetLongPathNameW has returned a path prepended with the UNC prefix \"\\\\?\\\"\n");
            check += 4;
            checkLength -= 4;
        }

        if (checkLength < expectedLength || checkLength > expectedLength + 1) {
            printf("\tFailed: Found file path was the wrong length (was %i but expected %i)\n",
                    checkLength, expectedLength);
            printf("\tInput:  \"%S\"\n", path);
            printf("\tExpected:  \"%S\"\n", comparisonPath);
            printf("\tOutput: \"%S\"\n", check);
            return ReturnCode::Failure;
        }

        delta = StrCmpW(check, comparisonPath);
        if (delta != 0) {
            printf("\tFailed: Found file path name does not match expected (delta: %i)\n", delta);
            printf("\tInput:  \"%S\"\n", path);
            printf("\tExpected:  \"%S\"\n", comparisonPath);
            printf("\tOutput: \"%S\"\n", check);
            return ReturnCode::Failure;
        }
    }

    return ReturnCode::Success;
}

static ReturnCode
TestGetLongPathNameW(const WCHAR* path, const WCHAR* comparisonPath)
{
    WCHAR* buffer = 0;
    DWORD length;
    DWORD expectedLength;

    SetLastError(0); // Throw away any lingering error
    expectedLength = lstrlenW(comparisonPath); /* GetLongPathNameW's return value accounts for null terminator */
    printf("Testing: GetLongPathNameW. Input path length: %i\n", lstrlenW(path));

    memset(shortBuffer, 0, sizeof(shortBuffer));
    length = GetLongPathNameW(path, shortBuffer, sizeof(shortBuffer) / sizeof(*shortBuffer));
    if (length < expectedLength) {
        printf("\tError: Failed to retrieve required path length from %S, expected %i got %i.\n",
                path, expectedLength, length);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    if (length > MAX_PATH) {
        buffer = longBuffer;
        memset(longBuffer, 0, sizeof(longBuffer));
        length = GetLongPathNameW(path, longBuffer, sizeof(longBuffer) / sizeof(*longBuffer));
        if (length < expectedLength) {
            printf("\tError: Failed to retrieve required path length from %S, expected %i got %i.\n",
                    path, expectedLength, length);
            PrintWindowsError(GetLastError());
            return ReturnCode::Error;
        }
    } else {
        buffer = shortBuffer;
    }

    { // Do checks
        int delta;
        WCHAR* check = buffer;
        DWORD checkLength = length;

        /* UNC prefix check.
            * Wine's policy is to invisibly act as if longpath support is always enabled so simply ignore the
            * prefix if it shows up. */
        if (wcsncmp(buffer, L"\\\\?\\", 4) == 0) {
            if (verbose)
                printf("\tNote: GetLongPathNameW has returned a path prepended with the UNC prefix \"\\\\?\\\"\n");
            check += 4;
            checkLength -= 4;
        }

        if (checkLength < expectedLength || checkLength > expectedLength + 1) {
            printf("\tFailed: Found file path was the wrong length (was %i but expected %i)\n",
                    checkLength, expectedLength);
            //printf("\tInput:  \"%S%S%S\"\n", path, filename, extension);
            printf("\tInput:     \"%S\"\n", path);
            printf("\tExpected:  \"%S\"\n", comparisonPath);
            printf("\tOutput:    \"%S\"\n", check);
            return ReturnCode::Failure;
        }

        delta = StrCmpW(check, comparisonPath);
        if (delta != 0) {
            printf("\tFailed: Found file path name does not match expected (delta: %i)\n", delta);
            printf("\tInput:  \"%S\"\n", path);
            printf("\tExpected:  \"%S\"\n", comparisonPath);
            printf("\tOutput: \"%S\"\n", check);
            return ReturnCode::Failure;
        }
    }

    return ReturnCode::Success;
}

static ReturnCode
TestMoveFileW(const WCHAR* sourceFile, const WCHAR* destinationFile)
{
    HANDLE file;

    SetLastError(0); // Throw away any lingering error
    printf("Testing: MoveFileW. Input path lengths: %i, %i\n", lstrlenW(sourceFile), lstrlenW(destinationFile));

    if (!MoveFileW(sourceFile, destinationFile)) {
        printf("\tError: Failed to move file %S to %S\n", sourceFile, destinationFile);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    file = CreateFileW(sourceFile, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    CloseHandle(file);
    if (file != INVALID_HANDLE_VALUE) {
        printf("\tFailed: Source file still exists after move %S\n", sourceFile);
        PrintWindowsError(GetLastError());
        return ReturnCode::Failure;
    }

    file = CreateFileW(destinationFile, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    CloseHandle(file);
    if (file == INVALID_HANDLE_VALUE) {
        printf("\tFailed: Destination file not found after move %S\n", destinationFile);
        PrintWindowsError(GetLastError());
        return ReturnCode::Failure;
    }

    // Cleanup for next move test
    if (!MoveFileW(destinationFile, sourceFile)) {
        printf("\tError: Failed to move file %S to %S\n", sourceFile, destinationFile);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    file = CreateFileW(sourceFile, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    CloseHandle(file);
    if (file == INVALID_HANDLE_VALUE) {
        printf("\tFailed: Source file not restored %S\n", sourceFile);
        PrintWindowsError(GetLastError());
        return ReturnCode::Failure;
    }

    return ReturnCode::Success;
}

static ReturnCode
TestMoveFileExW(const WCHAR* sourceFile, const WCHAR* destinationFile, DWORD flags)
{
    HANDLE file;

    SetLastError(0); // Throw away any lingering error
    printf("Testing: MoveFileExW. Input path lengths: %i, %i\n", lstrlenW(sourceFile), lstrlenW(destinationFile));

    if (!MoveFileExW(sourceFile, destinationFile, flags)) {
        printf("\tError: Failed to move file %S to %S\n", sourceFile, destinationFile);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    file = CreateFileW(sourceFile, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    CloseHandle(file);
    if (file != INVALID_HANDLE_VALUE) {
        printf("\tError: Source file still exists after move %S\n", sourceFile);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    file = CreateFileW(destinationFile, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    CloseHandle(file);
    if (file == INVALID_HANDLE_VALUE) {
        printf("\tError: Destination file not found after move %S\n", destinationFile);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    // Cleanup for next move test
    if (!MoveFileExW(destinationFile, sourceFile, 0)) {
        printf("\tError: Failed to move file %S to %S\n", sourceFile, destinationFile);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    file = CreateFileW(sourceFile, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    CloseHandle(file);
    if (file == INVALID_HANDLE_VALUE) {
        printf("\tError: Source file not restored %S\n", sourceFile);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    return ReturnCode::Success;
}

static ReturnCode
TestMoveFileWithProgressW(const WCHAR* sourceFile, const WCHAR* destinationFile,
                            LPPROGRESS_ROUTINE callback, DWORD flags)
{
    HANDLE file;

    SetLastError(0); // Throw away any lingering error
    printf("Testing: MoveFileWithProgressW. Input path lengths: %i, %i\n", lstrlenW(sourceFile), lstrlenW(destinationFile));

    if (!MoveFileWithProgressW(sourceFile, destinationFile, callback, 0, flags)) {
        printf("\tError: Failed to move file %S to %S\n", sourceFile, destinationFile);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    file = CreateFileW(sourceFile, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    CloseHandle(file);
    if (file != INVALID_HANDLE_VALUE) {
        printf("\tError: Source file still exists after move %S\n", sourceFile);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    file = CreateFileW(destinationFile, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    CloseHandle(file);
    if (file == INVALID_HANDLE_VALUE) {
        printf("\tError: Destination file not found after move %S\n", destinationFile);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    // Cleanup for next move test
    if (!MoveFileWithProgressW(destinationFile, sourceFile, 0, 0, 0)) {
        printf("\tError: Failed to move file %S to %S\n", sourceFile, destinationFile);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    file = CreateFileW(sourceFile, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    CloseHandle(file);
    if (file == INVALID_HANDLE_VALUE) {
        printf("\tError: Source file not restored %S\n", sourceFile);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    return ReturnCode::Success;
}

static ReturnCode
TestReplaceFileW(const WCHAR* replacedFile, const WCHAR* replacementFile, const WCHAR* tempBackupFile)
{
    char checkBuffer[32];
    BOOL successful;
    HANDLE replacedHandle;
    HANDLE replacementHandle;
    HANDLE tempBackupHandle;

    SetLastError(0); // Throw away any lingering error
    printf("Testing: ReplaceFileW. Input path lengths: %i, %i, %i\n",
            lstrlenW(replacedFile), lstrlenW(replacementFile), lstrlenW(tempBackupFile));

    if (!ReplaceFileW(replacedFile, replacementFile, tempBackupFile, 0, 0, 0)) {
        printf("\tError: Could not replace file\n");
        printf("\tTo Replace:  \"%S\"\n", replacedFile);
        printf("\tReplacement: \"%S\"\n", replacementFile);
        printf("\tTemporary Backup: \"%S\"\n", tempBackupFile);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    // Actual checks

    replacedHandle = CreateFileW(replacedFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (replacedHandle == INVALID_HANDLE_VALUE) {
        printf("\tError: Failed to open replaced file %S\n", replacedFile);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    memset(checkBuffer, 0, sizeof(checkBuffer) * sizeof(*checkBuffer));
    successful = ReadFile(replacedHandle, checkBuffer, sizeof(checkBuffer) - sizeof(*checkBuffer), 0, 0);
    CloseHandle(replacedHandle);
    if (!successful) {
        printf("\tError: Failed to read file %S\n", replacedFile);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    if (strstr(checkBuffer, "should be")) {
        printf("\tFailed: Replaced file still has original contents \"%S\"\n", checkBuffer);
        return ReturnCode::Failure;
    }

    if (strstr(checkBuffer, "has been") == 0) {
        printf("\tFailed: Replaced file does not have new contents \"%S\"\n", checkBuffer);
        return ReturnCode::Failure;
    }

    replacementHandle = CreateFileW(replacementFile, GENERIC_READ, 0, NULL,
                                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (replacementHandle != INVALID_HANDLE_VALUE) {
        printf("\tError: File to use as replacement still exists %S\n", replacementFile);
        CloseHandle(replacementHandle);
        return ReturnCode::Error;
    }

    if (tempBackupFile) {
        tempBackupHandle = CreateFileW(tempBackupFile, GENERIC_READ, 0, NULL,
                                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (tempBackupHandle == INVALID_HANDLE_VALUE) {
            printf("\tError: Temporary backup file not found %S\n", tempBackupFile);
            return ReturnCode::Error;
        }

        memset(checkBuffer, 0, sizeof(checkBuffer) * sizeof(*checkBuffer));
        successful = ReadFile(tempBackupHandle, checkBuffer, sizeof(checkBuffer) - sizeof(*checkBuffer), 0, 0);
        CloseHandle(tempBackupHandle);
        if (!successful) {
            printf("\tError: Failed to read file %S\n", tempBackupFile);
            PrintWindowsError(GetLastError());
            return ReturnCode::Error;
        }

        if (strstr(checkBuffer, "should be") == 0) {
            printf("\tFailed: Temporary backup file doesn't contain original contents \"%S\"\n", checkBuffer);
            return ReturnCode::Failure;
        }
    }

    return ReturnCode::Success;
}

static ReturnCode
TestSearchPathW(const WCHAR* path, const WCHAR* filename, const WCHAR* extension)
{
    WCHAR* buffer = 0;
    DWORD length;
    DWORD expectedLength;

    SetLastError(0); // Throw away any lingering error
    expectedLength = lstrlenW(path) + lstrlenW(filename) + lstrlenW(extension);
    printf("Testing: SearchPathW. Input path length: %i\n", expectedLength);

    memset(shortBuffer, 0, sizeof(shortBuffer));
    length = SearchPathW(path, filename, extension, sizeof(shortBuffer) / sizeof(*shortBuffer), shortBuffer, 0);
    if (length < expectedLength) {
        printf("\tError: Failed to retrieve required path length from %S%S%S, expected %i got %i.\n",
                path, filename, extension, expectedLength, length);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    if (length > MAX_PATH) {
        buffer = longBuffer;
        memset(longBuffer, 0, sizeof(longBuffer));
        length = SearchPathW(path, filename, extension, sizeof(longBuffer) / sizeof(*longBuffer), buffer, 0);
        if (length < expectedLength) {
            printf("\tError: Failed to retrieve required path length from %S%S%S, expected %i got %i.\n",
                    path, filename, extension, expectedLength, length);
            PrintWindowsError(GetLastError());
            return ReturnCode::Error;
        }
    } else {
        buffer = shortBuffer;
    }

    { // Do checks
        int delta;
        WCHAR* check = buffer;
        WCHAR* expectedFullPath = longBufferAlt;
        DWORD checkLength = length;

        /* UNC prefix check.
         * Wine's policy is to invisibly act as if longpath support is always enabled so simply ignore the
         * prefix if it shows up. */
        if (wcsncmp(buffer, L"\\\\?\\", 4) == 0) {
            if (verbose)
                printf("\tNote: GetFinalPathNameByHandleW has returned a path prepended with the UNC prefix \"\\\\?\\\"\n");
            check += 4;
            checkLength -= 4;
        }

        if (checkLength != expectedLength) {
            printf("\tFailed: Found file path was the wrong length (was %i but expected %i)\n",
                    checkLength, expectedLength);
            printf("\tInput:  \"%S%S%S\"\n", path, filename, extension);
            printf("\tOutput: \"%S\"\n", buffer);
            return ReturnCode::Failure;
        }

        memset(longBufferAlt, 0, sizeof(longBufferAlt));
        _snwprintf(longBufferAlt, sizeof(longBufferAlt) / sizeof(*longBufferAlt) - 1,
                    L"%s%s%s", path, filename, extension);

        delta = StrCmpNIW(expectedFullPath, check, checkLength);
        if (delta != 0) {
            printf("\tFailed: Found file path name does not match input (delta: %i)\n", delta);
            printf("\tInput:  \"%S\"\n", expectedFullPath);
            printf("\tOutput: \"%S\"\n", check);
            return ReturnCode::Failure;
        }
    }

    return ReturnCode::Success;
}

static ReturnCode
TestFindFirstStreamW(const WCHAR* path)
{
    HANDLE file;
    WIN32_FIND_STREAM_DATA data;

    printf("Testing: FindFirstStreamW and FindNextStreamW. Input path length: %i\n", lstrlenW(path));
    printf("Note that as of writing, FindFirstStreamW and FindNextStreamW are not implemented in Wine "
            "and so no tests has been written\n");

    file = FindFirstStreamW(path, FindStreamInfoStandard, &data, 0);

    if (file == INVALID_HANDLE_VALUE) {
        printf("\tError: Could not retrieve stream for file %S\n", path);
        PrintWindowsError(GetLastError());
        return ReturnCode::ExpectedFailure;
    }

    if (verbose) {
        do {
            printf("\tStream Name: %.*S\n", MAX_PATH + 36, data.cStreamName);
        } while (FindNextStreamW(file, &data));
    }

    /* TODO If this ever gets implemented there will need to be a test made to confirm the contents
     * of the streams. There didn't seem to be a way to test this on ext4 at time of writing. */

    return ReturnCode::UnexpectedSuccess;
}

static ReturnCode
TestGetCompressedFileSizeW(const WCHAR* path)
{
    DWORD size = 0;
    printf("Testing: GetCompressedFileSizeW. Input path length: %i\n", lstrlenW(path));

    size = GetCompressedFileSizeW(path, NULL); // High word not relevant to long path support
    if (size == INVALID_FILE_SIZE) {
        printf("\tError: Could not get size for file %S\n", path);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    } else if (size != fileSize) {
        printf("\tError: Retrieved file size didn't match (expected %i, got %i)\n", fileSize, size);
        printf("\tInput file path:  \"%S\"\n", path);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    return ReturnCode::Success;
}

static ReturnCode
TestGetFinalPathNameByHandleW(const WCHAR* path, DWORD flags)
{
    HANDLE file = INVALID_HANDLE_VALUE;
    DWORD length = 0;
    DWORD expectedLength;
    WCHAR* buffer = 0;
    ReturnCode result = ReturnCode::Success;

    SetLastError(0); // Throw away any lingering error
    expectedLength = lstrlenW(path);
    printf("Testing: GetFinalPathNameByHandleW. Input path length: %i, Flags:", expectedLength);
    if (flags & FILE_NAME_OPENED) {
        /* As of writing, FILE_NAME_OPENED is not supported by Wine (as it requires changes to the Wine
         * Server) and so acts the same as FILE_NAME_NORMALIZED. If this is changed in the future this test
         * may need additional code path/s. */
        printf(" FILE_NAME_OPENED");
    } else {
        printf(" FILE_NAME_NORMALIZED");
    }
    if (flags & VOLUME_NAME_NONE) {
        printf(" | VOLUME_NAME_NONE");
    } else if (flags & VOLUME_NAME_GUID) {
        printf(" | VOLUME_NAME_GUID");
    } else if (flags & VOLUME_NAME_NT) {
        printf(" | VOLUME_NAME_NT");
    } else {
        printf(" | VOLUME_NAME_DOS");
    }
    printf("\n");

    file = CreateFileW(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        printf("\tError: Failed to open file %S\n", path);
        PrintWindowsError(GetLastError());
        return ReturnCode::Error;
    }

    /* Test paths all have drive letters, strip them if testing with VOLUME_NAME_NONE but leave a leading
     * backslash behind */
    if (flags & VOLUME_NAME_NONE) {
        path += 2;
        expectedLength -= 2;
    }

    memset(shortBuffer, 0, sizeof(shortBuffer));
    length = GetFinalPathNameByHandleW(file, shortBuffer, sizeof(shortBuffer), flags);
    if (length < expectedLength) {
        printf("\tError: Failed to retrieve required path length from %S, expected %i got %i.\n",
                path, expectedLength, length);
        PrintWindowsError(GetLastError());
        result = ReturnCode::Error;
        goto end;
    }

    /* Note that for long paths GetFinalPathNameByHandleW has to be called twice. This first call is expected
     * to fail and is only necessary to determine the required length of the longer buffer that we will
     * allocate afterwards. Despite expecting a failure, the documentation still marks the buffer pointer
     * and length arguments as non-optional so you have to pass at least something in for the function to exit
     * with the expected failure code. */
    if (length / sizeof(WCHAR) > MAX_PATH) {
        buffer = longBuffer;
        memset(longBuffer, 0, sizeof(longBuffer));
        length = GetFinalPathNameByHandleW(file, longBuffer, sizeof(longBuffer), flags);
        if (length < expectedLength) {
            printf("\tError: Failed to retrieve required path length from %S, expected %i got %i.\n",
                    path, expectedLength, length);
            PrintWindowsError(GetLastError());
            result = ReturnCode::Error;
            goto end;
        }
    } else {
        buffer = shortBuffer;
    }

    { // Do checks
        if (flags & (VOLUME_NAME_NT | VOLUME_NAME_GUID)) {
            // Don't check for exact matches with NT or GUID
            WCHAR* check;

            if (length < expectedLength) {
                printf("\tFailed: Final path was the wrong length (was %i but expected at least %i)\n",
                        length, expectedLength);
                printf("\tInput:  \"%S\"\n", path);
                printf("\tOutput: \"%S\"\n", buffer);
                result = ReturnCode::Failure;
                goto end;
            }

            check = StrStrW(buffer, path + 2);
            if (!check) {
                printf("\tFailed: Output path name does not contain input path\n");
                printf("\tInput:  \"%S\"\n", path);
                printf("\tOutput: \"%S\"\n", buffer);
                result = ReturnCode::Failure;
                goto end;
            }
        } else {
            int delta;
            WCHAR* check = buffer;
            DWORD checkLength = length;

            /* UNC prefix check.
             * Wine's policy is to invisibly act as if longpath support is always enabled so simply ignore the
             * prefix if it shows up. */
            if (wcsncmp(buffer, L"\\\\?\\", 4) == 0) {
                if (verbose)
                    printf("\tNote: GetFinalPathNameByHandleW has returned a path prepended with the UNC prefix \"\\\\?\\\"\n");
                check += 4;
                checkLength -= 4;
            }

            if (checkLength != expectedLength) {
                printf("\tFailed: Final path was the wrong length (was %i but expected %i)\n",
                        checkLength, expectedLength);
                printf("\tInput:  \"%S\"\n", path);
                printf("\tOutput: \"%S\"\n", check);
                result = ReturnCode::Failure;
                goto end;
            }

            delta = StrCmpW(path, check);
            if (delta != 0) {
                printf("\tFailed: Final path name does not match input path (delta: %i)\n", delta);
                printf("\tInput:  \"%S\"\n", path);
                printf("\tOutput: \"%S\"\n", check);
                result = ReturnCode::Failure;
                goto end;
            }
        }
    }

end:
    CloseHandle(file);
    return result;
}

/* ===============================================================================================
 *                                  MAIN FUNCTION
 * =============================================================================================== */

struct ResultCounters
{
    unsigned successes = 0;
    unsigned warnings = 0;
    unsigned unexpected = 0;

    unsigned failures = 0;
    unsigned errors = 0;
    unsigned expected = 0;

    void AddResult(ReturnCode code) {
        switch (code) {
            case ReturnCode::Success: {
                successes++;
                printf("...Success!\n");
            } break;
            case ReturnCode::Warning: {
                warnings++;
                printf("...Success! (With Warnings)\n");
            } break;
            case ReturnCode::UnexpectedSuccess: {
                unexpected++;
                printf("...Success! (Unexpected)\n");
            } break;
            case ReturnCode::Failure: {
                failures++;
                printf("...FAILED!\n");
            } break;
            case ReturnCode::Error: {
                errors++;
                printf("...FAILED! (Error)\n");
            } break;
            case ReturnCode::ExpectedFailure: {
                expected++;
                printf("...FAILED! (Expected)\n");
            } break;
        }
    }
};

int
main( int argc, char *argv[ ] )
{
    ResultCounters results;
    // Directory management functions with opt-in long path support

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = true;
            printf("Printing verbose output\n");
        }
    }

    { // CreateDirectoryW
        results.AddResult(TestCreateDirectoryW(L"C:\\" CREATE_FOLDER));
        results.AddResult(TestCreateDirectoryW(L"C:\\" LONG_FOLDER L"\\" CREATE_FOLDER));
    }
    { // CreateDirectoryExW
        results.AddResult(TestCreateDirectoryExW(L"C:\\" SHORT_FOLDER, L"C:\\" CREATE_FOLDER_EX));
        results.AddResult(TestCreateDirectoryExW(L"C:\\" LONG_FOLDER, L"C:\\" LONG_FOLDER L"\\" CREATE_FOLDER_EX));
    }

    { // RemoveDirectoryW
        results.AddResult(TestRemoveDirectoryW(L"C:\\" REMOVE_FOLDER));
        results.AddResult(TestRemoveDirectoryW(L"C:\\" LONG_FOLDER L"\\" REMOVE_FOLDER));
    }

    { // SetCurrentDirectoryW
        results.AddResult(TestGetAndSetCurrentDirectoryW(L"C:\\" SHORT_FOLDER));
        results.AddResult(TestGetAndSetCurrentDirectoryW(L"C:\\" LONG_FOLDER));
    }

    // File management functions with opt-in long path support
    { // CopyFileW
        results.AddResult(TestCopyFileW(L"C:\\" SHORT_FILE_NAME FILE_EXT,
                                        L"C:\\" SHORT_FILE_NAME ALT_SUFFIX FILE_EXT,
                                        L"C:\\" SHORT_FILE_NAME COPYW_SUFFIX FILE_EXT));
        results.AddResult(TestCopyFileW(L"C:\\" LONG_FOLDER L"\\" LONG_FILE_NAME FILE_EXT,
                                        L"C:\\" LONG_FOLDER L"\\" LONG_FILE_NAME ALT_SUFFIX FILE_EXT,
                                        L"C:\\" LONG_FOLDER L"\\" LONG_FILE_NAME COPYW_SUFFIX FILE_EXT));
    }

    { // CopyFile2
        results.AddResult(TestCopyFile2(L"C:\\" SHORT_FILE_NAME FILE_EXT,
                                        L"C:\\" SHORT_FILE_NAME ALT_SUFFIX FILE_EXT,
                                        L"C:\\" SHORT_FILE_NAME COPY2_SUFFIX FILE_EXT));
        results.AddResult(TestCopyFile2(L"C:\\" LONG_FOLDER L"\\" LONG_FILE_NAME FILE_EXT,
                                        L"C:\\" LONG_FOLDER L"\\" LONG_FILE_NAME ALT_SUFFIX FILE_EXT,
                                        L"C:\\" LONG_FOLDER L"\\" LONG_FILE_NAME COPY2_SUFFIX FILE_EXT));
    }

    { // CopyFileExW
        results.AddResult(TestCopyFileExW(L"C:\\" SHORT_FILE_NAME FILE_EXT,
                                          L"C:\\" SHORT_FILE_NAME ALT_SUFFIX FILE_EXT,
                                          L"C:\\" SHORT_FILE_NAME COPYEXW_SUFFIX FILE_EXT));
        results.AddResult(TestCopyFileExW(L"C:\\" LONG_FOLDER L"\\" LONG_FILE_NAME FILE_EXT,
                                          L"C:\\" LONG_FOLDER L"\\" LONG_FILE_NAME ALT_SUFFIX FILE_EXT,
                                          L"C:\\" LONG_FOLDER L"\\" LONG_FILE_NAME COPYEXW_SUFFIX FILE_EXT));
    }

    { // CreateFileW
        results.AddResult(TestCreateFileW(L"C:\\" SHORT_FILE_NAME FILE_EXT,
                                          L"C:\\" SHORT_FILE_NAME CREATEW_SUFFIX FILE_EXT,
                                          FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING));
        results.AddResult(TestCreateFileW(L"C:\\" LONG_FOLDER L"\\" LONG_FILE_NAME FILE_EXT,
                                          L"C:\\" LONG_FOLDER L"\\" LONG_FILE_NAME CREATEW_SUFFIX FILE_EXT,
                                          FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING));
    }

    { // CreateFile2
        results.AddResult(TestCreateFile2(L"C:\\" SHORT_FILE_NAME FILE_EXT,
                                          L"C:\\" SHORT_FILE_NAME CREATE2_SUFFIX FILE_EXT,
                                          FILE_ATTRIBUTE_NORMAL, FILE_FLAG_NO_BUFFERING));
        results.AddResult(TestCreateFile2(L"C:\\" LONG_FOLDER L"\\" LONG_FILE_NAME FILE_EXT,
                                          L"C:\\" LONG_FOLDER L"\\" LONG_FILE_NAME CREATE2_SUFFIX FILE_EXT,
                                          FILE_ATTRIBUTE_NORMAL, FILE_FLAG_NO_BUFFERING));
    }

    // CreateHardLinkW
    {
        results.AddResult(TestCreateHardLinkW(L"C:\\" SHORT_FILE_NAME FILE_EXT,
                                              L"C:\\" SHORT_FILE_NAME HARDLINK_SUFFIX FILE_EXT));
        results.AddResult(TestCreateHardLinkW(L"C:\\" LONG_FOLDER L"\\" LONG_FILE_NAME FILE_EXT,
                                              L"C:\\" LONG_FOLDER L"\\" LONG_FILE_NAME HARDLINK_SUFFIX FILE_EXT));
    }

    /* Not implemented in Wine. Should it ever be, we'll also need a second version that deals with folders */
    #if 0
    { // CreateSymbolicLinkW
        results.AddResult(TestCreateSymbolicLinkToFileW(L"C:\\" SHORT_FILE_NAME FILE_EXT,
                                                        L"C:\\" SHORT_FILE_NAME SYMLINK_SUFFIX FILE_EXT));
        results.AddResult(TestCreateSymbolicLinkToFileW(L"C:\\" LONG_FOLDER L"\\" LONG_FILE_NAME FILE_EXT,
                                                        L"C:\\" LONG_FOLDER L"\\" LONG_FILE_NAME SYMLINK_SUFFIX FILE_EXT));
    }
    #endif

    { // DeleteFileW
        results.AddResult(TestDeleteFileW(L"C:\\" SHORT_FILE_NAME DELETE_FILE_SUFFIX FILE_EXT));
        results.AddResult(TestDeleteFileW(L"C:\\" LONG_FOLDER L"\\" LONG_FILE_NAME DELETE_FILE_SUFFIX FILE_EXT));
    }

    { // FindFirstFileW
        results.AddResult(TestFindFirstFileW(shortFile, SHORT_FILE_NAME FILE_EXT));
        results.AddResult(TestFindFirstFileW(longFile, LONG_FILE_NAME FILE_EXT));
    }

    { // FindFirstFileExW
        results.AddResult(TestFindFirstFileExW(shortFile, SHORT_FILE_NAME FILE_EXT, FIND_FIRST_EX_CASE_SENSITIVE));
        results.AddResult(TestFindFirstFileExW(longFile, LONG_FILE_NAME FILE_EXT, FIND_FIRST_EX_CASE_SENSITIVE));
        results.AddResult(TestFindFirstFileExW(shortFile, SHORT_FILE_NAME FILE_EXT, 0));
        results.AddResult(TestFindFirstFileExW(longFile, LONG_FILE_NAME FILE_EXT, 0));
    }

    { // FindNextFileW
        results.AddResult(TestFindNextFileW(L"C:\\" SHORT_FILE_NAME L"*",
                                            SHORT_FILE_NAME SEARCH_FILE_SUFFIX FILE_EXT,
                                            FIND_FIRST_EX_CASE_SENSITIVE | FIND_FIRST_EX_LARGE_FETCH));
        results.AddResult(TestFindNextFileW(L"C:\\" LONG_FOLDER L"\\*",
                                            LONG_FILE_NAME SEARCH_FILE_SUFFIX FILE_EXT,
                                            FIND_FIRST_EX_CASE_SENSITIVE | FIND_FIRST_EX_LARGE_FETCH));
        results.AddResult(TestFindNextFileW(L"C:\\" L"*",
                                            SHORT_FILE_NAME SEARCH_FILE_SUFFIX FILE_EXT,
                                            FIND_FIRST_EX_LARGE_FETCH));
        results.AddResult(TestFindNextFileW(L"C:\\" LONG_FOLDER L"\\*",
                                            LONG_FILE_NAME SEARCH_FILE_SUFFIX FILE_EXT,
                                            FIND_FIRST_EX_LARGE_FETCH));
    }

    { // GetFileAttributesW, GetFileAttributesExW, SetFileAttributesW all tested together
        results.AddResult(TestFileAttributesW(shortFile));
        results.AddResult(TestFileAttributesW(longFile));
    }

    { // GetFullPathNameW
        results.AddResult(TestGetFullPathNameW(shortFile, shortFile));
        results.AddResult(TestGetFullPathNameW(longFile, longFile));
    }

    { // GetLongPathNameW
        results.AddResult(TestGetLongPathNameW(shortFile, shortFile));
        /* Note that we don't check to see if the shorthand expansion actually works, all we care about is
         * whether or not the function can properly handle long paths */
        //results.AddResult(TestGetLongPathNameW(L"C:\\THIS_R~1\\THIS_R~1\\THIS_R~1" FILE_EXT, longFile));
        results.AddResult(TestGetLongPathNameW(longFile, longFile));
    }

    { // MoveFileW
        results.AddResult(TestMoveFileW(
                    L"C:\\" SHORT_FILE_NAME MOVE_FILE_SUFFIX FILE_EXT,
                    L"C:\\" SHORT_FILE_NAME MOVED_FILE_SUFFIX FILE_EXT));
        results.AddResult(TestMoveFileW(
                    L"C:\\" LONG_FOLDER L"\\" LONG_FILE_NAME MOVE_FILE_SUFFIX FILE_EXT,
                    L"C:\\" LONG_FOLDER L"\\" LONG_FILE_NAME MOVED_FILE_SUFFIX FILE_EXT));
    }

    { // MoveFileExW
        results.AddResult(TestMoveFileExW(
                    L"C:\\" SHORT_FILE_NAME MOVE_FILE_SUFFIX FILE_EXT,
                    L"C:\\" SHORT_FILE_NAME MOVED_FILE_SUFFIX FILE_EXT,
                    0));
        results.AddResult(TestMoveFileExW(
                    L"C:\\" LONG_FOLDER L"\\" LONG_FILE_NAME MOVE_FILE_SUFFIX FILE_EXT,
                    L"C:\\" LONG_FOLDER L"\\" LONG_FILE_NAME MOVED_FILE_SUFFIX FILE_EXT,
                    0));
    }

    { // MoveFileWithProgressW
        results.AddResult(TestMoveFileWithProgressW(
                    L"C:\\" SHORT_FILE_NAME MOVE_FILE_SUFFIX FILE_EXT,
                    L"C:\\" SHORT_FILE_NAME MOVED_FILE_SUFFIX FILE_EXT,
                    0, 0));
        results.AddResult(TestMoveFileWithProgressW(
                    L"C:\\" LONG_FOLDER L"\\" LONG_FILE_NAME MOVE_FILE_SUFFIX FILE_EXT,
                    L"C:\\" LONG_FOLDER L"\\" LONG_FILE_NAME MOVED_FILE_SUFFIX FILE_EXT,
                    0, 0));
    }

    { // ReplaceFileW
        results.AddResult(TestReplaceFileW(
                    L"C:\\" SHORT_FILE_NAME FIRST_FILE_SUFFIX FILE_EXT,
                    L"C:\\" SHORT_FILE_NAME SECOND_FILE_SUFFIX FILE_EXT,
                    L"C:\\" SHORT_FILE_NAME BACKUP_FILE_SUFFIX FILE_EXT));
        results.AddResult(TestReplaceFileW(
                    L"C:\\" LONG_FOLDER L"\\" LONG_FILE_NAME FIRST_FILE_SUFFIX FILE_EXT,
                    L"C:\\" LONG_FOLDER L"\\" LONG_FILE_NAME SECOND_FILE_SUFFIX FILE_EXT,
                    L"C:\\" LONG_FOLDER L"\\" LONG_FILE_NAME BACKUP_FILE_SUFFIX FILE_EXT));
    }

    { // SearchPathW
        results.AddResult(TestSearchPathW(L"C:\\", SHORT_FILE_NAME, FILE_EXT));
        results.AddResult(TestSearchPathW(L"C:\\", SHORT_FILE_NAME FILE_EXT, 0));
        results.AddResult(TestSearchPathW(L"C:\\" LONG_FOLDER L"\\", LONG_FILE_NAME, FILE_EXT));
        results.AddResult(TestSearchPathW(L"C:\\", LONG_FOLDER L"\\" LONG_FILE_NAME FILE_EXT, 0));
        results.AddResult(TestSearchPathW(L"C:\\" LONG_FOLDER L"\\", LONG_FILE_NAME FILE_EXT, 0));
    }

    { // FindFirstStreamW and FindNextStreamW
        results.AddResult(TestFindFirstStreamW(shortFile));
        results.AddResult(TestFindFirstStreamW(longFile));
    }

    { // GetCompressedFileSizeW
        results.AddResult(TestGetCompressedFileSizeW(shortFile));
        results.AddResult(TestGetCompressedFileSizeW(longFile));
        /* Not testing sparse or compressed files as Linux doesn't really support this at the level of the
         * file system the way Windows does with NTFS. */
    }

    { // GetFinalPathNameByHandleW
        results.AddResult(TestGetFinalPathNameByHandleW(shortFile, FILE_NAME_NORMALIZED | VOLUME_NAME_DOS));
        results.AddResult(TestGetFinalPathNameByHandleW(shortFile, FILE_NAME_NORMALIZED | VOLUME_NAME_NONE));
        results.AddResult(TestGetFinalPathNameByHandleW(shortFile, FILE_NAME_NORMALIZED | VOLUME_NAME_GUID));
        results.AddResult(TestGetFinalPathNameByHandleW(shortFile, FILE_NAME_NORMALIZED | VOLUME_NAME_NT));
        results.AddResult(TestGetFinalPathNameByHandleW(longFile, FILE_NAME_NORMALIZED | VOLUME_NAME_DOS));
        results.AddResult(TestGetFinalPathNameByHandleW(longFile, FILE_NAME_NORMALIZED | VOLUME_NAME_NONE));
        results.AddResult(TestGetFinalPathNameByHandleW(longFile, FILE_NAME_NORMALIZED | VOLUME_NAME_GUID));
        results.AddResult(TestGetFinalPathNameByHandleW(longFile, FILE_NAME_NORMALIZED | VOLUME_NAME_NT));

        // Not implemented in Wine. See note above in TestGetFinalPathNameByHandleW.
        #if 0
        results.AddResult(TestGetFinalPathNameByHandleW(shortFile, FILE_NAME_OPENED | VOLUME_NAME_DOS));
        results.AddResult(TestGetFinalPathNameByHandleW(shortFile, FILE_NAME_OPENED | VOLUME_NAME_NONE));
        results.AddResult(TestGetFinalPathNameByHandleW(shortFile, FILE_NAME_OPENED | VOLUME_NAME_GUID));
        results.AddResult(TestGetFinalPathNameByHandleW(shortFile, FILE_NAME_OPENED | VOLUME_NAME_NT));
        results.AddResult(TestGetFinalPathNameByHandleW(longFile, FILE_NAME_OPENED | VOLUME_NAME_DOS));
        results.AddResult(TestGetFinalPathNameByHandleW(longFile, FILE_NAME_OPENED | VOLUME_NAME_NONE));
        results.AddResult(TestGetFinalPathNameByHandleW(longFile, FILE_NAME_OPENED | VOLUME_NAME_GUID));
        results.AddResult(TestGetFinalPathNameByHandleW(longFile, FILE_NAME_OPENED | VOLUME_NAME_NT));
        #endif
    }

    printf("All Tests Finished.\n"
            "\tPassed: %u (with warnings: %u, unexpected successes: %u)\n"
            "\tFailed: %u (errors: %u, expected failures: %u)\n",
            results.successes + results.warnings + results.unexpected, results.warnings, results.unexpected,
            results.failures + results.errors + results.expected, results.errors, results.expected);
    return 0;
}
