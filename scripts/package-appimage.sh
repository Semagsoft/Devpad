#!/usr/bin/env bash
set -euo pipefail

VERSION=$(grep 'project(Devpad VERSION' CMakeLists.txt | sed 's/.*VERSION \([^ ]*\).*/\1/')
ARCH=$(uname -m)

NAME="Devpad-${VERSION}-linux-${ARCH}"
APPDIR="${NAME}-AppDir"
OUTPUT="${NAME}.AppImage"
BUILD_DIR="${BUILD_DIR:-build-appimage}"
CACHE_DIR="${XDG_CACHE_HOME:-$HOME/.cache}/linuxdeploy"
QMAKE="${QMAKE:-qmake6}"

info()  { echo -e "\033[1;34m[*]\033[0m $*"; }
ok()    { echo -e "\033[1;32m[+]\033[0m $*"; }
warn()  { echo -e "\033[1;33m[!]\033[0m $*" >&2; }
err()   { echo -e "\033[1;31m[E]\033[0m $*" >&2; exit 1; }

# ─── Step 0: Check prerequisites ───────────────────────────────
check_cmd() { command -v "$1" &>/dev/null || err "Missing required tool: $1"; }
check_cmd cmake

info "Packaging Devpad v${VERSION} for ${ARCH}"

# ─── Step 1: Build Release ──────────────────────────────────────
if [ ! -f "${BUILD_DIR}/Devpad" ]; then
    info "Building Devpad in Release mode (${BUILD_DIR})..."
    cmake -S . -B "${BUILD_DIR}" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr \
        -DBUILD_TESTS=OFF
    cmake --build "${BUILD_DIR}" -j "$(nproc)"
else
    ok "Build already up-to-date at ${BUILD_DIR}/Devpad"
fi

# ─── Step 2: Setup AppDir and install ───────────────────────────
info "Setting up AppDir: ${APPDIR}/"
rm -rf "${APPDIR}"
DESTDIR="${APPDIR}" cmake --install "${BUILD_DIR}" --prefix /usr

# ─── Step 3: Deploy binary dependencies with linuxdeploy ────────
LINUXDEPLOY="${CACHE_DIR}/linuxdeploy-${ARCH}.AppImage"

if [ ! -f "$LINUXDEPLOY" ]; then
    info "Downloading linuxdeploy..."
    mkdir -p "$CACHE_DIR"
    curl -fsSL \
        "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-${ARCH}.AppImage" \
        -o "$LINUXDEPLOY"
    chmod +x "$LINUXDEPLOY"
fi

info "Running linuxdeploy to resolve shared library dependencies..."
export APPIMAGE_EXTRACT_AND_RUN=1
# Strip errors are non-fatal (bundled strip too old for modern ELF); ignore exit code
"$LINUXDEPLOY" --appdir "${APPDIR}" || true

# Qt loads OpenSSL at runtime via dlopen, which linuxdeploy's ldd-based
# closure cannot see: it bundles libcrypto (dragged in by libssh/libsasl2)
# while libssl is missing. A half pair makes the loader reject the host's
# libssl and every HTTPS request fails with "TLS initialization failed".
# Remove any bundled OpenSSL -- the host always provides a consistent pair.
rm -f "${APPDIR}/usr/lib/libcrypto.so"* "${APPDIR}/usr/lib/libssl.so"* 2>/dev/null || true

# Also drop glibc core libs in case anything copied them into usr/lib:
# pairing the runner's glibc with the user's loader corrupts the process
# (GLIBC_PRIVATE symbol errors / crashes).
find "${APPDIR}/usr/lib" -maxdepth 1 \
    \( -name 'libc.so.*' -o -name 'libm.so.*' -o -name 'libpthread.so*' \
       -o -name 'libdl.so*' -o -name 'ld-linux*' \) -delete 2>/dev/null || true

# ─── Step 4: Bundle Qt plugins ─────────────────────────────────
info "Copying Qt plugins..."

qt_plugin_src=$(qmake6 -query QT_INSTALL_PLUGINS 2>/dev/null || echo "/usr/lib/qt6/plugins")
qt_plugin_dest="${APPDIR}/usr/plugins"

ESSENTIAL_PLUGIN_CATEGORIES=(
    platforms imageformats iconengines printsupport styles
    platformthemes platforminputcontexts tls generic
)

mkdir -p "${qt_plugin_dest}"
for cat in "${ESSENTIAL_PLUGIN_CATEGORIES[@]}"; do
    src="${qt_plugin_src}/${cat}"
    if [ -d "$src" ]; then
        mkdir -p "${qt_plugin_dest}/${cat}"
        for f in "$src"/*.so; do
            [ -f "$f" ] || continue
            cp -nL "$f" "${qt_plugin_dest}/${cat}/" 2>/dev/null || true
        done
    fi
done

# Copy extra shared libs needed by Qt plugins. Skip system libraries that
# must never be bundled (see package-linux.sh for the full rationale): the
# runner's glibc corrupts the host's process, and an OpenSSL half-pair
# breaks HTTPS with "TLS initialization failed".
NON_BUNDLE_LIB_PREFIXES=(
    libc.so. libm.so. libpthread.so. libdl.so. librt.so. libresolv.so.
    libutil.so. libnsl.so. libnss ld-linux libcrypto.so. libssl.so.
)

plugin_lib_path() {
    local path="$1" prefix
    local name; name=$(basename "$(readlink -f "$path")")
    for prefix in "${NON_BUNDLE_LIB_PREFIXES[@]}"; do
        [[ "$name" == "$prefix"* ]] && return 1
    done
    return 0
}

for f in $(find "${qt_plugin_dest}" -name '*.so' -type f 2>/dev/null); do
    while IFS= read -r line; do
        [[ "$line" != *"=> "* ]] && continue
        path=$(echo "$line" | awk '{print $3}')
        [ -z "$path" ] && continue
        [ -f "$path" ] || continue
        plugin_lib_path "$path" || continue
        cp -nL "$path" "${APPDIR}/usr/lib/" 2>/dev/null || true
    done < <(ldd "$f" 2>/dev/null || true)
done

# ─── Step 5: Create AppRun ─────────────────────────────────────
info "Creating launcher (AppRun)..."
cat > "${APPDIR}/AppRun" <<'WRAPPER'
#!/usr/bin/env bash
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
export PATH="${HERE}/usr/bin:${PATH}"
export LD_LIBRARY_PATH="${HERE}/usr/lib:${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export QT_PLUGIN_PATH="${HERE}/usr/plugins"
exec "${HERE}/usr/bin/Devpad" "$@"
WRAPPER
chmod 755 "${APPDIR}/AppRun"

# Copy desktop file and icon to root for AppImage discovery
# Remove symlinks created by linuxdeploy first (they resolve to the same file, breaking cp)
rm -f "${APPDIR}/com.semagsoft.Devpad.desktop" "${APPDIR}/com.semagsoft.Devpad.svg"
cp "${APPDIR}/usr/share/applications/com.semagsoft.Devpad.desktop" \
   "${APPDIR}/"
cp "${APPDIR}/usr/share/icons/hicolor/scalable/apps/com.semagsoft.Devpad.svg" \
   "${APPDIR}/com.semagsoft.Devpad.svg"

# ─── Step 5.5: Verify bundle hygiene ───────────────────────────
BROKEN_LIBS=$(find "${APPDIR}/usr/lib" -maxdepth 1 \
    \( -name 'libc.so.*' -o -name 'libm.so.*' -o -name 'libpthread.so*' \
       -o -name 'libdl.so*' -o -name 'ld-linux*' -o -name 'libcrypto.so*' \
       -o -name 'libssl.so*' \) -print -quit 2>/dev/null)
if [ -n "$BROKEN_LIBS" ]; then
    err "AppImage contains system libraries that must not be shipped (first hit: $BROKEN_LIBS).

Bundling glibc pairs the runner's libc with the user's dynamic loader (GLIBC_PRIVATE
crashes), and bundling libcrypto without libssl (which Qt dlopens at runtime) breaks
HTTPS with 'TLS initialization failed'."
fi

# ─── Step 6: Generate AppImage with appimagetool ────────────────
APIMAGETOOL="${CACHE_DIR}/appimagetool-${ARCH}.AppImage"

if [ ! -f "$APIMAGETOOL" ]; then
    info "Downloading appimagetool..."
    curl -fsSL \
        "https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-${ARCH}.AppImage" \
        -o "$APIMAGETOOL"
    chmod +x "$APIMAGETOOL"
fi

info "Running appimagetool to create ${OUTPUT}..."
export APPIMAGE_EXTRACT_AND_RUN=1
"$APIMAGETOOL" "${APPDIR}" "${OUTPUT}"

# ─── Step 7: Cleanup ────────────────────────────────────────────
rm -rf "${APPDIR}"

ok "AppImage created: ${OUTPUT} ($(du -h "$OUTPUT" | cut -f1))"
