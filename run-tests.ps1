# Replay every level's .dem under the build matching its sausage count, confirming it reaches Won().
# Usage:
#   .\run-tests.ps1                            # build + replay all levels (logs to run-tests.log)
#   .\run-tests.ps1 -Solve "3-14"              # build + solve named level
#   .\run-tests.ps1 -TestName "3-13 Cold Gate" # only levels whose name contains this substring
#   .\run-tests.ps1 -World 1                    # only world 1's levels (the route runs separately by name)
#   .\run-tests.ps1 -TestName "Cold Gate" -DemoOverride "C:\path\to\a.dem" # replay a specific demo file
[CmdletBinding()]
param(
    [string] $TestName = "",
    [int]    $World = 0,
    [string] $DemoOverride = "",
    [switch] $Solve,
    [switch] $DebugMode
)

$ErrorActionPreference = "Stop"
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) { throw "No Visual Studio installation found" }

$vsPath = & $vswhere -latest -property installationPath
if (-not $vsPath) { throw "No Visual Studio installation found" }

$msbuild = Join-Path $vsPath "MSBuild\Current\Bin\MSBuild.exe"
if (-not (Test-Path $msbuild)) { throw "Latest installation $vsPath does not have MSBuild.exe" }

if ($DebugMode) {
    $devenv = Join-Path $vsPath "Common7\IDE\devenv.exe"
}

$Configuration = if ($DebugMode) { "Debug" } else { "Release" }
$exe = ".\x64\$Configuration\SSRBruteForce.exe"

$levelDemos = @(
    [pscustomobject]@{ World = 1; Sausages = 1; Name = "Bay's Neck"; }
    [pscustomobject]@{ World = 1; Sausages = 1; Name = "Happy Pool"; }
    [pscustomobject]@{ World = 1; Sausages = 1; Name = "Maiden's Walk"; }
    [pscustomobject]@{ World = 1; Sausages = 2; Name = "Burning Wharf"; }
    [pscustomobject]@{ World = 1; Sausages = 2; Name = "Comely Hearth"; }
    [pscustomobject]@{ World = 1; Sausages = 2; Name = "Eastreach"; }
    [pscustomobject]@{ World = 1; Sausages = 2; Name = "Fiery Jut"; }
    [pscustomobject]@{ World = 1; Sausages = 2; Name = "Infant's Break"; }
    [pscustomobject]@{ World = 1; Sausages = 2; Name = "Inlet Shore"; }
    [pscustomobject]@{ World = 1; Sausages = 2; Name = "Little Fire"; }
    [pscustomobject]@{ World = 1; Sausages = 2; Name = "Merchant's Elegy"; }
    [pscustomobject]@{ World = 1; Sausages = 2; Name = "Seafinger"; }
    [pscustomobject]@{ World = 1; Sausages = 2; Name = "Southjaunt"; }
    [pscustomobject]@{ World = 1; Sausages = 3; Name = "Lachrymose Head"; }
    [pscustomobject]@{ World = 1; Sausages = 3; Name = "The Anchorage"; }
    [pscustomobject]@{ World = 1; Sausages = 3; Name = "The Clover"; }
    [pscustomobject]@{ World = 1; Sausages = -1; Name = "World 1 route"; }
    [pscustomobject]@{ World = 2; Sausages = 1; Name = "Emerson Jetty"; }
    [pscustomobject]@{ World = 2; Sausages = 1; Name = "Fallow Earth"; }
    [pscustomobject]@{ World = 2; Sausages = 1; Name = "Sad Farm"; }
    [pscustomobject]@{ World = 2; Sausages = 2; Name = "Barrow Set"; }
    [pscustomobject]@{ World = 2; Sausages = 2; Name = "Beautiful Horizon"; }
    [pscustomobject]@{ World = 2; Sausages = 2; Name = "Cove"; }
    [pscustomobject]@{ World = 2; Sausages = 2; Name = "Rough Field"; }
    [pscustomobject]@{ World = 2; Sausages = 2; Name = "The Paddock"; }
    [pscustomobject]@{ World = 2; Sausages = 2; Name = "Twisty Farm"; }
    [pscustomobject]@{ World = 2; Sausages = 8; Name = "The Great Tower"; }
    [pscustomobject]@{ World = 2; Sausages = -1; Name = "World 2 route"; }
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
    [pscustomobject]@{ Name = "5-1 The Gorge";         Dem = "5-1.dem";  Sausages = 3 }
    [pscustomobject]@{ Name = "5-2 Widow's Finger";    Dem = "5-2.dem";  Sausages = 3 }
    [pscustomobject]@{ Name = "5-3 Skeleton";          Dem = "5-3.dem";  Sausages = 3 }
    [pscustomobject]@{ Name = "5-4 Slope View";        Dem = "5-4.dem";  Sausages = 3 }
    [pscustomobject]@{ Name = "5-5 Land's End";        Dem = "5-5.dem";  Sausages = 1 }
    [pscustomobject]@{ Name = "5-6 Crater";            Dem = "5-6.dem";  Sausages = 2 }
    [pscustomobject]@{ Name = "5-7 Pressure Points";   Dem = "5-7.dem";  Sausages = 2 }
    [pscustomobject]@{ Name = "5-8 Open Baths";        Dem = "5-8.dem";  Sausages = 3 }
    [pscustomobject]@{ Name = "5-9 Drumlin";           Dem = "5-9.dem";  Sausages = 3 }
    [pscustomobject]@{ Name = "5-10 Tarry Ridge";      Dem = "5-10.dem"; Sausages = 2 }
    [pscustomobject]@{ Name = "5-11 Rough View";       Dem = "5-11.dem"; Sausages = 2 }
    [pscustomobject]@{ Name = "5-12 Baby Rock";        Dem = "5-12.dem"; Sausages = 2 }
)

function Build-Variant {
    param([int] $N)
    $env:_CL_ = "/DLAYERCACHE_ZSTD /DLAYERCACHE_ZSTD_LEVEL=3"
    if ($N -eq -1) {
      $env:_CL_ += " /DOVERWORLD_HACK=1"
      $N = -$N
    }
    $macro = (0..($N-1) | ForEach-Object { "o($_)" }) -join " "
    $env:_CL_ += " /DSAUSAGES=`"$macro`""
    # Rebuild since changing the _CL_ macro won't otherwise trigger a rebuild.
    & $msbuild SSRBruteForce.vcxproj /p:Configuration=$Configuration /p:Platform=x64 /p:PlatformToolset=v143 /v:minimal /m /t:Rebuild
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed for SAUSAGES=$N"
    }
}

# Candidate levels, optionally narrowed by -World and/or -TestName (substring match on the display name).
$candidates = $levelDemos
if ($World)    { $candidates = $candidates | Where-Object { $_.World -eq $World } }
if ($TestName) { $candidates = $candidates | Where-Object { $_.Name -like "*$TestName*" } }
if (@($candidates).Count -eq 0) { throw "No levels matched the given filters." }

# Build each distinct sausage count once (rebuilds are expensive), then replay every candidate under it.
$unified = [ordered]@{}
foreach ($lvl in $candidates) { $unified[$lvl.Name] = "SKIP" }

foreach ($n in ($candidates.Sausages | Sort-Object -Unique)) {
    echo "Building with $n sausages"
    Build-Variant -N $n
    echo "Running with $n sausages"
    foreach ($lvl in $candidates) {
        if ($lvl.Sausages -ne $n) { continue }

        # TODO: This is becoming a mess.
        if ($Solve) {
            echo "Solving $($lvl.Name)"
            if ($TestName) {
                & $exe $lvl.Name
            } else {
                & $exe $lvl.Name *>> $null
            }
            Copy-Item "solved.dem" "$($lvl.Name).dem"
        } else {
            echo "Testing $($lvl.Name)"
            if (-not $DemoOverride) {
                $dem = if ($lvl.Dem) { $lvl.Dem } else { "$($lvl.Name).dem" }
                $path = "../SSRDecompile/App/" + $dem
            } else {
                $path = $DemoOverride
            }
            if ($DebugMode) {
                $path = "../../" + $path
            }
            if ($TestName) {
                if ($DebugMode) {
                    & $devenv /debugexe $exe $lvl.Name $path
                } else {
                    & $exe $lvl.Name $path
                }
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
