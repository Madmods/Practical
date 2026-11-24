# Dot-sourced by build-easy.ps1. Downloads use official publishers only.
# Installer flags: learn.microsoft.com/visualstudio/install/use-command-line-parameters-to-install-visual-studio
# Python build package: docs.python.org/3.12/using/windows.html#the-nuget-org-packages
# WebView2 detection: learn.microsoft.com/microsoft-edge/webview2/concepts/distribution

function Test-BuildPython([string]$Path){
    $ErrorActionPreference='Continue'
    if(!(Test-Path -LiteralPath $Path -PathType Leaf)){return $false}
    $global:LASTEXITCODE=$null
    & $Path -c "import sys,struct,ssl,venv,ensurepip; raise SystemExit(sys.version_info[:2] != (3,12) or struct.calcsize('P') != 8)" 2>$null
    return ($LASTEXITCODE -eq 0)
}

function Get-RegisteredPython312 {
    # PEP 514: discover custom and all-user installations without requiring PATH.
    foreach($root in @('HKCU:\Software\Python\PythonCore','HKLM:\Software\Python\PythonCore','HKLM:\Software\WOW6432Node\Python\PythonCore')){
        foreach($tag in @('3.12','3.12-64')){
            $key=Get-Item -LiteralPath "$root\$tag\InstallPath" -ErrorAction SilentlyContinue
            if($key){
                $exe=$key.GetValue('ExecutablePath')
                if($exe){Write-Output $exe}
                $folder=$key.GetValue('')
                if($folder){Write-Output (Join-Path $folder 'python.exe')}
            }
        }
    }
}

function Get-Python312 {
    $ErrorActionPreference='Continue'
    $venv=Join-Path $native '.venv\Scripts\python.exe'
    $candidates=@($venv,(Join-Path $native 'third_party\python\3.12.10\tools\python.exe'),(Join-Path $env:LOCALAPPDATA 'Programs\Python\Python312\python.exe'))
    $candidates+=@(Get-RegisteredPython312)
    $launcher=Get-Command py.exe -ErrorAction SilentlyContinue
    if($launcher){
        $candidate=& $launcher.Source -3.12 -c "import sys; print(sys.executable)" 2>$null
        if($LASTEXITCODE -eq 0 -and $candidate){$candidates+=([string]$candidate)}
    }
    $python=Get-Command python.exe -ErrorAction SilentlyContinue
    if($python -and $python.Source -notmatch '\\WindowsApps\\'){$candidates+=$python.Source}
    foreach($candidate in $candidates){
        if(Test-Path -LiteralPath $candidate){
            if(Test-BuildPython $candidate){return $candidate}
            if($candidate -eq $venv){throw 'Native\.venv needs a working 64-bit Python 3.12 with SSL, venv and ensurepip. Rename that .venv folder and run setup again.'}
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
        throw "Signature verification failed for $(Split-Path $Path -Leaf). Windows status: $($signature.Status). Publisher: $($signature.SignerCertificate.Subject). $($signature.StatusMessage) Setup stopped."
    }
}

function Assert-NetworkArchive([string]$Archive){
    # Exact v2.2.2-A intake fingerprints are recorded in the upstream issue:
    # https://github.com/basil00/WinDivert/issues/411
    # They identify the release, not an antivirus verdict; Windows checks still apply.
    if((Get-FileHash -LiteralPath $Archive -Algorithm SHA256).Hash -ne '63CB41763BB4B20F600B6DE04E991A9C2BE73279E317D4D82F237B150C5F3F15'){
        throw 'WinDivert 2.2.2-A archive hash mismatch. Setup stopped before extraction.'
    }
}

function Assert-NetworkDriver([string]$Path){
    # The current release uses a different signing sponsor from older builds.
    # Pin the exact driver bytes and require Windows to validate the signature.
    # Never treat a matching hash as permission to accept an invalid signature.
    $signature=Get-AuthenticodeSignature -LiteralPath $Path
    if($signature.Status -ne 'Valid' -or !$signature.SignerCertificate){
        throw "Networking driver signature is not valid. Windows status: $($signature.Status). Publisher: $($signature.SignerCertificate.Subject). $($signature.StatusMessage) Setup stopped; review Windows Security Protection history."
    }
    if((Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash -ne '8DA085332782708D8767BCACE5327A6EC7283C17CFB85E40B03CD2323A90DDC2'){
        throw 'Networking driver does not match the pinned WinDivert 2.2.2-A driver. Setup stopped.'
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
        if($code -eq 1602 -or $code -eq 1223){throw "$Name installation was cancelled (exit $code). Run setup again when ready."}
        if($code -ne 0){throw "$Name installation failed (exit $code). Check the installer log; this code does not imply cancellation."}
    }finally{
        if(Test-Path -LiteralPath $download){Remove-Item -LiteralPath $download -Force}
    }
}

function Install-BuildPython {
    # CPython's official NuGet package supports build systems, venv and pip.
    # It needs no MSI registration, launcher installation or administrator access.
    $download=Get-SetupDownload 'https://api.nuget.org/v3-flatcontainer/python/3.12.10/python.3.12.10.nupkg' 'python-3.12.10.zip'
    try{
        $hash=(Get-FileHash -LiteralPath $download -Algorithm SHA256).Hash
        if($hash -ne '0EB85C2DFCCCCF1B17352DE4C397F69194035B7D37149EACC16F1147D93DE3B8'){throw 'Python build package hash mismatch. Setup stopped.'}
        $destination=Join-Path $native 'third_party\python\3.12.10'
        Expand-Archive -LiteralPath $download -DestinationPath $destination -Force
        $python=Join-Path $destination 'tools\python.exe'
        Assert-SetupSignature $python 'O=Python Software Foundation'
        Assert-SetupSignature (Join-Path $destination 'tools\python312.dll') 'O=Python Software Foundation'
        if(!(Test-BuildPython $python)){throw 'The local Python build package could not run with SSL, venv and ensurepip. Setup stopped.'}
        Write-Host 'OK: Project-local Python 3.12 prepared (no system installation).'
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
    Assert-NetworkArchive $Archive
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
        Assert-NetworkDriver (Join-Path $Destination 'WinDivert64.sys')
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
    }catch{
        throw "Networking setup stopped: $($_.Exception.Message) If Windows quarantined or blocked a file, leave it blocked; setup will not restore it, change security settings or try another download source."
    }finally{
        if(Test-Path -LiteralPath $download){Remove-Item -LiteralPath $download -Force}
        foreach($name in @('WinDivert.dll','WinDivert64.sys')){
            $file=Join-Path $stage $name
            if(Test-Path -LiteralPath $file){Remove-Item -LiteralPath $file -Force}
        }
    }
}

function Ensure-Prerequisites([switch]$CheckOnly,[switch]$RequireNetworking){
    $state=Get-PrerequisiteState
    $checks=@(@('C++ Build Tools',$state.Compiler),@('Windows SDK',$state.Sdk),@('64-bit Python 3.12',[bool]$state.Python),@('WebView2 Runtime',$state.WebView))
    if($RequireNetworking){$checks+=,@('Networking files',[bool]$state.Driver)}
    foreach($item in $checks){
        if($item[1]){Write-Host "OK: $($item[0])"}else{Write-Host "MISSING: $($item[0])" -ForegroundColor Yellow}
    }
    if(!$RequireNetworking){Write-Host 'Mouse-input throwing does not require networking files.'}
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
        Install-BuildPython
        $state=Get-PrerequisiteState
        if(!$state.Python){throw 'Python 3.12 is still unavailable. Check the local build package and rerun setup.'}
    }
    if(!$state.WebView){
        Install-SetupProgram 'https://go.microsoft.com/fwlink/p/?LinkId=2124703' 'MicrosoftEdgeWebview2Setup.exe' 'O=Microsoft Corporation' @('/silent','/install')
        $state=Get-PrerequisiteState
        if(!$state.WebView){throw 'WebView2 Runtime is still unavailable. Rerun setup after the installer completes.'}
    }
    if($RequireNetworking -and !$state.Driver){
        Install-NetworkRuntime
        $state=Get-PrerequisiteState
        if(!$state.Driver){throw 'Networking files are still missing. Check the Windows security notification. Setup does not bypass blocked files.'}
    }
    return $state
}
