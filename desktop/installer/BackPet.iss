﻿; =============================================
; === BackPet 桌面宠物 安装程序脚本 ===
; 使用 Inno Setup 6 编译:
;   ISCC.exe BackPet.iss
; 升级规则: 固定 AppId + 提升 MyAppVersion, 新安装包自动识别旧版、
;           沿用安装目录并覆盖文件; 用户数据 (%LOCALAPPDATA%/Roaming\BackPet)
;           在覆盖升级时保留, 仅在主动卸载时清理。
; =============================================

#define MyAppName "BackPet"
#define MyAppVersion "0.1.0"
#define MyAppPublisher "BackCat Project"
#define MyAppURL "https://github.com/ayangweb/BongoCat"
#define MyAppExeName "BackPet.exe"

; 源目录 (windeployqt 部署 + VC 运行时本地复制后的完整目录)
#define SourceDir "d:\WorkspaceHome\back_pet\code\desktop\dist"
; 应用图标 (同时内嵌于 exe 与安装包)
#define MyAppIcon "d:\WorkspaceHome\back_pet\code\desktop\app.ico"

[Setup]
; === 升级标识: AppId 终身不变, 版本号每次发版递增 ===
AppId={{8B5CF6A1-2D3E-4F5A-9B6C-7D8E9F0A1B2C}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
AppCopyright=Copyright (C) 2026 {#MyAppPublisher}
VersionInfoVersion=0.1.0.0
VersionInfoCompany={#MyAppPublisher}

; 安装位置与分组
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
; 覆盖升级时沿用上次目录与附加选项
UsePreviousAppDir=yes
UsePreviousTasks=yes
UsePreviousLanguage=yes

; 输出
OutputDir=d:\WorkspaceHome\back_pet\release
OutputBaseFilename=BackPet_Setup_{#MyAppVersion}
SetupIconFile={#MyAppIcon}
UninstallDisplayIcon={app}\{#MyAppExeName}
Compression=lzma2/max
SolidCompression=yes
LZMANumBlockThreads=2
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

; 界面与权限
WizardStyle=modern
DisableDirPage=no
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=dialog

; 安装/卸载前自动关闭运行中的程序
CloseApplications=force
RestartApplications=no

[Languages]
Name: "chinesesimp"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "创建桌面快捷方式"; GroupDescription: "附加选项:"
Name: "startup"; Description: "开机自启动"; GroupDescription: "附加选项:"; Flags: unchecked

[Files]
; 递归拷贝 dist 下的所有文件 (ignoreversion: 升级时直接覆盖)
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
; 开始菜单
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\{#MyAppExeName}"
Name: "{group}\卸载 {#MyAppName}"; Filename: "{uninstallexe}"

; 桌面快捷方式 (可选)
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

; 开机自启
Name: "{autostartup}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: startup

[Run]
; 安装完成后启动程序
Filename: "{app}\{#MyAppExeName}"; Description: "立即启动 {#MyAppName}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
; 仅主动卸载时清理用户配置 (覆盖升级不会执行本段)
Type: filesandordirs; Name: "{%LOCALAPPDATA}\BackPet"
Type: filesandordirs; Name: "{%APPDATA}\BackPet"
Type: dirifempty; Name: "{app}"

[Code]
// 安装前终止正在运行的 BackPet (CloseApplications 的双保险)
function InitializeSetup(): Boolean;
var
  ResultCode: Integer;
begin
  Result := True;
  Exec(ExpandConstant('{cmd}'), '/C taskkill /F /IM BackPet.exe 2>nul', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  Sleep(800);
end;

// 卸载前同样终止程序
function InitializeUninstall(): Boolean;
var
  ResultCode: Integer;
begin
  Result := True;
  Exec(ExpandConstant('{cmd}'), '/C taskkill /F /IM BackPet.exe 2>nul', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  Sleep(500);
end;
