param(
    [string]$CatalogPath = "docs/cemu-skylanders-catalog.json",
    [string]$SourceRoot = "assets/characters/renders/_review/Downloads",
    [string]$OutputRoot = "assets/characters/renders/_review",
    [string]$ManifestPath = "docs/local-render-assignment-manifest.json",
    [switch]$DryRun
)

$ErrorActionPreference = "Stop"

$gameFolderMap = @{
    "spyrosadventure" = "SpyrosAdventure"
    "spyros_adventure" = "SpyrosAdventure"
    "spyroadventure" = "SpyrosAdventure"
    "spyro_s_adventure" = "SpyrosAdventure"
    "giants" = "Giants"
    "swapforce" = "SwapForce"
    "swap_force" = "SwapForce"
    "trapteam" = "TrapTeam"
    "trap_team" = "TrapTeam"
    "superchargers" = "SuperChargers"
    "super_chargers" = "SuperChargers"
    "imaginators" = "Imaginators"
}

function Normalize-Key([string]$Value) {
    $normalized = $Value -replace "(?i)(actual|final|temp)$", ""
    $normalized = $normalized.ToLowerInvariant()
    $normalized = $normalized -replace "eon's", "eons"
    $normalized = $normalized -replace "dr\.", "dr"
    $normalized = $normalized -replace "skylanders?", ""
    $normalized = $normalized -replace "[-_]", " "
    $normalized = $normalized -replace "\b(actual|final|temp|transparent|render|figure|pose|large|small)\b", " "
    $normalized = $normalized -replace "\s+", " "
    $normalized = $normalized.Trim()
    $normalized = $normalized -replace "[^a-z0-9]+", ""
    $normalized = $normalized -replace "\d+$", ""
    return $normalized
}

function Get-Slug([string]$Value) {
    $slug = $Value.ToLowerInvariant()
    $slug = $slug -replace "eon's", "eons"
    $slug = $slug -replace "dr\.", "dr"
    $slug = $slug -replace "(\(|\))", " "
    $slug = $slug -replace "[^a-z0-9]+", "_"
    $slug = $slug -replace "^_+|_+$", ""
    return $slug
}

function Entry-Aliases($Entry) {
    $aliases = New-Object System.Collections.Generic.List[string]
    foreach ($value in @($Entry.assetKey, $Entry.displayName, $Entry.cemuName, $Entry.sclSearchName)) {
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            $aliases.Add((Normalize-Key $value))
        }
    }

    if (-not [string]::IsNullOrWhiteSpace($Entry.baseName) -and -not [string]::IsNullOrWhiteSpace($Entry.variantName)) {
        $aliases.Add((Normalize-Key "$($Entry.variantName) $($Entry.baseName)"))
        $aliases.Add((Normalize-Key "$($Entry.baseName) $($Entry.variantName)"))
    }

    if ($Entry.displayName -match "^Series\s+\d+\s+(.+)$") {
        $aliases.Add((Normalize-Key $Matches[1]))
    }

    return @($aliases | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Select-Object -Unique)
}

function Source-Aliases($File, [string]$SourceRootPath) {
    $aliases = New-Object System.Collections.Generic.List[string]
    $stem = [System.IO.Path]::GetFileNameWithoutExtension($File.Name)
    $stemAlias = Normalize-Key $stem
    $aliases.Add($stemAlias)

    $relativeDir = $File.Directory.FullName.Substring($SourceRootPath.Length).TrimStart("\", "/")
    $parts = @($relativeDir -split "[\\/]" | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
    $variantParts = @()
    if ($parts.Count -gt 1) {
        $variantParts = @($parts[1..($parts.Count - 1)])
    }

    foreach ($part in $variantParts) {
        $aliases.Add((Normalize-Key "$part $stemAlias"))
        $aliases.Add((Normalize-Key "$stemAlias $part"))
    }

    if ($variantParts.Count -gt 0) {
        $variantText = ($variantParts -join " ")
        $aliases.Add((Normalize-Key "$variantText $stemAlias"))
        $aliases.Add((Normalize-Key "$stemAlias $variantText"))
    }

    return @($aliases | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Select-Object -Unique)
}

function Source-VariantParts($File, [string]$SourceRootPath) {
    $relativeDir = $File.Directory.FullName.Substring($SourceRootPath.Length).TrimStart("\", "/")
    $parts = @($relativeDir -split "[\\/]" | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
    if ($parts.Count -le 1) { return @() }
    return @($parts[1..($parts.Count - 1)])
}

function Source-Game($File, [string]$SourceRootPath) {
    $relativeDir = $File.Directory.FullName.Substring($SourceRootPath.Length).TrimStart("\", "/")
    $firstPart = @($relativeDir -split "[\\/]" | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Select-Object -First 1)
    if ($firstPart.Count -eq 0) { return $null }
    $key = Get-Slug $firstPart[0]
    if ($gameFolderMap.ContainsKey($key)) {
        return $gameFolderMap[$key]
    }
    return $null
}

function Score-Match($Entry, [string[]]$SourceAliases, [string[]]$SourceVariantParts, [string]$SourceGame) {
    $score = 0
    $assetAlias = Normalize-Key $Entry.assetKey
    $displayAlias = Normalize-Key $Entry.displayName
    $variantBaseAlias = ""
    $baseVariantAlias = ""
    if (-not [string]::IsNullOrWhiteSpace($Entry.baseName) -and -not [string]::IsNullOrWhiteSpace($Entry.variantName)) {
        $variantBaseAlias = Normalize-Key "$($Entry.variantName) $($Entry.baseName)"
        $baseVariantAlias = Normalize-Key "$($Entry.baseName) $($Entry.variantName)"
    }

    if ($SourceAliases -contains $assetAlias) { $score += 30 }
    if ($SourceAliases -contains $displayAlias) { $score += 25 }
    if ($variantBaseAlias -and ($SourceAliases -contains $variantBaseAlias)) { $score += 18 }
    if ($baseVariantAlias -and ($SourceAliases -contains $baseVariantAlias)) { $score += 18 }
    if ($SourceGame -and $Entry.releaseGame -eq $SourceGame) { $score += 6 }

    foreach ($part in $SourceVariantParts) {
        $partAlias = Normalize-Key $part
        $variantAlias = Normalize-Key $Entry.variantName
        $roleAlias = Normalize-Key $Entry.role
        if ($variantAlias -and $partAlias -eq $variantAlias) { $score += 12 }
        if ($roleAlias -and $partAlias -eq $roleAlias) { $score += 8 }
        if ($partAlias -eq "minis" -and $Entry.role -eq "Mini") { $score += 8 }
        if ($partAlias -eq "traps" -and $Entry.toyKind -eq "Trap") { $score += 8 }
    }

    return $score
}

function Relative-Path([string]$Path) {
    return ($Path -replace "\\", "/")
}

$catalog = Get-Content -Raw -Path $CatalogPath | ConvertFrom-Json
$entries = @($catalog.entries)
if ($entries.Count -eq 0) {
    throw "Catalog contains no entries: $CatalogPath"
}
if (-not (Test-Path $SourceRoot)) {
    $manifest = [ordered]@{
        catalogSource = $CatalogPath
        sourceRoot = $SourceRoot
        outputRoot = $OutputRoot
        generatedAt = (Get-Date).ToUniversalTime().ToString("o")
        dryRun = [bool]$DryRun
        scannedCount = 0
        assignedCount = 0
        ambiguousCount = 0
        unmatchedCount = 0
        entries = @()
        note = "Source folder not found; no local render inbox files to assign."
    }
    if (-not $DryRun) {
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $ManifestPath) | Out-Null
        ($manifest | ConvertTo-Json -Depth 12) | Set-Content -Encoding UTF8 -Path $ManifestPath
    }
    Write-Host "Source folder not found: $SourceRoot"
    Write-Host "No local render inbox files to assign."
    if (-not $DryRun) { Write-Host "Wrote manifest: $ManifestPath" }
    return
}

$sourceRootPath = (Resolve-Path $SourceRoot).Path.TrimEnd("\", "/")
$aliasIndex = @{}
foreach ($entry in $entries) {
    foreach ($alias in @(Entry-Aliases $entry)) {
        if (-not $aliasIndex.ContainsKey($alias)) {
            $aliasIndex[$alias] = New-Object System.Collections.Generic.List[object]
        }
        $aliasIndex[$alias].Add($entry)
    }
}

$files = Get-ChildItem -Path $SourceRoot -Recurse -File |
    Where-Object { $_.Extension.ToLowerInvariant() -in @(".png", ".jpg", ".jpeg", ".webp") }

$manifestEntries = @()
foreach ($file in $files) {
    $sourceAliases = @(Source-Aliases $file $sourceRootPath)
    $sourceVariantParts = @(Source-VariantParts $file $sourceRootPath)
    $sourceGame = Source-Game $file $sourceRootPath
    $allMatches = New-Object System.Collections.Generic.List[object]

    foreach ($alias in $sourceAliases) {
        if (-not $aliasIndex.ContainsKey($alias)) { continue }
        foreach ($entry in $aliasIndex[$alias]) {
            if (-not $allMatches.Contains($entry)) {
                $allMatches.Add($entry)
            }
        }
    }

    $matches = New-Object System.Collections.Generic.List[object]
    if ($sourceGame) {
        foreach ($entry in $allMatches) {
            if ($entry.releaseGame -eq $sourceGame) {
                $matches.Add($entry)
            }
        }
    }
    if ($matches.Count -eq 0) {
        foreach ($entry in $allMatches) {
            $matches.Add($entry)
        }
    }

    if ($matches.Count -gt 1) {
        $scored = @($matches | ForEach-Object {
            [pscustomobject]@{
                entry = $_
                score = Score-Match $_ $sourceAliases $sourceVariantParts $sourceGame
            }
        } | Sort-Object score -Descending)
        $topScore = $scored[0].score
        $topMatches = @($scored | Where-Object { $_.score -eq $topScore -and $_.score -gt 0 } | ForEach-Object { $_.entry })
        if ($topMatches.Count -gt 0 -and $topMatches.Count -lt $matches.Count) {
            $matches = New-Object System.Collections.Generic.List[object]
            foreach ($entry in $topMatches) {
                $matches.Add($entry)
            }
        }
    }

    $status = "unmatched"
    $entry = $null
    $targetPath = $null
    $sourceHash = (Get-FileHash -Algorithm SHA256 -Path $file.FullName).Hash.ToLowerInvariant()
    $targetHash = $null
    $matchedAliases = @($sourceAliases | Where-Object { $aliasIndex.ContainsKey($_) })

    if ($matches.Count -eq 1) {
        $entry = $matches[0]
        $gameFolder = Join-Path $OutputRoot $entry.releaseGame
        $targetPath = Join-Path $gameFolder "$($entry.assetKey)$($file.Extension.ToLowerInvariant())"
        $status = "assigned"

        if (-not $DryRun) {
            New-Item -ItemType Directory -Force -Path $gameFolder | Out-Null
            if (Test-Path $targetPath) {
                $targetHash = (Get-FileHash -Algorithm SHA256 -Path $targetPath).Hash.ToLowerInvariant()
                if ($targetHash -eq $sourceHash) {
                    $status = "already_present"
                } else {
                    Copy-Item -LiteralPath $file.FullName -Destination $targetPath -Force
                    $status = "replaced_changed_hash"
                    $targetHash = $sourceHash
                }
            } else {
                Copy-Item -LiteralPath $file.FullName -Destination $targetPath
                $targetHash = $sourceHash
            }
        }
    } elseif ($matches.Count -gt 1) {
        $status = "ambiguous"
    }

    $manifestEntries += [pscustomobject]@{
        sourcePath = Relative-Path $file.FullName
        sourceGame = $sourceGame
        sourceAliases = @($sourceAliases)
        status = $status
        assetKey = if ($entry) { $entry.assetKey } else { $null }
        displayName = if ($entry) { $entry.displayName } else { $null }
        releaseGame = if ($entry) { $entry.releaseGame } else { $null }
        targetPath = if ($targetPath) { Relative-Path $targetPath } else { $null }
        sourceSha256 = $sourceHash
        targetSha256 = $targetHash
        matchedAliases = @($matchedAliases)
        ambiguousMatches = @($matches | ForEach-Object { $_.assetKey })
    }
}

$manifest = [ordered]@{
    catalogSource = $CatalogPath
    sourceRoot = $SourceRoot
    outputRoot = $OutputRoot
    generatedAt = (Get-Date).ToUniversalTime().ToString("o")
    dryRun = [bool]$DryRun
    scannedCount = @($files).Count
    assignedCount = @($manifestEntries | Where-Object { $_.status -in @("assigned", "already_present", "replaced_changed_hash") }).Count
    ambiguousCount = @($manifestEntries | Where-Object { $_.status -eq "ambiguous" }).Count
    unmatchedCount = @($manifestEntries | Where-Object { $_.status -eq "unmatched" }).Count
    entries = @($manifestEntries)
}

if (-not $DryRun) {
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $ManifestPath) | Out-Null
    ($manifest | ConvertTo-Json -Depth 12) | Set-Content -Encoding UTF8 -Path $ManifestPath
}

Write-Host "Scanned render files: $($manifest.scannedCount)"
Write-Host "Assigned render files: $($manifest.assignedCount)"
Write-Host "Ambiguous render files: $($manifest.ambiguousCount)"
Write-Host "Unmatched render files: $($manifest.unmatchedCount)"
if ($DryRun) {
    Write-Host "Dry run did not write manifest."
} else {
    Write-Host "Wrote manifest: $ManifestPath"
}
