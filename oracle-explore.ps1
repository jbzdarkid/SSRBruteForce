# Engine-divergence hunt, demos as the ONLY shared layer.
#
# For each level:
#   1. Build the C++ Level2 engine for that level's sausage count.
#   2. Level2 grows a single deep RRT-style tree from the start (frontier-biased expansion, novelty-gated by an
#      occupancy grid) and writes death-free LEAF demos into oracle-demos\<safe>\. A leaf's move path traverses all
#      its ancestors, so the leaves alone cover the whole tree -- few long demos instead of many short ones.
#   3. The C# oracle (real game, loaded from the extracted merged_binary blob) bulk-replays every one of those .dem files.
#      Level2 thought each was safe; any the game reports LOST -- or that ends in a different final state -- is a
#      genuine divergence. The offending .dem is its repro.
#
# Neither engine reads the other's state -- they only agree on the .dem move strings.
#
# Usage:
#   .\oracle-explore.ps1                       # build + hunt every level
#   .\oracle-explore.ps1 -TestName "3-8"       # only levels whose name contains this substring
[CmdletBinding()]
param(
    [string] $TestName = ""
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

# RRT search budget (hard-coded; raise $Iterations for a deeper tree -- RRT is cheap).
$Iterations = 100000
$RolloutLen = 40
$Bin        = 3
$Seed       = 0xC0FFEE

# Build the C# oracle (net8.0 SDK project) up front so it's always current with Oracle\*.cs.
echo "Building oracle"
dotnet build ".\Oracle\Oracle.csproj" -c Release --nologo -v quiet
if ($LASTEXITCODE -ne 0) { throw "Oracle build failed" }

# All levels with their sausage count (needed only to pick the C++ build; the oracle reads any level from its dat).
$levels = @(
    [pscustomobject]@{ Name = "1-1 Lachrymose Head";   Sausages = 3 }
    [pscustomobject]@{ Name = "1-2 Southjaunt";        Sausages = 2 }
    [pscustomobject]@{ Name = "1-3 Infant's Break";    Sausages = 2 }
    [pscustomobject]@{ Name = "1-4 Comely Hearth";     Sausages = 2 }
    [pscustomobject]@{ Name = "1-5 Little Fire";       Sausages = 2 }
    [pscustomobject]@{ Name = "1-6 Eastreach";         Sausages = 2 }
    [pscustomobject]@{ Name = "1-7 Bay's Neck";        Sausages = 1 }
    [pscustomobject]@{ Name = "1-8 Burning Wharf";     Sausages = 2 }
    [pscustomobject]@{ Name = "1-9 Happy Pool";        Sausages = 1 }
    [pscustomobject]@{ Name = "1-10 Maiden's Walk";    Sausages = 1 }
    [pscustomobject]@{ Name = "1-11 Fiery Jut";        Sausages = 2 }
    [pscustomobject]@{ Name = "1-12 Merchant's Elegy"; Sausages = 2 }
    [pscustomobject]@{ Name = "1-13 Seafinger";        Sausages = 2 }
    [pscustomobject]@{ Name = "1-14 The Clover";       Sausages = 3 }
    [pscustomobject]@{ Name = "1-15 Inlet Shore";      Sausages = 2 }
    [pscustomobject]@{ Name = "1-16 The Anchorage";    Sausages = 3 }
    [pscustomobject]@{ Name = "2-1 Emerson Jetty";     Sausages = 1 }
    [pscustomobject]@{ Name = "2-2 Sad Farm";          Sausages = 1 }
    [pscustomobject]@{ Name = "2-3 Cove";              Sausages = 2 }
    [pscustomobject]@{ Name = "2-5 The Paddock";       Sausages = 2 }
    [pscustomobject]@{ Name = "2-6 Beautiful Horizon"; Sausages = 2 }
    [pscustomobject]@{ Name = "2-7 Barrow Set";        Sausages = 2 }
    [pscustomobject]@{ Name = "2-8 Rough Field";       Sausages = 2 }
    [pscustomobject]@{ Name = "2-9 Fallow Earth";      Sausages = 1 }
    [pscustomobject]@{ Name = "2-10 Twisty Farm";      Sausages = 2 }
    [pscustomobject]@{ Name = "3-1 Cold Jag";          Sausages = 3 }
    [pscustomobject]@{ Name = "3-2 Cold Finger";       Sausages = 3 }
    [pscustomobject]@{ Name = "3-3 Cold Escarpment";   Sausages = 2 }
    [pscustomobject]@{ Name = "3-4 Cold Trail";        Sausages = 3 }
    [pscustomobject]@{ Name = "3-5 Cold Cliff";        Sausages = 3 }
    [pscustomobject]@{ Name = "3-6 Cold Pit";          Sausages = 2 }
    [pscustomobject]@{ Name = "3-7 Cold Plateau";      Sausages = 2 }
    [pscustomobject]@{ Name = "3-8 Cold Head";         Sausages = 2 }
    [pscustomobject]@{ Name = "3-9 Cold Ladder";       Sausages = 3 }
    [pscustomobject]@{ Name = "3-10 Cold Sausage";     Sausages = 5 }
    [pscustomobject]@{ Name = "3-11 Cold Terrace";     Sausages = 3 }
    [pscustomobject]@{ Name = "3-12 Cold Horizon";     Sausages = 2 }
    [pscustomobject]@{ Name = "3-13 Cold Gate";        Sausages = 7 }
    [pscustomobject]@{ Name = "3-14 Cold Frustration"; Sausages = 3 }
    [pscustomobject]@{ Name = "4-1 Wretch's Retreat";  Sausages = 2 }
    [pscustomobject]@{ Name = "4-2 Toad's Folly";      Sausages = 3 }
    [pscustomobject]@{ Name = "4-3 Sludge Coast";      Sausages = 2 }
    [pscustomobject]@{ Name = "4-4 Foul Fen";          Sausages = 1 }
    [pscustomobject]@{ Name = "4-5 Crunchy Leaves";    Sausages = 3 }
    [pscustomobject]@{ Name = "4-6 Gator Paddock";     Sausages = 1 }
    [pscustomobject]@{ Name = "5-1 The Gorge";         Sausages = 3 }
    [pscustomobject]@{ Name = "5-2 Widow's Finger";    Sausages = 3 }
    [pscustomobject]@{ Name = "5-3 Skeleton";          Sausages = 3 }
    [pscustomobject]@{ Name = "5-4 Slope View";        Sausages = 3 }
    [pscustomobject]@{ Name = "5-5 Land's End";        Sausages = 1 }
    [pscustomobject]@{ Name = "5-6 Crater";            Sausages = 2 }
    [pscustomobject]@{ Name = "5-7 Pressure Points";   Sausages = 2 }
    [pscustomobject]@{ Name = "5-8 Open Baths";        Sausages = 3 }
    [pscustomobject]@{ Name = "5-9 Drumlin";           Sausages = 3 }
    [pscustomobject]@{ Name = "5-10 Tarry Ridge";      Sausages = 2 }
    [pscustomobject]@{ Name = "5-11 Rough View";       Sausages = 2 }
    [pscustomobject]@{ Name = "5-12 Baby Rock";        Sausages = 2 }
)

function Build-Variant {
    param([int] $N)
    $macro = (0..($N-1) | ForEach-Object { "o($_)" }) -join " "
    $env:_CL_ = "/DSAUSAGES=`"$macro`""
    # Rebuild (not Build) avoids LNK1257 from stale PGO objects across SAUSAGES changes.
    & $msbuild SSRBruteForce.vcxproj /p:Configuration=$Configuration /p:Platform=x64 /p:PlatformToolset=v143 /v:minimal /m /t:Rebuild *> $null
    if ($LASTEXITCODE -ne 0) { throw "Build failed for SAUSAGES=$N" }
}

# Candidate levels, optionally narrowed by -TestName (substring match on the display name).
$candidates = $levels
if ($TestName) { $candidates = $candidates | Where-Object { $_.Name -like "*$TestName*" } }
if (@($candidates).Count -eq 0) { throw "No level name contains $TestName." }

# Build each distinct sausage count once (rebuilds are expensive), then hunt every candidate under it.
$results = [ordered]@{}
foreach ($n in ($candidates.Sausages | Sort-Object -Unique)) {
    echo "Building with $n sausages"
    Build-Variant -N $n
    echo "Exploring with $n sausages"
    foreach ($lvl in $candidates) {
        if ($lvl.Sausages -ne $n) { continue }
        $name = $lvl.Name
        $safe = ($name.ToCharArray() | ForEach-Object { if ($_ -match '[a-zA-Z0-9]') { $_ } else { '_' } }) -join ''
        $folder = ".\oracle-demos\$safe"

        # Level2 grows one RRT tree and writes leaf demos into $folder (clearing any prior ones).
        & $exe $name rrt $Iterations $RolloutLen $Bin $Seed *> $null
        $demoCount = (Get-ChildItem $folder -Filter *.dem -EA SilentlyContinue | Measure-Object).Count
        if ($demoCount -eq 0) { $results[$name] = @{ Lost = 0; Posdiff = 0; Reasons = ""; Repro = "" }; continue }

        # The oracle prints one "Reason: <reason>; Count: <n>; Sample: <path>" line per outcome: empty reason = agree,
        # "Final state" = a position divergence, anything else (Lost/Burned/Drowned/Fork Lost) = a loss.
        $lost = 0; $posdiff = 0; $reasons = ""; $repro = ""
        $out = & $oracle $name $folder
        foreach ($m in ($out | Select-String "^Reason: (.*); Count: (\d+)(?:; Sample: (.*))?$")) {
            $r = $m.Matches[0].Groups[1].Value; $c = [int]$m.Matches[0].Groups[2].Value; $s = $m.Matches[0].Groups[3].Value
            if     ($r -eq "")            { continue }
            elseif ($r -eq "Final state") { $posdiff += $c }
            else                          { $lost += $c; $reasons += "$($r -replace ' ','')=$c " }
            if (-not $repro -and $s) {
                $reproDir = ".\oracle-demos\_repros"; New-Item -ItemType Directory -Force -Path $reproDir | Out-Null
                $repro = "$reproDir\${safe}_$(Split-Path $s -Leaf)"
                Copy-Item -LiteralPath $s -Destination $repro -Force -EA SilentlyContinue
            }
        }
        $reasons = $reasons.Trim()
        $results[$name] = @{ Lost = $lost; Posdiff = $posdiff; Reasons = $reasons; Repro = $repro }

        $col = if (($lost + $posdiff) -gt 0) { "Red" } else { "Green" }
        Write-Host ("{0,-22} lost={1} [{2}] posdiff={3}" -f $name, $lost, $reasons, $posdiff) -ForegroundColor $col
    }
}

echo "=== Divergence summary (Level2 accepted the move; game disagrees) ==="
$any = $false
foreach ($k in $results.Keys) {
    $r = $results[$k]
    if (($r.Lost + $r.Posdiff) -gt 0) {
        $any = $true
        Write-Host ("  {0,-24} lost={1} [{2}] posdiff={3}" -f $k, $r.Lost, $r.Reasons, $r.Posdiff) -ForegroundColor Red
        if ($r.Repro) { Write-Host ("    repro: {0}" -f $r.Repro) -ForegroundColor DarkYellow }
    }
}
if (-not $any) { Write-Host "  No divergences." -ForegroundColor Green }
if ($any) { exit 1 }
