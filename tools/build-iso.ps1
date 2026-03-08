param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path,
    [string]$IsoName = "wevoa.iso",
    [switch]$RebuildImage
)

$ErrorActionPreference = "Stop"

function Resolve-Tool {
    param(
        [string]$Name,
        [string[]]$Candidates
    )
    $cmd = Get-Command $Name -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    foreach ($c in $Candidates) {
        if (Test-Path $c) { return $c }
    }
    throw "Tool not found: $Name"
}

function Invoke-Checked {
    param(
        [string]$Exe,
        [string[]]$ArgList
    )
    & $Exe @ArgList
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed ($LASTEXITCODE): $Exe $($ArgList -join ' ')"
    }
}

function Convert-ToMsysPath {
    param([string]$PathValue)
    $full = [System.IO.Path]::GetFullPath($PathValue)
    $drive = $full.Substring(0, 1).ToLowerInvariant()
    $rest = $full.Substring(2).Replace("\", "/")
    return "/$drive$rest"
}

$buildImageScript = Join-Path $PSScriptRoot "build-image.ps1"
$imgPath = Join-Path $Root "build\wevoa.img"
if ($RebuildImage -or -not (Test-Path $imgPath)) {
    & powershell -NoProfile -ExecutionPolicy Bypass -File $buildImageScript -Root $Root
    if ($LASTEXITCODE -ne 0) {
        throw "Image build failed."
    }
}

if (-not (Test-Path $imgPath)) {
    throw "Missing image: $imgPath"
}

$xorriso = Resolve-Tool -Name "xorriso" -Candidates @("C:\msys64\usr\bin\xorriso.exe")

$buildDir = Join-Path $Root "build"
$isoRoot = Join-Path $buildDir "iso-root"
$bootDir = Join-Path $isoRoot "boot"
$bootImgInsideIso = "boot/wevoa.img"
$isoPath = Join-Path $buildDir $IsoName

if (Test-Path $isoRoot) {
    Remove-Item -Recurse -Force $isoRoot
}
New-Item -ItemType Directory -Force -Path $bootDir | Out-Null

$bootImgPath = Join-Path $bootDir "wevoa.img"
Copy-Item -Force $imgPath $bootImgPath

Set-Content -Path (Join-Path $isoRoot "README.TXT") -Value @(
    "Wevoa boot ISO."
    "Boot image: $bootImgInsideIso"
)

if (Test-Path $isoPath) {
    Remove-Item -Force $isoPath
}

$isoPathMsys = Convert-ToMsysPath $isoPath
$isoRootMsys = Convert-ToMsysPath $isoRoot

Invoke-Checked -Exe $xorriso -ArgList @(
    "-as", "mkisofs",
    "-V", "WEVOA_001",
    "-iso-level", "3",
    "-R",
    "-J",
    "-b", $bootImgInsideIso,
    "-hard-disk-boot",
    "-o", $isoPathMsys,
    $isoRootMsys
)

Write-Host "Built $isoPath"
