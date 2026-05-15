param(
    [string]$CatalogPath = "docs/cemu-skylanders-catalog.json",
    [string]$RenderRoot = "assets/characters/renders",
    [string]$IconRoot = "assets/characters/icons",
    [string]$JsonReportPath = "docs/supported-skylanders-asset-gaps.json",
    [string]$MarkdownReportPath = "docs/supported-skylanders-asset-gaps.md"
)

$ErrorActionPreference = "Stop"

function Test-Asset([string]$Root, [string]$Game, [string]$AssetKey) {
    foreach ($extension in @("png", "jpg", "jpeg", "webp")) {
        $approved = Join-Path $Root "$Game/$AssetKey.$extension"
        $review = Join-Path $Root "_review/$Game/$AssetKey.$extension"
        if ((Test-Path $approved) -or (Test-Path $review)) {
            return $true
        }
    }
    return $false
}

function Find-BaseVariant($Entry, $Entries) {
    if (-not $Entry.isVariant -or [string]::IsNullOrWhiteSpace($Entry.baseName)) {
        return $null
    }

    $sameKindBaseAndHalf = {
        param($Candidate)
        $Candidate.assetKey -ne $Entry.assetKey -and
            $Candidate.toyKind -eq $Entry.toyKind -and
            $Candidate.baseName -eq $Entry.baseName -and
            -not $Candidate.isVariant -and
            [string]$Candidate.swapHalf -eq [string]$Entry.swapHalf
    }

    $sameGame = @($Entries | Where-Object { (& $sameKindBaseAndHalf $_) -and $_.releaseGame -eq $Entry.releaseGame } | Select-Object -First 1)
    if ($sameGame.Count -gt 0) { return $sameGame[0] }

    $anyGame = @($Entries | Where-Object { & $sameKindBaseAndHalf $_ } | Select-Object -First 1)
    if ($anyGame.Count -gt 0) { return $anyGame[0] }

    return $null
}

function Resolve-AssetStatus([string]$Root, $Entry, $Entries) {
    if (Test-Asset $Root $Entry.releaseGame $Entry.assetKey) {
        return [pscustomobject]@{ found = $true; fallbackAssetKey = $null }
    }

    $fallback = Find-BaseVariant $Entry $Entries
    if ($fallback -and (Test-Asset $Root $fallback.releaseGame $fallback.assetKey)) {
        return [pscustomobject]@{ found = $true; fallbackAssetKey = $fallback.assetKey }
    }

    return [pscustomobject]@{ found = $false; fallbackAssetKey = if ($fallback) { $fallback.assetKey } else { $null } }
}

$catalog = Get-Content -Raw -Path $CatalogPath | ConvertFrom-Json
$entries = @($catalog.entries)
if ($entries.Count -eq 0) {
    throw "Catalog contains no entries: $CatalogPath"
}

$rows = foreach ($entry in $entries) {
    $renderStatus = Resolve-AssetStatus $RenderRoot $entry $entries
    $iconStatus = Resolve-AssetStatus $IconRoot $entry $entries
    $hasRender = $renderStatus.found
    $hasIcon = $iconStatus.found -or $hasRender
    if (-not $hasRender -or -not $hasIcon) {
        [pscustomobject]@{
            assetKey = $entry.assetKey
            displayName = $entry.displayName
            toyKind = $entry.toyKind
            releaseGame = $entry.releaseGame
            skyId = $entry.skyId
            skyVar = $entry.skyVar
            missingRender = -not $hasRender
            missingIcon = -not $hasIcon
            renderFallbackAssetKey = $renderStatus.fallbackAssetKey
            iconFallbackAssetKey = if ($iconStatus.found) { $iconStatus.fallbackAssetKey } elseif ($hasRender) { "render" } else { $iconStatus.fallbackAssetKey }
            sclSearchName = $entry.sclSearchName
            wikiIconTitle = $entry.wikiIconTitle
        }
    }
}

New-Item -ItemType Directory -Force -Path (Split-Path -Parent $JsonReportPath) | Out-Null

$json = [ordered]@{
    catalogSource = $CatalogPath
    generatedAt = (Get-Date).ToUniversalTime().ToString("o")
    totalCatalogEntries = $entries.Count
    missingCount = @($rows).Count
    entries = @($rows)
}
($json | ConvertTo-Json -Depth 8) | Set-Content -Encoding UTF8 -Path $JsonReportPath

$lines = @(
    "# Supported Skylanders Asset Gaps",
    "",
    "Generated from $CatalogPath.",
    "",
    "- Total Cemu-supported entries: $($entries.Count)",
    "- Entries missing at least one asset: $(@($rows).Count)",
    "",
    "| Asset Key | Display Name | Kind | Game | Sky ID | Sky Var | Missing Render | Missing Icon | Render Fallback | Icon Fallback | SCL Search | Wiki Icon Title |",
    "|---|---|---|---|---:|---:|:---:|:---:|---|---|---|---|"
)

foreach ($row in $rows) {
    $lines += "| $($row.assetKey) | $($row.displayName) | $($row.toyKind) | $($row.releaseGame) | $($row.skyId) | $($row.skyVar) | $($row.missingRender) | $($row.missingIcon) | $($row.renderFallbackAssetKey) | $($row.iconFallbackAssetKey) | $($row.sclSearchName) | $($row.wikiIconTitle) |"
}

$lines | Set-Content -Encoding UTF8 -Path $MarkdownReportPath
Write-Host "Missing asset rows: $(@($rows).Count)"
Write-Host "Wrote JSON report: $JsonReportPath"
Write-Host "Wrote Markdown report: $MarkdownReportPath"
