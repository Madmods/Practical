# Dot-sourced by build-easy.ps1. Downloads use official publishers only.
# Installer flags: learn.microsoft.com/visualstudio/install/use-command-line-parameters-to-install-visual-studio
# Python flags: docs.python.org/3.12/using/windows.html
# WebView2 detection: learn.microsoft.com/microsoft-edge/webview2/concepts/distribution

function Get-Python312 {
    $ErrorActionPreference='Continue'
    $venv=Join-Path $native '.venv\Scripts\python.exe'
    $candidates=@($venv,(Join-Path $env:LOCALAPPDATA 'Programs\Python\Python312\python.exe'))
    $launcher=Get-Command py.exe -ErrorAction SilentlyContinue
    if($launcher){
        $candidate=& $launcher.Source -3.12 -c "import sys; print(sys.executable)" 2>$null
        if($LASTEXITCODE -eq 0 -and $candidate){$candidates+=([string]$candidate)}
    }
    $python=Get-Command python.exe -ErrorAction SilentlyContinue
    if($python -and $python.Source -notmatch '\\WindowsApps\\'){$candidates+=$python.Source}
    foreach($candidate in $candidates){
        if(Test-Path -LiteralPath $candidate){
            & $candidate -c "import sys,struct; raise SystemExit(sys.version_info[:2] != (3,12) or struct.calcsize('P') != 8)" 2>$null
            if($LASTEXITCODE -eq 0){return $candidate}
            if($candidate -eq $venv){throw 'Native\.venv needs 64-bit Python 3.12. Rename that .venv folder and run setup again.'}
        }
    }
    return $null
}

function Get-PrerequisiteState {
    $vsRoot=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\2022\BuildTools'
    $compilers=@(Get-ChildItem -LiteralPath (Join-Path $vsRoot 'VC\Tools\MSVC') -Directory -ErrorAction SilentlyContinue | Where-Object {Test-Path -LiteralPath (Join-Path $_.FullName 'bin\Hostx64\x64\cl.exe')})
    $kitsRoot=(Get-ItemProperty -LiteralPath 'HKLM:\SOFTWARE\Microsoft\Windows Kits\Installed Roots' -Name KitsRoot10 -ErrorAction SilentlyContinue).KitsRoot10
    if(!$kitsRoot){$kitsRoot=Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10'}
    $sdks=@(Get-ChildItem -LiteralPath (Join-Path $kitsRoot 'Include') -Directory -ErrorAction SilentlyContinue | Where-Object {
        (Test-Path -LiteralPath (Join-Path $_.FullName 'um\Windows.h')) -and
        (Test-Path -LiteralPath (Join-Path $kitsRoot ('Lib\'+$_.Name+'\um\x64\kernel32.lib'))) -and
        (Test-Path -LiteralPath (Join-Path $kitsRoot ('bin\'+$_.Name+'\x64\rc.exe')))
    })
    $webview=$false
    $client='{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}'
    foreach($key in @("HKLM:\SOFTWARE\WOW6432Node\Microsoft\EdgeUpdate\Clients\$client","HKCU:\Software\Microsoft\EdgeUpdate\Clients\$client","HKLM:\SOFTWARE\Microsoft\EdgeUpdate\Clients\$client")){
        $pv=(Get-ItemProperty -LiteralPath $key -Name pv -ErrorAction SilentlyContinue).pv
        $version=$null
        if($pv -and [version]::TryParse($pv,[ref]$version) -and $version -gt [version]'0.0.0.0'){$webview=$true}
    }
    $driverFolder=$null
    foreach($folder in @((Join-Path $native 'third_party\windivert\bin'),(Join-Path $app 'network_decoder\runtime'),(Join-Path $native 'build\network_decoder\runtime'))){
        if((Test-Path -LiteralPath (Join-Path $folder 'WinDivert.dll')) -and (Test-Path -LiteralPath (Join-Path $folder 'WinDivert64.sys'))){$driverFolder=$folder;break}
    }
    [pscustomobject]@{
        VsRoot=$vsRoot
        Compiler=((Test-Path -LiteralPath (Join-Path $vsRoot 'Common7\Tools\VsDevCmd.bat')) -and $compilers.Count -gt 0)
        Sdk=($sdks.Count -gt 0)
        Python=(Get-Python312)
        WebView=$webview
        Driver=$driverFolder
    }
}

function Get-SetupDownload([string]$Url,[string]$Name){
    if($Url -notmatch '^https://'){throw 'Setup downloads require HTTPS.'}
    $cache=Join-Path $native 'third_party\.downloads'
    New-Item -ItemType Directory -Path $cache -Force | Out-Null
    # A new staging folder prevents reuse of a partial or rejected download.
    $stage=Join-Path $cache ([guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $stage | Out-Null
    $target=Join-Path $stage $Name
    Write-Host "Downloading $Name from $(([uri]$Url).Host)..." -ForegroundColor Cyan
    try{
        [Net.ServicePointManager]::SecurityProtocol=[Net.ServicePointManager]::SecurityProtocol -bor [Net.SecurityProtocolType]::Tls12
        Invoke-WebRequest -UseBasicParsing -Uri $Url -OutFile $target -ErrorAction Stop
        if(!(Test-Path -LiteralPath $target) -or (Get-Item -LiteralPath $target).Length -eq 0){throw 'The download is missing or empty.'}
    }catch{
        throw "Download stopped for $Name. Check your connection and Windows security notification. Setup does not bypass blocked downloads. $($_.Exception.Message)"
    }
    return $target
}

function Assert-SetupSignature([string]$Path,[string]$Publisher){
    $signature=Get-AuthenticodeSignature -LiteralPath $Path
    if($signature.Status -ne 'Valid' -or !$signature.SignerCertificate -or $signature.SignerCertificate.Subject -notmatch $Publisher){
        throw "Signature verification failed for $(Split-Path $Path -Leaf). Setup stopped."
    }
}

function Start-SetupInstaller([string]$Path,[string[]]$InstallerArguments){
    $process=Start-Process -FilePath $Path -ArgumentList $InstallerArguments -Wait -PassThru -WindowStyle Hidden
    return $process.ExitCode
}

function Install-SetupProgram([string]$Url,[string]$Name,[string]$Publisher,[string[]]$InstallerArguments){
    $download=Get-SetupDownload $Url $Name
    try{
        Assert-SetupSignature $download $Publisher
        $code=Start-SetupInstaller $download $InstallerArguments
        if($code -eq 3010 -or $code -eq 1641){throw 'Installation requires a Windows restart. Restart when ready, then run the same BAT file again.'}
        if($code -ne 0){throw "$Name installation stopped (exit $code). If you cancelled it, run setup again when ready."}
    }finally{
        if(Test-Path -LiteralPath $download){Remove-Item -LiteralPath $download -Force}
    }
}

function Assert-X64Image([string]$Path){
    $stream=[IO.File]::OpenRead($Path)
    $reader=New-Object IO.BinaryReader($stream)
    try{
        if($reader.ReadUInt16() -ne 0x5a4d){throw 'Invalid executable header.'}
        $stream.Position=0x3c
        $stream.Position=$reader.ReadUInt32()
        if($reader.ReadUInt32() -ne 0x4550 -or $reader.ReadUInt16() -ne 0x8664){throw 'Expected a Windows x64 executable.'}
    }finally{$reader.Dispose();$stream.Dispose()}
}

function Expand-NetworkRuntime([string]$Archive,[string]$Destination){
    Add-Type -AssemblyName System.IO.Compression
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $zip=[IO.Compression.ZipFile]::OpenRead($Archive)
    try{
        # Select explicit entries; never extract paths provided by the archive.
        foreach($name in @('WinDivert.dll','WinDivert64.sys')){
            $entries=@($zip.Entries | Where-Object {$_.FullName -cmatch ('^WinDivert-2\.2\.2-A/x64/'+[regex]::Escape($name)+'$')})
            if($entries.Count -ne 1 -or $entries[0].Length -gt 10MB){throw "Invalid networking archive entry: $name"}
            [IO.Compression.ZipFileExtensions]::ExtractToFile($entries[0],(Join-Path $Destination $name),$false)
            Assert-X64Image (Join-Path $Destination $name)
        }
        Assert-SetupSignature (Join-Path $Destination 'WinDivert64.sys') 'O=Cloudveil Technology Inc\.'
    }finally{$zip.Dispose()}
}

function Install-NetworkRuntime {
    $download=Get-SetupDownload 'https://github.com/basil00/WinDivert/releases/download/v2.2.2/WinDivert-2.2.2-A.zip' 'WinDivert-2.2.2-A.zip'
    $stage=Join-Path (Split-Path $download) 'runtime'
    New-Item -ItemType Directory -Path $stage | Out-Null
    try{
        Expand-NetworkRuntime $download $stage
        $destination=Join-Path $native 'third_party\windivert\bin'
        New-Item -ItemType Directory -Path $destination -Force | Out-Null
        foreach($name in @('WinDivert.dll','WinDivert64.sys')){Copy-Item -LiteralPath (Join-Path $stage $name) -Destination (Join-Path $destination $name) -Force}
    }finally{
        if(Test-Path -LiteralPath $download){Remove-Item -LiteralPath $download -Force}
        foreach($name in @('WinDivert.dll','WinDivert64.sys')){
            $file=Join-Path $stage $name
            if(Test-Path -LiteralPath $file){Remove-Item -LiteralPath $file -Force}
        }
    }
}

function Ensure-Prerequisites([switch]$CheckOnly){
    $state=Get-PrerequisiteState
    foreach($item in @(@('C++ Build Tools',$state.Compiler),@('Windows SDK',$state.Sdk),@('64-bit Python 3.12',[bool]$state.Python),@('WebView2 Runtime',$state.WebView),@('Networking files',[bool]$state.Driver))){
        if($item[1]){Write-Host "OK: $($item[0])"}else{Write-Host "MISSING: $($item[0])" -ForegroundColor Yellow}
    }
    if($CheckOnly){return $state}
    if(!$state.Compiler -or !$state.Sdk){
        Write-Host 'Installing C++ tools and Windows SDK. Allow the normal installer prompt; this can take several minutes.'
        $arguments=@('--passive','--wait','--norestart','--installPath',('"'+$state.VsRoot+'"'),'--add','Microsoft.VisualStudio.Workload.VCTools','--add','Microsoft.VisualStudio.Component.VC.Tools.x86.x64','--add','Microsoft.VisualStudio.Component.Windows11SDK.26100','--includeRecommended')
        if(Test-Path -LiteralPath (Join-Path $state.VsRoot 'Common7\Tools\VsDevCmd.bat')){$arguments=@('modify')+$arguments}
        Install-SetupProgram 'https://aka.ms/vs/17/release/vs_BuildTools.exe' 'vs_BuildTools.exe' 'O=Microsoft Corporation' $arguments
        $state=Get-PrerequisiteState
        if(!$state.Compiler -or !$state.Sdk){throw 'C++ tools/SDK are still missing. Check Visual Studio Installer, then rerun setup.'}
    }
    if(!$state.Python){
        $target=Join-Path $env:LOCALAPPDATA 'Programs\Python\Python312'
        Install-SetupProgram 'https://www.python.org/ftp/python/3.12.10/python-3.12.10-amd64.exe' 'python-3.12.10-amd64.exe' 'O=Python Software Foundation' @('/passive','/norestart','InstallAllUsers=0',('TargetDir="'+$target+'"'),'Include_launcher=1','InstallLauncherAllUsers=0','Include_pip=1','Include_test=0','PrependPath=0','AssociateFiles=0')
        $state=Get-PrerequisiteState
        if(!$state.Python){throw 'Python 3.12 is still unavailable. Check the Python installer and rerun setup.'}
    }
    if(!$state.WebView){
        Install-SetupProgram 'https://go.microsoft.com/fwlink/p/?LinkId=2124703' 'MicrosoftEdgeWebview2Setup.exe' 'O=Microsoft Corporation' @('/silent','/install')
        $state=Get-PrerequisiteState
        if(!$state.WebView){throw 'WebView2 Runtime is still unavailable. Rerun setup after the installer completes.'}
    }
    if(!$state.Driver){
        Install-NetworkRuntime
        $state=Get-PrerequisiteState
        if(!$state.Driver){throw 'Networking files are still missing. Check the Windows security notification. Setup does not bypass blocked files.'}
    }
    return $state
}
