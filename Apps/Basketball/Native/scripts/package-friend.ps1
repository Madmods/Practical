param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
$native=Split-Path $PSScriptRoot -Parent
$app=Split-Path $native -Parent
$runtime=Join-Path $native 'build\network_decoder\runtime'
$exe=Join-Path $app 'Vanta.exe'
$catalogPath=Join-Path $app 'vanta-release.json'
$catalog=Get-Content -LiteralPath $catalogPath -Raw | ConvertFrom-Json
$exeHash=(Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash.ToLowerInvariant()
if($catalog.sha256 -ne $exeHash){throw 'Executable and release catalog do not match.'}
foreach($relative in @('vanta-network-host.exe','capture-network-lab.exe','WinDivert.dll','WinDivert64.sys','_internal\python312.dll','_internal\scan_aes_schedules.dll','_internal\replication-type-maps.json','licenses\WinDivert\COPYING.LESSER.txt')){if(!(Test-Path -LiteralPath (Join-Path $runtime $relative))){throw ('Missing runtime file: '+$relative)}}
$helperHash=(Get-FileHash -LiteralPath (Join-Path $runtime 'vanta-network-host.exe') -Algorithm SHA256).Hash.ToLowerInvariant()
$name='Vanta-Friend-'+$catalog.version+'-'+$exeHash.Substring(0,8)+'-'+$helperHash.Substring(0,8)
$package=Join-Path (Join-Path $native 'build\friend-package') $name
$zipPath=Join-Path $OutputDirectory ($name+'.zip')
if((Test-Path -LiteralPath $package) -or (Test-Path -LiteralPath $zipPath)){throw 'This package already exists; nothing was overwritten.'}
New-Item -ItemType Directory -Path $package -Force | Out-Null
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
Copy-Item -LiteralPath $exe,$catalogPath -Destination $package
foreach($setting in @('bat-timing-settings.json','ff3-qb-settings.json','ff3-play-settings.json')){[IO.File]::WriteAllText((Join-Path $package $setting),'{}',[Text.UTF8Encoding]::new($false))}
$network=Join-Path $package 'network_decoder'
New-Item -ItemType Directory -Path $network | Out-Null
Copy-Item -LiteralPath $runtime -Destination $network -Recurse
foreach($file in Get-ChildItem -LiteralPath (Join-Path $network 'runtime') -File -Recurse){if($file.Name -match '^(state|plan|schema-cache|last-capture|ff3-.*)\.json$' -or $file.Extension -in @('.pcap','.pcapng','.csv','.jsonl','.py','.obj','.pdb')){throw ('Unexpected runtime file: '+$file.Name)}}
$licenses=Join-Path $package 'Licenses'
New-Item -ItemType Directory -Path $licenses | Out-Null
Copy-Item -LiteralPath (Join-Path $native 'ui\fonts\OFL.txt') -Destination (Join-Path $licenses 'Inter-OFL.txt')
Copy-Item -LiteralPath (Join-Path $native 'third_party\webview2\LICENSE.txt') -Destination (Join-Path $licenses 'WebView2-LICENSE.txt')
Copy-Item -LiteralPath (Join-Path $native 'third_party\webview2\NOTICE.txt') -Destination (Join-Path $licenses 'WebView2-NOTICE.txt')
Copy-Item -LiteralPath (Join-Path $native 'packaging\READ-ME-FIRST.txt') -Destination $package
$entries=@(Get-ChildItem -LiteralPath $package -File -Recurse | Sort-Object FullName | ForEach-Object {[ordered]@{path=$_.FullName.Substring($package.Length+1).Replace('\','/');bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()}})
$manifest=[ordered]@{product='Vanta';version=$catalog.version;executableSha256=$exeHash;helperSha256=$helperHash;settings='Fresh defaults';files=$entries}
[IO.File]::WriteAllText((Join-Path $package 'PACKAGE-MANIFEST.json'),($manifest | ConvertTo-Json -Depth 5),[Text.UTF8Encoding]::new($false))
Compress-Archive -LiteralPath $package -DestinationPath $zipPath -CompressionLevel Optimal
Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive=[IO.Compression.ZipFile]::OpenRead($zipPath)
try{
 $files=@(Get-ChildItem -LiteralPath $package -File -Recurse)
 if(@($archive.Entries | Where-Object {$_.Name}).Count -ne $files.Count){throw 'ZIP file count mismatch.'}
 foreach($file in $files){
  $relative=$file.FullName.Substring($package.Length+1).Replace('\','/')
  $entry=$archive.GetEntry($name+'/'+$relative);if(!$entry){throw ('Missing ZIP entry: '+$relative)}
  $stream=$entry.Open();$sha=[Security.Cryptography.SHA256]::Create()
  try{$actual=[BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-','')}
  finally{$stream.Dispose();$sha.Dispose()}
  if($actual -ne (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash){throw ('ZIP content mismatch: '+$relative)}
 }
}finally{$archive.Dispose()}
[ordered]@{zip=$zipPath;staged=$package;bytes=(Get-Item -LiteralPath $zipPath).Length;sha256=(Get-FileHash -LiteralPath $zipPath -Algorithm SHA256).Hash.ToLowerInvariant();executableSha256=$exeHash;helperSha256=$helperHash;verifiedFiles=$files.Count} | ConvertTo-Json
