; makensis would normally be invoked from the root of the source tree
!addincludedir "installers\windows"
!addplugindir "installers\windows"

!include "MUI2.nsh"
!include "fileassoc.nsh"

!ifndef PRODUCT_NAME
!define PRODUCT_NAME "unspecified-product"
!endif

!define EXE "${PRODUCT_NAME}.exe"

!ifndef NATIVE_EXTENSION
!define NATIVE_EXTENSION "unspecified-extension"
!endif

!ifndef VERSION
!define VERSION "unspecified-version"
!endif

!ifndef PUBLISHER
!define PUBLISHER "unspecified-publisher"
!endif

!ifndef COPYRIGHT
!define COPYRIGHT "unspecified-copyright"
!endif

!searchreplace COPYRIGHT "${COPYRIGHT}" "(c)" "�"

!define UNINSTALL_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"

Var INSTDIR_BASE

Name "${PRODUCT_NAME}"

VIProductVersion "1.0.0.0"
VIAddVersionKey ProductName "${PRODUCT_NAME}"
VIAddVersionKey Comments "${PRODUCT_NAME}"
VIAddVersionKey LegalCopyright "${COPYRIGHT}"
VIAddVersionKey FileDescription "${PRODUCT_NAME}"
VIAddVersionKey FileVersion "1.0.0.0"
VIAddVersionKey ProductVersion "1.0.0.0"

InstallDir ""

; Take the highest execution level available
; This means that if it's possible to, we become an administrator
RequestExecutionLevel highest

!macro ONINIT un
	Function ${un}.onInit
		; Make sure we're not still running first
		ExecCmd::exec "%SystemRoot%\System32\tasklist /NH /FI \
			$\"IMAGENAME eq ${EXE}$\" | \
			%SystemRoot%\System32\find /I $\"${EXE}$\""
		Pop $0
		ExecCmd::wait $0
		Pop $0
		IntCmp $0 1 notRunning
			MessageBox MB_OK|MB_ICONEXCLAMATION \
				"${PRODUCT_NAME} is still running. Please close it before making changes." /SD IDOK
			Abort
		notRunning:

		; The value of SetShellVarContext detetmines whether SHCTX is HKLM or HKCU
		; and whether SMPROGRAMS refers to all users or just the current user
		UserInfo::GetAccountType
		Pop $0
		${If} $0 == "Admin"
			; If we're an admin, default to installing to C:\Program Files
			SetShellVarContext all
			StrCpy $INSTDIR_BASE "$PROGRAMFILES64"
		${Else}
			; If we're just a user, default to installing to ~\AppData\Local
			SetShellVarContext current
			StrCpy $INSTDIR_BASE "$LOCALAPPDATA"
		${EndIf}

		${If} $INSTDIR == ""
			; This only happens in the installer, because the uninstaller already knows INSTDIR
			ReadRegStr $0 SHCTX "Software\${PRODUCT_NAME}" ""

			${If} $0 != ""
				; If we're already installed, use the existing directory
				StrCpy $INSTDIR "$0"
			${Else}
				StrCpy $INSTDIR "$INSTDIR_BASE\${PRODUCT_NAME}"
			${Endif}
		${Endif}
	FunctionEnd
!macroend

; Define the function twice, once for the installer and again for the uninstaller
!insertmacro ONINIT ""
!insertmacro ONINIT "un"

; Installer Icons
!insertmacro MUI_DEFAULT MUI_ICON "source\app\icon\Installer.ico"
!insertmacro MUI_DEFAULT MUI_UNICON "source\app\icon\Installer.ico"

Icon "${MUI_ICON}"
UninstallIcon "${MUI_UNICON}"

WindowIcon on

; This bitmap needs to be in BMP3 (Windows 3.x) format, for some reason
!define MUI_WELCOMEFINISHPAGE_BITMAP "installers\windows\welcomepage.bmp"
!define MUI_WELCOMEPAGE_TEXT \
"Setup will guide you through the installation of ${PRODUCT_NAME}.$\r$\n$\r$\n\
${PRODUCT_NAME} is a tool for the visualisation and analysis of graphs, \
brought to you by Kajeka.$\r$\n$\r$\n\
Click Next to continue."
!insertmacro MUI_PAGE_WELCOME

!insertmacro MUI_PAGE_LICENSE "source\app\ui\licensing\EULA.rtf"

!define MUI_COMPONENTSPAGE_NODESC
!insertmacro MUI_PAGE_COMPONENTS

!insertmacro MUI_PAGE_DIRECTORY

Var STARTMENU_FOLDER
!define MUI_STARTMENUPAGE_REGISTRY_ROOT "SHCTX"
!define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\${PRODUCT_NAME}"
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"
!insertmacro MUI_PAGE_STARTMENU ${PRODUCT_NAME} $STARTMENU_FOLDER

!insertmacro MUI_PAGE_INSTFILES

!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_CHECKED
!define MUI_FINISHPAGE_RUN_TEXT "Start ${PRODUCT_NAME}"
!define MUI_FINISHPAGE_RUN_FUNCTION "Launch"

!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

Section "-Main Component"
	SetOutPath "$INSTDIR"

	File /r "installer\*.*"

	WriteRegStr SHCTX "Software\${PRODUCT_NAME}" "" $INSTDIR

	; These registry entries are necessary for the program to show up in the Add/Remove programs dialog
	WriteRegStr SHCTX "${UNINSTALL_KEY}" "DisplayName" "${PRODUCT_NAME}"
	WriteRegStr SHCTX "${UNINSTALL_KEY}" "DisplayVersion" "${VERSION}"
	WriteRegStr SHCTX "${UNINSTALL_KEY}" "DisplayIcon" "$INSTDIR\${EXE}"
	WriteRegStr SHCTX "${UNINSTALL_KEY}" "Publisher" "${PUBLISHER}"
	WriteRegDWORD SHCTX "${UNINSTALL_KEY}" "EstimatedSize" "${BUILD_SIZE}"
	WriteRegStr SHCTX "${UNINSTALL_KEY}" "InstallLocation" "$INSTDIR"
	WriteRegStr SHCTX "${UNINSTALL_KEY}" "UninstallString" '"$INSTDIR\Uninstall.exe"'
	WriteRegDWORD SHCTX "${UNINSTALL_KEY}" "NoModify" 1
	WriteRegDWORD SHCTX "${UNINSTALL_KEY}" "NoRepair" 1

	WriteUninstaller "$INSTDIR\Uninstall.exe"

	!insertmacro MUI_STARTMENU_WRITE_BEGIN ${PRODUCT_NAME}
		CreateDirectory "$SMPROGRAMS\$STARTMENU_FOLDER\"
		CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\${PRODUCT_NAME}.lnk" "$INSTDIR\${EXE}"
	!insertmacro MUI_STARTMENU_WRITE_END

	!insertmacro UPDATEFILEASSOC
SectionEnd

; File Associations
SectionGroup /e "File associations"
	Section "${PRODUCT_NAME} file (.${NATIVE_EXTENSION})"
		!insertmacro APP_ASSOCIATE "${NATIVE_EXTENSION}" "${PRODUCT_NAME}" "${PRODUCT_NAME} File" \
				"$INSTDIR\${EXE},0" "Open with ${PRODUCT_NAME}" \
				"$INSTDIR\${EXE} $\"%1$\""
	SectionEnd
	Section "Graph Modelling Language file (.gml)"
		!insertmacro APP_ASSOCIATE "gml" "${PRODUCT_NAME}.gml" "${PRODUCT_NAME} GML File" \
				"$INSTDIR\${EXE},0" "Open with ${PRODUCT_NAME}" \
				"$INSTDIR\${EXE} $\"%1$\""
	SectionEnd
	Section "Graph Markup Language file (.graphml)"
		!insertmacro APP_ASSOCIATE "graphml" "${PRODUCT_NAME}.graphml" "${PRODUCT_NAME} GraphML File" \
				"$INSTDIR\${EXE},0" "Open with ${PRODUCT_NAME}" \
				"$INSTDIR\${EXE} $\"%1$\""
	SectionEnd
	Section "Biopax OWL file (.owl)"
		!insertmacro APP_ASSOCIATE "owl" "${PRODUCT_NAME}.owl" "${PRODUCT_NAME} OWL File" \
				"$INSTDIR\${EXE},0" "Open with ${PRODUCT_NAME}" \
				"$INSTDIR\${EXE} $\"%1$\""
	SectionEnd
SectionGroupEnd

Section /o "Desktop shortcut"
	CreateShortCut "$DESKTOP\${PRODUCT_NAME}.lnk" "$INSTDIR\${EXE}"
SectionEnd

;Launch function
Function Launch
	ShellExecAsUser::ShellExecAsUser "" "$INSTDIR\${EXE}"
FunctionEnd

Section "Uninstall"
	RMDir /r "$INSTDIR"

	!insertmacro MUI_STARTMENU_GETFOLDER ${PRODUCT_NAME} $STARTMENU_FOLDER
	Delete "$SMPROGRAMS\$STARTMENU_FOLDER\${PRODUCT_NAME}.lnk"
	RMDir /r "$SMPROGRAMS\$STARTMENU_FOLDER"

	Delete "$DESKTOP\${PRODUCT_NAME}.lnk"

	DeleteRegKey /ifempty SHCTX "Software\${PRODUCT_NAME}"

	DeleteRegKey SHCTX "${UNINSTALL_KEY}"

	!insertmacro APP_UNASSOCIATE "${NATIVE_EXTENSION}" "${PRODUCT_NAME}"
	!insertmacro APP_UNASSOCIATE "gml" "${PRODUCT_NAME}.gml"
	!insertmacro UPDATEFILEASSOC
SectionEnd
