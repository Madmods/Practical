[CmdletBinding()]
param(
    [ValidateRange(5,300)][int]$DurationSeconds = 30,
    [string]$OutputDirectory,
    [switch]$Elevated,
    [switch]$PrepareOnly
)
$ErrorActionPreference = 'Stop'
if (-not $OutputDirectory) {
    $OutputDirectory = Join-Path $PSScriptRoot ('..\build\packet-captures\' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
}
$OutputDirectory = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$statePath = Join-Path $OutputDirectory 'capture-state.json'
function Write-State([string]$Status, [string]$Message) {
    @{status=$Status; message=$Message; timeUtc=[DateTime]::UtcNow.ToString('o'); outputDirectory=$OutputDirectory} |
        ConvertTo-Json | Set-Content -LiteralPath $statePath -Encoding UTF8
}
$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if ($PrepareOnly) {
    Get-Process -Name RobloxPlayerBeta -ErrorAction Stop | Select-Object Id,ProcessName |
        ConvertTo-Json | Set-Content -LiteralPath (Join-Path $OutputDirectory 'process-preview.json') -Encoding UTF8
    Write-State 'prepared' 'Script parsed and Roblox found. No capture or elevation requested.'
    return
}
if (-not $isAdmin) {
    if ($Elevated) { throw 'Administrator access was not granted.' }
    Write-State 'awaiting-elevation' 'Approve the Windows administrator prompt to begin the bounded capture.'
    $captureArgs = '-NoProfile -ExecutionPolicy Bypass -File "{0}" -DurationSeconds {1} -OutputDirectory "{2}" -Elevated' -f $PSCommandPath,$DurationSeconds,$OutputDirectory
    try {
        $captureProcess = Start-Process -FilePath "$env:SystemRoot\System32\WindowsPowerShell\v1.0\powershell.exe" -ArgumentList $captureArgs -Verb RunAs -WindowStyle Hidden -PassThru
        Write-Output "Capture helper PID: $($captureProcess.Id)"
        Write-Output "Capture output: $OutputDirectory"
    } catch {
        Write-State 'failed' $_.Exception.Message
        throw
    }
    return
}
$pktmon = Join-Path $env:SystemRoot 'System32\pktmon.exe'
$captureStarted = $false
$filtersAdded = $false
$transcribing = $false
$captureMutex = New-Object Threading.Mutex($false, 'Global\VantaGamePacketCapture')
$ownsMutex = $false
function Invoke-Pktmon {
    param([string[]]$CommandArguments)
    $commandOutput = & $pktmon @CommandArguments 2>&1
    $commandExitCode = $LASTEXITCODE
    $commandText = ($commandOutput | Out-String).Trim()
    Write-Host $commandText
    if ($commandExitCode -ne 0) { throw "pktmon $($CommandArguments -join ' ') failed ($commandExitCode): $commandText" }
    return $commandText
}
try {
    $ownsMutex = $captureMutex.WaitOne(0)
    if (-not $ownsMutex) { throw 'Another Vanta packet capture is already running.' }
    Start-Transcript -Path (Join-Path $OutputDirectory 'capture.log') | Out-Null
    $transcribing = $true
    Write-State 'checking' 'Checking Packet Monitor and Roblox UDP endpoints.'
    $monitorStatus = Invoke-Pktmon -CommandArguments @('status')
    if ($monitorStatus -notmatch '(?i)not running|stopped') {
        throw 'Packet Monitor is already active, or its stopped state could not be confirmed. Existing capture left alone.'
    }
    $existingFilters = Invoke-Pktmon -CommandArguments @('filter','list')
    if ($existingFilters -notmatch '(?im)no filters|^\s*None\s*$') {
        throw 'Existing Packet Monitor filters found, or empty state could not be confirmed. Existing filters left alone.'
    }
    $gameProcesses = @(Get-Process -Name RobloxPlayerBeta -ErrorAction Stop)
    $endpoints = @(foreach ($gameProcess in $gameProcesses) {
        Get-NetUDPEndpoint -OwningProcess $gameProcess.Id -ErrorAction SilentlyContinue |
            Where-Object { $_.LocalAddress -notin @('127.0.0.1','::1') } |
            Select-Object LocalAddress,LocalPort,OwningProcess
    })
    $ports = @($endpoints | Select-Object -ExpandProperty LocalPort -Unique)
    if ($ports.Count -eq 0) { throw 'No non-loopback Roblox UDP endpoints found. Join a game and try again.' }
    @{startedUtc=[DateTime]::UtcNow.ToString('o'); durationSeconds=$DurationSeconds; processes=@($gameProcesses | Select-Object Id,ProcessName); endpoints=$endpoints; filter='UDP local-port snapshot; either packet endpoint can match; NIC capture only'; note='Ports are sampled at capture start. Rejoining can change ports. Payloads are raw bytes and may be encrypted.'} |
        ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $OutputDirectory 'capture-info.json') -Encoding UTF8
    foreach ($port in $ports) {
        Invoke-Pktmon -CommandArguments @('filter','add',"VantaUDP$port",'-t','UDP','-p',"$port") | Out-Null
        $filtersAdded = $true
    }
    $etlPath = Join-Path $OutputDirectory 'game-packets.etl'
    Invoke-Pktmon -CommandArguments @('start','--capture','--comp','nics','--pkt-size','0','--file-name',$etlPath,'--file-size','128','--log-mode','circular') | Out-Null
    $captureStarted = $true
    Write-State 'capturing' "Capturing Roblox UDP ports $($ports -join ', ') for $DurationSeconds seconds."
    $captureClock = [Diagnostics.Stopwatch]::StartNew()
    while ($captureClock.Elapsed.TotalSeconds -lt $DurationSeconds) {
        if (Test-Path -LiteralPath (Join-Path $OutputDirectory 'stop-request.txt')) { break }
        Start-Sleep -Milliseconds 500
        $alive = @($gameProcesses | Where-Object { -not $_.HasExited })
        if ($alive.Count -eq 0) { break }
    }
    Invoke-Pktmon -CommandArguments @('counters') | Set-Content -LiteralPath (Join-Path $OutputDirectory 'counters.txt') -Encoding UTF8
    Invoke-Pktmon -CommandArguments @('stop') | Out-Null
    $captureStarted = $false
    Invoke-Pktmon -CommandArguments @('etl2pcap',$etlPath,'--out',(Join-Path $OutputDirectory 'game-packets.pcapng')) | Out-Null
    if (-not (Test-Path -LiteralPath (Join-Path $OutputDirectory 'game-packets.pcapng'))) { throw 'PCAPNG conversion did not produce an output file.' }
    Write-State 'complete' 'Capture saved as game-packets.pcapng. Open with Wireshark to inspect packets. Raw payloads may be encrypted.'
} catch {
    Write-State 'failed' $_.Exception.Message
    Write-Host $_.Exception.Message
    exit 1
} finally {
    if ($captureStarted) { & $pktmon stop 2>&1 | Out-Host }
    if ($filtersAdded) { & $pktmon filter remove 2>&1 | Out-Host }
    if ($transcribing) { Stop-Transcript | Out-Null }
    if ($ownsMutex) { $captureMutex.ReleaseMutex() }
    $captureMutex.Dispose()
}




