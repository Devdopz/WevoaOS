param(
    [string]$Distro = "Ubuntu-24.04"
)

$ErrorActionPreference = "Stop"

$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$drive = $root.Substring(0,1).ToLowerInvariant()
$rest = $root.Substring(2).Replace("\", "/")
$wslPath = "/mnt/$drive$rest"

Write-Host "Building Wevoa in WSL distro '$Distro'..."
Write-Host "Path: $wslPath"

$cmd = "cd `"$wslPath`" && make"
wsl -d $Distro -- sh -lc $cmd

if ($LASTEXITCODE -ne 0) {
    throw "WSL build failed with exit code $LASTEXITCODE."
}

Write-Host "Build complete."

