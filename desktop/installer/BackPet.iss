; =============================================
; === BackPet 桌面宠物 安装程序脚本 ===
; 使用 Inno Setup 编译生成 .exe 安装包
; 下载地址: https://jrsoftware.org/isdl.php
; =============================================

#define MyAppName "BackPet"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "BackCat Project"
#define MyAppURL "https://github.com/BongoCat/BackPet"
#define MyAppExeName "BackPet.exe"

; 源目录 (windeployqt 打包后的完整目录)
#define SourceDir "d:\Workspace\test_project\Back-Cat-Project-trae-agent-4vI8Jf\desktop\output\Back_Pet_Release"

[Setup]
; 基本设置
AppId={{8B5CF6A1-2D3E-4F5A-9B6C-7D8E9F0A1B2C}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
OutputDir=d:\Workspace\test_project\Back-Cat-Project-trae-agent-4vI8Jf\desktop\output
OutputBaseFilename=BackPet_Setup_{#MyAppVersion}
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64

; 界面设置
UninstallDisplayIcon={app}\{#MyAppExeName}
WizardStyle=modern
DisableDirPage=no
PrivilegesRequired=admin

; 卸载设置
UninstallFilesDir={app}

[Languages]
Name: "chinesesimp"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "创建桌面快捷方式"; GroupDescription: "附加选项:"
Name: "startup"; Description: "开机自启动"; GroupDescription: "附加选项:"; Flags: unchecked

[Files]
; 递归拷贝打包目录下的所有文件
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
; 开始菜单
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\卸载 {#MyAppName}"; Filename: "{uninstallexe}"

; 桌面快捷方式 (可选)
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

; 开机自启
Name: "{autostartup}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: startup

[Run]
; 安装完成后启动程序
Filename: "{app}\{#MyAppExeName}"; Description: "立即启动 {#MyAppName}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
; 卸载时清理配置文件
Type: filesandordirs; Name: "{%LOCALAPPDATA}\BackPet"
Type: dirifempty; Name: "{app}"

[Code]
// 安装前检查是否已有实例运行
function InitializeSetup(): Boolean;
var
  ResultCode: Integer;
begin
  Result := True;
  // 尝试终止正在运行的 BackPet
  if Exec(ExpandConstant('{cmd}'), '/C taskkill /F /IM BackPet.exe 2>nul', '', SW_HIDE, ewWaitUntilTerminated, ResultCode) then
  begin
    Sleep(1000);
  end;
end;

// 卸载前也终止程序
function InitializeUninstall(): Boolean;
var
  ResultCode: Integer;
begin
  Result := True;
  if Exec(ExpandConstant('{cmd}'), '/C taskkill /F /IM BackPet.exe 2>nul', '', SW_HIDE, ewWaitUntilTerminated, ResultCode) then
  begin
    Sleep(500);
  end;
end;
