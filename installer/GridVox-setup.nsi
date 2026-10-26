;==============================================================================
;  GRID VOX - Windows VST3 installer (NSIS 3.x / Unicode)
;
;  Build:
;      makensis /DVERSION=1.0.0 /DSRCDIR=<dir containing "GRID VOX.vst3"> ^
;               GridVox-setup.nsi
;
;  <SRCDIR> must contain a Windows VST3 bundle with the layout
;      GRID VOX.vst3\Contents\x86_64-win\GRID VOX.vst3
;  which is exactly what `cmake --build build --config Release --target
;  GridVox_VST3` produces on Windows (build\GridVox_artefacts\Release\VST3).
;
;  The installer writes the bundle to the system-wide VST3 folder
;  (C:\Program Files\Common Files\VST3) and registers an uninstaller in
;  "Apps & features". No components are added outside that folder.
;==============================================================================

Unicode true
SetCompressor /SOLID lzma
SetCompressorDictSize 32

!include "MUI2.nsh"
!include "FileFunc.nsh"
!include "LogicLib.nsh"
!include "x64.nsh"

!ifndef VERSION
  !define VERSION "1.0.0"
!endif
!ifndef SRCDIR
  !define SRCDIR "..\dist\windows"
!endif

!define PRODUCT_NAME    "GRID VOX"
!define PRODUCT_PUBLISHER "Grid Audio"
!define PRODUCT_VERSION "${VERSION}"
!define BUNDLE_NAME     "GRID VOX.vst3"
!define UNINST_KEY      "Software\Microsoft\Windows\CurrentVersion\Uninstall\GridVoxVST3"

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
BrandingText "${PRODUCT_NAME} vocal tracking chain"
OutFile "GRID-VOX-${PRODUCT_VERSION}-Setup.exe"
InstallDir "$COMMONFILES64\VST3"
InstallDirRegKey HKLM "${UNINST_KEY}" "InstallLocation"
RequestExecutionLevel admin
ShowInstDetails show
ShowUninstDetails show

VIProductVersion "${VERSION}.0"
VIAddVersionKey "ProductName"     "${PRODUCT_NAME}"
VIAddVersionKey "CompanyName"     "${PRODUCT_PUBLISHER}"
VIAddVersionKey "FileDescription" "${PRODUCT_NAME} VST3 installer"
VIAddVersionKey "FileVersion"     "${PRODUCT_VERSION}"
VIAddVersionKey "LegalCopyright"  "(c) ${PRODUCT_PUBLISHER}"

;------------------------------------------------------------------ appearance
!define MUI_ABORTWARNING
!define MUI_ICON   "${__FILEDIR__}\gridvox.ico"
!define MUI_UNICON "${__FILEDIR__}\gridvox.ico"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP_NOSTRETCH
!define MUI_FINISHPAGE_NOAUTOCLOSE
!define MUI_FINISHPAGE_TEXT "GRID VOX was installed to:$\r$\n$INSTDIR$\r$\n$\r$\nRestart your DAW (or use its plug-in rescan command) so the plug-in appears in the VST3 list.$\r$\n$\r$\nFL Studio: Options > Manage plug-ins > Find installed plug-ins$\r$\nREAPER:   Options > Preferences > Plug-ins > VST > Re-scan"

;------------------------------------------------------------------ pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${__FILEDIR__}\LICENSE.txt"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

;------------------------------------------------------------------ payload check
!if /FILEEXISTS "${SRCDIR}\${BUNDLE_NAME}\Contents\x86_64-win\GRID VOX.vst3"
!else
  !error "Windows VST3 payload not found. Build it first: cmake --build build --config Release --target GridVox_VST3  (see INSTALL-WINDOWS.md) then pass /DSRCDIR=<build>\\GridVox_artefacts\\Release\\VST3"
!endif

;------------------------------------------------------------------ sections
Section "GRID VOX VST3 plug-in" SecCore
  SectionIn RO
  SetOutPath "$INSTDIR"
  SetOverwrite try

  ; remove a previous copy so no stale files linger in the bundle
  RMDir /r "$INSTDIR\${BUNDLE_NAME}"

  SetOutPath "$INSTDIR"
  File /r "${SRCDIR}\${BUNDLE_NAME}"

  WriteUninstaller "$INSTDIR\GRID VOX Uninstall.exe"
  CreateDirectory "$SMPROGRAMS\GRID VOX"
  CreateShortCut  "$SMPROGRAMS\GRID VOX\Uninstall GRID VOX.lnk" "$INSTDIR\GRID VOX Uninstall.exe"

  ${GetSize} "$INSTDIR\${BUNDLE_NAME}" "/S=0K" $0 $1 $2
  WriteRegStr   HKLM "${UNINST_KEY}" "DisplayName"     "${PRODUCT_NAME} (VST3)"
  WriteRegStr   HKLM "${UNINST_KEY}" "DisplayVersion"  "${PRODUCT_VERSION}"
  WriteRegStr   HKLM "${UNINST_KEY}" "Publisher"       "${PRODUCT_PUBLISHER}"
  WriteRegStr   HKLM "${UNINST_KEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr   HKLM "${UNINST_KEY}" "DisplayIcon"     "$INSTDIR\GRID VOX Uninstall.exe,0"
  WriteRegStr   HKLM "${UNINST_KEY}" "UninstallString" '"$INSTDIR\GRID VOX Uninstall.exe"'
  WriteRegStr   HKLM "${UNINST_KEY}" "QuietUninstallString" '"$INSTDIR\GRID VOX Uninstall.exe" /S'
  WriteRegDWORD HKLM "${UNINST_KEY}" "NoModify" 1
  WriteRegDWORD HKLM "${UNINST_KEY}" "NoRepair" 1
  WriteRegDWORD HKLM "${UNINST_KEY}" "EstimatedSize" "$0"
SectionEnd

Section /o "Desktop shortcut to the uninstaller" SecDesktop
  CreateShortCut "$DESKTOP\Uninstall GRID VOX.lnk" "$INSTDIR\GRID VOX Uninstall.exe"
SectionEnd

Section "Documentation" SecDocs
  SetOutPath "$INSTDIR\GRID VOX"
  File "${__FILEDIR__}\README.txt"
  File "${__FILEDIR__}\LICENSE.txt"
SectionEnd

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecCore}    "Installs the 64-bit VST3 plug-in into the shared VST3 folder."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecDesktop} "Optional shortcut to the uninstaller."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecDocs}    "Read-me and licence notes (installed next to the plug-in)."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

;------------------------------------------------------------------ uninstall
Function .onInit
  ; 64-bit Windows is required for a 64-bit VST3
  ${IfNot} ${RunningX64}
    MessageBox MB_ICONSTOP "GRID VOX is a 64-bit VST3 plug-in and needs 64-bit Windows."
    Abort
  ${EndIf}
FunctionEnd

Section "Uninstall"
  Delete "$INSTDIR\${BUNDLE_NAME}\Contents\x86_64-win\GRID VOX.vst3"
  Delete "$INSTDIR\${BUNDLE_NAME}\Contents\Resources\moduleinfo.json"
  Delete "$INSTDIR\${BUNDLE_NAME}\Contents\moduleinfo.json"
  RMDir  "$INSTDIR\${BUNDLE_NAME}\Contents\x86_64-win"
  RMDir  "$INSTDIR\${BUNDLE_NAME}\Contents\Resources"
  RMDir  "$INSTDIR\${BUNDLE_NAME}\Contents"
  RMDir /r "$INSTDIR\${BUNDLE_NAME}"
  RMDir /r "$INSTDIR\GRID VOX"

  Delete "$SMPROGRAMS\GRID VOX\Uninstall GRID VOX.lnk"
  RMDir  "$SMPROGRAMS\GRID VOX"
  Delete "$DESKTOP\Uninstall GRID VOX.lnk"
  Delete "$INSTDIR\GRID VOX Uninstall.exe"

  DeleteRegKey HKLM "${UNINST_KEY}"
SectionEnd
