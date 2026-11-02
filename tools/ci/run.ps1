# PowerShell counterpart of tools/ci/run.sh, for the Windows workflows that
# use a VS Developer environment (msvc and clang-cl; see tools/ci/vsdevenv.ps1).
# Same environment settings as run.sh:
#
#   CPPDOCS_PLATFORM, CPPDOCS_COMPILER, CPPDOCS_PRESET (required), CXX,
#   CPPDOCS_AREAS, CPPDOCS_TESTS, CPPDOCS_SNIPPETS, CPPDOCS_CMAKE_ARGS,
#   CPPDOCS_SNIPPET_TOOLCHAIN, CPPDOCS_SNIPPET_ARGS, CPPDOCS_BASELINE,
#   CPPDOCS_OS_VERSION, CPPDOCS_OUT
#
# It writes results-<platform>-<compiler>.json in the repo root, the summary to
# $CPPDOCS_OUT/summary.md and $GITHUB_STEP_SUMMARY, and exits non-zero if any
# step failed. The tests go through tools/run_tests.ps1 (the `msvc` or
# `clang-cl` preset); the snippet checker runs even if the tests failed.
$ErrorActionPreference = 'Continue'
foreach ($v in 'CPPDOCS_PLATFORM', 'CPPDOCS_COMPILER', 'CPPDOCS_PRESET') {
    if (-not [Environment]::GetEnvironmentVariable($v)) { Write-Error "set $v"; exit 2 }
}
$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
Set-Location $root

function Env-Or($name, $default) {
    $v = [Environment]::GetEnvironmentVariable($name)
    if ($v) { $v } else { $default }
}
function Words($s) { if ($s) { @($s -split '\s+' | Where-Object { $_ }) } else { @() } }

$preset = $env:CPPDOCS_PRESET
$areas = Env-Or 'CPPDOCS_AREAS' ''
$runTests = Env-Or 'CPPDOCS_TESTS' 'true'
$runSnippets = Env-Or 'CPPDOCS_SNIPPETS' 'true'
$out = Env-Or 'CPPDOCS_OUT' 'ci-out'
# `python` first: on Windows, python3.exe can be the Microsoft Store stub.
$py = (Get-Command python -ErrorAction SilentlyContinue)
if (-not $py) { $py = Get-Command python3 }
$py = $py.Source
$results = "results-$($env:CPPDOCS_PLATFORM)-$($env:CPPDOCS_COMPILER).json"
New-Item -ItemType Directory -Force $out | Out-Null
Remove-Item -ErrorAction SilentlyContinue "$out/tests.json", "$out/snippets.json"

$steps = @()
$status = 0
function Step($name, $code) {
    $script:steps += @('--step', "$name=$code")
    if ($code -ne 0) { $script:status = 1 }
}

Write-Output '::group::toolchain'
Write-Output "platform=$($env:CPPDOCS_PLATFORM) compiler=$($env:CPPDOCS_COMPILER) preset=$preset areas='$areas'"
$cxx = Env-Or 'CXX' 'cl'
if ($cxx -eq 'cl') { & cl 2>&1 | Select-Object -First 1 } else { & $cxx --version }
cmake --version | Select-Object -First 1
ninja --version
& $py --version
Write-Output '::endgroup::'

if ($runTests -eq 'true') {
    $env:CPPDOCS_KEEP_GOING = '1'   # run_tests.ps1: build with -k 0, show every compile error
    # tools/run_tests.ps1 configures, builds every area and runs ctest (only
    # the areas' labels with -L), then writes the JSON. Native Windows builds
    # are fast enough that building all areas for an `areas` run is fine.
    $ctestArgs = @()
    if ($areas) { $ctestArgs = @('-L', $areas) }
    $cmakeArgs = Words $env:CPPDOCS_CMAKE_ARGS
    if ($cmakeArgs.Count -gt 0) { Write-Warning "CPPDOCS_CMAKE_ARGS isn't supported by run_tests.ps1; ignored" }
    Write-Output "::group::tests: tools/run_tests.ps1 -Json $out/tests.json $preset $($ctestArgs -join ' ')"
    & (Join-Path $root 'tools/run_tests.ps1') -Json "$out/tests.json" $preset @ctestArgs 2>&1 | Tee-Object "$out/test.log"
    Step 'tests' $LASTEXITCODE
    Write-Output '::endgroup::'
    Copy-Item -ErrorAction SilentlyContinue "build/$preset/Testing/Temporary/LastTest.log" $out
}

$snippetsMode = (& $py tools/ci/cppdocs_ci.py snippet-mode --compiler $env:CPPDOCS_COMPILER `
    --platform $env:CPPDOCS_PLATFORM --baseline (Env-Or 'CPPDOCS_BASELINE' '')).Trim()
if ($runSnippets -eq 'true') {
    $paths = @('docs')
    if ($areas) {
        $paths = @(& $py tools/ci/cppdocs_ci.py areas --regex $areas --paths)
        if ($LASTEXITCODE -ne 0) { $paths = @() }
    }
    $snipArgs = @('--toolchain', (Env-Or 'CPPDOCS_SNIPPET_TOOLCHAIN' 'auto'), '-q', '--json', "$out/snippets.json")
    if ($env:CXX) { $snipArgs += @('--cxx', $env:CXX) }
    if ($env:CPPDOCS_BASELINE) { $snipArgs += @('--baseline', $env:CPPDOCS_BASELINE) }
    if ($snippetsMode -eq 'report-only') {
        $snipArgs += '--report-only'
        Write-Output "snippets: report-only for $($env:CPPDOCS_COMPILER) on $($env:CPPDOCS_PLATFORM) (no reviewed baseline; #159)"
    }
    $snipArgs += Words $env:CPPDOCS_SNIPPET_ARGS
    if ($paths.Count -gt 0) {
        Write-Output "::group::snippets ($($paths -join ' '))"
        & $py tools/check_snippets.py @paths @snipArgs 2>&1 | Tee-Object "$out/snippets.log"
        Step 'snippets' $LASTEXITCODE
        Write-Output '::endgroup::'
    } else {
        Step 'snippets' 1
    }
}

& $py tools/ci/cppdocs_ci.py collect --platform $env:CPPDOCS_PLATFORM --compiler $env:CPPDOCS_COMPILER `
    --preset $preset --os-version (Env-Or 'CPPDOCS_OS_VERSION' '') --areas $areas `
    --run-tests $runTests --run-snippets $runSnippets `
    --tests "$out/tests.json" --snippets "$out/snippets.json" --snippets-mode $snippetsMode `
    @steps -o $results --summary "$out/summary.md"
if ($LASTEXITCODE -ne 0) { $status = 1 }
if ($env:GITHUB_STEP_SUMMARY -and (Test-Path "$out/summary.md")) {
    Get-Content "$out/summary.md" -Encoding utf8 | Add-Content $env:GITHUB_STEP_SUMMARY -Encoding utf8
}
exit $status
