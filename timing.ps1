# Timing check: replay each level's .dem through BOTH engines (the C++ solver and the C# oracle) and report only REAL
# timing mismatches -- |delta| > 1 with both engines timing the move -- i.e. genuine timing-model errors, not the
# sub-beat backpedal tiebreaker or post-win trailing moves. Builds each sausage count once, then checks every level
# under it. World 1 and World 2 are timing-validated (all green); World 3 is being explored. 2-4 Great Tower has no
# oracle .dat, so it is omitted.
# Usage:
#   .\check.ps1                                  # build + check all levels (Results table of per-level mismatch counts)
#   .\check.ps1 -TestName "2-7 Barrow Set"       # only levels whose name contains this substring (shows full per-move detail)
#   .\check.ps1 -DemoOverride "C:\path\to\a.dem" # custom demo path (use with -TestName for a single level)
[CmdletBinding()]
param(
    [string] $TestName = "",
    [string] $DemoOverride = ""
)

$ErrorActionPreference = "Stop"
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) { throw "No Visual Studio installation found" }

$vsPath = & $vswhere -latest -property installationPath
if (-not $vsPath) { throw "No Visual Studio installation found" }

$msbuild = Join-Path $vsPath "MSBuild\Current\Bin\MSBuild.exe"
if (-not (Test-Path $msbuild)) { throw "Latest installation $vsPath does not have MSBuild.exe" }

$Configuration = "Release"
$exe    = ".\x64\$Configuration\SSRBruteForce.exe"
$oracle = ".\Oracle\bin\Release\net8.0\Oracle.exe"

# name -> (demo file, sausage count). Demo numbering diverges from display order for 2-7/2-8/2-10 (game-encounter order,
# verified by replaying each demo to a win). 2-4 Great Tower has no oracle .dat, so it is omitted.
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
    [pscustomobject]@{ Name = "2-7 Barrow Set";        Dem = "2-10.dem"; Sausages = 2 }
    [pscustomobject]@{ Name = "2-8 Rough Field";       Dem = "2-7.dem";  Sausages = 2 }
    [pscustomobject]@{ Name = "2-9 Fallow Earth";      Dem = "2-9.dem";  Sausages = 1 }
    [pscustomobject]@{ Name = "2-10 Twisty Farm";      Dem = "2-8.dem";  Sausages = 2 }
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
)

function Build-Variant {
    param([int] $N)
    $macro = (0..($N-1) | ForEach-Object { "o($_)" }) -join " "
    $env:_CL_ = "/DSAUSAGES=`"$macro`" /DLAYERCACHE_ZSTD /DLAYERCACHE_ZSTD_LEVEL=3"
    # Rebuild (not Build) avoids LNK1257 from stale PGO objects across SAUSAGES changes.
    & $msbuild SSRBruteForce.vcxproj /p:Configuration=$Configuration /p:Platform=x64 /p:PlatformToolset=v143 /v:minimal /m /t:Rebuild
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed for SAUSAGES=$N"
    }
}

function Get-MoveUnits {
    param([string[]] $lines)
    # Each timed move prints a state row ending in "@t=<move> @T=<cumulative>"; pull the per-move value. The C++ side
    # formats integers with a locale thousands separator (160,000), so strip commas before parsing.
    $units = New-Object System.Collections.Generic.List[long]
    foreach ($line in $lines) {
        $match = [regex]::Match($line, '@t=([\d,]+)')
        if ($match.Success) { $units.Add([long]($match.Groups[1].Value -replace ',', '')) }
    }
    return ,$units
}

# Replay one level+demo through both engines and return the count of REAL timing mismatches -- moves both engines timed
# that differ by more than the sub-beat tiebreaker (|delta| > 1). A one-off delta is the intentional backpedal reward;
# a missing oracle value is a post-win trailing move -- neither is a model error. With -Detail, also print the per-move
# table and the summary line (used when narrowed to specific levels).
function Compare-Timing {
    param([string] $Name, [string] $Demo, [switch] $Detail)
    $cppName     = $Name -replace '^\d+-\d+ ', ''  # world-1/2 C++ levels dropped their numeric prefix; the oracle still needs it
    $cppUnits    = Get-MoveUnits (& $exe    $cppName $Demo timing 2>&1)
    $oracleUnits = Get-MoveUnits (& $oracle $Name    $Demo timing 2>&1)

    $moveCount   = [Math]::Max($cppUnits.Count, $oracleUnits.Count)
    $mismatches  = 0
    $cppTotal    = 0L
    $oracleTotal = 0L
    if ($Detail) { Write-Host ("{0,4}  {1,10}  {2,10}  {3,10}" -f "mv", "oracle", "cpp", "delta") }
    for ($i = 0; $i -lt $moveCount; $i++) {
        $oracleMove = if ($i -lt $oracleUnits.Count) { $oracleUnits[$i] } else { $null }
        $cppMove    = if ($i -lt $cppUnits.Count)    { $cppUnits[$i] }    else { $null }
        if ($null -ne $oracleMove) { $oracleTotal += $oracleMove }
        if ($null -ne $cppMove)    { $cppTotal    += $cppMove }
        $isMismatch = ($null -ne $oracleMove -and $null -ne $cppMove -and [Math]::Abs($cppMove - $oracleMove) -gt 1)
        if ($isMismatch) { $mismatches++ }
        if ($Detail) {
            $delta = if ($null -ne $oracleMove -and $null -ne $cppMove) { $cppMove - $oracleMove } else { $null }
            $flag  = if ($isMismatch) { "  <-- DIFF" } else { "" }
            Write-Host ("{0,4}  {1,10}  {2,10}  {3,10}{4}" -f ($i + 1), $oracleMove, $cppMove, $delta, $flag)
        }
    }
    if ($Detail) {
        Write-Host ""
        Write-Host "moves: oracle=$($oracleUnits.Count) cpp=$($cppUnits.Count)   totals: oracle=$oracleTotal cpp=$cppTotal   REAL mismatches (|delta|>1, both timed): $mismatches"
    }
    return $mismatches
}

# Candidate levels, optionally narrowed by -TestName (substring match on the display name).
$candidates = $levelDemos
if ($TestName) { $candidates = $candidates | Where-Object { $_.Name -like "*$TestName*" } }
if (@($candidates).Count -eq 0) { throw "No level name contains $TestName." }

# Build each distinct sausage count once (rebuilds are expensive), then check every candidate under it.
$unified = [ordered]@{}
foreach ($lvl in $candidates) { $unified[$lvl.Name] = "SKIP" }

foreach ($n in ($candidates.Sausages | Sort-Object -Unique)) {
    echo "Building with $n sausages"
    Build-Variant -N $n
    echo "Checking with $n sausages"
    foreach ($lvl in $candidates) {
        if ($lvl.Sausages -ne $n) { continue }

        echo "Checking $($lvl.Name)"
        if (-not $DemoOverride) {
            $path = "../SSRDecompile/App/" + $lvl.Dem
        } else {
            $path = $DemoOverride
        }

        # Show the full per-move table when narrowed to specific levels, like run-tests shows exe output.
        $unified[$lvl.Name] = Compare-Timing -Name $lvl.Name -Demo $path -Detail:([bool]$TestName)
    }
}

echo "=== Results ==="
$failCount = 0
foreach ($level in $candidates) {
    $status = $unified[$level.Name]
    echo "[$status] $($level.Name)" # REAL-mismatch count per level; 0 is a pass.
    if ($status -ne 0) { $failCount++ }
}
if ($failCount -gt 0) { exit 1 }

