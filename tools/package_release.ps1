param(
    [string]$BuildDir = "build-bundled",
    [string]$Configuration = "Release",
    [string]$OutputRoot = "out/releases",
    [string]$PackageName = "SkylandersEngine",
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$BuildPath = Join-Path $RepoRoot $BuildDir
$ReleaseSource = Join-Path $BuildPath $Configuration
$OutputRootPath = Join-Path $RepoRoot $OutputRoot
$Stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$PackagePath = Join-Path $OutputRootPath "$PackageName-$Stamp"

if (-not $SkipBuild) {
    cmake --build $BuildPath --config $Configuration
}

if (-not (Test-Path $ReleaseSource)) {
    throw "Release output was not found: $ReleaseSource"
}

$RequiredFiles = @(
    "SkylandersEngine.exe",
    "SpyrosAdventure.exe",
    "Giants.exe",
    "SwapForce.exe",
    "TrapTeam.exe",
    "SuperChargers.exe",
    "Imaginators.exe",
    "cemu/Cemu.exe",
    "assets/index.html",
    "assets/app.js",
    "assets/style.css"
)

foreach ($relative in $RequiredFiles) {
    $candidate = Join-Path $ReleaseSource $relative
    if (-not (Test-Path $candidate)) {
        throw "Required release file is missing: $candidate"
    }
}

New-Item -ItemType Directory -Force -Path $PackagePath | Out-Null

$RootFiles = Get-ChildItem -LiteralPath $ReleaseSource -File | Where-Object {
    $_.Extension -notin @(".pdb", ".ilk", ".exp", ".lib")
}

foreach ($file in $RootFiles) {
    Copy-Item -LiteralPath $file.FullName -Destination (Join-Path $PackagePath $file.Name) -Force
}

$DirectoriesToCopy = @(
    "assets",
    "cemu"
)

foreach ($directory in $DirectoriesToCopy) {
    $source = Join-Path $ReleaseSource $directory
    if (Test-Path $source) {
        Copy-Item -LiteralPath $source -Destination (Join-Path $PackagePath $directory) -Recurse -Force
    }
}

$DocsPath = Join-Path $PackagePath "docs"
New-Item -ItemType Directory -Force -Path $DocsPath | Out-Null

$DocsToCopy = @(
    "README.md",
    "docs/PROJECT_MAP.md",
    "docs/IMPLEMENTATION_STATUS.md",
    "docs/ConsultantHandoffSummary.md",
    "docs/DOCUMENTATION_MAINTENANCE.md",
    "docs/V1_6_RELEASE_CHECKLIST.md",
    "docs/FIGURE_PERSISTENCE.md",
    "docs/V2_NATIVE_OVERLAY_ROADMAP.md"
)

foreach ($relative in $DocsToCopy) {
    $source = Join-Path $RepoRoot $relative
    if (Test-Path $source) {
        if ($relative -eq "README.md") {
            Copy-Item -LiteralPath $source -Destination (Join-Path $PackagePath "README.md") -Force
        } else {
            Copy-Item -LiteralPath $source -Destination (Join-Path $DocsPath (Split-Path -Leaf $relative)) -Force
        }
    }
}

foreach ($folder in @("roms", "keys", "figures")) {
    $destination = Join-Path $PackagePath $folder
    New-Item -ItemType Directory -Force -Path $destination | Out-Null
    $readme = Join-Path $RepoRoot "$folder/README.md"
    if (Test-Path $readme) {
        Copy-Item -LiteralPath $readme -Destination (Join-Path $destination "README.md") -Force
    }
}

New-Item -ItemType Directory -Force -Path (Join-Path $PackagePath "runtime/config") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $PackagePath "runtime/controllerProfiles") | Out-Null

$Manifest = [ordered]@{
    packageName = $PackageName
    createdAt = (Get-Date).ToUniversalTime().ToString("o")
    sourceBuild = $ReleaseSource
    configuration = $Configuration
    notes = @(
        "ROMs, keys, runtime config, and figures are intentionally empty/user-supplied.",
        "Run tools/smoke_check_release.ps1 against this folder from the source checkout before handing off."
    )
}

$Manifest | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $PackagePath "package_manifest.json") -Encoding UTF8

Write-Host "Packaged release:"
Write-Host $PackagePath
