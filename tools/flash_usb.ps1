<#
  Flash the CM7 firmware over USB (ROM DFU bootloader) - no ST-LINK, no BOOT0 switch.

  1. If the board is running the app (USB COM port VID 0483 / PID 5740), open that port
     at 1200 baud. The app sees this, resets itself and jumps into the DFU bootloader.
  2. Wait for "STM32 Bootloader" (VID 0483 / PID DF11) to enumerate.
  3. Program + verify the .elf with STM32_Programmer_CLI and start the app.

  If the board is already in DFU (BOOT0 switch, or a previous attempt) step 1 is skipped.
  Usage:  powershell -ExecutionPolicy Bypass -File tools\flash_usb.ps1 [-Elf <path>]
#>
param(
    [string]$Elf = (Join-Path $PSScriptRoot "..\adbms-code\CM7\Debug\H7adbms-code_CM7.elf")
)

$ErrorActionPreference = "Stop"

function Find-ProgrammerCli {
    $candidates = @(
        "C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe"
    ) + (Get-ChildItem "C:\ST\STM32CubeIDE_*\STM32CubeIDE\plugins\*cubeprogrammer*\tools\bin\STM32_Programmer_CLI.exe" -ErrorAction SilentlyContinue |
         Sort-Object FullName -Descending | ForEach-Object { $_.FullName })
    foreach ($c in $candidates) { if ($c -and (Test-Path $c)) { return $c } }
    throw "STM32_Programmer_CLI.exe not found (install STM32CubeProgrammer or STM32CubeIDE)."
}

function Test-DfuPresent {
    [bool](Get-PnpDevice -PresentOnly -ErrorAction SilentlyContinue |
           Where-Object { $_.InstanceId -match 'VID_0483&PID_DF11' })
}

function Find-AppComPort {
    foreach ($d in Get-CimInstance Win32_PnPEntity -Filter "PNPDeviceID LIKE 'USB\\VID_0483&PID_5740%'") {
        if ($d.Name -match '\((COM\d+)\)') { return $Matches[1] }
    }
    return $null
}

$Elf = (Resolve-Path $Elf).Path
$cli = Find-ProgrammerCli
Write-Host "ELF : $Elf"
Write-Host "CLI : $cli"

if (-not (Test-DfuPresent)) {
    $port = Find-AppComPort
    if (-not $port) {
        throw "Board not found: no app USB COM port (VID 0483/PID 5740) and no DFU device. Is USB (J6) plugged in and the app running?"
    }
    Write-Host "App found on $port - requesting reboot into DFU (1200-baud touch)..."
    $sp = New-Object System.IO.Ports.SerialPort $port, 1200
    try {
        $sp.Open()
        Start-Sleep -Milliseconds 100
    } finally {
        try { $sp.Close() } catch { }   # the port vanishes as the board resets; that's expected
    }

    $deadline = (Get-Date).AddSeconds(10)
    while (-not (Test-DfuPresent)) {
        if ((Get-Date) -gt $deadline) {
            throw "DFU bootloader did not appear within 10 s. Is the running firmware new enough to support the 1200-baud trigger?"
        }
        Start-Sleep -Milliseconds 250
    }
    Start-Sleep -Milliseconds 500   # let Windows finish binding the DFU driver
}

Write-Host "DFU bootloader present - programming..."
& $cli -c port=usb1 -w $Elf -v -s 0x08000000
if ($LASTEXITCODE -ne 0) { throw "STM32_Programmer_CLI failed (exit code $LASTEXITCODE)." }
Write-Host "Done - app is running."
