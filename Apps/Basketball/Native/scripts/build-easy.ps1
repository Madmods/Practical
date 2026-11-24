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
        if(!$state.Compiler -or !$state.Sdk -or !$state.Python -or !$state.WebView){
            Write-Host 'Run 1 - Setup Vanta.bat to install the missing prerequisites.'
            exit 2
        }
        Write-Host 'All prerequisites are ready.' -ForegroundColor Green
        exit 0
    }
    $python=$state.Python

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
        Invoke-BuildStep 'Build and check Vanta (this can take a few minutes)' (Join-Path $PSScriptRoot 'build.cmd')
        Invoke-BuildStep 'Build networking components for Packet mode' (Join-Path $PSScriptRoot 'build-network-decoder-lab.cmd')
        $venv=Join-Path $native '.venv\Scripts\python.exe'
        $builtRuntime=Join-Path $native 'build\network_decoder\runtime'
        $helperArguments=@('-B',(Join-Path $PSScriptRoot 'build-network-host.py'))
        if($state.Driver -and $state.Driver -ne $builtRuntime){$helperArguments+=@('--windivert-dir',$state.Driver)}
        Invoke-BuildStep 'Package Packet mode helper' $venv $helperArguments
        Invoke-BuildStep 'Check helper imports (no capture)' (Join-Path $builtRuntime 'vanta-network-host.exe') @('--self-test')
        $network=Join-Path $app 'network_decoder'
        New-Item -ItemType Directory -Path $network -Force | Out-Null
        Copy-Item -LiteralPath $builtRuntime -Destination $network -Recurse -Force
        if(!$state.Driver){Write-Host 'Mouse mode is ready. Packet mode still needs an approved WinDivert runtime; no driver was downloaded.' -ForegroundColor Yellow}

        Write-Host "`nBUILD FINISHED. Double-click 3 - Start Vanta.bat." -ForegroundColor Green
        Write-Host 'Login: username 1, password 1. Join a supported game and click Continue.'
    }finally{Pop-Location}
}catch{
    Write-Host "`nBUILD STOPPED: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}
exit 0
