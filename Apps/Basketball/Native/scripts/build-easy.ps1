param([switch]$CheckOnly,[switch]$SetupOnly)
$ErrorActionPreference='Stop'
# Keep Windows PowerShell's standard modules available when a terminal with
# another PowerShell version launches the batch file. This is process-local.
$standardModules=Join-Path $PSHOME 'Modules'
$otherModules=@($env:PSModulePath -split ';' | Where-Object {$_ -and $_ -ne $standardModules})
$env:PSModulePath=(@($standardModules)+$otherModules)-join ';'
Import-Module (Join-Path $standardModules 'Microsoft.PowerShell.Utility\Microsoft.PowerShell.Utility.psd1') -Force
$native=Split-Path $PSScriptRoot -Parent
$app=Split-Path $native -Parent

. (Join-Path $PSScriptRoot 'prerequisites.ps1')

function Invoke-BuildStep([string]$Label,[string]$Program,[string[]]$StepArguments=@()) {
    Write-Host "`n=== $Label ===" -ForegroundColor Cyan
    & $Program @StepArguments
    if($LASTEXITCODE -ne 0){throw "$Label failed (exit $LASTEXITCODE). Read the error above."}
}

try {
    Write-Host 'VANTA - SIMPLE BUILD' -ForegroundColor Cyan
    Write-Host 'Keep the entire extracted GitHub folder together.'
    $state=Ensure-Prerequisites -CheckOnly:$CheckOnly
    if($CheckOnly){
        if(!$state.Compiler -or !$state.Sdk -or !$state.Python -or !$state.WebView -or !$state.Driver){
            Write-Host 'Run 1 - Setup Vanta.bat to install the missing prerequisites.'
            exit 2
        }
        Write-Host 'All prerequisites are ready.' -ForegroundColor Green
        exit 0
    }
    $python=$state.Python
    $driverFolder=$state.Driver
    $builtRuntime=Join-Path $native 'build\network_decoder\runtime'

    $exe=Join-Path $app 'Vanta.exe'
    $open=@(Get-Process Vanta -ErrorAction SilentlyContinue | Where-Object {$_.Path -eq $exe})
    if($open.Count){throw 'Close this copy of Vanta using Quit, then run the build again. Your settings are preserved.'}
    Push-Location $native
    try {
        Write-Host "`n=== Prepare SDK and Python build dependencies ===" -ForegroundColor Cyan
        & (Join-Path $PSScriptRoot 'setup-dependencies.ps1') -PythonPath $python
        if($SetupOnly){
            Write-Host "`nSETUP FINISHED. Double-click 2 - Build Vanta.bat." -ForegroundColor Green
            exit 0
        }
        $venv=Join-Path $native '.venv\Scripts\python.exe'
        Invoke-BuildStep 'Build and check Vanta (this can take a few minutes)' (Join-Path $PSScriptRoot 'build.cmd')
        Invoke-BuildStep 'Build native networking components' (Join-Path $PSScriptRoot 'build-network-decoder-lab.cmd')
        $helperArguments=@('-B',(Join-Path $PSScriptRoot 'build-network-host.py'))
        if($driverFolder -and $driverFolder -ne $builtRuntime){$helperArguments+=@('--windivert-dir',$driverFolder)}
        Invoke-BuildStep 'Package networking helper' $venv $helperArguments
        $builtRuntime=Join-Path $native 'build\network_decoder\runtime'
        $networkReady=(Test-Path -LiteralPath (Join-Path $builtRuntime 'WinDivert.dll')) -and (Test-Path -LiteralPath (Join-Path $builtRuntime 'WinDivert64.sys'))
        if(!$networkReady){throw 'Networking files are missing from the built runtime. Check the Windows security notification, then rerun setup.'}
        Invoke-BuildStep 'Check networking runtime (no capture)' (Join-Path $builtRuntime 'vanta-network-host.exe') @('--self-test')
        $network=Join-Path $app 'network_decoder'
        New-Item -ItemType Directory -Path $network -Force | Out-Null
        try{Copy-Item -LiteralPath $builtRuntime -Destination $network -Recurse -Force}
        catch{throw 'Could not install the networking folder. Close Vanta and its networking helper, then run this build again.'}
        Write-Host "`nBUILD FINISHED. Double-click 3 - Start Vanta.bat." -ForegroundColor Green
        Write-Host 'Login: username 1, password 1. Join a supported game and click Continue.'
    }finally{Pop-Location}
}catch{
    Write-Host "`nBUILD STOPPED: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}
exit 0
