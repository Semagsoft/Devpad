#!/usr/bin/env bash
set -euo pipefail

VERSION=$(grep 'project(Devpad VERSION' CMakeLists.txt | sed 's/.*VERSION \([^ ]*\).*/\1/')
ARCH=$(uname -m)

NAME="Devpad-${VERSION}-macos-${ARCH}"
BUILD_DIR="${BUILD_DIR:-build-release-macos}"
APP_BUNDLE="${NAME}.app"
DMG_OUTPUT="${NAME}.dmg"

info()  { echo -e "\033[1;34m[*]\033[0m $*"; }
ok()    { echo -e "\033[1;32m[+]\033[0m $*"; }
warn()  { echo -e "\033[1;33m[!]\033[0m $*" >&2; }
err()   { echo -e "\033[1;31m[E]\033[0m $*" >&2; exit 1; }

# ─── Step 0: Check prerequisites ───────────────────────────────
check_cmd() { command -v "$1" &>/dev/null || err "Missing required tool: $1"; }
check_cmd cmake
check_cmd macdeployqt

info "Packaging Devpad v${VERSION} for macOS (${ARCH})"

# ─── Step 1: Clean rebuild in Release mode ─────────────────────
info "Building Devpad in Release mode (${BUILD_DIR})..."
rm -rf "${BUILD_DIR}"
cmake -S . -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTS=OFF
cmake --build "${BUILD_DIR}" -j "$(sysctl -n hw.logicalcpu)"
ok "Build complete"

# ─── Step 2: Create .app bundle structure ──────────────────────
info "Creating .app bundle..."
rm -rf "${APP_BUNDLE}"
mkdir -p "${APP_BUNDLE}/Contents/MacOS"
mkdir -p "${APP_BUNDLE}/Contents/Resources"

cp "${BUILD_DIR}/Devpad" "${APP_BUNDLE}/Contents/MacOS/"
cp resources/Info.plist "${APP_BUNDLE}/Contents/"
cp resources/Devpad.icns "${APP_BUNDLE}/Contents/Resources/"

# Copy license
cp LICENSE "${APP_BUNDLE}/Contents/Resources/LICENSE.txt"

ok ".app bundle created at ${APP_BUNDLE}"

# ─── Step 3: Deploy Qt frameworks with macdeployqt ─────────────
info "Running macdeployqt to bundle Qt frameworks..."
macdeployqt "${APP_BUNDLE}" -verbose=1
ok "macdeployqt complete"

# ─── Step 4: Ad-hoc code sign ──────────────────────────────────
info "Ad-hoc code signing..."
codesign --deep --force --sign - "${APP_BUNDLE}/Contents/MacOS/Devpad" 2>/dev/null || true
codesign --deep --force --sign - "${APP_BUNDLE}" 2>/dev/null || true
ok "Ad-hoc code signing complete"

# ─── Step 5: Create .dmg disk image ────────────────────────────
info "Creating .dmg disk image..."
rm -f "${DMG_OUTPUT}"

# Create a temporary directory for dmg contents
DMG_DIR=$(mktemp -d)
cp -R "${APP_BUNDLE}" "${DMG_DIR}/"
ln -s /Applications "${DMG_DIR}/Applications"

hdiutil create -volname "Devpad ${VERSION}" \
    -srcfolder "${DMG_DIR}" \
    -ov \
    -format UDZO \
    "${DMG_OUTPUT}"

rm -rf "${DMG_DIR}"

ok "Disk image created: ${DMG_OUTPUT} ($(du -h "${DMG_OUTPUT}" | cut -f1))"

# ─── Step 6: Create portable .tar.gz archive ────────────────────
info "Creating portable archive..."
TARBALL="${NAME}.tar.gz"
tar czf "${TARBALL}" "${APP_BUNDLE}"
ok "Archive created: ${TARBALL} ($(du -h "${TARBALL}" | cut -f1))"

# ─── Step 7: Cleanup build dir ──────────────────────────────────
info "Cleaning up build directory..."
rm -rf "${BUILD_DIR}"

ok "Packaging complete: ${DMG_OUTPUT} and ${TARBALL}"
