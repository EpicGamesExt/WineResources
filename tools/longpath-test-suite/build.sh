#!/usr/bin/env bash

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)

mkdir -p ${SCRIPT_DIR}/bin
mkdir -p ${SCRIPT_DIR}/build

pushd ${SCRIPT_DIR}/build

cmake \
    -DCMAKE_TOOLCHAIN_FILE=toolchain-mingw.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=.. ..

cmake \
    --build . \
    --target install

popd
