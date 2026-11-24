# Offline regression checks: never download software or launch an installer.
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'prerequisites.ps1')
$script:checks=0
function Assert([bool]$Condition,[string]$Message){
    if(!$Condition){throw "FAILED: $Message"}
    $script:checks++
}
function Assert-Throws([scriptblock]$Action,[string]$Pattern){
    $caught=$null
    try{& $Action}catch{$caught=$_.Exception.Message}
    Assert ($caught -and $caught -match $Pattern) "Expected error matching $Pattern; got $caught"
}

$originals=@{}
foreach($name in @('Test-BuildPython','Get-RegisteredPython312','Get-PrerequisiteState','Install-BuildPython','Install-SetupProgram','Install-NetworkRuntime','Get-SetupDownload','Assert-SetupSignature','Assert-NetworkArchive','Assert-NetworkDriver','Expand-NetworkRuntime','Get-FileHash','Start-SetupInstaller','Invoke-WebRequest','Get-AuthenticodeSignature')){
    $command=Get-Command $name -ErrorAction SilentlyContinue
    if($command -and $command.CommandType -eq 'Function'){$originals[$name]=$command.ScriptBlock}
}
$testRoot=Join-Path ([IO.Path]::GetTempPath()) ('vanta-setup-test-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $testRoot | Out-Null
$native=$testRoot
$app=$testRoot
try{
    function Get-PrerequisiteState {return $script:state}
    function Install-SetupProgram($Url,$Name,$Publisher,$InstallerArguments){
        $script:installs+=,@($Url,$Name,$Publisher,$InstallerArguments)
        switch -Wildcard ($Name){
            'vs_*' {$script:state.Compiler=$true;$script:state.Sdk=$true}
            'MicrosoftEdge*' {$script:state.WebView=$true}
        }
    }
    function Install-BuildPython {$script:pythonPrepares++;$script:state.Python='python.exe'}
    function Install-NetworkRuntime {$script:networkInstalls++;$script:state.Driver='existing-runtime'}
    $script:state=[pscustomobject]@{VsRoot=(Join-Path $testRoot 'Build Tools');Compiler=$true;Sdk=$true;Python='python.exe';WebView=$true;Driver='existing-runtime'}
    $script:installs=@();$script:networkInstalls=0;$script:pythonPrepares=0
    $result=Ensure-Prerequisites -RequireNetworking
    Assert ($script:installs.Count -eq 0 -and $script:networkInstalls -eq 0 -and $script:pythonPrepares -eq 0) 'Existing prerequisites must skip all installers/downloads'
    $script:state.Compiler=$false;$script:state.Sdk=$false;$script:state.Python=$null;$script:state.WebView=$false;$script:state.Driver=$null
    $inputResult=Ensure-Prerequisites -CheckOnly
    Assert ($script:networkInstalls -eq 0) 'Read-only input-method check never downloads a driver'
    $result=Ensure-Prerequisites -RequireNetworking -CheckOnly
    Assert ($script:installs.Count -eq 0 -and $script:networkInstalls -eq 0 -and $script:pythonPrepares -eq 0) 'CheckOnly must stay read-only even when everything is missing'
    $result=Ensure-Prerequisites -RequireNetworking
    Assert ($script:installs.Count -eq 2 -and $script:networkInstalls -eq 1 -and $script:pythonPrepares -eq 1) 'Install each missing prerequisite once'
    Assert ($result.Compiler -and $result.Sdk -and $result.Python -and $result.WebView -and $result.Driver) 'Recheck installed state'
    Assert ($script:installs[0][3] -contains '--norestart') 'C++ installer cannot restart Windows automatically'
    Assert ($script:installs[0][3] -contains ('"'+$script:state.VsRoot+'"')) 'Quote installation paths containing spaces'
    Assert (@($script:installs | Where-Object {$_[1] -like 'python-*'}).Count -eq 0) 'Python never launches a system installer'
    Assert ($script:installs[1][0] -eq 'https://go.microsoft.com/fwlink/p/?LinkId=2124703') 'WebView2 uses the official bootstrapper'

    $devFolder=Join-Path $script:state.VsRoot 'Common7\Tools'
    New-Item -ItemType Directory -Path $devFolder -Force | Out-Null
    [IO.File]::WriteAllText((Join-Path $devFolder 'VsDevCmd.bat'),'fixture')
    $script:state.Sdk=$false;$script:installs=@();$script:networkInstalls=0;$script:pythonPrepares=0
    $result=Ensure-Prerequisites -RequireNetworking
    Assert ($script:installs.Count -eq 1 -and $script:installs[0][3][0] -eq 'modify') 'Modify an existing Build Tools installation when only its SDK is missing'
    Assert ($script:networkInstalls -eq 0) 'Keep existing networking files during an SDK installation'
    $script:state.Driver=$null
    $inputResult=Ensure-Prerequisites
    Assert ($script:networkInstalls -eq 0 -and !$inputResult.Driver) 'Input-method setup succeeds without downloading networking files'
    $script:state.Driver='existing-runtime'

    # A successful installer exit is insufficient unless detection succeeds too.
    function Install-BuildPython {}
    $script:state.Python=$null
    Assert-Throws {Ensure-Prerequisites -RequireNetworking | Out-Null} 'Python 3.12 is still unavailable'
    $script:state.Python='python.exe'

    Set-Item Function:Install-SetupProgram $originals['Install-SetupProgram']
    function Get-SetupDownload {
        $path=Join-Path $testRoot 'fake-installer.exe'
        [IO.File]::WriteAllText($path,'test fixture')
        return $path
    }
    function Assert-SetupSignature {if($script:rejectSignature){throw 'Signature verification failed'}}
    function Start-SetupInstaller {$script:launches++;return $script:installerExit}
    $script:launches=0;$script:rejectSignature=$true;$script:installerExit=0
    Assert-Throws {Install-SetupProgram 'https://example.test/setup.exe' 'setup.exe' 'Test' @()} 'Signature verification failed'
    Assert ($script:launches -eq 0) 'Reject signatures before executing anything'
    $script:rejectSignature=$false
    foreach($exitCode in @(3010,1641,1602,1223,1603,42)){
        $script:installerExit=$exitCode
        $pattern=if($exitCode -eq 3010 -or $exitCode -eq 1641){'Windows restart'}elseif($exitCode -eq 1602 -or $exitCode -eq 1223){'was cancelled'}elseif($exitCode -eq 1603){'failed \(exit 1603\).*does not imply cancellation'}else{"exit $exitCode"}
        Assert-Throws {Install-SetupProgram 'https://example.test/setup.exe' 'setup.exe' 'Test' @()} $pattern
        Assert (!(Test-Path -LiteralPath (Join-Path $testRoot 'fake-installer.exe'))) 'Remove temporary installers after failure/restart'
    }
    $script:installerExit=0
    Install-SetupProgram 'https://example.test/setup.exe' 'setup.exe' 'Test' @()
    Assert ($script:launches -eq 7) 'Only one execution per accepted installer'

    Set-Item Function:Install-BuildPython $originals['Install-BuildPython']
    Assert-Throws {Install-BuildPython} 'Python build package hash mismatch'
    Assert (!(Test-Path -LiteralPath (Join-Path $native 'third_party\python'))) 'Reject altered packages before extracting or executing Python'

    # Native exit codes must remain visible across the probe function's scope.
    $probe=Join-Path $testRoot 'probe.ps1'
    [IO.File]::WriteAllText($probe,'param([string]$c);$global:LASTEXITCODE=0')
    Assert (Test-BuildPython $probe) 'Accept a successful interpreter probe'
    [IO.File]::WriteAllText($probe,'param([string]$c);$global:LASTEXITCODE=7')
    Assert (!(Test-BuildPython $probe)) 'Reject a failed interpreter probe without using an earlier exit code'

    # Registered custom installations work without PATH or a launcher.
    $registered=Join-Path $testRoot 'registered-python.exe'
    [IO.File]::WriteAllText($registered,'fixture')
    function Get-RegisteredPython312 {return $registered}
    function Test-BuildPython($Path) {return ($Path -eq $registered)}
    function Get-Command {}
    try{Assert ((Get-Python312) -eq $registered) 'Reuse a compatible interpreter registered at a custom location'}
    finally{Remove-Item Function:Get-Command}
    Set-Item Function:Test-BuildPython $originals['Test-BuildPython']
    Set-Item Function:Get-RegisteredPython312 $originals['Get-RegisteredPython312']

    Set-Item Function:Assert-SetupSignature $originals['Assert-SetupSignature']
    function Get-AuthenticodeSignature {return $script:signature}
    $script:signature=[pscustomobject]@{Status='Valid';SignerCertificate=[pscustomobject]@{Subject='CN=Microsoft Corporation, O=Microsoft Corporation'}}
    Assert-SetupSignature 'fixture.exe' 'O=Microsoft Corporation'
    $script:signature.Status='NotSigned'
    Assert-Throws {Assert-SetupSignature 'fixture.exe' 'O=Microsoft Corporation'} 'Signature verification failed'
    $script:signature.Status='Valid'
    Assert-Throws {Assert-SetupSignature 'fixture.exe' 'O=Python Software Foundation'} 'Signature verification failed'

    # Current release identity is pinned by bytes, independently of sponsor names.
    function Get-FileHash {return [pscustomobject]@{Hash=$script:digest}}
    $script:signature=[pscustomobject]@{Status='Valid';StatusMessage='Signature verified.';SignerCertificate=[pscustomobject]@{Subject='CN=Current signing sponsor'}}
    $script:digest='8DA085332782708D8767BCACE5327A6EC7283C17CFB85E40B03CD2323A90DDC2'
    Assert-NetworkDriver 'fixture.sys'
    Assert $true 'Accept the exact current driver with a valid Windows signature, without requiring the obsolete sponsor'
    $script:digest='changed-file'
    Assert-Throws {Assert-NetworkDriver 'fixture.sys'} 'does not match the pinned'
    $script:digest='8DA085332782708D8767BCACE5327A6EC7283C17CFB85E40B03CD2323A90DDC2'
    foreach($status in @('NotSigned','HashMismatch','NotTrusted','UnknownError')){
        $script:signature.Status=$status
        Assert-Throws {Assert-NetworkDriver 'fixture.sys'} ("Windows status: "+$status)
    }
    $script:signature.Status='Valid';$script:signature.SignerCertificate=$null
    Assert-Throws {Assert-NetworkDriver 'fixture.sys'} 'signature is not valid'
    $script:digest='63CB41763BB4B20F600B6DE04E991A9C2BE73279E317D4D82F237B150C5F3F15'
    Assert-NetworkArchive 'fixture.zip'
    Assert $true 'Accept only the pinned official-release archive bytes'
    $script:digest='changed-archive'
    Assert-Throws {Assert-NetworkArchive 'fixture.zip'} 'archive hash mismatch'
    if($originals.ContainsKey('Get-FileHash')){Set-Item Function:Get-FileHash $originals['Get-FileHash']}else{Remove-Item Function:Get-FileHash}

    Set-Item Function:Get-SetupDownload $originals['Get-SetupDownload']
    $script:requests=0
    function Invoke-WebRequest {$script:requests++;throw 'Simulated blocked download'}
    Assert-Throws {Get-SetupDownload 'http://example.test/setup.exe' 'setup.exe'} 'require HTTPS'
    Assert ($script:requests -eq 0) 'Reject insecure URLs before any request'
    Assert-Throws {Get-SetupDownload 'https://example.test/setup.exe' 'setup.exe'} 'does not bypass blocked downloads'
    Assert ($script:requests -eq 1) 'Blocked download stops without retries or fallback mirrors'

    # Synthetic ZIPs prove only the expected x64 entries can be written.
    Add-Type -AssemblyName System.IO.Compression
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    function Assert-NetworkArchive {} # These ZIPs exercise extraction, not release fingerprints.
    function Assert-NetworkDriver {} # Synthetic PE fixtures have no real publisher signature.
    function New-TestZip([string]$Path,[string[]]$Entries,[int]$Machine=0x8664){
        $zip=[IO.Compression.ZipFile]::Open($Path,[IO.Compression.ZipArchiveMode]::Create)
        try{
            foreach($entryName in $Entries){
                $entry=$zip.CreateEntry($entryName)
                $stream=$entry.Open();$writer=New-Object IO.BinaryWriter($stream)
                try{
                    $bytes=New-Object byte[] 128
                    $bytes[0]=0x4d;$bytes[1]=0x5a;$bytes[0x3c]=0x40;$bytes[0x40]=0x50;$bytes[0x41]=0x45
                    $bytes[0x44]=$Machine -band 0xff;$bytes[0x45]=($Machine -shr 8) -band 0xff
                    $writer.Write($bytes)
                }finally{$writer.Dispose();$stream.Dispose()}
            }
        }finally{$zip.Dispose()}
    }
    $entries=@('WinDivert-2.2.2-A/x64/WinDivert.dll','WinDivert-2.2.2-A/x64/WinDivert64.sys','../../escape.exe')
    $zip=Join-Path $testRoot 'good.zip';$dest=Join-Path $testRoot 'good'
    New-Item -ItemType Directory -Path $dest | Out-Null
    New-TestZip $zip $entries
    Expand-NetworkRuntime $zip $dest
    Assert (@(Get-ChildItem -LiteralPath $dest).Count -eq 2) 'Extract only the two exact runtime files'
    Assert (!(Test-Path -LiteralPath (Join-Path $testRoot 'escape.exe'))) 'Never extract archive-supplied traversal paths'
    $zip=Join-Path $testRoot 'x86.zip';$dest=Join-Path $testRoot 'x86'
    New-Item -ItemType Directory -Path $dest | Out-Null
    New-TestZip $zip $entries 0x14c
    Assert-Throws {Expand-NetworkRuntime $zip $dest} 'Windows x64'
    $zip=Join-Path $testRoot 'duplicate.zip';$dest=Join-Path $testRoot 'duplicate'
    New-Item -ItemType Directory -Path $dest | Out-Null
    New-TestZip $zip @($entries[0],$entries[0],$entries[1])
    Assert-Throws {Expand-NetworkRuntime $zip $dest} 'Invalid networking archive entry'

    # Failed driver verification must never publish files to the dependency folder.
    Set-Item Function:Install-NetworkRuntime $originals['Install-NetworkRuntime']
    $script:networkRequests=0
    function Get-SetupDownload {
        $script:networkRequests++
        $file=Join-Path $testRoot 'rejected-network.zip'
        [IO.File]::WriteAllText($file,'fixture')
        return $file
    }
    function Expand-NetworkRuntime($Archive,$Destination){
        foreach($name in @('WinDivert.dll','WinDivert64.sys')){[IO.File]::WriteAllText((Join-Path $Destination $name),'fixture')}
        throw 'Windows status: NotTrusted'
    }
    Assert-Throws {Install-NetworkRuntime} 'Windows status: NotTrusted.*leave it blocked'
    Assert (!(Test-Path -LiteralPath (Join-Path $native 'third_party\windivert\bin'))) 'Do not install files after failed verification'
    Assert (!(Test-Path -LiteralPath (Join-Path $testRoot 'runtime\WinDivert64.sys'))) 'Remove rejected staging files'
    Assert ($script:networkRequests -eq 1) 'Never retry or switch download sources after a verification failure'
    Write-Host "PASS: $script:checks prerequisite checks; no downloads or installers executed." -ForegroundColor Green
}finally{
    foreach($name in $originals.Keys){Set-Item "Function:$name" $originals[$name]}
    foreach($name in @('Invoke-WebRequest','Get-AuthenticodeSignature','Get-FileHash')){
        if(!$originals.ContainsKey($name)){Remove-Item "Function:$name" -ErrorAction SilentlyContinue}
    }
    $resolved=[IO.Path]::GetFullPath($testRoot)
    $prefix=Join-Path ([IO.Path]::GetTempPath()) 'vanta-setup-test-'
    if(!$resolved.StartsWith($prefix,[StringComparison]::OrdinalIgnoreCase)){throw 'Refusing to clean an unexpected test directory.'}
    Remove-Item -LiteralPath $resolved -Recurse -Force
}
