param(
    [string]$OutputPath = "..\dist\TrayClockTooltip.exe",
    [switch]$Run,
    [switch]$RunInstallPrompt
)

$ErrorActionPreference = "Stop"

$root = $PSScriptRoot
$source = Join-Path $root "TrayClockTooltip.c"
$resource = Join-Path $root "TrayClockTooltip.rc"
$resourceObject = Join-Path $root "TrayClockTooltip.res.o"
$iconPath = Join-Path $root "TrayClockTooltip.ico"
$resolvedOutput = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath(
    (Join-Path $root $OutputPath)
)
$outputDir = Split-Path -Parent $resolvedOutput

if (-not (Test-Path $outputDir)) {
    New-Item -ItemType Directory -Path $outputDir | Out-Null
}

Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;

public static class NativeMethods
{
    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool DestroyIcon(IntPtr hIcon);
}
"@

function New-TrayClockIcon {
    param([string]$Path)

    $bitmap = New-Object System.Drawing.Bitmap 32, 32
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $graphics.Clear([System.Drawing.Color]::Transparent)

    $faceBounds = New-Object System.Drawing.Rectangle 2, 2, 28, 28
    $faceBrush = New-Object System.Drawing.Drawing2D.LinearGradientBrush -ArgumentList @(
        $faceBounds,
        [System.Drawing.Color]::FromArgb(18, 120, 220),
        [System.Drawing.Color]::FromArgb(0, 36, 112),
        [System.Drawing.Drawing2D.LinearGradientMode]::Vertical
    )
    $graphics.FillEllipse($faceBrush, $faceBounds)

    $rimPen = New-Object System.Drawing.Pen -ArgumentList @([System.Drawing.Color]::FromArgb(185, 235, 255), 1)
    $graphics.DrawEllipse($rimPen, $faceBounds)

    $tickPen = New-Object System.Drawing.Pen -ArgumentList @([System.Drawing.Color]::FromArgb(232, 248, 255), 1.6)
    $tickPen.StartCap = [System.Drawing.Drawing2D.LineCap]::Round
    $tickPen.EndCap = [System.Drawing.Drawing2D.LineCap]::Round
    $tickLines = @(
        @(16, 5, 16, 8), @(22, 6, 20, 9), @(26, 10, 23, 12),
        @(27, 16, 24, 16), @(26, 22, 23, 20), @(22, 26, 20, 23),
        @(16, 27, 16, 24), @(10, 26, 12, 23), @(6, 22, 9, 20),
        @(5, 16, 8, 16), @(6, 10, 9, 12), @(10, 6, 12, 9)
    )
    foreach ($line in $tickLines) {
        $graphics.DrawLine($tickPen, [single]$line[0], [single]$line[1], [single]$line[2], [single]$line[3])
    }

    $handPen = New-Object System.Drawing.Pen -ArgumentList @([System.Drawing.Color]::White, 2)
    $graphics.DrawLine($handPen, 16, 16, 16, 10)
    $graphics.DrawLine($handPen, 16, 16, 23, 18)

    $hIcon = $bitmap.GetHicon()
    $icon = [System.Drawing.Icon]::FromHandle($hIcon)
    $stream = [System.IO.File]::Create($Path)
    try {
        $icon.Save($stream)
    }
    finally {
        $stream.Dispose()
        $icon.Dispose()
        [NativeMethods]::DestroyIcon($hIcon) | Out-Null
        $handPen.Dispose()
        $tickPen.Dispose()
        $rimPen.Dispose()
        $faceBrush.Dispose()
        $graphics.Dispose()
        $bitmap.Dispose()
    }
}

New-TrayClockIcon $iconPath

windres -O coff -o $resourceObject $resource
# Omit unwind tables; this C GUI app does not use stack unwinding and the flags keep the EXE small.
gcc -municode -Os -s -mwindows -Wall -Wextra -fno-unwind-tables -fno-asynchronous-unwind-tables -o $resolvedOutput $source $resourceObject -lws2_32 -lshell32 -lgdi32 -luser32 -ladvapi32 -lole32 -lwtsapi32

Write-Host "Built: $resolvedOutput"

if ($RunInstallPrompt) {
    Start-Process -FilePath $resolvedOutput -ArgumentList "--install-prompt"
}
elseif ($Run) {
    Start-Process -FilePath $resolvedOutput
}
