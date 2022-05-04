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

# TODO: Load this from the cmake file somehow.
version="1.0.0"
build=${AUTOBUILD_BUILD_ID:=0}
echo "${version}.${build}" > "${stage}/VERSION.txt"

pushd "$TAILSLIDE_SOURCE_DIR"
    case "$AUTOBUILD_PLATFORM" in

        # ------------------------ windows, windows64 ------------------------
        windows*)
            exit 1
        ;;

        # ------------------------- darwin, darwin64 -------------------------
        darwin*)
            exit 1
        ;;

        # -------------------------- linux, linux64 --------------------------
        linux*)
            # Linux build environment at Linden comes pre-polluted with stuff that can
            # seriously damage 3rd-party builds.  Environmental garbage you can expect
            # includes:
            #
            #    DISTCC_POTENTIAL_HOSTS     arch           root        CXXFLAGS
            #    DISTCC_LOCATION            top            branch      CC
            #    DISTCC_HOSTS               build_name     suffix      CXX
            #    LSDISTCC_ARGS              repo           prefix      CFLAGS
            #    cxx_version                AUTOBUILD      SIGN        CPPFLAGS
            #
            # So, clear out bits that shouldn't affect our configure-directed build
            # but which do nonetheless.
            #
            unset DISTCC_HOSTS CC CXX CFLAGS CPPFLAGS CXXFLAGS

            # No, don't do that.
#            # Prefer gcc-4.6 if available.
#            if [[ -x /usr/bin/gcc-4.6 && -x /usr/bin/g++-4.6 ]]; then
#                export CC=/usr/bin/gcc-4.6
#                export CXX=/usr/bin/g++-4.6
#            fi

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
                  cmake "$top" -DCMAKE_INSTALL_LIBDIR="$stage/lib/release" -DCMAKE_BUILD_TYPE=Release
              make
              cmake --install . --prefix "$stage"
              mkdir -p "$stage/lib/release"
              mv "$stage/lib/libtailslide.a" "$stage/lib/release/libtailslide.a"
            popd
            # clean the build artifacts
            rm -rf "$stage/build"
        ;;
    esac
    mkdir -p "$stage/LICENSES"
    cp "$top/LICENSE" "$stage/LICENSES/tailslide.txt"
popd

mkdir -p "$stage"/docs/tailslide/
