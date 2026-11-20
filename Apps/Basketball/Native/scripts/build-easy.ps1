param([switch]$CheckOnly)
$ErrorActionPreference='Stop'
# Keep Windows PowerShell's standard modules available when a terminal with
# another PowerShell version launches the batch file. This is process-local.
$standardModules=Join-Path $PSHOME 'Modules'
$otherModules=@($env:PSModulePath -split ';' | Where-Object {$_ -and $_ -ne $standardModules})
$env:PSModulePath=(@($standardModules)+$otherModules)-join ';'
Import-Module (Join-Path $standardModules 'Microsoft.PowerShell.Utility\Microsoft.PowerShell.Utility.psd1') -Force
$native=Split-Path $PSScriptRoot -Parent
$app=Split-Path $native -Parent

function Get-Python312 {
    $ErrorActionPreference='Continue'
    $venv=Join-Path $native '.venv\Scripts\python.exe'
    if(Test-Path -LiteralPath $venv){
        & $venv -c "import sys; raise SystemExit(sys.version_info[:2] != (3, 12))" 2>$null
        if($LASTEXITCODE -eq 0){return $venv}
        throw 'Native\.venv uses another Python version. Rename that .venv folder and run this build again.'
    }
    $launcher=Get-Command py.exe -ErrorAction SilentlyContinue
    if($launcher){
        $candidate=& $launcher.Source -3.12 -c "import sys; print(sys.executable)" 2>$null
        if($LASTEXITCODE -eq 0 -and $candidate -and (Test-Path -LiteralPath $candidate)){return [string]$candidate}
    }
    $python=Get-Command python.exe -ErrorAction SilentlyContinue
    if($python -and $python.Source -notmatch '\\WindowsApps\\'){
        & $python.Source -c "import sys; raise SystemExit(sys.version_info[:2] != (3, 12))" 2>$null
        if($LASTEXITCODE -eq 0){return $python.Source}
    }
    return $null
}

function Invoke-BuildStep([string]$Label,[string]$Program,[string[]]$StepArguments=@()) {
    Write-Host "`n=== $Label ===" -ForegroundColor Cyan
    & $Program @StepArguments
    if($LASTEXITCODE -ne 0){throw "$Label failed (exit $LASTEXITCODE). Read the error above."}
}

try {
    Write-Host 'VANTA - SIMPLE BUILD' -ForegroundColor Cyan
    Write-Host 'Keep the entire extracted GitHub folder together.'
    $vsRoot=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\2022\BuildTools'
    $devCmd=Join-Path $vsRoot 'Common7\Tools\VsDevCmd.bat'
    $compilers=@(Get-ChildItem -LiteralPath (Join-Path $vsRoot 'VC\Tools\MSVC') -Directory -ErrorAction SilentlyContinue | Where-Object {Test-Path -LiteralPath (Join-Path $_.FullName 'bin\Hostx64\x64\cl.exe')})
    $python=Get-Python312
    $missing=$false
    if(!(Test-Path -LiteralPath $devCmd) -or !$compilers.Count){
        Write-Host "`nMISSING: Visual Studio 2022 Build Tools with Desktop development with C++ and a Windows SDK." -ForegroundColor Yellow
        Write-Host 'Install Build Tools 2022 at its default location:'
        Write-Host 'https://learn.microsoft.com/en-us/visualstudio/releases/2022/release-history'
        $missing=$true
    }else{Write-Host 'OK: Visual Studio 2022 C++ Build Tools'}
    $kitsRoot=(Get-ItemProperty -LiteralPath 'HKLM:\SOFTWARE\Microsoft\Windows Kits\Installed Roots' -Name KitsRoot10 -ErrorAction SilentlyContinue).KitsRoot10
    if(!$kitsRoot){$kitsRoot=Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10'}
    $sdks=@(Get-ChildItem -LiteralPath (Join-Path $kitsRoot 'Include') -Directory -ErrorAction SilentlyContinue | Where-Object {(Test-Path -LiteralPath (Join-Path $_.FullName 'um\Windows.h')) -and (Test-Path -LiteralPath (Join-Path $kitsRoot ('Lib\'+$_.Name+'\um\x64\kernel32.lib'))) -and (Test-Path -LiteralPath (Join-Path $kitsRoot ('bin\'+$_.Name+'\x64\rc.exe')))})
    if(!$sdks.Count){
        Write-Host "`nMISSING: Windows SDK headers, x64 libraries or resource compiler. Add a Windows 10/11 SDK in the Visual Studio Installer." -ForegroundColor Yellow
        $missing=$true
    }else{Write-Host 'OK: Windows SDK'}
    if(!$python){
        Write-Host "`nMISSING: Python 3.12 (64-bit), with the Python launcher or Python added to PATH." -ForegroundColor Yellow
        Write-Host 'https://www.python.org/downloads/release/python-31210/'
        $missing=$true
    }else{Write-Host 'OK: Python 3.12'}
    Write-Host 'To open the app, install Microsoft Edge WebView2 Runtime if it is missing:'
    Write-Host 'https://developer.microsoft.com/en-us/microsoft-edge/webview2/'
    if($missing){Write-Host "`nInstall the missing tools, then run this file again.";exit 2}

    $installedRuntime=Join-Path $app 'network_decoder\runtime'
    $builtRuntime=Join-Path $native 'build\network_decoder\runtime'
    $driverFolder=$null
    foreach($folder in @((Join-Path $native 'third_party\windivert\bin'),$installedRuntime,$builtRuntime)){
        if((Test-Path -LiteralPath (Join-Path $folder 'WinDivert.dll')) -and (Test-Path -LiteralPath (Join-Path $folder 'WinDivert64.sys'))){$driverFolder=$folder;break}
    }
    if($driverFolder){Write-Host 'OK: Existing networking DLL and driver found'}
    else{
        Write-Host "`nNetworking needs WinDivert.dll and WinDivert64.sys for QBA." -ForegroundColor Yellow
        Write-Host 'Put approved x64 copies in Apps\Basketball\Native\third_party\windivert\bin,'
        Write-Host 'or copy the whole network_decoder folder from the friend ZIP into Apps\Basketball.'
        Write-Host 'The application can still be built without those two files.'
    }
    if($CheckOnly){Write-Host "`nBuild tools are ready. Double-click 2 - Build Vanta.bat." -ForegroundColor Green;exit 0}

    $exe=Join-Path $app 'Vanta.exe'
    $open=@(Get-Process Vanta -ErrorAction SilentlyContinue | Where-Object {$_.Path -eq $exe})
    if($open.Count){throw 'Close this copy of Vanta using Quit, then run the build again. Your settings are preserved.'}
    Push-Location $native
    try {
        Write-Host "`n=== Prepare SDK and Python build dependencies ===" -ForegroundColor Cyan
        & (Join-Path $PSScriptRoot 'setup-dependencies.ps1') -PythonPath $python
        $venv=Join-Path $native '.venv\Scripts\python.exe'
        Invoke-BuildStep 'Build and check Vanta (this can take a few minutes)' (Join-Path $PSScriptRoot 'build.cmd')
        Invoke-BuildStep 'Build native networking components' (Join-Path $PSScriptRoot 'build-network-decoder-lab.cmd')
        $helperArguments=@('-B',(Join-Path $PSScriptRoot 'build-network-host.py'))
        if($driverFolder -and $driverFolder -ne $builtRuntime){$helperArguments+=@('--windivert-dir',$driverFolder)}
        Invoke-BuildStep 'Package networking helper' $venv $helperArguments
        $builtRuntime=Join-Path $native 'build\network_decoder\runtime'
        $networkReady=(Test-Path -LiteralPath (Join-Path $builtRuntime 'WinDivert.dll')) -and (Test-Path -LiteralPath (Join-Path $builtRuntime 'WinDivert64.sys'))
        if($networkReady){Invoke-BuildStep 'Check networking runtime (no capture)' (Join-Path $builtRuntime 'vanta-network-host.exe') @('--self-test')}
        $network=Join-Path $app 'network_decoder'
        New-Item -ItemType Directory -Path $network -Force | Out-Null
        try{Copy-Item -LiteralPath $builtRuntime -Destination $network -Recurse -Force}
        catch{throw 'Could not install the networking folder. Close Vanta and its networking helper, then run this build again.'}
        Write-Host "`nBUILD FINISHED. Double-click 3 - Start Vanta.bat." -ForegroundColor Green
        Write-Host 'Login: username 1, password 1. Join a supported game and click Continue.'
        if(!$networkReady){Write-Host 'QBA networking still needs the two WinDivert files described above. Add them and build again.' -ForegroundColor Yellow}
    }finally{Pop-Location}
}catch{
    Write-Host "`nBUILD STOPPED: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}
exit 0
