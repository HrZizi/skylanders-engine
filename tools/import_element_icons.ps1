param(
    [string]$WikiApiUrl = "https://skylanders.fandom.com/api.php",
    [string]$OutputRoot = "assets/elements/_review",
    [string]$ManifestPath = "docs/element-icon-manifest.json"
)

$ErrorActionPreference = "Stop"
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

$elements = @(
    @{ key = "air"; title = "File:AirSymbolSkylanders.png" },
    @{ key = "dark"; title = "File:DarkSymbolSkylanders.png" },
    @{ key = "earth"; title = "File:EarthSymbolSkylanders.png" },
    @{ key = "fire"; title = "File:FireSymbolSkylanders.png" },
    @{ key = "life"; title = "File:LifeSymbolSkylanders.png" },
    @{ key = "light"; title = "File:LightSymbolSkylanders.png" },
    @{ key = "magic"; title = "File:MagicSymbolSkylanders.png" },
    @{ key = "tech"; title = "File:TechSymbolSkylanders.png" },
    @{ key = "undead"; title = "File:UndeadSymbolSkylanders.png" },
    @{ key = "water"; title = "File:WaterSymbolSkylanders.png" },
    @{ key = "kaos"; title = "File:Kaos symbol.png" }
)

function Get-ImageExtension([string]$Url) {
    $path = ([Uri]$Url).AbsolutePath
    $extension = [System.IO.Path]::GetExtension($path)
    if ([string]::IsNullOrWhiteSpace($extension)) {
        return ".png"
    }
    $extension = $extension.ToLowerInvariant()
    if (@(".jpg", ".jpeg", ".png", ".webp") -contains $extension) {
        return $extension
    }
    return ".png"
}

function Resolve-WikiImage($WebClient, [string]$ApiUrl, [string]$FileTitle) {
    $encodedTitle = [Uri]::EscapeDataString($FileTitle)
    $url = "${ApiUrl}?action=query&titles=$encodedTitle&prop=imageinfo&iiprop=url&format=json&origin=*"
    $json = $WebClient.DownloadString($url) | ConvertFrom-Json
    foreach ($page in $json.query.pages.PSObject.Properties) {
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

New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $ManifestPath) | Out-Null

$web = New-Object System.Net.WebClient
$web.Headers.Add("User-Agent", "Mozilla/5.0 SkylandersPortElementIconReview/1.0")
$manifestEntries = @()

foreach ($element in $elements) {
    $resolved = Resolve-WikiImage $web $WikiApiUrl $element.title
    $status = "no_exact_icon"
    $imageUrl = $null
    $localPath = $null
    $hash = $null
    $resolvedTitle = $null
    $descriptionUrl = $null

    if ($resolved) {
        $imageUrl = $resolved.url
        $resolvedTitle = $resolved.title
        $descriptionUrl = $resolved.descriptionUrl
        $extension = Get-ImageExtension $imageUrl
        $outPath = Join-Path $OutputRoot "$($element.key)$extension"
        try {
            $web.DownloadFile($imageUrl, $outPath)
            $hash = (Get-FileHash -Algorithm SHA256 -Path $outPath).Hash.ToLowerInvariant()
            $localPath = ($outPath -replace "\\", "/")
            $status = "candidate_downloaded"
            Write-Host "Downloaded $($element.key): $resolvedTitle"
        } catch {
            $status = "download_failed"
            Write-Warning "Failed $($element.key): $($_.Exception.Message)"
        }
    }

    $manifestEntries += [pscustomobject]@{
        element = $element.key
        requestedTitle = $element.title
        resolvedTitle = $resolvedTitle
        status = $status
        imageUrl = $imageUrl
        descriptionUrl = $descriptionUrl
        localPath = $localPath
        sha256 = $hash
    }
}

$manifest = [ordered]@{
    source = "https://skylanders.fandom.com/wiki/Elements"
    api = $WikiApiUrl
    downloadedAt = (Get-Date).ToUniversalTime().ToString("o")
    entries = @($manifestEntries)
}

($manifest | ConvertTo-Json -Depth 10) | Set-Content -Encoding UTF8 -Path $ManifestPath

Write-Host "Matched element icons: $(($manifestEntries | Where-Object { $_.status -eq 'candidate_downloaded' }).Count) / $($manifestEntries.Count)"
Write-Host "Wrote manifest: $ManifestPath"
