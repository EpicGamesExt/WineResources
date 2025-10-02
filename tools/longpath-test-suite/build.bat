@ECHO off

SET SCRIPT_DIR=%~dp0

mkdir %SCRIPT_DIR%bin
mkdir %SCRIPT_DIR%build

pushd %SCRIPT_DIR%build
cmake -A x64 -DCMAKE_INSTALL_PREFIX=.. ..
cmake --build . --target install --config Release
popd
