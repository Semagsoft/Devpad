!define PRODUCT_NAME "Devpad"
!define PRODUCT_VERSION "1.0"
!define PRODUCT_PUBLISHER "Semagsoft"
!define PRODUCT_WEB_SITE "https://semagsoft.com"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\Devpad.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "Devpad-${PRODUCT_VERSION}-Setup.exe"
InstallDir "$PROGRAMFILES64\Devpad"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
RequestExecutionLevel admin

; compressor
SetCompressor /SOLID lzma

; ----- Includes -----
!include "MUI2.nsh"
!include "FileFunc.nsh"
!include "WinCore.nsh"

; ----- Interface -----
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

; Pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN "$INSTDIR\Devpad.exe"
!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\README.md"
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; Languages
!insertmacro MUI_LANGUAGE "English"

; ----- Helper macros for file associations -----
!macro APP_ASSOCIATE EXT PROGID DESCRIPTION ICON
    WriteRegStr HKLM "Software\Classes\.${EXT}" "" "${PROGID}"
    WriteRegStr HKLM "Software\Classes\.${EXT}\OpenWithProgids" "${PROGID}" ""
    WriteRegStr HKLM "Software\Classes\${PROGID}" "" "${DESCRIPTION}"
    WriteRegStr HKLM "Software\Classes\${PROGID}\DefaultIcon" "" "${ICON}"
    WriteRegStr HKLM "Software\Classes\${PROGID}\shell\open\command" "" '"$INSTDIR\Devpad.exe" "%1"'
!macroend

!macro APP_UNASSOCIATE EXT PROGID
    DeleteRegKey HKLM "Software\Classes\.${EXT}"
!macroend

; ----- Sections -----
Section "Devpad" SecCore
    SectionIn RO
    SetOutPath "$INSTDIR"
    ; Main executable
    File "..\build\Devpad.exe"

    ; Qt DLLs
    File "..\build\dist\Qt6Core.dll"
    File "..\build\dist\Qt6Gui.dll"
    File "..\build\dist\Qt6Widgets.dll"
    File "..\build\dist\Qt6Multimedia.dll"
    File "..\build\dist\Qt6Network.dll"
    File "..\build\dist\Qt6PrintSupport.dll"
    File "..\build\dist\Qt6Svg.dll"

    ; MinGW runtime & Qt dependency DLLs
    File "..\build\dist\libgcc_s_seh-1.dll"
    File "..\build\dist\libstdc++-6.dll"
    File "..\build\dist\libwinpthread-1.dll"
    File "..\build\dist\zlib1.dll"
    File "..\build\dist\libpng16-16.dll"
    File "..\build\dist\libfreetype-6.dll"
    File "..\build\dist\libharfbuzz-0.dll"
    File "..\build\dist\libdouble-conversion.dll"
    File "..\build\dist\libpcre2-16-0.dll"
    File "..\build\dist\libmd4c.dll"
    File "..\build\dist\libb2-1.dll"
    File "..\build\dist\libicuin78.dll"
    File "..\build\dist\libicuuc78.dll"
    File "..\build\dist\libicudt78.dll"
    File "..\build\dist\libzstd.dll"
    File "..\build\dist\libbrotlidec.dll"
    File "..\build\dist\libbrotlicommon.dll"
    File "..\build\dist\libjpeg-8.dll"
    File "..\build\dist\libexpat-1.dll"
    File "..\build\dist\libgraphite2.dll"
    File "..\build\dist\libpcre2-8-0.dll"
    File "..\build\dist\libintl-8.dll"
    File "..\build\dist\libiconv-2.dll"
    File "..\build\dist\libglib-2.0-0.dll"
    File "..\build\dist\libgio-2.0-0.dll"
    File "..\build\dist\libgmodule-2.0-0.dll"
    File "..\build\dist\libgobject-2.0-0.dll"
    File "..\build\dist\libffi-8.dll"
    File "..\build\dist\libfontconfig-1.dll"
    File "..\build\dist\libpixman-1-0.dll"
    File "..\build\dist\libcairo-2.dll"
    File "..\build\dist\libpangocairo-1.0-0.dll"
    File "..\build\dist\libpango-1.0-0.dll"
    File "..\build\dist\libpangoft2-1.0-0.dll"
    File "..\build\dist\libpangowin32-1.0-0.dll"
    File "..\build\dist\libcairo-gobject-2.dll"
    File "..\build\dist\libfribidi-0.dll"
    File "..\build\dist\libthai-0.dll"
    File "..\build\dist\libdatrie-1.dll"
    File "..\build\dist\libxml2-16.dll"
    File "..\build\dist\liblzma-5.dll"
    File "..\build\dist\librsvg-2-2.dll"
    File "..\build\dist\libcrypto-3-x64.dll"
    File "..\build\dist\libsharpyuv-0.dll"
    File "..\build\dist\libbrotlienc.dll"
    File "..\build\dist\libbz2-1.dll"

    ; FFmpeg multimedia
    File "..\build\dist\avcodec-62.dll"
    File "..\build\dist\avformat-62.dll"
    File "..\build\dist\avutil-60.dll"
    File "..\build\dist\swresample-6.dll"
    File "..\build\dist\swscale-9.dll"

    ; D3D compiler
    File "..\build\dist\D3Dcompiler_47.dll"

    ; Qt plugins
    SetOutPath "$INSTDIR\platforms"
    File "..\build\dist\platforms\qwindows.dll"

    SetOutPath "$INSTDIR\styles"
    File "..\build\dist\styles\qmodernwindowsstyle.dll"

    SetOutPath "$INSTDIR\imageformats"
    File "..\build\dist\imageformats\qgif.dll"
    File "..\build\dist\imageformats\qico.dll"
    File "..\build\dist\imageformats\qjpeg.dll"
    File "..\build\dist\imageformats\qsvg.dll"

    SetOutPath "$INSTDIR\iconengines"
    File "..\build\dist\iconengines\qsvgicon.dll"

    SetOutPath "$INSTDIR\multimedia"
    File "..\build\dist\multimedia\ffmpegmediaplugin.dll"

    SetOutPath "$INSTDIR\networkinformation"
    File "..\build\dist\networkinformation\qglib.dll"
    File "..\build\dist\networkinformation\qnetworklistmanager.dll"

    SetOutPath "$INSTDIR\tls"
    File "..\build\dist\tls\qcertonlybackend.dll"
    File /nonfatal "..\build\dist\tls\qopensslbackend.dll"
    File "..\build\dist\tls\qschannelbackend.dll"

    SetOutPath "$INSTDIR\generic"
    File "..\build\dist\generic\qtuiotouchplugin.dll"

    ; Translations
    SetOutPath "$INSTDIR\translations"
    File /nonfatal "..\build\dist\translations\*.qm"

    ; Devpad translations
    SetOutPath "$INSTDIR"
    File /nonfatal "..\build\devpad_*.qm"

    ; License
    SetOutPath "$INSTDIR"
    File "..\LICENSE"

SectionEnd

; ----- Shortcuts -----
Section "Shortcuts" SecShortcuts
    SetShellVarContext all
    CreateDirectory "$SMPROGRAMS\Devpad"
    CreateShortCut "$SMPROGRAMS\Devpad\Devpad.lnk" "$INSTDIR\Devpad.exe" "" "$INSTDIR\Devpad.exe" 0
    CreateShortCut "$SMPROGRAMS\Devpad\Uninstall.lnk" "$INSTDIR\uninst.exe"
    CreateShortCut "$DESKTOP\Devpad.lnk" "$INSTDIR\Devpad.exe" "" "$INSTDIR\Devpad.exe" 0
SectionEnd

; ----- File Associations -----
Section "File Associations" SecAssoc
    SetShellVarContext all

    ; Create ProgID
    WriteRegStr HKLM "Software\Classes\Devpad.File\shell\open\command" "" '"$INSTDIR\Devpad.exe" "%1"'
    WriteRegStr HKLM "Software\Classes\Devpad.File\DefaultIcon" "" "$INSTDIR\Devpad.exe,0"

    ; Register extensions
    !insertmacro APP_ASSOCIATE "cpp"  "Devpad.File" "C++ Source"          "$INSTDIR\Devpad.exe,0"
    !insertmacro APP_ASSOCIATE "c"    "Devpad.File" "C Source"            "$INSTDIR\Devpad.exe,0"
    !insertmacro APP_ASSOCIATE "h"    "Devpad.File" "C/C++ Header"        "$INSTDIR\Devpad.exe,0"
    !insertmacro APP_ASSOCIATE "hpp"  "Devpad.File" "C++ Header"          "$INSTDIR\Devpad.exe,0"
    !insertmacro APP_ASSOCIATE "cc"   "Devpad.File" "C++ Source"          "$INSTDIR\Devpad.exe,0"
    !insertmacro APP_ASSOCIATE "cxx"  "Devpad.File" "C++ Source"          "$INSTDIR\Devpad.exe,0"
    !insertmacro APP_ASSOCIATE "cs"   "Devpad.File" "C# Source"           "$INSTDIR\Devpad.exe,0"
    !insertmacro APP_ASSOCIATE "java" "Devpad.File" "Java Source"         "$INSTDIR\Devpad.exe,0"
    !insertmacro APP_ASSOCIATE "py"   "Devpad.File" "Python Source"       "$INSTDIR\Devpad.exe,0"
    !insertmacro APP_ASSOCIATE "js"   "Devpad.File" "JavaScript Source"   "$INSTDIR\Devpad.exe,0"
    !insertmacro APP_ASSOCIATE "ts"   "Devpad.File" "TypeScript Source"   "$INSTDIR\Devpad.exe,0"
    !insertmacro APP_ASSOCIATE "tsx"  "Devpad.File" "TypeScript JSX"      "$INSTDIR\Devpad.exe,0"
    !insertmacro APP_ASSOCIATE "html" "Devpad.File" "HTML Document"       "$INSTDIR\Devpad.exe,0"
    !insertmacro APP_ASSOCIATE "css"  "Devpad.File" "CSS Stylesheet"      "$INSTDIR\Devpad.exe,0"
    !insertmacro APP_ASSOCIATE "xml"  "Devpad.File" "XML Document"        "$INSTDIR\Devpad.exe,0"
    !insertmacro APP_ASSOCIATE "svg"  "Devpad.File" "SVG Document"        "$INSTDIR\Devpad.exe,0"
    !insertmacro APP_ASSOCIATE "sql"  "Devpad.File" "SQL Script"          "$INSTDIR\Devpad.exe,0"
    !insertmacro APP_ASSOCIATE "rs"   "Devpad.File" "Rust Source"         "$INSTDIR\Devpad.exe,0"
    !insertmacro APP_ASSOCIATE "go"   "Devpad.File" "Go Source"           "$INSTDIR\Devpad.exe,0"
    !insertmacro APP_ASSOCIATE "md"   "Devpad.File" "Markdown Document"   "$INSTDIR\Devpad.exe,0"
    !insertmacro APP_ASSOCIATE "sh"   "Devpad.File" "Shell Script"        "$INSTDIR\Devpad.exe,0"
    !insertmacro APP_ASSOCIATE "cmake" "Devpad.File" "CMake Script"       "$INSTDIR\Devpad.exe,0"
    !insertmacro APP_ASSOCIATE "json" "Devpad.File" "JSON Document"       "$INSTDIR\Devpad.exe,0"
    !insertmacro APP_ASSOCIATE "yaml" "Devpad.File" "YAML Document"       "$INSTDIR\Devpad.exe,0"
    !insertmacro APP_ASSOCIATE "yml"  "Devpad.File" "YAML Document"       "$INSTDIR\Devpad.exe,0"
    !insertmacro APP_ASSOCIATE "txt"  "Devpad.File" "Text Document"       "$INSTDIR\Devpad.exe,0"

    ; "Open with Devpad" context menu for all files
    WriteRegStr HKLM "Software\Classes\*\shell\Open with Devpad\command" "" '"$INSTDIR\Devpad.exe" "%1"'

    ; Refresh shell
    System::Call 'shell32.dll::SHChangeNotify(i 0x08000000, i 0, i 0, i 0)'
SectionEnd

; ----- Uninstaller -----
Section -Uninstaller
    WriteUninstaller "$INSTDIR\uninst.exe"

    ; Add/Remove Programs
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
    WriteRegDWORD HKLM "${PRODUCT_UNINST_KEY}" "NoModify" 1
    WriteRegDWORD HKLM "${PRODUCT_UNINST_KEY}" "NoRepair" 1
SectionEnd

; ----- Uninstall section -----
Section Uninstall
    SetShellVarContext all

    ; Remove shortcuts
    Delete "$SMPROGRAMS\Devpad\Devpad.lnk"
    Delete "$SMPROGRAMS\Devpad\Uninstall.lnk"
    RmDir "$SMPROGRAMS\Devpad"
    Delete "$DESKTOP\Devpad.lnk"

    ; Remove application files
    RmDir /r "$INSTDIR"

    ; Remove file associations
    !insertmacro APP_UNASSOCIATE "cpp"  "Devpad.File"
    !insertmacro APP_UNASSOCIATE "c"    "Devpad.File"
    !insertmacro APP_UNASSOCIATE "h"    "Devpad.File"
    !insertmacro APP_UNASSOCIATE "hpp"  "Devpad.File"
    !insertmacro APP_UNASSOCIATE "cc"   "Devpad.File"
    !insertmacro APP_UNASSOCIATE "cxx"  "Devpad.File"
    !insertmacro APP_UNASSOCIATE "cs"   "Devpad.File"
    !insertmacro APP_UNASSOCIATE "java" "Devpad.File"
    !insertmacro APP_UNASSOCIATE "py"   "Devpad.File"
    !insertmacro APP_UNASSOCIATE "js"   "Devpad.File"
    !insertmacro APP_UNASSOCIATE "ts"   "Devpad.File"
    !insertmacro APP_UNASSOCIATE "tsx"  "Devpad.File"
    !insertmacro APP_UNASSOCIATE "html" "Devpad.File"
    !insertmacro APP_UNASSOCIATE "css"  "Devpad.File"
    !insertmacro APP_UNASSOCIATE "xml"  "Devpad.File"
    !insertmacro APP_UNASSOCIATE "svg"  "Devpad.File"
    !insertmacro APP_UNASSOCIATE "sql"  "Devpad.File"
    !insertmacro APP_UNASSOCIATE "rs"   "Devpad.File"
    !insertmacro APP_UNASSOCIATE "go"   "Devpad.File"
    !insertmacro APP_UNASSOCIATE "md"   "Devpad.File"
    !insertmacro APP_UNASSOCIATE "sh"   "Devpad.File"
    !insertmacro APP_UNASSOCIATE "cmake" "Devpad.File"
    !insertmacro APP_UNASSOCIATE "json" "Devpad.File"
    !insertmacro APP_UNASSOCIATE "yaml" "Devpad.File"
    !insertmacro APP_UNASSOCIATE "yml"  "Devpad.File"
    !insertmacro APP_UNASSOCIATE "txt"  "Devpad.File"

    DeleteRegKey HKLM "Software\Classes\*\shell\Open with Devpad"
    DeleteRegKey HKLM "Software\Classes\Devpad.File"
    DeleteRegKey HKLM "${PRODUCT_UNINST_KEY}"
    DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"

    ; Refresh shell
    System::Call 'shell32.dll::SHChangeNotify(i 0x08000000, i 0, i 0, i 0)'
SectionEnd
