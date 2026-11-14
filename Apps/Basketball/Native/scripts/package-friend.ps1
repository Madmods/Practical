$ErrorActionPreference = 'Stop'
$native = Split-Path $PSScriptRoot -Parent
$app = Split-Path $native -Parent
$project = Split-Path (Split-Path $app -Parent) -Parent
$exe = Join-Path $app 'Vanta.exe'
$catalogPath = Join-Path $app 'vanta-release.json'
$catalog = Get-Content -LiteralPath $catalogPath -Raw | ConvertFrom-Json
$exeHash = (Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash
if ($catalog.sha256 -ne $exeHash) { throw 'Executable and release catalog do not match.' }
$settings = Join-Path $app 'bat-timing-settings.json'
$parsedSettings = Get-Content -LiteralPath $settings -Raw | ConvertFrom-Json
foreach ($property in $parsedSettings.PSObject.Properties) {
    if ($property.Value -isnot [ValueType]) { throw 'Unexpected nonnumeric gameplay setting; review before sharing.' }
}
$name = 'Vanta-Basketball-Friend-' + $catalog.version + '-' + $exeHash.Substring(0,8)
$share = Join-Path $project 'Share'
$package = Join-Path $share $name
$zipPath = Join-Path $share ($name + '.zip')
if ((Test-Path -LiteralPath $package) -or (Test-Path -LiteralPath $zipPath)) {
    throw 'This package already exists. Keep it or choose a new package name; nothing was overwritten.'
}
New-Item -ItemType Directory -Path $package -Force | Out-Null
Copy-Item -LiteralPath $exe,$catalogPath,$settings -Destination $package
$licenses = Join-Path $package 'Licenses'
New-Item -ItemType Directory -Path $licenses | Out-Null
Copy-Item -LiteralPath (Join-Path $native 'ui\fonts\OFL.txt') -Destination (Join-Path $licenses 'Inter-OFL.txt')
Copy-Item -LiteralPath (Join-Path $native 'third_party\webview2\LICENSE.txt') -Destination (Join-Path $licenses 'WebView2-LICENSE.txt')
Copy-Item -LiteralPath (Join-Path $native 'third_party\webview2\NOTICE.txt') -Destination (Join-Path $licenses 'WebView2-NOTICE.txt')
$readme = @'
VANTA BASKETBALL - FRIEND PACKAGE

SETUP
1. Right-click the ZIP and choose Extract All.
2. Keep Vanta.exe and vanta-release.json together in the extracted folder.
3. Run Vanta.exe on a Windows x64 PC. Do not run it from inside the ZIP.
4. This local-preview build uses username 1 and password 1.
5. Join Practical Basketball or Football Fusion 3 and let the four checks finish.
6. Click Continue. The current game selects its own workspace automatically.
Football Fusion 3 currently has detection and a separate workspace only;
gameplay features have not been added. Basketball retains its existing features.
Switching games pauses Basketball inputs and requires Continue for the new game.

CONTROLS AND SETTINGS
The included bat-timing-settings.json contains the sender's gameplay settings.
Adaptive Shot Learning is on by default. Enable Auto Time and play normally:
three consistent uncontested Early/Late results adjust your own release timing.
Perfect results retain calibration; unclear/contested results do not train it.
It learns again each session and when ping/shot conditions change. Initial
shots may need adjustment; timing across all PCs is not yet live-verified.
The automatic percentage window can move beyond its starting 77-81% range.
No manual per-friend calibration is required. Appearance/profile storage is local
to each Windows account and is not included. Always on top defaults on under
Settings > Appearance. Auto guard uses G with the included settings.

REQUIREMENTS
Microsoft Edge WebView2 Runtime must be installed. If the app reports that it
is missing, get the Evergreen Runtime from Microsoft's official page:
https://developer.microsoft.com/en-us/microsoft-edge/webview2/
The UI, fonts and WebView2 loader are embedded in the executable.
No separate Visual C++ runtime DLL imports were found in this build.

UPDATES AND SUPPORT
This is a local-preview build; no account/licensing server is configured.
Its compatibility check requires a supported client version. If an update
makes that check fail, ask the sender for a matching new build.
Replace Vanta.exe and vanta-release.json together when updating.
This package does not include source code, developer tools, logs, or accounts.

Recent changes: subtle ball-side guarding, private-gym partner recognition,
earlier block qualification and always-on-top settings. Live block success
and timing across different machines/pings have not been fully verified.
'@
Set-Content -LiteralPath (Join-Path $package 'READ-ME-FIRST.txt') -Value $readme -Encoding utf8
Compress-Archive -LiteralPath $package -DestinationPath $zipPath -CompressionLevel Optimal

# Validate every archived file against the staged package, without running it.
Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive = [IO.Compression.ZipFile]::OpenRead($zipPath)
try {
    $entries = @($archive.Entries | Where-Object { $_.Name })
    $files = @(Get-ChildItem -LiteralPath $package -File -Recurse)
    if ($entries.Count -ne $files.Count) { throw 'ZIP file count mismatch.' }
    foreach ($file in $files) {
        $relative = $file.FullName.Substring($package.Length + 1).Replace('\','/')
        $entry = $archive.GetEntry($name + '/' + $relative)
        if (!$entry) { throw "Missing ZIP entry: $relative" }
        $stream = $entry.Open()
        $sha = [Security.Cryptography.SHA256]::Create()
        try { $actual = [BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-','') }
        finally { $stream.Dispose(); $sha.Dispose() }
        if ($actual -ne (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash) {
            throw "ZIP hash mismatch: $relative"
        }
    }
} finally { $archive.Dispose() }
[ordered]@{
    zip=$zipPath
    bytes=(Get-Item -LiteralPath $zipPath).Length
    sha256=(Get-FileHash -LiteralPath $zipPath -Algorithm SHA256).Hash
    executableSha256=$exeHash
    verifiedFiles=$files.Count
} | ConvertTo-Json
