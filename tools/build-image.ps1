param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path,
    [int]$Stage2Sectors = 32,
    [int]$KernelMaxSectors = 512,
    [int]$ImageSizeMB = 64
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

$gcc = Resolve-Tool -Name "gcc" -Candidates @("C:\msys64\ucrt64\bin\gcc.exe")
$ld = Resolve-Tool -Name "ld" -Candidates @("C:\msys64\ucrt64\bin\ld.exe")
$objcopy = Resolve-Tool -Name "objcopy" -Candidates @("C:\msys64\ucrt64\bin\objcopy.exe")
$nasm = Resolve-Tool -Name "nasm" -Candidates @("C:\Users\Windows10\AppData\Local\bin\NASM\nasm.exe")

$buildDir = Join-Path $Root "build"
$objDir = Join-Path $buildDir "obj"
$bootStage1Dir = Join-Path $buildDir "boot\stage1"
$bootStage2Dir = Join-Path $buildDir "boot\stage2"
$kernelBuildDir = Join-Path $buildDir "kernel"

New-Item -ItemType Directory -Force -Path $objDir, $bootStage1Dir, $bootStage2Dir, $kernelBuildDir | Out-Null

$kernelSources = @(
    "kernel\arch\x86_64\idt.c",
    "kernel\dev\ata.c",
    "kernel\dev\console.c",
    "kernel\dev\keyboard.c",
    "kernel\mm\pmm.c",
    "kernel\mm\kheap.c",
    "kernel\fs\fs.c",
    "kernel\storage\pstore.c",
    "kernel\net\net.c",
    "kernel\gui\ui_compositor.c",
    "kernel\gui\ui_shell.c",
    "kernel\gui\ui_style.c",
    "kernel\gui\ui_icons.c",
    "kernel\gui\gui.c",
    "kernel\lib\string.c",
    "kernel\lib\print.c",
    "kernel\core\config.c",
    "kernel\core\panic.c",
    "kernel\core\wstatus.c",
    "kernel\core\main.c",
    "kernel\shell\shell.c"
)

$cflags = @(
    "-ffreestanding",
    "-fno-builtin",
    "-fno-stack-protector",
    "-fno-pic",
    "-fno-pie",
    "-fno-asynchronous-unwind-tables",
    "-fno-unwind-tables",
    "-fno-exceptions",
    "-m64",
    "-mno-red-zone",
    "-mgeneral-regs-only",
    "-mcmodel=large",
    "-Wall",
    "-Wextra",
    "-Werror",
    "-std=c11",
    "-I$(Join-Path $Root 'kernel\include')"
)

$kernelObjects = New-Object System.Collections.Generic.List[string]

foreach ($srcRel in $kernelSources) {
    $src = Join-Path $Root $srcRel
    $obj = Join-Path $objDir ("{0}.o" -f [IO.Path]::GetFileNameWithoutExtension($srcRel))
    Invoke-Checked -Exe $gcc -ArgList (@($cflags) + @("-c", $src, "-o", $obj))
    $kernelObjects.Add($obj)
}

$entryObj = Join-Path $objDir "entry.o"
$idtAsmObj = Join-Path $objDir "idt_asm.o"
Invoke-Checked -Exe $nasm -ArgList @("-f", "win64", (Join-Path $Root "kernel\arch\x86_64\entry.asm"), "-o", $entryObj)
Invoke-Checked -Exe $nasm -ArgList @("-f", "win64", (Join-Path $Root "kernel\arch\x86_64\idt.asm"), "-o", $idtAsmObj)

# Keep entry first so _start is at the first kernel bytes in the flat binary.
$orderedObjects = @($entryObj, $idtAsmObj) + $kernelObjects

$kernelElf = Join-Path $kernelBuildDir "kernel.elf"
$kernelBin = Join-Path $kernelBuildDir "kernel.bin"
$ldArgs = @(
    "-nostdlib",
    "--image-base", "0x0",
    "-T", (Join-Path $Root "kernel\linker.ld"),
    "-o", $kernelElf
) + $orderedObjects
Invoke-Checked -Exe $ld -ArgList $ldArgs
Invoke-Checked -Exe $objcopy -ArgList @("-O", "binary", $kernelElf, $kernelBin)

$kernelSizeBytes = (Get-Item $kernelBin).Length
$kernelSectors = [int](($kernelSizeBytes + 511) / 512)
if ($kernelSectors -le 0) {
    throw "Kernel binary is empty."
}
if ($kernelSectors -gt $KernelMaxSectors) {
    throw "Kernel is too large ($kernelSectors sectors > $KernelMaxSectors)."
}

$stage2Bin = Join-Path $bootStage2Dir "stage2.bin"
Invoke-Checked -Exe $nasm -ArgList @(
    "-f", "bin",
    (Join-Path $Root "boot\stage2\stage2.asm"),
    "-D", "STAGE2_SECTORS=$Stage2Sectors",
    "-D", "KERNEL_SECTORS=$kernelSectors",
    "-o", $stage2Bin
)

$stage2SizeBytes = (Get-Item $stage2Bin).Length
$stage2MaxBytes = $Stage2Sectors * 512
if ($stage2SizeBytes -gt $stage2MaxBytes) {
    throw "Stage2 is too large ($stage2SizeBytes bytes > $stage2MaxBytes)."
}

$stage1Bin = Join-Path $bootStage1Dir "stage1.bin"
Invoke-Checked -Exe $nasm -ArgList @(
    "-f", "bin",
    (Join-Path $Root "boot\stage1\stage1.asm"),
    "-D", "STAGE2_SECTORS=$Stage2Sectors",
    "-o", $stage1Bin
)

$stage1SizeBytes = (Get-Item $stage1Bin).Length
if ($stage1SizeBytes -ne 512) {
    throw "Stage1 must be exactly 512 bytes (got $stage1SizeBytes)."
}

$imgPath = Join-Path $buildDir "wevoa.img"
$imgBytes = [int64]$ImageSizeMB * 1024 * 1024

$fs = [System.IO.File]::Open($imgPath, [System.IO.FileMode]::Create, [System.IO.FileAccess]::ReadWrite, [System.IO.FileShare]::None)
try {
    $fs.SetLength($imgBytes)

    $stage1 = [System.IO.File]::ReadAllBytes($stage1Bin)
    $stage2 = [System.IO.File]::ReadAllBytes($stage2Bin)
    $kernel = [System.IO.File]::ReadAllBytes($kernelBin)

    $fs.Seek(0, [System.IO.SeekOrigin]::Begin) | Out-Null
    $fs.Write($stage1, 0, $stage1.Length)

    $fs.Seek(512, [System.IO.SeekOrigin]::Begin) | Out-Null
    $fs.Write($stage2, 0, $stage2.Length)

    $kernelOffset = (1 + $Stage2Sectors) * 512
    $fs.Seek($kernelOffset, [System.IO.SeekOrigin]::Begin) | Out-Null
    $fs.Write($kernel, 0, $kernel.Length)
}
finally {
    $fs.Dispose()
}

Write-Host "Built $imgPath"
Write-Host "Stage2 sectors: $Stage2Sectors"
Write-Host "Kernel sectors: $kernelSectors"
