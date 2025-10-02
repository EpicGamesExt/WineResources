#!/bin/bash
REPO_ROOT=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
# FIXME Use the current shell's WINEPREFIX if it has been set
#export WINEPREFIX=${REPO_ROOT}/temp/wine

#Slightly shorter
#LONG_FOLDER=this_right_here_is_a_noticeably_lengthy_name_for_the_topmost_parent_directory_that_sits_directly_under_the_c_drive/this_right_here_is_a_noticeably_lengthy_name_for_the_nested_subdirectory_that_sits_directly_under_the_topmost_parent_directory
#Long enough for just the folder path to be longer than MAX_PATH without any one element
LONG_FOLDER=this_right_here_is_a_noticeably_lengthy_name_for_the_topmost_parent_directory_that_sits_directly_under_the_c_drive/this_right_here_is_a_noticeably_lengthy_name_for_the_nested_subdirectory_that_sits_directly_under_the_topmost_parent_directory/this_right_here_is_a_noticeably_lengthy_name_for_yet_another_nested_subdirectory_sitting_further_under_the_parent_directory

LONG_NAME=this_right_here_is_a_noticeably_lengthy_name_for_the_file_that_sits_directly_under_the_nested_subdirectory
# TODO Argument for wine invocation

#FIXME Wineboot if drive_c doesn't exist yet
#wineboot && wineserver --wait
mkdir -p ${WINEPREFIX}/drive_c/${LONG_FOLDER}/relative_subfolder
mkdir -p ${WINEPREFIX}/drive_c/short_folder/relative_subfolder
# Example text should be at least 16 bytes long for sanity checks
echo "Some example text." > ${WINEPREFIX}/drive_c/${LONG_FOLDER}/${LONG_NAME}.txt
echo "Some example text." > ${WINEPREFIX}/drive_c/shorty.txt
echo "Some alternate example text." > ${WINEPREFIX}/drive_c/${LONG_FOLDER}/${LONG_NAME}Alt.txt
echo "Some alternate example text." > ${WINEPREFIX}/drive_c/shortyAlt.txt

# For testing RemoveDirectoryW
mkdir -p ${WINEPREFIX}/drive_c/remove
mkdir -p ${WINEPREFIX}/drive_c/${LONG_FOLDER}/remove

# Big file for testing CopyFile2
lines=10000
line="Sphinx of black quartz, judge my vow\n"
echo $line > ${WINEPREFIX}/drive_c/${LONG_FOLDER}/${LONG_NAME}Copy2.txt
for i in `seq $lines`
do
    echo $line >> ${WINEPREFIX}/drive_c/${LONG_FOLDER}/${LONG_NAME}Copy2.txt
done
echo $line > ${WINEPREFIX}/drive_c/shortyCopy2.txt
for i in `seq $lines`
do
    echo $line >> ${WINEPREFIX}/drive_c/shortyCopy2.txt
done

# For testing DeleteFileW
echo "This file is for deleting." > ${WINEPREFIX}/drive_c/${LONG_FOLDER}/${LONG_NAME}Delete.txt
echo "This file is for deleting." > ${WINEPREFIX}/drive_c/shortyDelete.txt

# For testing FindNextFileW
echo "Some example text." > ${WINEPREFIX}/drive_c/${LONG_FOLDER}/${LONG_NAME}Search.txt
echo "Some example text." > ${WINEPREFIX}/drive_c/shortySearch.txt

# For testing MoveFileW and variants
echo "This file is for moving." > ${WINEPREFIX}/drive_c/${LONG_FOLDER}/${LONG_NAME}Move.txt
echo "This file is for moving." > ${WINEPREFIX}/drive_c/shortyMove.txt

# For testing ReplaceFileW
echo "This file should be replaced." > ${WINEPREFIX}/drive_c/${LONG_FOLDER}/${LONG_NAME}Rep.txt
echo "This file should be replaced." > ${WINEPREFIX}/drive_c/shortyRep.txt
echo "This file has been replaced." > ${WINEPREFIX}/drive_c/${LONG_FOLDER}/${LONG_NAME}Rep2.txt
echo "This file has been replaced." > ${WINEPREFIX}/drive_c/shortyRep2.txt

# Remove any leftover files from previous runs
rm -f ${WINEPREFIX}/drive_c/create
rm -f ${WINEPREFIX}/drive_c/${LONG_FOLDER}/create
rm -f ${WINEPREFIX}/drive_c/createEx
rm -f ${WINEPREFIX}/drive_c/${LONG_FOLDER}/createEx
rm -f ${WINEPREFIX}/drive_c/shortyCopyW.txt
rm -f ${WINEPREFIX}/drive_c/${LONG_FOLDER}/${LONG_NAME}CopyW.txt
rm -f ${WINEPREFIX}/drive_c/shortyCopy2.txt
rm -f ${WINEPREFIX}/drive_c/${LONG_FOLDER}/${LONG_NAME}Copy2.txt
rm -f ${WINEPREFIX}/drive_c/shortyCopyExW.txt
rm -f ${WINEPREFIX}/drive_c/${LONG_FOLDER}/${LONG_NAME}CopyExW.txt
rm -f ${WINEPREFIX}/drive_c/shortyNew.txt
rm -f ${WINEPREFIX}/drive_c/${LONG_FOLDER}/${LONG_NAME}New.txt
rm -f ${WINEPREFIX}/drive_c/shortyNew2.txt
rm -f ${WINEPREFIX}/drive_c/${LONG_FOLDER}/${LONG_NAME}New2.txt
rm -f ${WINEPREFIX}/drive_c/shortySymlink.txt
rm -f ${WINEPREFIX}/drive_c/${LONG_FOLDER}/${LONG_NAME}Symlink.txt
rm -f ${WINEPREFIX}/drive_c/shortyHardlink.txt
rm -f ${WINEPREFIX}/drive_c/${LONG_FOLDER}/${LONG_NAME}Hardlink.txt

$1 ${REPO_ROOT}/bin/longpath.exe
