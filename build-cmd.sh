#!/usr/bin/env bash

cd "$(dirname "$0")"

# turn on verbose debugging output for logs.
exec 4>&1; export BASH_XTRACEFD=4; set -x
# make errors fatal
set -e
# bleat on references to undefined shell variables
set -u

TAILSLIDE_SOURCE_DIR="."

top="$(pwd)"
stage="$top"/stage

mkdir -p "${stage}"

# load autobuild provided shell functions and variables
case "$AUTOBUILD_PLATFORM" in
    windows*)
        autobuild="$(cygpath -u "$AUTOBUILD")"
    ;;
    *)
        autobuild="$AUTOBUILD"
    ;;
esac
source_environment_tempfile="$stage/source_environment.sh"
"$autobuild" source_environment > "$source_environment_tempfile"
. "$source_environment_tempfile"

static_lib_ext="a"
lib_prefix="lib"
native_top="${top}"
native_stage="${stage}"
native_stage_lib="$stage/lib/release"

case "$AUTOBUILD_PLATFORM" in
    windows*)
        load_vsvars
        static_lib_ext="lib"
        lib_prefix=""
        native_top="$(cygpath -m "$top")"
        native_stage="$(cygpath -m "$stage")"
        native_stage_lib="$(cygpath -m "$native_stage_lib")"
    ;;
esac

pushd "$TAILSLIDE_SOURCE_DIR"

    # So, clear out bits that shouldn't affect our configure-directed build
    # but which do nonetheless.
    #
    unset DISTCC_HOSTS CC CXX CFLAGS CPPFLAGS CXXFLAGS

    # Default target per autobuild build --address-size
    opts="${TARGET_OPTS:--m$AUTOBUILD_ADDRSIZE $LL_BUILD_RELEASE}"

    # Handle any deliberate platform targeting
    if [ ! "${TARGET_CPPFLAGS:-}" ]; then
        # Remove sysroot contamination from build environment
        unset CPPFLAGS
    else
        # Incorporate special pre-processing flags
        export CPPFLAGS="$TARGET_CPPFLAGS"
    fi

    mkdir -p "$stage/build"
    pushd "$stage/build"
      # Release
      # Note that the library is _not_ stripped!
      CFLAGS="$opts" CXXFLAGS="$opts" \
          cmake "$native_top" -DCMAKE_INSTALL_LIBDIR="$native_stage_lib" -DCMAKE_BUILD_TYPE=Release \
          -DTAILSLIDE_BUILD_TESTS=off -DTAILSLIDE_BUILD_CLI=off
      cmake --build . --config Release
      cmake --install . --prefix "$native_stage"
      mkdir -p "$stage/lib/release"
      mv "$stage/lib/${lib_prefix}tailslide.$static_lib_ext" "$stage/lib/release/${lib_prefix}tailslide.$static_lib_ext"
    popd
    # clean the build artifacts
    rm -rf "$stage/build"

    mkdir -p "$stage/LICENSES"
    cp "$top/LICENSE" "$stage/LICENSES/tailslide.txt"
popd

mkdir -p "$stage"/docs/tailslide/
