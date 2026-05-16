param(
    [Parameter(Mandatory = $true)]
    [string]$PackagePath,
    [switch]$SkipNodeCheck
)

$ErrorActionPreference = "Stop"

$ResolvedPackage = Resolve-Path -LiteralPath $PackagePath
$Package = $ResolvedPackage.Path

$RequiredFiles = @(
    "SkylandersEngine.exe",
    "SpyrosAdventure.exe",
    "Giants.exe",
    "SwapForce.exe",
    "TrapTeam.exe",
    "SuperChargers.exe",
    "Imaginators.exe",
    "cemu/Cemu.exe",
    "cemu/resources",
    "cemu/gameProfiles",
    "assets/index.html",
    "assets/app.js",
    "assets/style.css",
    "roms/README.md",
    "keys/README.md",
    "figures/README.md",
    "README.md",
    "docs/V1_6_RELEASE_CHECKLIST.md"
)

$Missing = @()
foreach ($relative in $RequiredFiles) {
    $candidate = Join-Path $Package $relative
    if (-not (Test-Path $candidate)) {
        $Missing += $relative
    }
}

if ($Missing.Count -gt 0) {
    Write-Host "Missing release files:" -ForegroundColor Red
    foreach ($item in $Missing) {
        Write-Host "  $item" -ForegroundColor Red
    }
    exit 1
}

if (-not $SkipNodeCheck) {
    $node = Get-Command node -ErrorAction SilentlyContinue
    if ($node) {
        node --check (Join-Path $Package "assets/app.js")
    } else {
        Write-Host "node was not found; skipping assets/app.js syntax check." -ForegroundColor Yellow
    }
}

Write-Host "Static release smoke check passed." -ForegroundColor Green
Write-Host ""
Write-Host "Manual smoke checklist:"
Write-Host "1. Add a valid ROM to roms/ or launch a stub and choose it in the file picker."
Write-Host "2. Add user-supplied keys.txt to keys/."
Write-Host "3. Launch Giants.exe and confirm bundled Cemu starts without a Cemu picker."
Write-Host "4. Toggle the overlay with F1 and Start+Back."
Write-Host "5. Place a figure, collect a small amount of gold/XP, clear/remove, then place again and verify persistence."
Write-Host "6. Open Controls, confirm device scan results, map one keyboard input and one controller input if available."
Write-Host "7. Run Update Graphic Packs from diagnostics and confirm launch remains non-blocking on failure."
