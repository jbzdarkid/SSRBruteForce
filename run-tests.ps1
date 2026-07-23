# Replay every level's .dem under the build matching its sausage count, confirming it reaches Won().
# Usage:
#   .\run-tests.ps1                            # build + replay all levels (logs to run-tests.log)
#   .\run-tests.ps1 -Solve "3-14"              # build + solve named level
#   .\run-tests.ps1 -TestName "3-13 Cold Gate" # only levels whose name contains this substring
#   .\run-tests.ps1 -DemoDir "C:\path\to\dems" # Custom path to a demo directory (default ..\SSRDecompile\App)
[CmdletBinding()]
param(
    [string] $TestName = "",
    [string] $DemoOverride = "",
    [switch] $Solve
)

$ErrorActionPreference = "Stop"
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) { throw "No Visual Studio installation found" }

$vsPath = & $vswhere -latest -property installationPath
if (-not $vsPath) { throw "No Visual Studio installation found" }

$msbuild = Join-Path $vsPath "MSBuild\Current\Bin\MSBuild.exe"
if (-not (Test-Path $msbuild)) { throw "Latest installation $vsPath does not have MSBuild.exe" }

$Configuration = "Release"
$exe = ".\x64\$Configuration\SSRBruteForce.exe"

$levelDemos = @(
    [pscustomobject]@{ Name = "1-1 Lachrymose Head";   Dem = "1-1.dem";  Sausages = 3 }
    [pscustomobject]@{ Name = "1-2 Southjaunt";        Dem = "1-2.dem";  Sausages = 2 }
    [pscustomobject]@{ Name = "1-3 Infant's Break";    Dem = "1-3.dem";  Sausages = 2 }
    [pscustomobject]@{ Name = "1-5 Little Fire";       Dem = "1-4.dem";  Sausages = 2 }
    [pscustomobject]@{ Name = "1-7 Bay's Neck";        Dem = "1-5.dem";  Sausages = 1 }
    [pscustomobject]@{ Name = "1-8 Burning Wharf";     Dem = "1-6.dem";  Sausages = 2 }
    [pscustomobject]@{ Name = "1-6 Eastreach";         Dem = "1-7.dem";  Sausages = 2 }
    [pscustomobject]@{ Name = "1-4 Comely Hearth";     Dem = "1-8.dem";  Sausages = 2 }
    [pscustomobject]@{ Name = "1-9 Happy Pool";        Dem = "1-9.dem";  Sausages = 1 }
    [pscustomobject]@{ Name = "1-10 Maiden's Walk";    Dem = "1-10.dem"; Sausages = 1 }
    [pscustomobject]@{ Name = "1-11 Fiery Jut";        Dem = "1-11.dem"; Sausages = 2 }
    [pscustomobject]@{ Name = "1-12 Merchant's Elegy"; Dem = "1-12.dem"; Sausages = 2 }
    [pscustomobject]@{ Name = "1-13 Seafinger";        Dem = "1-13.dem"; Sausages = 2 }
    [pscustomobject]@{ Name = "1-14 The Clover";       Dem = "1-14.dem"; Sausages = 3 }
    [pscustomobject]@{ Name = "1-15 Inlet Shore";      Dem = "1-15.dem"; Sausages = 2 }
    [pscustomobject]@{ Name = "1-16 The Anchorage";    Dem = "1-16.dem"; Sausages = 3 }
    [pscustomobject]@{ Name = "2-1 Emerson Jetty";     Dem = "2-1.dem";  Sausages = 1 }
    [pscustomobject]@{ Name = "2-2 Sad Farm";          Dem = "2-2.dem";  Sausages = 1 }
    [pscustomobject]@{ Name = "2-3 Cove";              Dem = "2-3.dem";  Sausages = 2 }
    [pscustomobject]@{ Name = "2-5 The Paddock";       Dem = "2-5.dem";  Sausages = 2 }
    [pscustomobject]@{ Name = "2-6 Beautiful Horizon"; Dem = "2-6.dem";  Sausages = 2 }
    [pscustomobject]@{ Name = "2-8 Rough Field";       Dem = "2-7.dem";  Sausages = 2 }
    [pscustomobject]@{ Name = "2-10 Twisty Farm";      Dem = "2-8.dem";  Sausages = 2 }
    [pscustomobject]@{ Name = "2-9 Fallow Earth";      Dem = "2-9.dem";  Sausages = 1 }
    [pscustomobject]@{ Name = "2-7 Barrow Set";        Dem = "2-10.dem"; Sausages = 2 }
    [pscustomobject]@{ Name = "3-1 Cold Jag";          Dem = "3-1.dem";  Sausages = 3 }
    [pscustomobject]@{ Name = "3-2 Cold Finger";       Dem = "3-2.dem";  Sausages = 3 }
    [pscustomobject]@{ Name = "3-3 Cold Escarpment";   Dem = "3-3.dem";  Sausages = 2 }
    [pscustomobject]@{ Name = "3-14 Cold Frustration"; Dem = "3-4.dem";  Sausages = 3 }
    [pscustomobject]@{ Name = "3-4 Cold Trail";        Dem = "3-5.dem";  Sausages = 3 }
    [pscustomobject]@{ Name = "3-5 Cold Cliff";        Dem = "3-6.dem";  Sausages = 3 }
    [pscustomobject]@{ Name = "3-6 Cold Pit";          Dem = "3-7.dem";  Sausages = 2 }
    [pscustomobject]@{ Name = "3-7 Cold Plateau";      Dem = "3-8.dem";  Sausages = 2 }
    [pscustomobject]@{ Name = "3-8 Cold Head";         Dem = "3-9.dem";  Sausages = 2 }
    [pscustomobject]@{ Name = "3-9 Cold Ladder";       Dem = "3-10.dem"; Sausages = 3 }
    [pscustomobject]@{ Name = "3-10 Cold Sausage";     Dem = "3-11.dem"; Sausages = 5 }
    [pscustomobject]@{ Name = "3-11 Cold Terrace";     Dem = "3-12.dem"; Sausages = 3 }
    [pscustomobject]@{ Name = "3-12 Cold Horizon";     Dem = "3-13.dem"; Sausages = 2 }
    [pscustomobject]@{ Name = "3-13 Cold Gate";        Dem = "3-14.dem"; Sausages = 7 }
    [pscustomobject]@{ Name = "4-1 Wretch's Retreat";  Dem = "4-1.dem";  Sausages = 2 }
    [pscustomobject]@{ Name = "4-2 Toad's Folly";      Dem = "4-2.dem";  Sausages = 3 }
    [pscustomobject]@{ Name = "4-3 Sludge Coast";      Dem = "4-3.dem";  Sausages = 2 }
    [pscustomobject]@{ Name = "4-4 Foul Fen";          Dem = "4-4.dem";  Sausages = 1 }
    [pscustomobject]@{ Name = "4-5 Crunchy Leaves";    Dem = "4-5.dem";  Sausages = 3 }
    [pscustomobject]@{ Name = "4-6 Gator Paddock";     Dem = "4-6.dem";  Sausages = 1 }
)

function Build-Variant {
    param([int] $N)
    $macro = (0..($N-1) | ForEach-Object { "o($_)" }) -join " "
    $env:_CL_ = "/DSAUSAGES=`"$macro`""
    # Rebuild (not Build) avoids LNK1257 from stale PGO objects across SAUSAGES changes.
    # & $msbuild SSRBruteForce.vcxproj /p:Configuration=$Configuration /p:Platform=x64 /p:PlatformToolset=v143 /v:minimal /m /t:Rebuild
    & $msbuild SSRBruteForce.vcxproj /p:Configuration=$Configuration /p:Platform=x64 /p:PlatformToolset=v143 /v:minimal /m
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed for SAUSAGES=$N"
    }
}

# Candidate levels, optionally narrowed by -TestName (substring match on the display name).
$candidates = $levelDemos
if ($TestName) { $candidates = $candidates | Where-Object { $_.Name -like "*$TestName*" } }
if (@($candidates).Count -eq 0) { throw "No level name contains $TestName." }

# Build each distinct sausage count once (rebuilds are expensive), then replay every candidate under it.
$unified = [ordered]@{}
foreach ($lvl in $candidates) { $unified[$lvl.Name] = "SKIP" }

foreach ($n in ($candidates.Sausages | Sort-Object -Unique)) {
    echo "Building with $n sausages"
    Build-Variant -N $n
    echo "Running with $n sausages"
    foreach ($lvl in $candidates) {
        if ($lvl.Sausages -ne $n) { continue }

        if ($Solve) {
            echo "Solving $($lvl.Name)"
            if ($TestName) {
                & $exe $lvl.Name
            } else {
                & $exe $lvl.Name *>> $null
            }
        } else {
            echo "Testing $($lvl.Name)"
            if (-not $DemoOverride) {
                $path = "../SSRDecompile/App/" + $level.Dem
            } else {
                $path = $DemoOverride
            }
            if ($TestName) {
                & $exe $lvl.Name $path
            } else {
                & $exe $lvl.Name $path *>> $null
            }
        }

        if ($LASTEXITCODE -eq 4) { continue } # Wrong number of sausages, value will be set in another iteration
        if ($LASTEXITCODE -eq 0) { $unified[$lvl.Name] = "PASS" }
        else                     { $unified[$lvl.Name] = "FAIL" }
    }
}

echo "=== Results ==="
$failCount = 0
foreach ($level in $candidates) {
    $status = $unified[$level.Name]
    echo "[$status] $($level.Name)"
    if ($status -ne "PASS") { $failCount++ }
}
if ($failCount -gt 0) { exit 1 }
