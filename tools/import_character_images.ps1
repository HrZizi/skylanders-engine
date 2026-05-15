param(
    [string]$CatalogPath = "docs/cemu-skylanders-catalog.json",
    [string]$OutputRoot = "assets/characters/renders/_review",
    [string]$ManifestPath = "docs/character-image-manifest.json",
    [int]$MaxCandidatesPerEntry = 3,
    [switch]$DryRun
)

$ErrorActionPreference = "Stop"
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

function Normalize-Text([string]$Value) {
    if ([string]::IsNullOrWhiteSpace($Value)) { return "" }
    return ([regex]::Replace([System.Net.WebUtility]::HtmlDecode($Value).ToLowerInvariant(), "[^a-z0-9]+", " ")).Trim()
}

function Resolve-Url([string]$BaseUrl, [string]$MaybeRelative) {
    if ([string]::IsNullOrWhiteSpace($MaybeRelative)) { return $null }
    if ($MaybeRelative.StartsWith("//")) { return "https:$MaybeRelative" }
    try { return ([Uri]::new([Uri]$BaseUrl, $MaybeRelative)).AbsoluteUri } catch { return $null }
}

function Get-ImageExtension([string]$Url) {
    $path = ([Uri]$Url).AbsolutePath
    $extension = [System.IO.Path]::GetExtension($path).ToLowerInvariant()
    if (@(".jpg", ".jpeg", ".png", ".webp") -contains $extension) { return $extension }
    return ".jpg"
}

function Get-ImageCandidates([string]$PageUrl, [string]$Html) {
    $candidates = @()
    $imageTags = [regex]::Matches($Html, '<img\b[^>]*>', [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
    foreach ($tagMatch in $imageTags) {
        $tag = $tagMatch.Value
        $attrs = @{}
        $attrMatches = [regex]::Matches($tag, '([a-zA-Z0-9_:-]+)\s*=\s*["'']([^"'']+)["'']')
        foreach ($attr in $attrMatches) {
            $attrs[$attr.Groups[1].Value.ToLowerInvariant()] = [System.Net.WebUtility]::HtmlDecode($attr.Groups[2].Value)
        }

        $rawUrl = $attrs["data-src"]
        if ([string]::IsNullOrWhiteSpace($rawUrl)) { $rawUrl = $attrs["data-lazy-src"] }
        if ([string]::IsNullOrWhiteSpace($rawUrl)) { $rawUrl = $attrs["src"] }
        $imageUrl = Resolve-Url $PageUrl $rawUrl
        if ($null -eq $imageUrl) { continue }

        $label = @($attrs["alt"], $attrs["title"], $imageUrl) -join " "
        $candidates += [pscustomobject]@{
            imageUrl = $imageUrl
            label = $label
            alt = $attrs["alt"]
            title = $attrs["title"]
        }
    }
    return $candidates
}

function Get-CandidateScore($Entry, $Candidate) {
    $needles = @(
        $Entry.sclSearchName,
        $Entry.displayName,
        $Entry.baseName
    ) | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Select-Object -Unique
    $haystack = Normalize-Text $Candidate.label
    if ([string]::IsNullOrWhiteSpace($haystack)) { return 0 }

    $best = 0
    foreach ($needleText in $needles) {
        $needle = Normalize-Text $needleText
        if ([string]::IsNullOrWhiteSpace($needle)) { continue }
        $score = 0
        if ($haystack -eq $needle) { $score += 180 }
        if ($haystack.Contains($needle)) { $score += 110 }
        foreach ($token in $needle.Split(" ", [System.StringSplitOptions]::RemoveEmptyEntries)) {
            if ($token.Length -gt 2 -and $haystack.Contains($token)) { $score += 12 }
        }
        if ($score -gt $best) { $best = $score }
    }
    return $best
}

$catalog = Get-Content -Raw -Path $CatalogPath | ConvertFrom-Json
$entries = @($catalog.entries)
if ($entries.Count -eq 0) {
    throw "Catalog contains no entries: $CatalogPath"
}

if ($DryRun) {
    Write-Host "Dry run OK: $($entries.Count) catalog entries loaded from $CatalogPath"
    return
}

New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $ManifestPath) | Out-Null

$web = New-Object System.Net.WebClient
$web.Headers.Add("User-Agent", "SkylandersPortImageReview/2.0")
$pages = @{}

foreach ($url in ($entries | Select-Object -ExpandProperty sourceUrl -Unique)) {
    if ([string]::IsNullOrWhiteSpace($url)) { continue }
    Write-Host "Fetching $url"
    try {
        $html = $web.DownloadString($url)
        $pages[$url] = @(Get-ImageCandidates $url $html)
    } catch {
        Write-Warning "Failed to fetch ${url}: $($_.Exception.Message)"
        $pages[$url] = @()
    }
}

$manifestEntries = @()
foreach ($entry in $entries) {
    $pageCandidates = @($pages[$entry.sourceUrl])
    $ranked = @(
        $pageCandidates |
            ForEach-Object {
                [pscustomobject]@{
                    candidate = $_
                    score = Get-CandidateScore $entry $_
                }
            } |
            Where-Object { $_.score -gt 0 } |
            Sort-Object -Property score -Descending |
            Select-Object -First $MaxCandidatesPerEntry
    )

    $downloaded = @()
    $candidateIndex = 0
    foreach ($rank in $ranked) {
        $candidateIndex++
        $imageUrl = $rank.candidate.imageUrl
        $extension = Get-ImageExtension $imageUrl
        $gameFolder = Join-Path $OutputRoot $entry.releaseGame
        New-Item -ItemType Directory -Force -Path $gameFolder | Out-Null
        $fileName = if ($candidateIndex -eq 1) { "$($entry.assetKey)$extension" } else { "$($entry.assetKey)-candidate-$candidateIndex$extension" }
        $outPath = Join-Path $gameFolder $fileName

        try {
            $web.DownloadFile($imageUrl, $outPath)
            $hash = (Get-FileHash -Algorithm SHA256 -Path $outPath).Hash.ToLowerInvariant()
            $downloaded += [pscustomobject]@{
                imageUrl = $imageUrl
                localPath = ($outPath -replace "\\", "/")
                sha256 = $hash
                score = $rank.score
                matchType = if ($rank.score -ge 180) { "exact" } elseif ($rank.score -ge 110) { "contains" } else { "token" }
            }
            Write-Host "Downloaded render $($entry.assetKey): $imageUrl"
        } catch {
            $downloaded += [pscustomobject]@{
                imageUrl = $imageUrl
                localPath = $null
                sha256 = $null
                score = $rank.score
                error = $_.Exception.Message
            }
            Write-Warning "Failed render $($entry.assetKey): $($_.Exception.Message)"
        }
    }

    $manifestEntries += [pscustomobject]@{
        id = $entry.id
        assetKey = $entry.assetKey
        displayName = $entry.displayName
        toyKind = $entry.toyKind
        releaseGame = $entry.releaseGame
        skyId = $entry.skyId
        skyVar = $entry.skyVar
        sourceUrl = $entry.sourceUrl
        sclSearchName = $entry.sclSearchName
        status = if ($downloaded.Count -gt 0) { "candidate_downloaded" } else { "no_candidate" }
        candidates = @($downloaded)
    }
}

$manifest = [ordered]@{
    source = "https://skylanderscharacterlist.com/"
    catalogSource = $CatalogPath
    downloadedAt = (Get-Date).ToUniversalTime().ToString("o")
    entries = @($manifestEntries)
}

($manifest | ConvertTo-Json -Depth 12) | Set-Content -Encoding UTF8 -Path $ManifestPath
Write-Host "Render candidates: $(($manifestEntries | Where-Object { $_.status -eq 'candidate_downloaded' }).Count) / $($manifestEntries.Count)"
Write-Host "Wrote manifest: $ManifestPath"
