$ErrorActionPreference='Stop'
$native=Split-Path $PSScriptRoot -Parent
$source=Get-Content -LiteralPath (Join-Path $native 'src\vanta_release.hpp') -Raw
$version=[regex]::Match($source,'Version = L"([^"]+)"').Groups[1].Value
if(-not $version){throw 'Release version not found.'}
$exe=Join-Path $native 'build\Vanta-Basketball.exe'
$algorithm=[Security.Cryptography.SHA256]::Create()
$stream=[IO.File]::OpenRead($exe)
try{$digest=[BitConverter]::ToString($algorithm.ComputeHash($stream)).Replace('-','').ToLowerInvariant()}
finally{$stream.Dispose();$algorithm.Dispose()}
$catalog=[ordered]@{version=$version;sha256=$digest;channel='local'}
$json=$catalog | ConvertTo-Json
[IO.File]::WriteAllText((Join-Path $native 'build\vanta-release.json'),$json,[Text.UTF8Encoding]::new($false))
