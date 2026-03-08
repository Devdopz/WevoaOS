param(
    [string]$VmName = "WevoaISO",
    [string]$IsoPath = "build/wevoa.iso",
    [int]$MemoryMB = 256
)

$ErrorActionPreference = "Stop"

function Resolve-VBoxManage {
    $cmd = Get-Command VBoxManage -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }

    $candidates = @(
        "C:\Program Files\Oracle\VirtualBox\VBoxManage.exe",
        "D:\vritualbox\VBoxManage.exe",
        "D:\VirtualBox\VBoxManage.exe"
    )
    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) { return $candidate }
    }

    throw "VBoxManage not found. Install VirtualBox or add VBoxManage to PATH."
}

function Resolve-HostResolution {
    $w = 1920
    $h = 1080
    try {
        $mode = Get-CimInstance -ClassName Win32_VideoController -ErrorAction Stop |
            Where-Object { $_.CurrentHorizontalResolution -gt 0 -and $_.CurrentVerticalResolution -gt 0 } |
            Sort-Object CurrentHorizontalResolution, CurrentVerticalResolution -Descending |
            Select-Object -First 1
        if ($mode) {
            $w = [int]$mode.CurrentHorizontalResolution
            $h = [int]$mode.CurrentVerticalResolution
        }
    } catch {
        try {
            Add-Type -AssemblyName System.Windows.Forms -ErrorAction Stop
            $b = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
            if ($b.Width -gt 0 -and $b.Height -gt 0) {
                $w = [int]$b.Width
                $h = [int]$b.Height
            }
        } catch {
            # Fallback defaults already set.
        }
    }

    if ($w -gt 2560) {
        $ratio = [double]$h / [double]$w
        $w = 2560
        $h = [int][Math]::Round($w * $ratio)
    }
    if ($h -gt 1600) {
        $ratio = [double]$w / [double]$h
        $h = 1600
        $w = [int][Math]::Round($h * $ratio)
    }
    if ($w -lt 1280) { $w = 1280 }
    if ($h -lt 720) { $h = 720 }

    $w = [int]($w - ($w % 8))
    $h = [int]($h - ($h % 8))
    if ($w -lt 1280) { $w = 1280 }
    if ($h -lt 720) { $h = 720 }

    return @{
        Width = $w
        Height = $h
    }
}

function Wait-VMState {
    param(
        [string]$VBox,
        [string]$Name,
        [string]$TargetState,
        [int]$MaxTries = 150
    )
    for ($i = 0; $i -lt $MaxTries; $i++) {
        $stateLine = & $VBox showvminfo $Name --machinereadable | Select-String -Pattern '^VMState='
        if ($stateLine -and $stateLine -match "VMState=""$TargetState""") {
            return $true
        }
        Start-Sleep -Milliseconds 200
    }
    return $false
}

function Wait-VMUnlocked {
    param(
        [string]$VBox,
        [string]$Name,
        [int]$MaxTries = 250
    )
    for ($i = 0; $i -lt $MaxTries; $i++) {
        $info = & $VBox showvminfo $Name --machinereadable
        $sessionLine = $info | Select-String -Pattern '^SessionState='
        if ($sessionLine -and $sessionLine -match 'SessionState="unlocked"') {
            return $true
        }
        # Some VirtualBox versions omit SessionState when fully powered off.
        if (-not $sessionLine) {
            $stateLine = $info | Select-String -Pattern '^VMState='
            if ($stateLine -and ($stateLine -match 'VMState="poweroff"' -or $stateLine -match 'VMState="aborted"')) {
                return $true
            }
        }
        Start-Sleep -Milliseconds 200
    }
    return $false
}

$vbox = Resolve-VBoxManage
$root = Resolve-Path (Join-Path $PSScriptRoot "..")
$iso = Join-Path $root $IsoPath

if (-not (Test-Path $iso)) {
    throw "Missing ISO: $iso. Build it first with tools/build-iso.ps1."
}

$existing = & $vbox list vms | Select-String -SimpleMatch """$VmName"""
if (-not $existing) {
    & $vbox createvm --name $VmName --ostype "Other_64" --register | Out-Null
    & $vbox storagectl $VmName --name "IDE" --add ide | Out-Null
}

$info = & $vbox showvminfo $VmName --machinereadable
$isSaved = $info | Select-String -SimpleMatch 'VMState="saved"'
if ($isSaved) {
    & $vbox discardstate $VmName | Out-Null
    if (-not (Wait-VMUnlocked -VBox $vbox -Name $VmName)) {
        throw "VM session did not unlock after discardstate."
    }
    $info = & $vbox showvminfo $VmName --machinereadable
}

$isRunning = $info | Select-String -SimpleMatch 'VMState="running"'
if ($isRunning) {
    & $vbox controlvm $VmName poweroff | Out-Null
    $stopped = Wait-VMState -VBox $vbox -Name $VmName -TargetState "poweroff"
    if (-not $stopped) {
        $stopped = Wait-VMState -VBox $vbox -Name $VmName -TargetState "aborted"
    }
    if (-not $stopped) {
        throw "Failed to stop $VmName before ISO reconfiguration."
    }
    if (-not (Wait-VMUnlocked -VBox $vbox -Name $VmName)) {
        throw "VM session did not unlock before ISO reconfiguration."
    }
}

# Apply critical CPU/boot flags every run so existing 32-bit VMs get repaired.
# VBoxVGA gives BIOS/VBE guests the broadest custom-mode compatibility.
& $vbox modifyvm $VmName --ostype "Other_64" --memory $MemoryMB --vram 128 --graphicscontroller vboxvga --acpi on --ioapic on --pae on --longmode on --boot1 dvd --boot2 disk --boot3 none --boot4 none | Out-Null

# Offer widescreen VBE modes for BIOS guest graphics.
$hostRes = Resolve-HostResolution
$hostMode = "{0}x{1}x32" -f $hostRes.Width, $hostRes.Height
$mode2 = "2560x1440x32"
$mode3 = "1920x1080x32"
$mode4 = "1600x900x32"
& $vbox setextradata $VmName "CustomVideoMode1" $hostMode | Out-Null
& $vbox setextradata $VmName "CustomVideoMode2" $mode2 | Out-Null
& $vbox setextradata $VmName "CustomVideoMode3" $mode3 | Out-Null
& $vbox setextradata $VmName "CustomVideoMode4" $mode4 | Out-Null
& $vbox setextradata $VmName "VBoxInternal2/CustomVideoMode1" $hostMode | Out-Null
& $vbox setextradata $VmName "VBoxInternal2/CustomVideoMode2" $mode2 | Out-Null
& $vbox setextradata $VmName "VBoxInternal2/CustomVideoMode3" $mode3 | Out-Null
& $vbox setextradata $VmName "VBoxInternal2/CustomVideoMode4" $mode4 | Out-Null

# Attach directly; this replaces prior media without a fragile pre-detach step.
& $vbox storageattach $VmName --storagectl "IDE" --port 1 --device 0 --type dvddrive --medium $iso | Out-Null

& $vbox startvm $VmName
