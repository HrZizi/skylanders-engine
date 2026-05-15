param(
    [string]$CatalogPath = "docs/cemu-skylanders-catalog.json",
    [string]$WikiApiUrl = "https://skylanders.fandom.com/api.php",
    [string[]]$WikiPageTitles = @("Skylanders", "Senseis", "Vehicles", "LightCore_Skylanders"),
    [string]$OutputRoot = "assets/characters/icons/_review",
    [string]$ManifestPath = "docs/wiki-icon-manifest.json",
    [switch]$DryRun
)

$ErrorActionPreference = "Stop"
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

function Get-ImageExtension([string]$Url) {
    $path = ([Uri]$Url).AbsolutePath
    $extension = [System.IO.Path]::GetExtension($path).ToLowerInvariant()
    if (@(".jpg", ".jpeg", ".png", ".webp") -contains $extension) { return $extension }
    return ".png"
}

function Normalize-WikiKey([string]$Value) {
    $key = $Value.ToLowerInvariant()
    $key = $key -replace "^file:", ""
    $key = $key -replace "\.(png|jpg|jpeg|webp|gif)$", ""
    $key = $key -replace "eon's", "eons"
    $key = $key -replace "dr\.", "dr"
    $key = $key -replace "\b(icon|symbol|image|render|figure|skylander|vehicle|sensei)\b", " "
    $key = $key -replace "[^a-z0-9]+", ""
    return $key
}

function Resolve-WikiImage($WebClient, [string]$ApiUrl, [string]$FileTitle) {
    $encodedTitle = [Uri]::EscapeDataString($FileTitle)
    $url = "${ApiUrl}?action=query&titles=$encodedTitle&prop=imageinfo&iiprop=url&format=json&origin=*"
    $json = $WebClient.DownloadString($url) | ConvertFrom-Json
    $pages = $json.query.pages.PSObject.Properties
    foreach ($page in $pages) {
        if ($page.Value.missing) { continue }
        if ($page.Value.imageinfo -and $page.Value.imageinfo.Count -gt 0) {
            return [pscustomobject]@{
                title = $page.Value.title
                url = $page.Value.imageinfo[0].url
                descriptionUrl = $page.Value.imageinfo[0].descriptionurl
            }
        }
    }
    return $null
}

function Get-PageImages($WebClient, [string]$ApiUrl, [string]$PageTitle) {
    $encodedTitle = [Uri]::EscapeDataString($PageTitle)
    $url = "${ApiUrl}?action=parse&page=$encodedTitle&prop=images&format=json&origin=*"
    $json = $WebClient.DownloadString($url) | ConvertFrom-Json
    if (-not $json.parse -or -not $json.parse.images) { return @() }
    return @($json.parse.images | ForEach-Object {
        [pscustomobject]@{
            pageTitle = $PageTitle
            fileTitle = "File:$_"
            normalizedKey = Normalize-WikiKey $_
            isIcon = ($_ -match "(?i)icon")
        }
    })
}

function Candidate-Titles($Entry, $PageImageIndex) {
    $titles = New-Object System.Collections.Generic.List[object]
    foreach ($title in @(
        $Entry.wikiIconTitle,
        "File:$($Entry.displayName) Icon.png",
        "File:$($Entry.displayName) symbol.png",
        "File:$($Entry.baseName) Icon.png",
        "File:$($Entry.baseName) symbol.png"
    )) {
        if ([string]::IsNullOrWhiteSpace($title)) { continue }
        $titles.Add([pscustomobject]@{
            title = $title
            source = "generated"
        })
    }

    $nameKeys = @(
        Normalize-WikiKey $Entry.displayName,
        Normalize-WikiKey $Entry.baseName,
        Normalize-WikiKey $Entry.sclSearchName
    ) | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Select-Object -Unique

    foreach ($key in $nameKeys) {
        if (-not $PageImageIndex.ContainsKey($key)) { continue }
        foreach ($image in @($PageImageIndex[$key] | Sort-Object @{ Expression = "isIcon"; Descending = $true }, fileTitle)) {
            $titles.Add([pscustomobject]@{
                title = $image.fileTitle
                source = "page:$($image.pageTitle)"
            })
        }
    }

    return @($titles | Group-Object title | ForEach-Object { $_.Group[0] })
}

function Relative-Path([string]$Path) {
    return ($Path -replace "\\", "/")
}

$catalog = Get-Content -Raw -Path $CatalogPath | ConvertFrom-Json
$entries = @($catalog.entries)
if ($entries.Count -eq 0) {
    throw "Catalog contains no entries: $CatalogPath"
}

if ($DryRun) {
    Write-Host "Dry run OK: $($entries.Count) catalog entries loaded from $CatalogPath"
    Write-Host "Configured Wiki pages: $($WikiPageTitles -join ', ')"
    return
}

New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $ManifestPath) | Out-Null

$web = New-Object System.Net.WebClient
$web.Headers.Add("User-Agent", "Mozilla/5.0 SkylandersPortWikiIconReview/3.0")

$pageImages = @()
$pageErrors = @()
foreach ($pageTitle in $WikiPageTitles) {
    try {
        $images = @(Get-PageImages $web $WikiApiUrl $pageTitle)
        $pageImages += $images
        Write-Host "Loaded $($images.Count) page images from $pageTitle"
    } catch {
        $pageErrors += "$pageTitle :: $($_.Exception.Message)"
        Write-Warning "Failed to load Wiki page image list ${pageTitle}: $($_.Exception.Message)"
    }
}

$pageImageIndex = @{}
foreach ($image in $pageImages) {
    if ([string]::IsNullOrWhiteSpace($image.normalizedKey)) { continue }
    if (-not $pageImageIndex.ContainsKey($image.normalizedKey)) {
        $pageImageIndex[$image.normalizedKey] = New-Object System.Collections.Generic.List[object]
    }
    $pageImageIndex[$image.normalizedKey].Add($image)
}

$manifestEntries = @()
foreach ($entry in $entries) {
    $requested = @(Candidate-Titles $entry $pageImageIndex)
    $resolved = $null
    $resolvedFrom = $null
    $resolvedSource = $null
    $errors = @()

    foreach ($candidate in $requested) {
        try {
            $resolved = Resolve-WikiImage $web $WikiApiUrl $candidate.title
        } catch {
            $errors += "$($candidate.title) :: $($_.Exception.Message)"
            $resolved = $null
        }
        if ($resolved) {
            $resolvedFrom = $candidate.title
            $resolvedSource = $candidate.source
            break
        }
    }

    $status = "no_icon"
    $localPath = $null
    $hash = $null
    $imageUrl = $null
    $resolvedTitle = $null
    $descriptionUrl = $null

    if ($resolved) {
        $imageUrl = $resolved.url
        $resolvedTitle = $resolved.title
        $descriptionUrl = $resolved.descriptionUrl
        $extension = Get-ImageExtension $imageUrl
        $gameFolder = Join-Path $OutputRoot $entry.releaseGame
        New-Item -ItemType Directory -Force -Path $gameFolder | Out-Null
        $outPath = Join-Path $gameFolder "$($entry.assetKey)$extension"
        try {
            $web.DownloadFile($imageUrl, $outPath)
            $hash = (Get-FileHash -Algorithm SHA256 -Path $outPath).Hash.ToLowerInvariant()
            $localPath = Relative-Path $outPath
            $status = "candidate_downloaded"
            Write-Host "Downloaded icon $($entry.assetKey): $resolvedTitle"
        } catch {
            $status = "download_failed"
            $errors += $_.Exception.Message
            Write-Warning "Failed icon $($entry.assetKey): $($_.Exception.Message)"
        }
    } else {
        Write-Warning "No icon for $($entry.displayName)"
    }

    $manifestEntries += [pscustomobject]@{
        id = $entry.id
        assetKey = $entry.assetKey
        displayName = $entry.displayName
        toyKind = $entry.toyKind
        releaseGame = $entry.releaseGame
        skyId = $entry.skyId
        skyVar = $entry.skyVar
        requestedTitles = @($requested | ForEach-Object { $_.title })
        resolvedFrom = $resolvedFrom
        resolvedSource = $resolvedSource
        resolvedTitle = $resolvedTitle
        status = $status
        imageUrl = $imageUrl
        descriptionUrl = $descriptionUrl
        localPath = $localPath
        sha256 = $hash
        errors = @($errors)
    }
}

$manifest = [ordered]@{
    sourcePages = @($WikiPageTitles | ForEach-Object { "https://skylanders.fandom.com/wiki/$_" })
    api = $WikiApiUrl
    catalogSource = $CatalogPath
    downloadedAt = (Get-Date).ToUniversalTime().ToString("o")
    pageImageCount = @($pageImages).Count
    pageErrors = @($pageErrors)
    entries = @($manifestEntries)
}

($manifest | ConvertTo-Json -Depth 12) | Set-Content -Encoding UTF8 -Path $ManifestPath
Write-Host "Matched icons: $(($manifestEntries | Where-Object { $_.status -eq 'candidate_downloaded' }).Count) / $($manifestEntries.Count)"
Write-Host "Wrote manifest: $ManifestPath"
