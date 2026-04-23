param(
    [Parameter(Mandatory = $true)][string]$Mode,
    [Parameter(Mandatory = $true)][string]$Workspace,
    [string]$Seconds = "15",
    [string]$GameBin = ""
)

$capturesDir = Join-Path $Workspace "captures"
$captureBin = Join-Path $Workspace "build\profile\tracy-capture\tracy-capture.exe"
$csvexportBin = Join-Path $Workspace "build\profile\tracy-csvexport\tracy-csvexport.exe"
$captureFile = Join-Path $capturesDir "latest.tracy"
$capturePidFile = Join-Path $capturesDir ".latest-capture.pid"
$captureLogFile = Join-Path $capturesDir ".latest-capture.log"

function Export-Capture {
    New-Item -ItemType Directory -Force -Path $capturesDir | Out-Null

    if (!(Test-Path $captureFile) -or ((Get-Item $captureFile).Length -le 0)) {
        throw "No Tracy capture file created at $captureFile"
    }

    & $csvexportBin $captureFile | Set-Content -Encoding utf8 (Join-Path $capturesDir "latest-zones.csv")
    & $csvexportBin -e $captureFile | Set-Content -Encoding utf8 (Join-Path $capturesDir "latest-zones-self.csv")
    & $csvexportBin -f CorePhys $captureFile | Set-Content -Encoding utf8 (Join-Path $capturesDir "latest-corephys-zones.csv")
    & $csvexportBin -f renderer $captureFile | Set-Content -Encoding utf8 (Join-Path $capturesDir "latest-renderer-zones.csv")
    & $csvexportBin -f Sim $captureFile | Set-Content -Encoding utf8 (Join-Path $capturesDir "latest-sim-zones.csv")
    & $csvexportBin -m $captureFile | Set-Content -Encoding utf8 (Join-Path $capturesDir "latest-messages.csv")
}

function Prepare-Capture {
    New-Item -ItemType Directory -Force -Path $capturesDir | Out-Null
    Remove-Item -Force -ErrorAction SilentlyContinue $captureFile, $capturePidFile, $captureLogFile

    $process = Start-Process -FilePath $captureBin `
        -ArgumentList @("-f", "-a", "127.0.0.1", "-o", $captureFile, "-s", $Seconds) `
        -RedirectStandardOutput $captureLogFile `
        -RedirectStandardError $captureLogFile `
        -PassThru `
        -WindowStyle Hidden
    Set-Content -Encoding ascii -Path $capturePidFile -Value $process.Id
}

function Finalize-Capture {
    if (Test-Path $capturePidFile) {
        $capturePid = Get-Content $capturePidFile
        if ($capturePid) {
            Wait-Process -Id $capturePid -ErrorAction SilentlyContinue
        }
        Remove-Item -Force -ErrorAction SilentlyContinue $capturePidFile
    }

    Export-Capture
}

switch ($Mode) {
    "prepare" {
        Prepare-Capture
    }
    "finalize" {
        Finalize-Capture
    }
    "run" {
        if ([string]::IsNullOrWhiteSpace($GameBin)) {
            throw "run mode requires a game binary path"
        }

        try {
            Prepare-Capture
            & $GameBin
        }
        finally {
            Finalize-Capture
        }
    }
    default {
        throw "Unknown mode: $Mode"
    }
}
