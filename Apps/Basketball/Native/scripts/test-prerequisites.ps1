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
foreach($name in @('Get-PrerequisiteState','Install-SetupProgram','Install-NetworkRuntime','Get-SetupDownload','Assert-SetupSignature','Start-SetupInstaller','Invoke-WebRequest','Get-AuthenticodeSignature')){
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
            'python-*' {$script:state.Python='python.exe'}
            'MicrosoftEdge*' {$script:state.WebView=$true}
        }
    }
    function Install-NetworkRuntime {$script:networkInstalls++;$script:state.Driver='existing-runtime'}
    $script:state=[pscustomobject]@{VsRoot=(Join-Path $testRoot 'Build Tools');Compiler=$true;Sdk=$true;Python='python.exe';WebView=$true;Driver='existing-runtime'}
    $script:installs=@();$script:networkInstalls=0
    $result=Ensure-Prerequisites
    Assert ($script:installs.Count -eq 0 -and $script:networkInstalls -eq 0) 'Existing prerequisites must skip all installers/downloads'
    $script:state.Compiler=$false;$script:state.Sdk=$false;$script:state.Python=$null;$script:state.WebView=$false;$script:state.Driver=$null
    $result=Ensure-Prerequisites -CheckOnly
    Assert ($script:installs.Count -eq 0 -and $script:networkInstalls -eq 0) 'CheckOnly must stay read-only even when everything is missing'
    $result=Ensure-Prerequisites
    Assert ($script:installs.Count -eq 3 -and $script:networkInstalls -eq 1) 'Install each missing prerequisite once'
    Assert ($result.Compiler -and $result.Sdk -and $result.Python -and $result.WebView -and $result.Driver) 'Recheck installed state'
    Assert ($script:installs[0][3] -contains '--norestart') 'C++ installer cannot restart Windows automatically'
    Assert ($script:installs[0][3] -contains ('"'+$script:state.VsRoot+'"')) 'Quote installation paths containing spaces'
    Assert ($script:installs[1][3] -contains 'InstallAllUsers=0') 'Python uses a per-user installation'
    Assert ($script:installs[2][0] -eq 'https://go.microsoft.com/fwlink/p/?LinkId=2124703') 'WebView2 uses the official bootstrapper'

    $devFolder=Join-Path $script:state.VsRoot 'Common7\Tools'
    New-Item -ItemType Directory -Path $devFolder -Force | Out-Null
    [IO.File]::WriteAllText((Join-Path $devFolder 'VsDevCmd.bat'),'fixture')
    $script:state.Sdk=$false;$script:installs=@();$script:networkInstalls=0
    $result=Ensure-Prerequisites
    Assert ($script:installs.Count -eq 1 -and $script:installs[0][3][0] -eq 'modify') 'Modify an existing Build Tools installation when only its SDK is missing'
    Assert ($script:networkInstalls -eq 0) 'Keep existing networking files during an SDK installation'

    # A successful installer exit is insufficient unless detection succeeds too.
    function Install-SetupProgram {}
    $script:state.Python=$null
    Assert-Throws {Ensure-Prerequisites | Out-Null} 'Python 3.12 is still unavailable'
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
    foreach($exitCode in @(3010,1641,1602,42)){
        $script:installerExit=$exitCode
        $pattern=if($exitCode -eq 3010 -or $exitCode -eq 1641){'Windows restart'}else{"exit $exitCode"}
        Assert-Throws {Install-SetupProgram 'https://example.test/setup.exe' 'setup.exe' 'Test' @()} $pattern
        Assert (!(Test-Path -LiteralPath (Join-Path $testRoot 'fake-installer.exe'))) 'Remove temporary installers after failure/restart'
    }
    $script:installerExit=0
    Install-SetupProgram 'https://example.test/setup.exe' 'setup.exe' 'Test' @()
    Assert ($script:launches -eq 5) 'Only one execution per accepted installer'

    Set-Item Function:Assert-SetupSignature $originals['Assert-SetupSignature']
    function Get-AuthenticodeSignature {return $script:signature}
    $script:signature=[pscustomobject]@{Status='Valid';SignerCertificate=[pscustomobject]@{Subject='CN=Microsoft Corporation, O=Microsoft Corporation'}}
    Assert-SetupSignature 'fixture.exe' 'O=Microsoft Corporation'
    $script:signature.Status='NotSigned'
    Assert-Throws {Assert-SetupSignature 'fixture.exe' 'O=Microsoft Corporation'} 'Signature verification failed'
    $script:signature.Status='Valid'
    Assert-Throws {Assert-SetupSignature 'fixture.exe' 'O=Python Software Foundation'} 'Signature verification failed'

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
    function Assert-SetupSignature {} # Synthetic PE fixtures have no real publisher signature.
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
    Write-Host "PASS: $script:checks prerequisite checks; no downloads or installers executed." -ForegroundColor Green
}finally{
    foreach($name in $originals.Keys){Set-Item "Function:$name" $originals[$name]}
    foreach($name in @('Invoke-WebRequest','Get-AuthenticodeSignature')){
        if(!$originals.ContainsKey($name)){Remove-Item "Function:$name" -ErrorAction SilentlyContinue}
    }
    $resolved=[IO.Path]::GetFullPath($testRoot)
    $prefix=Join-Path ([IO.Path]::GetTempPath()) 'vanta-setup-test-'
    if(!$resolved.StartsWith($prefix,[StringComparison]::OrdinalIgnoreCase)){throw 'Refusing to clean an unexpected test directory.'}
    Remove-Item -LiteralPath $resolved -Recurse -Force
}
