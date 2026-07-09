#!/usr/bin/env bash
set -euo pipefail

VERSION=$(grep 'project(Devpad VERSION' CMakeLists.txt | sed 's/.*VERSION \([^ ]*\).*/\1/')
ARCH=$(uname -m)

NAME="Devpad-${VERSION}-linux-${ARCH}"
STAGING="${NAME}"
TARBALL="${NAME}.tar.gz"
BUILD_DIR="${BUILD_DIR:-build-release}"

info()  { echo -e "\033[1;34m[*]\033[0m $*"; }
ok()    { echo -e "\033[1;32m[+]\033[0m $*"; }
warn()  { echo -e "\033[1;33m[!]\033[0m $*" >&2; }
err()   { echo -e "\033[1;31m[E]\033[0m $*" >&2; exit 1; }

# ─── Step 0: Check prerequisites ───────────────────────────────
check_cmd() { command -v "$1" &>/dev/null || err "Missing required tool: $1"; }
check_cmd cmake
check_cmd strip

HAS_PATCHELF=false
if command -v patchelf &>/dev/null; then
    HAS_PATCHELF=true
    info "patchelf available — will set rpaths on binaries/libs"
else
    warn "patchelf not found — falling back to LD_LIBRARY_PATH wrapper"
fi

# ─── Step 1: Build Release ──────────────────────────────────────
if [ ! -f "${BUILD_DIR}/Devpad" ]; then
    info "Building Devpad in Release mode..."
    cmake -S . -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF
    cmake --build "${BUILD_DIR}" -j "$(nproc)"
else
    ok "Build already up-to-date at ${BUILD_DIR}/Devpad"
fi

# ─── Step 2: Setup staging directory ────────────────────────────
info "Setting up staging directory: ${STAGING}/"
rm -rf "${STAGING}"
mkdir -p "${STAGING}"/{bin,lib}
mkdir -p "${STAGING}/share/applications"
mkdir -p "${STAGING}/share/metainfo"
mkdir -p "${STAGING}/share/icons/hicolor/scalable/apps"
mkdir -p "${STAGING}/share/licenses/Devpad"

# ─── Step 3: Install application files ──────────────────────────
info "Installing binary and data files..."
install -m 755 "${BUILD_DIR}/Devpad" "${STAGING}/bin/Devpad"
install -m 644 resources/com.semagsoft.Devpad.desktop   "${STAGING}/share/applications/"
install -m 644 resources/com.semagsoft.Devpad.metainfo.xml "${STAGING}/share/metainfo/"
install -m 644 resources/icons/devpad.svg               "${STAGING}/share/icons/hicolor/scalable/apps/com.semagsoft.Devpad.svg"
install -m 644 LICENSE                                  "${STAGING}/share/licenses/Devpad/"

# ─── Step 4: Gather runtime library dependencies ────────────────
info "Resolving shared library dependencies..."

declare -A SEEN

# Essential Qt plugins for Devpad (no KDE-internal plugins)
ESSENTIAL_PLUGIN_CATEGORIES=(
    platforms
    imageformats
    iconengines
    printsupport
    styles
    platformthemes
    platforminputcontexts
    wayland-shell-integration
    wayland-graphics-integration-client
    tls
    egldeviceintegrations
    generic
)

# Exclude KDE-internal plugins that drag in the entire desktop
EXCLUDE_PLUGIN_PATTERNS=(
    'kwin'
    'plasma'
    'kdecorations'
    'kaccounts'
    'kpeople'
    'kactivitymanager'
    'kcm_'
    'kstyle_config'
    'ksysguard'
    'ksystemstats'
    'powerdevil'
    'sensors'
    'kfontinst'
    'notificationmanager'
    'taskmanager'
    'kglobalacceld'
    'baloo'
    'kio'
    'kparts'
    'krunner'
    'knewstuff'
    'attica'
    'kfontinst'
    'polkit'
    'KSysGuard'
)

copy_lib_files() {
    local dir="$1"
    local base="$2"
    [ -d "$dir" ] || return

    for f in "$dir/$base"*; do
        [ -f "$f" ] || [ -L "$f" ] || continue
        local name; name=$(basename "$f")
        [ -n "${SEEN[$name]:-}" ] && continue
        SEEN["$name"]=1
        cp -a "$f" "${STAGING}/lib/"
    done
}

# Resolve library path and copy all symlinks
resolve_and_copy_lib() {
    local src="$1"
    local real; real=$(readlink -f "$src" 2>/dev/null || echo "$src")
    local dir; dir=$(dirname "$real")
    local base; base=$(basename "$real")

    # Derive stem (everything up to first .so)
    local stem="${base%%.so*}.so"

    copy_lib_files "$dir" "$stem"
}

# Collect ldd output from binary
info "  scanning Devpad binary..."
while IFS= read -r line; do
    [[ "$line" != *"=> "* ]] && continue
    path=$(echo "$line" | awk '{print $3}')
    [ -z "$path" ] && continue
    [ "$path" = "linux-vdso.so.1" ] && continue
    [ -f "$path" ] || continue
    copy_lib_files "$(dirname "$(readlink -f "$path")")" "$(basename "$(readlink -f "$path")" | sed 's/\.so.*/.so/')"
done < <(ldd "${BUILD_DIR}/Devpad" 2>/dev/null || true)

# Bundle Qt plugins — only essential categories
QT_PLUGIN_SRC="/usr/lib/qt6/plugins"
QT_PLUGIN_DEST="${STAGING}/plugins"

if [ -d "$QT_PLUGIN_SRC" ]; then
    for cat in "${ESSENTIAL_PLUGIN_CATEGORIES[@]}"; do
        src="${QT_PLUGIN_SRC}/${cat}"
        if [ -d "$src" ] && [ "$(find "$src" -maxdepth 1 -name '*.so' -type f 2>/dev/null | head -c1)" ]; then
            info "  plugins/${cat}/"
            mkdir -p "${QT_PLUGIN_DEST}/${cat}"
            for f in "$src"/*.so; do
                [ -f "$f" ] || continue
                bn=$(basename "$f")
                # Skip KDE-internal patterns
                skip=false
                for pat in "${EXCLUDE_PLUGIN_PATTERNS[@]}"; do
                    if [[ "$bn" == *"$pat"* ]]; then
                        skip=true
                        break
                    fi
                done
                $skip && continue
                cp -a "$f" "${QT_PLUGIN_DEST}/${cat}/"
            done
        fi
    done

    # Scan bundled plugins for additional dependencies
    info "  scanning Qt plugins for deps..."
    for plugin in $(find "${QT_PLUGIN_DEST}" -name '*.so' -type f 2>/dev/null); do
        while IFS= read -r line; do
            [[ "$line" != *"=> "* ]] && continue
            path=$(echo "$line" | awk '{print $3}')
            [ -z "$path" ] && continue
            [ -f "$path" ] || continue
            copy_lib_files "$(dirname "$(readlink -f "$path")")" "$(basename "$(readlink -f "$path")" | sed 's/\.so.*/.so/')"
        done < <(ldd "$plugin" 2>/dev/null || true)
    done
fi

# ─── Step 5: Fix rpaths ──────────────────────────────────────────
if $HAS_PATCHELF; then
    info "Setting rpath on binary and bundled libraries..."
    patchelf --set-rpath '$ORIGIN/../lib' "${STAGING}/bin/Devpad"

    for lib in "${STAGING}/lib/"*.so*; do
        [ -L "$lib" ] && continue
        patchelf --set-rpath '$ORIGIN' "$lib" 2>/dev/null || true
    done

    for plugin in "${QT_PLUGIN_DEST}/"*/*.so; do
        [ -L "$plugin" ] && continue
        patchelf --set-rpath '$ORIGIN/../../lib' "$plugin" 2>/dev/null || true
    done
else
    info "No patchelf — wrapper script will use LD_LIBRARY_PATH"
fi

# ─── Step 6: Strip debug symbols ────────────────────────────────────
info "Stripping debug symbols..."
strip --strip-unneeded "${STAGING}/bin/Devpad" 2>/dev/null || true
find "${STAGING}/lib" -name '*.so*' -type f -exec strip --strip-unneeded {} \; 2>/dev/null || true
find "${QT_PLUGIN_DEST}" -name '*.so' -type f -exec strip --strip-unneeded {} \; 2>/dev/null || true

# ─── Step 7: Create wrapper script ──────────────────────────────
info "Creating launcher wrapper..."
if $HAS_PATCHELF; then
    cat > "${STAGING}/bin/Devpad.sh" <<'WRAPPER'
#!/usr/bin/env bash
set -euo pipefail
SELF_DIR="$(cd "$(dirname "$0")" && pwd)"
APP_DIR="$(cd "${SELF_DIR}/.." && pwd)"
export QT_PLUGIN_PATH="${APP_DIR}/plugins"
exec "${SELF_DIR}/Devpad" "$@"
WRAPPER
else
    cat > "${STAGING}/bin/Devpad.sh" <<'WRAPPER'
#!/usr/bin/env bash
set -euo pipefail
SELF_DIR="$(cd "$(dirname "$0")" && pwd)"
APP_DIR="$(cd "${SELF_DIR}/.." && pwd)"
export LD_LIBRARY_PATH="${APP_DIR}/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export QT_PLUGIN_PATH="${APP_DIR}/plugins"
exec "${SELF_DIR}/Devpad" "$@"
WRAPPER
fi
chmod 755 "${STAGING}/bin/Devpad.sh"

ln -sf "bin/Devpad.sh" "${STAGING}/Devpad.sh"

# ─── Step 8: Show what we built ─────────────────────────────────
echo ""
info "Package contents:"
find "${STAGING}" -type f | sort | while IFS= read -r f; do
    size=$(du -h "$f" | cut -f1)
    echo "    ${size}  ${f#${STAGING}/}"
done

total_libs=$(find "${STAGING}/lib" -type f | wc -l)
total_plugins=$(find "${STAGING}/plugins" -name '*.so' -type f | wc -l)
lib_size=$(du -sh "${STAGING}/lib" | cut -f1)
plugin_size=$(du -sh "${STAGING}/plugins" | cut -f1)

echo ""
info "Summary:"
echo "    libs:     ${total_libs} files (${lib_size})"
echo "    plugins:  ${total_plugins} files (${plugin_size})"
echo "    bin:      ${STAGING}/bin/Devpad $(du -h "${STAGING}/bin/Devpad" | cut -f1)"
echo "    data:     share/"

# ─── Step 9: Create tarball ────────────────────────────────────
echo ""
info "Creating tarball: ${TARBALL}"
tar czf "${TARBALL}" "${STAGING}"

total_size=$(du -h "${TARBALL}" | cut -f1)
ok "Done! Tarball created: ${TARBALL} (${total_size})"