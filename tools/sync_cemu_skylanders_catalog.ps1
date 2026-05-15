param(
    [string]$CemuSource = "extern/cemu/src/Cafe/OS/libs/nsyshid/Skylander.cpp",
    [string]$CatalogJsonPath = "docs/cemu-skylanders-catalog.json",
    [string]$GeneratedIncludePath = "src/GeneratedCharacterCatalog.inc"
)

$ErrorActionPreference = "Stop"

function Normalize-Name([string]$Value) {
    $value = $Value.Trim()
    $value = $value -replace "Dragon.*Peak", "Dragon's Peak"
    $value = $value -replace "Drill Seargeant", "Drill Sergeant"
    $value = $value -replace "Gear Head VVind Up", "Gear Head Wind Up"
    return $value
}

function Get-Slug([string]$Value) {
    $slug = $Value.ToLowerInvariant()
    $slug = $slug -replace "eon's", "eons"
    $slug = $slug -replace "dr\.", "dr"
    $slug = $slug -replace "(\(|\))", " "
    $slug = $slug -replace "[^a-z0-9]+", "_"
    $slug = $slug -replace "^_+|_+$", ""
    if ([string]::IsNullOrWhiteSpace($slug)) {
        return "entry"
    }
    return $slug
}

function Get-ToyKind([int]$SkyId, [string]$Name) {
    if ($SkyId -ge 210 -and $SkyId -le 220) { return "Trap" }
    if ($SkyId -ge 3220 -and $SkyId -le 3241) { return "Vehicle" }
    if ($SkyId -ge 3500 -and $SkyId -le 3503) { return "Trophy" }
    if (($SkyId -ge 200 -and $SkyId -le 209) -or
        ($SkyId -ge 230 -and $SkyId -le 233) -or
        ($SkyId -ge 300 -and $SkyId -le 308) -or
        ($SkyId -ge 3200 -and $SkyId -le 3204) -or
        ($SkyId -ge 3300 -and $SkyId -le 3303)) {
        return "Item"
    }
    return "Skylander"
}

function Get-ReleaseGame([int]$SkyId, [int]$SkyVar, [string]$ToyKind, [string]$Name) {
    if ($ToyKind -eq "Trap") { return "TrapTeam" }
    if ($ToyKind -eq "Vehicle" -or $ToyKind -eq "Trophy") { return "SuperChargers" }
    if ($ToyKind -eq "Item") {
        if ($SkyId -ge 3200) { return "SwapForce" }
        if ($SkyId -ge 305 -and $SkyId -le 308) { return "TrapTeam" }
        if ($SkyId -ge 230 -and $SkyId -le 233) { return "TrapTeam" }
        if ($SkyId -ge 208 -and $SkyId -le 209) { return "Giants" }
        return "SpyrosAdventure"
    }

    switch ($SkyVar) {
        0x1206 { return "Giants" }
        0x1402 { return "Giants" }
        0x1403 { return "Giants" }
        0x1602 { return "Giants" }
        0x1603 { return "Giants" }
        0x1801 { return "Giants" }
        0x1C02 { return "Giants" }
        0x1C03 { return "Giants" }
        0x2206 { return "SwapForce" }
        0x2402 { return "SwapForce" }
        0x2403 { return "SwapForce" }
        0x2404 { return "SwapForce" }
        0x2602 { return "SwapForce" }
        0x2603 { return "SwapForce" }
        0x2805 { return "SwapForce" }
        0x2C02 { return "SwapForce" }
        0x3000 { return "TrapTeam" }
        0x3402 { return "TrapTeam" }
        0x3403 { return "TrapTeam" }
        0x3805 { return "TrapTeam" }
        0x3810 { return "TrapTeam" }
        0x3C02 { return "TrapTeam" }
        0x4502 { return "SuperChargers" }
        0x4503 { return "SuperChargers" }
        0x450D { return "SuperChargers" }
        0x450E { return "SuperChargers" }
        0x4515 { return "SuperChargers" }
        0x4810 { return "SuperChargers" }
    }

    if ($SkyId -ge 3400 -and $SkyId -le 3428) { return "SuperChargers" }
    if ($SkyId -ge 3000 -and $SkyId -le 3015) { return "SwapForce" }
    if ($SkyId -ge 1000 -and $SkyId -le 2015) { return "SwapForce" }
    if ($SkyId -ge 601 -and $SkyId -le 631) { return "Imaginators" }
    if ($SkyId -ge 450 -and $SkyId -le 543) { return "TrapTeam" }
    if ($SkyId -ge 100 -and $SkyId -le 115) { return "Giants" }
    if ($SkyId -ge 404 -and $SkyId -le 430) { return "SpyrosAdventure" }
    return "SpyrosAdventure"
}

function Get-SourceUrl([string]$Game) {
    switch ($Game) {
        "SpyrosAdventure" { return "https://skylanderscharacterlist.com/spyros-adventure-figures/" }
        "Giants" { return "https://skylanderscharacterlist.com/giants-figures/" }
        "SwapForce" { return "https://skylanderscharacterlist.com/swap-force-figures/" }
        "TrapTeam" { return "https://skylanderscharacterlist.com/trap-team-figures/" }
        "SuperChargers" { return "https://skylanderscharacterlist.com/superchargers-figures/" }
        "Imaginators" { return "https://skylanderscharacterlist.com/imaginators-figures/" }
        default { return "https://skylanderscharacterlist.com/" }
    }
}

function Get-BaseName([string]$Name) {
    $base = $Name -replace "\s+\((Top|Bottom)\)$", ""
    $base = $base -replace "\s+\(Nintendo Only\)$", ""
    $prefixPattern = "^(Series \d+|Legendary|LightCore|Eon's Elite|Dark|Nitro|Jade|Granite|Molten|Scarlet|Punch|Polar|Royal|Volcanic|Springtime|Big Bang|Anchors Away|Tidal Wave|Mega Ram|Horn Blast|Hyper Beam|Twin Blade|Phantom|Turbo|Full Blast|Blizzard|Super Gulp|Fizzy Frenzy|Sure Shot|Gnarly|Heavy Duty|Winterfest|King Cobra|Power Punch|Eggcellent|Power Blue|Frightful|Steel Plated|Birthday Bash|Missile-Tow|Eggcited|Enchanted|Quick Draw|Kickoff|Gear Head|Jolly|Hog Wild|Golden|E3)\s+"
    return ($base -replace $prefixPattern, "").Trim()
}

function Get-VariantName([string]$Name, [string]$BaseName, [int]$SkyVar) {
    if ($SkyVar -eq 0 -and $Name -notmatch "\((Top|Bottom)\)") {
        return ""
    }
    if ($Name -match "\((Top|Bottom)\)$") {
        return $Matches[1]
    }
    $variant = $Name.Replace($BaseName, "").Trim()
    $variant = $variant -replace "\s+\(Nintendo Only\)$", " Nintendo Only"
    return $variant.Trim()
}

function Get-SwapHalf([string]$Name) {
    if ($Name -match "\((Top|Bottom)\)$") {
        return $Matches[1].ToLowerInvariant()
    }
    return ""
}

function Get-Role([int]$SkyId, [string]$ToyKind, [string]$Name) {
    if ($ToyKind -ne "Skylander") { return $ToyKind }
    if ($Name -match "\((Top|Bottom)\)$") { return "SWAP Force $($Matches[1])" }
    if ($SkyId -ge 502 -and $SkyId -le 543) { return "Mini" }
    if ($SkyId -ge 601 -and $SkyId -le 631) { return "Sensei" }
    if ($SkyId -ge 3400 -and $SkyId -le 3428) { return "SuperCharger" }
    if ($SkyId -ge 450 -and $SkyId -le 485) { return "Trap Team" }
    if (@(101, 102, 104, 107, 109, 110, 112, 114) -contains $SkyId) { return "Giant" }
    return "Core"
}

$elementByBase = @{
    "Whirlwind" = "Air"; "Sonic Boom" = "Air"; "Warnado" = "Air"; "Lightning Rod" = "Air"; "Jet Vac" = "Air"; "Jet-Vac" = "Air"; "Swarm" = "Air"; "Free Ranger" = "Air"; "Boom Jet" = "Air"; "Scratch" = "Air"; "Pop Thorn" = "Air"; "Gusto" = "Air"; "Thunderbolt" = "Air"; "Fling Kong" = "Air"; "Blades" = "Air"; "Air Strike" = "Air"; "Bad Juju" = "Air"; "Wild Storm" = "Air"; "Stormblade" = "Air"; "Hurricane Jet Vac" = "Air"
    "Bash" = "Earth"; "Terrafin" = "Earth"; "Dino Rang" = "Earth"; "Dino-Rang" = "Earth"; "Prism Break" = "Earth"; "Crusher" = "Earth"; "Flashwing" = "Earth"; "Rubble Rouser" = "Earth"; "Doom Stone" = "Earth"; "Slobber Tooth" = "Earth"; "Scorp" = "Earth"; "Wallop" = "Earth"; "Head Rush" = "Earth"; "Fist Bump" = "Earth"; "Rocky Roll" = "Earth"; "Tri-Tip" = "Earth"; "Barbella" = "Earth"; "Golden Queen" = "Earth"; "Smash Hit" = "Earth"; "Shark Shooter Terrafin" = "Earth"
    "Sunburn" = "Fire"; "Eruptor" = "Fire"; "Ignitor" = "Fire"; "Flameslinger" = "Fire"; "Hot Head" = "Fire"; "Hot Dog" = "Fire"; "Blast Zone" = "Fire"; "Fire Kraken" = "Fire"; "Fryno" = "Fire"; "Smolderdash" = "Fire"; "Wildfire" = "Fire"; "Ka Boom" = "Fire"; "Trail Blazer" = "Fire"; "Torch" = "Fire"; "Ember" = "Fire"; "Flare Wolf" = "Fire"; "Spitfire" = "Fire"; "Lava Lance Eruptor" = "Fire"
    "Camo" = "Life"; "Zook" = "Life"; "Stealth Elf" = "Life"; "Stump Smash" = "Life"; "Tree Rex" = "Life"; "Shroomboom" = "Life"; "Stink Bomb" = "Life"; "Grilla Drilla" = "Life"; "Bumble Blast" = "Life"; "Zoo Lou" = "Life"; "Bushwack" = "Life"; "Bushwhack" = "Life"; "Tuff Luck" = "Life"; "Food Fight" = "Life"; "High Five" = "Life"; "Boom Bloom" = "Life"; "Ambush" = "Life"; "Chompy Mage" = "Life"; "Crash Bandicoot" = "Life"; "Thrillipede" = "Life"; "Super Shot Stealth Elf" = "Life"
    "Spyro" = "Magic"; "Voodood" = "Magic"; "Double Trouble" = "Magic"; "Wrecking Ball" = "Magic"; "Dark Spyro" = "Magic"; "Pop Fizz" = "Magic"; "Ninjini" = "Magic"; "Hoot Loop" = "Magic"; "Trap Shadow" = "Magic"; "Star Strike" = "Magic"; "Dune Bug" = "Magic"; "Blastermind" = "Magic"; "Enigma" = "Magic"; "Deja Vu" = "Magic"; "Cobra Candabra" = "Magic"; "Mysticat" = "Magic"; "Buckshot" = "Magic"; "Splat" = "Magic"; "Big Bubble Pop Fizz" = "Magic"
    "Trigger Happy" = "Tech"; "Drobot" = "Tech"; "Drill Sergeant" = "Tech"; "Boomer" = "Tech"; "Bouncer" = "Tech"; "Sprocket" = "Tech"; "Magna Charge" = "Tech"; "Spy Rise" = "Tech"; "Countdown" = "Tech"; "Wind Up" = "Tech"; "Jawbreaker" = "Tech"; "Gearshift" = "Tech"; "Chopper" = "Tech"; "Tread Head" = "Tech"; "Dr. Krankcase" = "Tech"; "Blaster-Tron" = "Tech"; "Ro-Bow" = "Tech"; "Chain Reaction" = "Tech"; "Dr. Neo Cortex" = "Tech"; "High Volt" = "Tech"; "Double Dare Trigger Happy" = "Tech"
    "Hex" = "Undead"; "Chop Chop" = "Undead"; "Ghost Roaster" = "Undead"; "Cynder" = "Undead"; "Eye Brawl" = "Undead"; "Eye-Brawl" = "Undead"; "Fright Rider" = "Undead"; "Night Shift" = "Undead"; "Roller Brawl" = "Undead"; "Grim Creeper" = "Undead"; "Krypt King" = "Undead"; "Short Cut" = "Undead"; "Bat Spin" = "Undead"; "Funny Bone" = "Undead"; "Chopscotch" = "Undead"; "Pit Boss" = "Undead"; "Wolfgang" = "Undead"; "Grave Clobber" = "Undead"; "Fiesta" = "Undead"; "Bone Bash Roller Brawl" = "Undead"
    "Zap" = "Water"; "Wham Shell" = "Water"; "Wham-Shell" = "Water"; "Gill Grunt" = "Water"; "Slam Bam" = "Water"; "Chill" = "Water"; "Thumpback" = "Water"; "Wash Buckler" = "Water"; "Freeze Blade" = "Water"; "Rip Tide" = "Water"; "Punk Shock" = "Water"; "Snap Shot" = "Water"; "Lob Star" = "Water"; "Lob-Star" = "Water"; "Flip Wreck" = "Water"; "Echo" = "Water"; "King Pen" = "Water"; "Tidepool" = "Water"; "Dive-Clops" = "Water"; "Deep Dive Gill Grunt" = "Water"
    "Knight Light" = "Light"; "Spotlight" = "Light"; "Aurora" = "Light"; "Astroblast" = "Light"
    "Knight Mare" = "Dark"; "Blackout" = "Dark"; "Starcast" = "Dark"; "Nightfall" = "Dark"
    "Kaos" = "Kaos"
}

function Get-Element([string]$BaseName, [string]$Name, [string]$ToyKind) {
    if ($ToyKind -ne "Skylander") { return "" }
    foreach ($candidate in @($BaseName, ($BaseName -replace "-", " "), $Name)) {
        if ($elementByBase.ContainsKey($candidate)) {
            return $elementByBase[$candidate]
        }
    }
    return ""
}

function Cpp-Escape([string]$Value) {
    return ($Value -replace "\\", "\\") -replace '"', '\"'
}

function Game-Cpp([string]$Game) {
    return "G::$Game"
}

function ToyKind-Cpp([string]$ToyKind) {
    return "T::$ToyKind"
}

$source = Get-Content -Raw -Path $CemuSource
$pattern = '\{\{(?<skyId>\d+),\s*(?<skyVar>0x[0-9A-Fa-f]+)\},\s*"(?<name>[^"]+)"\}'
$matches = [regex]::Matches($source, $pattern)
if ($matches.Count -eq 0) {
    throw "No Cemu Skylander entries found in $CemuSource"
}

$usedIds = @{}
$entries = @()

foreach ($match in $matches) {
    $skyId = [int]$match.Groups["skyId"].Value
    $skyVarText = $match.Groups["skyVar"].Value
    $skyVar = [Convert]::ToInt32($skyVarText.Substring(2), 16)
    $cemuName = Normalize-Name $match.Groups["name"].Value
    $displayName = $cemuName
    $toyKind = Get-ToyKind $skyId $cemuName
    $releaseGame = Get-ReleaseGame $skyId $skyVar $toyKind $displayName
    $baseName = Get-BaseName $displayName
    $variantName = Get-VariantName $displayName $baseName $skyVar
    $swapHalf = Get-SwapHalf $displayName
    $swapGroup = if ($swapHalf) { Get-Slug $baseName } else { "" }
    $assetKey = Get-Slug $displayName
    if ($usedIds.ContainsKey($assetKey)) {
        $assetKey = "{0}_{1:x4}_{2:x4}" -f $assetKey, $skyId, $skyVar
    }
    $usedIds[$assetKey] = $true
    $element = Get-Element $baseName $displayName $toyKind
    $role = Get-Role $skyId $toyKind $displayName
    $sourceUrl = Get-SourceUrl $releaseGame
    $wikiName = if ($swapHalf) { $baseName } else { $displayName }

    $entries += [pscustomobject]@{
        id = $assetKey
        assetKey = $assetKey
        displayName = $displayName
        cemuName = $cemuName
        toyKind = $toyKind
        element = $element
        role = $role
        baseName = $baseName
        variantName = $variantName
        isVariant = -not [string]::IsNullOrWhiteSpace($variantName)
        releaseGame = $releaseGame
        skyId = $skyId
        skyVar = $skyVar
        skyVarHex = ("0x{0:X4}" -f $skyVar)
        sourceUrl = $sourceUrl
        sclSearchName = $displayName
        wikiIconTitle = "File:$wikiName Icon.png"
        swapHalf = $swapHalf
        swapGroup = $swapGroup
        renderPath = "characters/renders/$releaseGame/$assetKey.png"
        iconPath = "characters/icons/$releaseGame/$assetKey.png"
        figurePath = "$releaseGame/$assetKey.sky"
    }
}

New-Item -ItemType Directory -Force -Path (Split-Path -Parent $CatalogJsonPath) | Out-Null
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $GeneratedIncludePath) | Out-Null

$catalog = [ordered]@{
    source = $CemuSource
    generatedAt = (Get-Date).ToUniversalTime().ToString("o")
    expectedCount = 481
    count = $entries.Count
    entries = @($entries)
}
($catalog | ConvertTo-Json -Depth 8) | Set-Content -Encoding UTF8 -Path $CatalogJsonPath

$lines = @(
    "// Generated by tools/sync_cemu_skylanders_catalog.ps1. Do not edit by hand."
)
foreach ($entry in $entries) {
    $line = '    character("{0}", "{1}", "{2}", {3}, "{4}", "{5}", "{6}", "{7}", {8}, "{9}", {10}, {11}, 0x{12:X4}, "{13}", "{14}", "{15}", "{16}", "{17}"),' -f `
        (Cpp-Escape $entry.id),
        (Cpp-Escape $entry.displayName),
        (Cpp-Escape $entry.cemuName),
        (ToyKind-Cpp $entry.toyKind),
        (Cpp-Escape $entry.element),
        (Cpp-Escape $entry.role),
        (Cpp-Escape $entry.baseName),
        (Cpp-Escape $entry.variantName),
        ($(if ($entry.isVariant) { "true" } else { "false" })),
        (Cpp-Escape $entry.assetKey),
        (Game-Cpp $entry.releaseGame),
        $entry.skyId,
        $entry.skyVar,
        (Cpp-Escape $entry.sourceUrl),
        (Cpp-Escape $entry.sclSearchName),
        (Cpp-Escape $entry.wikiIconTitle),
        (Cpp-Escape $entry.swapHalf),
        (Cpp-Escape $entry.swapGroup)
    $lines += $line
}
$lines | Set-Content -Encoding UTF8 -Path $GeneratedIncludePath

Write-Host "Parsed Cemu Skylander entries: $($entries.Count)"
Write-Host "Wrote catalog JSON: $CatalogJsonPath"
Write-Host "Wrote generated include: $GeneratedIncludePath"
