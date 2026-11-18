#!/bin/sh
# Package Devpad for FreeBSD x86_64 as a relocatable tarball.
#
# Unlike package-linux.sh this does NOT bundle Qt/QScintilla shared libraries:
# FreeBSD has first-class packages for all runtime dependencies, so the
# tarball ships only the Devpad binary, first-party plugins and desktop data.
# Users install the dependencies with pkg (listed in README.txt in the bundle).
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(dirname "$SCRIPT_DIR")
cd "$REPO_ROOT"

VERSION=$(grep 'project(Devpad VERSION' CMakeLists.txt | sed 's/.*VERSION \([^ ]*\).*/\1/')
case "$(uname -m)" in
    amd64) ARCH=x86_64 ;;
    *)     ARCH=$(uname -m) ;;
esac

NAME="Devpad-${VERSION}-freebsd-${ARCH}"
STAGING="${PWD}/${NAME}"
TARBALL="${PWD}/${NAME}.tar.gz"
BUILD_DIR="${BUILD_DIR:-build-freebsd-pkg}"
JOBS=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)

info() { printf '[*] %s\n' "$*"; }
ok()   { printf '[+] %s\n' "$*"; }
err()  { printf '[E] %s\n' "$*" >&2; exit 1; }

info "Version: ${VERSION}; architecture: ${ARCH}"

# ─── Step 1: Build Release ───────────────────────────────────────
if [ ! -f "${BUILD_DIR}/Devpad" ]; then
    info "Configuring ${BUILD_DIR} (install prefix ${STAGING})..."
    cmake -S . -B "${BUILD_DIR}" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="${STAGING}" \
        -DBUILD_TESTS=OFF
    info "Building with ${JOBS} jobs..."
    cmake --build "${BUILD_DIR}" -j"${JOBS}"
else
    info "Reusing existing build at ${BUILD_DIR}"
fi

# ─── Step 2: Stage install tree ──────────────────────────────────
info "Staging into ${NAME}/"
rm -rf "${STAGING}"
cmake --install "${BUILD_DIR}" --prefix "${STAGING}"
[ -x "${STAGING}/bin/Devpad" ] || err "install did not produce bin/Devpad"

# The system plugin directory is baked in at configure time (the staging
# prefix for standalone builds, /usr/local when reusing a CI build tree). The
# wrapper below overrides it via DEVPAD_PLUGINS_DIR so the tarball stays
# relocatable (the env var is an additive override in PluginManager).
cat > "${STAGING}/bin/Devpad.sh" <<'WRAPPER'
#!/bin/sh
set -eu
SELF_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
APP_DIR=$(dirname "$SELF_DIR")
DEVPAD_PLUGINS_DIR="${APP_DIR}/lib/devpad/plugins"
export DEVPAD_PLUGINS_DIR
exec "${SELF_DIR}/Devpad" "$@"
WRAPPER
chmod 755 "${STAGING}/bin/Devpad.sh"

# Top-level launcher for convenience. A real script (not a symlink) keeps the
# inner wrapper's dirname-based path resolution correct when invoked from the
# extraction root.
cat > "${STAGING}/Devpad.sh" <<'OUTER'
#!/bin/sh
set -eu
ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec "${ROOT_DIR}/bin/Devpad.sh" "$@"
OUTER
chmod 755 "${STAGING}/Devpad.sh"

cat > "${STAGING}/README.txt" <<EOF
Devpad ${VERSION} for FreeBSD (${ARCH})

This bundle contains the Devpad binary, first-party plugins and desktop data.
It uses the Qt/QScintilla libraries installed on your system; install the
runtime dependencies first:

    pkg install qt6-base qt6-multimedia qt6-svg qscintilla2-qt6 qtermwidget

Extract this archive anywhere, then run:

    ./Devpad.sh                 # relocatable launcher (sets the plugin path)

Or install the files from bin/, lib/ and share/ into /usr/local.

Run with --help to see the available command-line options.
EOF

# ─── Step 3: Sanity checks ───────────────────────────────────────
# No system or Qt libraries should ever be copied into the tarball: bundling
# them would pair the build host's libcs with the user's runtime.
if find "${STAGING}" \( -name 'libc.so*' -o -name 'libthr.so*' \
        -o -name 'libQt6*.so*' -o -name 'libqscintilla2*.so*' \
        -o -name 'libqtermwidget*.so*' \) -print -quit | grep -q .; then
    err "the staging tree contains bundled system/Qt libraries"
fi

# ─── Step 4: Tarball ─────────────────────────────────────────────
info "Creating ${TARBALL}"
tar czf "${TARBALL}" "${NAME}"
ok "Done: ${TARBALL}"
