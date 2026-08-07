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
cp "${BUILD_DIR}/generated/Info.plist" "${APP_BUNDLE}/Contents/"
cp resources/Devpad.icns "${APP_BUNDLE}/Contents/Resources/"

# Copy license
cp LICENSE "${APP_BUNDLE}/Contents/Resources/LICENSE.txt"

ok ".app bundle created at ${APP_BUNDLE}"

# ─── Step 3: Deploy Qt frameworks with macdeployqt ─────────────
info "Running macdeployqt to bundle Qt frameworks..."
macdeployqt "${APP_BUNDLE}" -verbose=1
ok "macdeployqt complete"

# ─── Step 4: Code signing ────────────────────────────────────
# Use a Developer ID identity (hardened runtime + timestamp) when one is
# available; fall back to ad-hoc signing for local builds.
SIGN_IDENTITY="${DEVELOPER_ID:-}"
if [ -z "$SIGN_IDENTITY" ]; then
    SIGN_IDENTITY=$(security find-identity -v -p codesigning 2>/dev/null \
        | grep -o 'Developer ID Application: [^)]*' \
        | head -1 || true)
fi

if [ -n "$SIGN_IDENTITY" ]; then
    info "Code signing with identity: $SIGN_IDENTITY"
    codesign --force --deep --sign "$SIGN_IDENTITY" \
        --options runtime \
        --timestamp \
        "${APP_BUNDLE}"
else
    warn "No Developer ID identity found; using ad-hoc signature"
    codesign --deep --force --sign - "${APP_BUNDLE}"
fi
codesign --verify --deep "${APP_BUNDLE}"
ok "Code signing complete"

# ─── Step 5: Notarization (optional) ──────────────────────────
# Requires: NOTARIZE=1, APPLE_ID, APPLE_APP_PASSWORD, APPLE_TEAM_ID
notarize_app() {
    info "Submitting ${APP_BUNDLE} to Apple for notarization..."
    xcrun notarytool submit "${APP_BUNDLE}" \
        --apple-id "${APPLE_ID}" \
        --password "${APPLE_APP_PASSWORD}" \
        --team-id "${APPLE_TEAM_ID}" \
        --wait || err "Notarization submission failed"

    info "Stapling notarization ticket..."
    xcrun stapler staple "${APP_BUNDLE}"
    ok "Notarization and stapling complete"
}

if [ "${NOTARIZE:-0}" = "1" ]; then
    if [ -z "${APPLE_ID:-}" ] || [ -z "${APPLE_APP_PASSWORD:-}" ] || [ -z "${APPLE_TEAM_ID:-}" ]; then
        err "NOTARIZE=1 requires APPLE_ID, APPLE_APP_PASSWORD and APPLE_TEAM_ID env vars"
    fi
    if [ -z "$SIGN_IDENTITY" ]; then
        err "Notarization requires a Developer ID signing identity"
    fi
    notarize_app
fi

# ─── Step 6: Create .dmg disk image ───────────────────────────
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

# ─── Step 7: Create portable .tar.gz archive ────────────────────
info "Creating portable archive..."
TARBALL="${NAME}.tar.gz"
tar czf "${TARBALL}" "${APP_BUNDLE}"
ok "Archive created: ${TARBALL} ($(du -h "${TARBALL}" | cut -f1))"

# ─── Step 8: Cleanup build dir ──────────────────────────────────
info "Cleaning up build directory..."
rm -rf "${BUILD_DIR}"

ok "Packaging complete: ${DMG_OUTPUT} and ${TARBALL}"
