@ECHO off

SET SCRIPT_DIR=%~dp0
SET LONG_FOLDER_1=this_right_here_is_a_noticeably_lengthy_name_for_the_topmost_parent_directory_that_sits_directly_under_the_c_drive
SET LONG_FOLDER_2=this_right_here_is_a_noticeably_lengthy_name_for_the_nested_subdirectory_that_sits_directly_under_the_topmost_parent_directory

mkdir C:\%LONG_FOLDER_1%\%LONG_FOLDER_2%
echo "Some example text." > C:\%LONG_FOLDER_1%\%LONG_FOLDER_2%\this_right_here_is_a_noticeably_lengthy_name_for_the_file_that_sits_directly_under_the_nested_subdirectory.txt
echo "Some example text." > C:\shorty.txt

%SCRIPT_DIR%bin\longpath.exe

rmdir /S /Q C:\%LONG_FOLDER_1%
del /Q /F C:\shorty.txt