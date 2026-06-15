$ErrorActionPreference = "Stop"

# -- Colored output helpers --
function Write-Step  { param($msg) Write-Host "`n  [>>] $msg" -ForegroundColor Cyan }
function Write-Ok    { param($msg) Write-Host "  [OK] $msg" -ForegroundColor Green }
function Write-Err   { param($msg) Write-Host "  [!!] $msg" -ForegroundColor Red }
function Write-Info  { param($msg) Write-Host "  [--] $msg" -ForegroundColor Gray }

# -- Path setup --
$ScriptDir   = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent $ScriptDir
$ExeSource   = Join-Path $ProjectRoot "hw-diag\hw_diag.exe"
$LicenseFile = Join-Path $ScriptDir "license.txt"
$OutputDir   = Join-Path $ProjectRoot "output"

$InnoSetupDir = "${env:ProgramFiles(x86)}\Inno Setup 6"
$ISCCExe      = Join-Path $InnoSetupDir "ISCC.exe"
$ISDLPage     = "https://jrsoftware.org/isdl.php"

# -- Banner --
Write-Host ""
Write-Host "  ========================================" -ForegroundColor Yellow
Write-Host "    HW-Diag Installer Builder" -ForegroundColor Yellow
Write-Host "  ========================================" -ForegroundColor Yellow
Write-Host ""

# -- Step 1: Check source exe --
Write-Step "Checking hw_diag.exe..."

if (-not (Test-Path $ExeSource)) {
    Write-Err "hw_diag.exe NOT FOUND!"
    Write-Info "Expected location: $ExeSource"
    Write-Info "Please build the project first (run hw-diag\build.bat)"
    Read-Host "Press Enter to exit"
    exit 1
}

$sizeMB = [math]::Round((Get-Item $ExeSource).Length / 1MB, 2)
$msg = "hw_diag.exe found (" + $sizeMB + " MB)"
Write-Ok $msg

# -- Step 2: Check / Install Inno Setup 6 --
Write-Step "Checking Inno Setup 6..."

if (Test-Path $ISCCExe) {
    Write-Ok "Inno Setup 6 already installed"
    Write-Info "Path: $ISCCExe"
}
else {
    Write-Info "Fetching latest download link from Inno Setup website..."
    $installerPath = Join-Path $env:TEMP "innosetup-6.exe"

    try {
        [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

        # Fetch the download page and extract the .exe link
        $wc = New-Object System.Net.WebClient
        $pageHtml = $wc.DownloadString($ISDLPage)
        # Pattern: href="...is/6/innosetup-X.X.X.exe"  or  is-X.X.X.exe
        $match = [regex]::Match($pageHtml, 'href="(https?://files\.jrsoftware\.org/is/6/[^"]*innosetup[^"]*\.exe)"')
        if (-not $match.Success) {
            $match = [regex]::Match($pageHtml, 'href="([^"]*innosetup[^"]*\.exe)"')
        }
        if (-not $match.Success) {
            throw "Could not find download link on the page. Please install manually."
        }

        $InnoSetupUrl = $match.Groups[1].Value
        Write-Info "Download URL: $InnoSetupUrl"
        Write-Info "Downloading..."
        $wc.DownloadFile($InnoSetupUrl, $installerPath)
        Write-Ok "Download complete"
    }
    catch {
        Write-Err "Download failed!"
        Write-Info $_.Exception.Message
        Write-Info ""
        Write-Info "Please install Inno Setup 6 manually:"
        Write-Info "  1. Visit https://jrsoftware.org/isdl.php"
        Write-Info "  2. Download and install the latest version"
        Write-Info "  3. Re-run this script"
        Read-Host "Press Enter to exit"
        exit 1
    }

    Write-Info "Installing Inno Setup 6 (silent)..."
    try {
        $p = Start-Process -FilePath $installerPath -ArgumentList "/VERYSILENT /NORESTART /SUPPRESSMSGBOXES /SP-" -Wait -PassThru
        if ($p.ExitCode -ne 0) { throw "Installer exit code: $($p.ExitCode)" }
        Write-Ok "Inno Setup 6 installed successfully"
    }
    catch {
        Write-Err "Installation failed!"
        Write-Info $_.Exception.Message
        Write-Info "Please install Inno Setup 6 manually: https://jrsoftware.org/isdl.php"
        Read-Host "Press Enter to exit"
        exit 1
    }
    finally {
        if (Test-Path $installerPath) { Remove-Item $installerPath -Force }
    }

    if (-not (Test-Path $ISCCExe)) {
        Write-Err "ISCC.exe still not found after installation!"
        Read-Host "Press Enter to exit"
        exit 1
    }
    Write-Ok "ISCC.exe ready"
}

# -- Step 3: Detect language file --
$langFile = Join-Path $InnoSetupDir "Languages\ChineseSimplified.isl"
$langLines = 'Name: "english"; MessagesFile: "compiler:Default.isl"'
if (Test-Path $langFile) {
    $langLines = 'Name: "chinesesimplified"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"' + "`n" + $langLines
    Write-Ok "Chinese language pack detected - installer supports Chinese UI"
}
else {
    Write-Info "Chinese language pack not found - installer will use English UI"
}

# -- Step 4: Generate .iss script --
Write-Step "Generating installer script..."

$issLines = @()
$issLines += '; Auto-generated installer script'
$issLines += '#define MyAppName "Hardware Diagnostic Tool"'
$issLines += '#define MyAppNameEn "HW-Diag"'
$issLines += '#define MyAppVersion "1.0.0"'
$issLines += '#define MyAppPublisher "HW-Diag"'
$issLines += '#define MyAppExeName "hw_diag.exe"'
$issLines += ''
$issLines += '[Setup]'
$issLines += 'AppId={{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}'
$issLines += 'AppName={#MyAppName}'
$issLines += 'AppVersion={#MyAppVersion}'
$issLines += 'AppPublisher={#MyAppPublisher}'
$issLines += 'DefaultDirName={autopf}\{#MyAppNameEn}'
$issLines += 'DefaultGroupName={#MyAppName}'
$issLines += 'OutputBaseFilename=HW-Diag-Setup'
$issLines += "OutputDir=$OutputDir"
$issLines += 'Compression=lzma2/ultra64'
$issLines += 'SolidCompression=yes'
$issLines += 'WizardStyle=modern'
$issLines += 'PrivilegesRequired=lowest'
$issLines += 'ArchitecturesAllowed=x64compatible'
$issLines += 'ArchitecturesInstallIn64BitMode=x64compatible'
$issLines += 'UninstallDisplayIcon={app}\{#MyAppName}'
$issLines += 'LanguageDetectionMethod=uilanguage'
$issLines += 'ShowLanguageDialog=yes'
$issLines += "LicenseFile=$LicenseFile"
$issLines += ''
$issLines += '[Languages]'
$issLines += $langLines
$issLines += ''
$issLines += '[Tasks]'
$issLines += 'Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked'
$issLines += ''
$issLines += '[Files]'
$issLines += "Source: `"$ExeSource`"; DestDir: `"{app}`"; Flags: ignoreversion"
$issLines += ''
$issLines += '[Icons]'
$issLines += 'Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"'
$issLines += 'Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"'
$issLines += 'Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon'
$issLines += ''
$issLines += '[Run]'
$issLines += 'Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, ''&'', ''&&'')}}"; Flags: nowait postinstall skipifsilent'

$tempIss = Join-Path $ScriptDir "_generated.iss"
$issContent = $issLines -join "`n"
[System.IO.File]::WriteAllText($tempIss, $issContent, [System.Text.UTF8Encoding]::new($false))
Write-Ok "Installer script generated: _generated.iss"

# -- Step 5: Create output directory --
if (-not (Test-Path $OutputDir)) {
    New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
}

# -- Step 6: Compile installer --
Write-Step "Compiling installer package..."

try {
    $args = "/O`"$OutputDir`" /FHW-Diag-Setup `"$tempIss`""
    Write-Info "Running: ISCC.exe $args"

    $p = Start-Process -FilePath $ISCCExe -ArgumentList $args -Wait -PassThru -NoNewWindow

    if ($p.ExitCode -ne 0) {
        Write-Err "Compilation failed! Exit code: $($p.ExitCode)"
        Write-Info "Check the output above for error details"
        Read-Host "Press Enter to exit"
        exit 1
    }

    Write-Ok "Installer compiled successfully"
}
catch {
    Write-Err "Compilation error!"
    Write-Info $_.Exception.Message
    Read-Host "Press Enter to exit"
    exit 1
}
finally {
    if (Test-Path $tempIss) { Remove-Item $tempIss -Force }
}

# -- Step 7: Show result --
Write-Step "Build result..."

$setupExe = Join-Path $OutputDir "HW-Diag-Setup.exe"
if (Test-Path $setupExe) {
    $mb = [math]::Round((Get-Item $setupExe).Length / 1MB, 2)
    Write-Host ""
    Write-Host "  ========================================" -ForegroundColor Green
    Write-Host "    BUILD SUCCESS!" -ForegroundColor Green
    Write-Host "  ========================================" -ForegroundColor Green
    Write-Host ""
    Write-Host "  Output: $setupExe" -ForegroundColor Yellow
    $sizeMsg = "  Size:   " + $mb + " MB"
    Write-Host $sizeMsg -ForegroundColor Yellow
    Write-Host ""

    $open = Read-Host "  Open output folder? (Y/N)"
    if ($open -eq "Y" -or $open -eq "y") {
        explorer $OutputDir
    }
}
else {
    Write-Err "Installer file not generated!"
    Write-Info "Check the output above for error details"
    Read-Host "Press Enter to exit"
    exit 1
}
