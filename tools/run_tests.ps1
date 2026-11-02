# PowerShell counterpart of tools/run_tests.sh for Windows (PowerShell 5.1 or 7):
# configure, build and run the GoogleTest suite with one CMake workflow preset,
# then list every skipped test with its reason.
#
#   tools/run_tests.ps1                          # msvc (default), from a VS Developer PowerShell
#   tools/run_tests.ps1 clang-cl
#   $env:CXX = 'g++'; tools/run_tests.ps1 mingw  # MSYS2 MinGW-w64 on PATH
#   tools/run_tests.ps1 msvc -L containers       # one area (ctest arguments)
#   tools/run_tests.ps1 -Json build/results-msvc.json msvc
#
# There's no Docker re-exec here: the gcc-16 preset is for tools/run_tests.sh.
# Arguments are parsed by hand (no param block): ctest options such as -L must
# pass through untouched, and PowerShell would take them as parameter names.
$Json = ''
$rest = @($args)
if ($rest.Count -ge 2 -and $rest[0] -eq '-Json') {
    $Json = $rest[1]
    $rest = @($rest | Select-Object -Skip 2)
}
$Preset = 'msvc'
if ($rest.Count -ge 1) {
    $Preset = $rest[0]
    $rest = @($rest | Select-Object -Skip 1)
}
$CtestArgs = $rest
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

$status = 0
if ($env:CPPDOCS_KEEP_GOING) {
    # CI (tools/ci/run.ps1): build with Ninja's -k 0 so one run shows every
    # compile error, then test whatever built (only the ctest arguments' areas).
    cmake --preset $Preset
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    cmake --build --preset $Preset -- -k 0
    $status = $LASTEXITCODE
    ctest --preset $Preset @CtestArgs
    if ($LASTEXITCODE -ne 0) { $status = $LASTEXITCODE }
} elseif ($CtestArgs.Count -gt 0) {
    cmake --preset $Preset
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    cmake --build --preset $Preset
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    ctest --preset $Preset @CtestArgs
    $status = $LASTEXITCODE
} else {
    cmake --workflow --preset $Preset
    $status = $LASTEXITCODE
}

# ctest only says "(Skipped)"; the reason is the line after "...: Skipped" in the log.
$log = Join-Path $root "build/$Preset/Testing/Temporary/LastTest.log"
if (Test-Path $log) {
    $name = ''
    $skipped = @()
    $lines = Get-Content $log
    for ($i = 0; $i -lt $lines.Count; $i++) {
        if ($lines[$i] -match '^\d+/\d+ Test: (\S+)') { $name = $Matches[1] }
        elseif ($lines[$i] -match ': Skipped$' -and $i + 1 -lt $lines.Count) {
            $skipped += ('  {0,-50} {1}' -f $name, $lines[$i + 1])
        }
    }
    if ($skipped.Count -gt 0) {
        Write-Output ''
        Write-Output 'Skipped tests and why:'
        $skipped | Sort-Object | Write-Output
    }
}

if ($Json) {
    $py = Get-Command python3 -ErrorAction SilentlyContinue
    if (-not $py) { $py = Get-Command python -ErrorAction SilentlyContinue }
    if ($py) {
        & $py.Source tools/ctest_to_json.py --build-dir "build/$Preset" --preset $Preset -o $Json
    } else {
        Write-Warning "no python3 or python on PATH; can't write $Json"
    }
}
exit $status
