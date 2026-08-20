#!/usr/bin/env bash

set -e
set -u

CC=${CC:-gcc}
CXX=${CXX:-g++}
BUILD_DIR="build"
BUILD_TYPE="Release"
CLEAN=0
JOBS=$(nproc)
TARGET="all"
CCACHE=0
VCPKG=0

while [ $# -gt 0 ]; do
    case "$1" in
    -d | --debug)
        BUILD_TYPE="Debug"
        shift
        ;;
    -c | --clean)
        CLEAN=1
        shift
        ;;
    -j | --jobs)
        JOBS="$2"
        shift 2
        ;;
    --cc | -CC)
        CC="$2"
        shift 2
        ;;
    --cxx | -CXX)
        CXX="$2"
        shift 2
        ;;
    --ccache)
        CCACHE=1
        shift
        ;;
    --vcpkg)
        VCPKG=1
        VCPKG_ROOT="$2"
        shift 2
        ;;
    --triplet)
        VCPKG_TRIPLET="$2"
        shift 2
        ;;
    --host-triplet)
        VCPKG_HOST_TRIPLET="$2"
        shift 2
        ;;
    *)
        TARGET="$1"
        shift
        ;;
    esac

done

echo "=== CONFIG ==="
echo "[BUILD_TYPE]: $BUILD_TYPE"
echo "[CLEAN]: $CLEAN"
echo "[JOBS]: $JOBS"
echo "[TARGET]: $TARGET"
echo "[CCACHE]: $CCACHE"
echo "[Compiler(c)]: $CC"
echo "[Compiler(C++)]: $CXX"
echo "[VCPKG]: $VCPKG"
echo "=== END ==="

CMAKE_OPTS=()
CMAKE_OPTS+=("-GNinja")
CMAKE_OPTS+=("-DCMAKE_BUILD_TYPE=$BUILD_TYPE")
CMAKE_OPTS+=("-DCMAKE_C_COMPILER=$CC")
CMAKE_OPTS+=("-DCMAKE_CXX_COMPILER=$CXX")

if [ $VCPKG -eq 1 ]; then
    CMAKE_OPTS+=("-DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake")
    if [ "$VCPKG_TRIPLET" != "" ]; then
        CMAKE_OPTS+=("-DVCPKG_TARGET_TRIPLET=$VCPKG_TRIPLET")
    fi
    if [ "$VCPKG_HOST_TRIPLET" != "" ]; then
        CMAKE_OPTS+=("-DVCPKG_HOST_TRIPLET=$VCPKG_HOST_TRIPLET")
    fi
fi

if [ $CLEAN -eq 1 ]; then
    echo "cleaning: $BUILD_DIR"
    rm -rf "$BUILD_DIR"
fi

if [ $CCACHE -eq 1 ]; then
    CMAKE_OPTS+=("-DCMAKE_C_COMPILER_LAUNCHER=ccache")
    CMAKE_OPTS+=("-DCMAKE_CXX_COMPILER_LAUNCHER=ccache")
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake .. "${CMAKE_OPTS[@]}"
if [ $CCACHE -eq 1 ]; then
    ccache -z
fi
if [ $TARGET = "all" ]; then
    ninja -j$JOBS
else
    ninja -j$JOBS $TARGET
fi
if [ $CCACHE -eq 1 ]; then
    ccache -s
fi
