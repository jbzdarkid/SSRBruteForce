# Engine-divergence hunt, demos as the ONLY shared layer.
#
# For each level:
#   1. Build the C++ Level2 engine for that level's sausage count.
#   2. Level2 explores from the start and writes a set of distinctive death-free demos (standard .dem files, since
#      Level2 never enters a losing state) into Oracle\demos\<safe>\.
#   3. The C# oracle (real game, loaded from the level's game-native dat) bulk-replays every one of those .dem files.
#      Level2 thought each was safe; any the game reports LOST is a genuine divergence. The offending .dem is its repro.
#
# Neither engine reads the other's state -- they only agree on the .dem move strings.
#
# Usage:
#   .\oracle-explore.ps1                       # World 3 + 4 (the interesting levels), 40000 rollouts each
#   .\oracle-explore.ps1 -TestName "3-8"       # only levels whose name contains this substring
#   .\oracle-explore.ps1 -AllWorlds            # all 45 levels
#   .\oracle-explore.ps1 -Rollouts 80000       # deeper search
#   .\oracle-explore.ps1 -Seeds 1,2,3,4        # shuffle the RNG across several seeds to widen state coverage
[CmdletBinding()]
param(
  [string] $TestName = "",
  [int]    $Rollouts = 40000,
  [int[]]  $Seeds = @(0xC0FFEE),
  [switch] $AllWorlds
)
$ErrorActionPreference = "Stop"
$ws      = $PSScriptRoot
$oracle  = "$ws\Oracle\bin\Release\net8.0\Oracle.exe"
$exe     = "$ws\x64\Release\SSRBruteForce.exe"

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vsPath  = & $vswhere -latest -property installationPath
$msbuild = Join-Path $vsPath "MSBuild\Current\Bin\MSBuild.exe"
if (-not (Test-Path $msbuild)) { throw "No MSBuild found" }

# Build the C# oracle (net8.0 SDK project) up front so it's always current with Oracle\*.cs.
Write-Host "=== Building oracle ===" -ForegroundColor Cyan
dotnet build "$ws\Oracle\Oracle.csproj" -c Release --nologo -v quiet
if ($LASTEXITCODE -ne 0) { throw "Oracle build failed" }

# All levels with their sausage count (needed only to pick the C++ build; the oracle reads any level from its dat).
$allLevels = @(
  @{N="1-1 Lachrymose Head";S=3},  
  @{N="1-2 Southjaunt";S=2},      
  @{N="1-3 Infant's Break";S=2}
  @{N="1-4 Comely Hearth";S=2},    
  @{N="1-5 Little Fire";S=2},     
  @{N="1-6 Eastreach";S=2}
  @{N="1-7 Bay's Neck";S=1},       
  @{N="1-8 Burning Wharf";S=2},   
  @{N="1-9 Happy Pool";S=1}
  @{N="1-10 Maiden's Walk";S=1},   
  @{N="1-11 Fiery Jut";S=2},      
  @{N="1-12 Merchant's Elegy";S=2}
  @{N="1-13 Seafinger";S=2},       
  @{N="1-14 The Clover";S=3},     
  @{N="1-15 Inlet Shore";S=2}
  @{N="1-16 The Anchorage";S=3},   
  @{N="2-1 Emerson Jetty";S=1},   
  @{N="2-2 Sad Farm";S=1}
  @{N="2-3 Cove";S=2},             
  @{N="2-5 The Paddock";S=2},     
  @{N="2-6 Beautiful Horizon";S=2}
  @{N="2-7 Barrow Set";S=2},       
  @{N="2-8 Rough Field";S=2},      
  @{N="2-9 Fallow Earth";S=1}
  @{N="2-10 Twisty Farm";S=2},    
  @{N="3-1 Cold Jag";S=3},        
  @{N="3-2 Cold Finger";S=3}
  @{N="3-3 Cold Escarpment";S=2},  
  @{N="3-4 Cold Trail";S=3}
  @{N="3-5 Cold Cliff";S=3},       
  @{N="3-6 Cold Pit";S=2},        
  @{N="3-7 Cold Plateau";S=2}
  @{N="3-8 Cold Head";S=2},        
  @{N="3-9 Cold Ladder";S=3},     
  @{N="3-10 Cold Sausage";S=5}
  @{N="3-11 Cold Terrace";S=3},    
  @{N="3-12 Cold Horizon";S=2},   
  @{N="3-13 Cold Gate";S=7}
  @{N="3-14 Cold Frustration";S=3}, 
  @{N="4-1 Wretch's Retreat";S=2}, 
  @{N="4-2 Toad's Folly";S=3},    
  @{N="4-3 Sludge Coast";S=2}
  @{N="4-4 Foul Fen";S=1},         
  @{N="4-5 Crunchy Leaves";S=3},  
  @{N="4-6 Gator Paddock";S=1}
)

$candidates = $allLevels
if ($TestName)           { $candidates = $candidates | Where-Object { $_.N -like "*$TestName*" } }
elseif (-not $AllWorlds) { $candidates = $candidates | Where-Object { $_.N -like "3-*" -or $_.N -like "4-*" } }
if (@($candidates).Count -eq 0) { throw "No level matches '$TestName'." }

function Build-Variant([int]$N) {
  $macro = (0..($N-1) | ForEach-Object { "o($_)" }) -join " "
  $env:_CL_ = "/DUSE_LEVEL2 /DSAUSAGES=`"$macro`""
  & $msbuild SSRBruteForce.vcxproj /p:Configuration=Release /p:Platform=x64 /p:PlatformToolset=v143 /v:minimal /m /t:Rebuild *> $null
  if ($LASTEXITCODE -ne 0) { throw "C++ build failed for $N sausages" }
}

$results = [ordered]@{}
foreach ($n in ($candidates | ForEach-Object { $_.S } | Sort-Object -Unique)) {
  Write-Host "=== Building Level2 with $n sausages ===" -ForegroundColor Cyan
  Build-Variant -N $n
  foreach ($lvl in ($candidates | Where-Object { $_.S -eq $n })) {
    $name = $lvl.N
    $safe = ($name.ToCharArray() | ForEach-Object { if ($_ -match '[a-zA-Z0-9]') { $_ } else { '_' } }) -join ''
    $folder = "$ws\oracle-demos\$safe"

    # Sweep every requested seed, aggregating divergences. Each seed regenerates $folder, so replay before the next
    # overwrites it, and copy the first diverging demo aside (into _repros) so it survives later seeds.
    $lost = 0; $posdiff = 0; $reasons = ""; $repro = ""
    foreach ($seed in $Seeds) {
      & $exe $name explore $Rollouts $seed *> $null
      $demoCount = (Get-ChildItem $folder -Filter *.dem -EA SilentlyContinue | Measure-Object).Count
      if ($demoCount -eq 0) { continue }

      $out = & $oracle $name $folder
      # The oracle prints one "Reason: <reason>; Count: <n>; Sample: <path>" line per outcome: empty reason = agree,
      # "Final state" = a position divergence, anything else (Lost/Burned/Drowned/Fork Lost) = a loss.
      foreach ($m in ($out | Select-String "^Reason: (.*); Count: (\d+)(?:; Sample: (.*))?$")) {
        $r = $m.Matches[0].Groups[1].Value; $c = [int]$m.Matches[0].Groups[2].Value; $s = $m.Matches[0].Groups[3].Value
        if     ($r -eq "")            { continue }
        elseif ($r -eq "Final state") { $posdiff += $c }
        else                          { $lost += $c; $reasons += "$($r -replace ' ','')=$c " }
        if (-not $repro -and $s) {
          $reproDir = "$ws\oracle-demos\_repros"; New-Item -ItemType Directory -Force -Path $reproDir | Out-Null
          $repro = "$reproDir\${safe}_seed$('{0:x}' -f $seed)_$(Split-Path $s -Leaf)"
          Copy-Item -LiteralPath $s -Destination $repro -Force -EA SilentlyContinue
        }
      }
    }
    $reasons = $reasons.Trim()
    $results[$name] = @{ Lost = $lost; Posdiff = $posdiff; Reasons = $reasons; Repro = $repro }

    $col = if (($lost + $posdiff) -gt 0) { "Red" } else { "Green" }
    Write-Host ("{0,-22} lost={1} [{2}] posdiff={3}" -f $name, $lost, $reasons, $posdiff) -ForegroundColor $col
  }
}

Write-Host "`n=== Divergence summary (Level2 accepted the move; game disagrees) ===" -ForegroundColor Yellow
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
