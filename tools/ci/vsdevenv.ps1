# Load the Visual Studio Developer environment (cl, link, the Windows SDK,
# VS's CMake and Ninja) for the later steps of a GitHub Actions job, without
# a third-party action:
#
#   tools/ci/vsdevenv.ps1 -Arch amd64     # x64 runners
#   tools/ci/vsdevenv.ps1 -Arch arm64     # windows-11-arm (native arm64 host)
#
# It finds the newest Visual Studio with the C++ tools for that architecture
# with vswhere (installed with every VS at a fixed path), runs VsDevCmd.bat
# -arch=<a> -host_arch=<a> in cmd, and copies every variable it set or
# changed into $GITHUB_ENV (PATH included: a GITHUB_ENV PATH replaces the
# whole value for later steps). Run outside Actions, it only prints the
# result. See https://learn.microsoft.com/en-us/visualstudio/ide/reference/command-prompt-powershell
# and https://github.com/microsoft/vswhere.
param(
    [ValidateSet('amd64', 'arm64')]
    [string]$Arch = 'amd64'
)
$ErrorActionPreference = 'Stop'

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $vswhere)) { throw "vswhere not found at $vswhere" }
$component = if ($Arch -eq 'arm64') { 'Microsoft.VisualStudio.Component.VC.Tools.ARM64' }
             else { 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64' }
$vs = & $vswhere -latest -products * -requires $component -property installationPath
if (-not $vs) { throw "no Visual Studio with $component" }
$devcmd = Join-Path $vs 'Common7\Tools\VsDevCmd.bat'
$version = & $vswhere -latest -products * -requires $component -property catalog_productDisplayVersion
Write-Output "Visual Studio $version at $vs"

$before = @{}
Get-ChildItem env: | ForEach-Object { $before[$_.Name] = $_.Value }
$lines = cmd /d /c "`"$devcmd`" -arch=$Arch -host_arch=$Arch -no_logo && set"
if ($LASTEXITCODE -ne 0) { throw "VsDevCmd.bat failed ($LASTEXITCODE)" }

$changed = @{}
foreach ($line in $lines) {
    if ($line -match '^([^=]+)=(.*)$') {
        $name, $value = $Matches[1], $Matches[2]
        if ($before[$name] -ne $value) { $changed[$name] = $value }
    }
}
foreach ($name in $changed.Keys) { Set-Item "env:$name" $changed[$name] }
if ($env:GITHUB_ENV) {
    foreach ($name in $changed.Keys) {
        "$name=$($changed[$name])" | Out-File $env:GITHUB_ENV -Append -Encoding utf8
    }
}

# Fail fast if the environment isn't the one asked for.
if ($env:VSCMD_ARG_TGT_ARCH -ne $Arch -and -not ($Arch -eq 'amd64' -and $env:VSCMD_ARG_TGT_ARCH -eq 'x64')) {
    throw "VsDevCmd target arch is '$($env:VSCMD_ARG_TGT_ARCH)', wanted $Arch"
}
Write-Output "VSCMD_ARG_HOST_ARCH=$($env:VSCMD_ARG_HOST_ARCH) VSCMD_ARG_TGT_ARCH=$($env:VSCMD_ARG_TGT_ARCH) VCToolsVersion=$($env:VCToolsVersion)"
Write-Output "cl: $((Get-Command cl).Source)"
& cl 2>&1 | Select-Object -First 1
Write-Output "cmake: $((Get-Command cmake).Source)"
Write-Output "ninja: $((Get-Command ninja).Source)"
$clangcl = Get-Command clang-cl -ErrorAction SilentlyContinue
if ($clangcl) { Write-Output "clang-cl: $($clangcl.Source)" }
