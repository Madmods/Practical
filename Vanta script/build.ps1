# Joins src/engine/*.luau then src/ui/*.luau (strict name order) into dist/PracticalComplete.luau.
# Pure byte concatenation: fragments are slices of one script and share one scope, so nothing is rewritten.
param([string]$Verify)
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$files = New-Object System.Collections.ArrayList
foreach ($dir in 'engine','ui') {
  $names = @(Get-ChildItem -LiteralPath (Join-Path $root "src\$dir") -Filter *.luau | ForEach-Object { $_.Name })
  [Array]::Sort($names, [StringComparer]::Ordinal)
  foreach ($n in $names) { [void]$files.Add((Join-Path $root "src\$dir\$n")) }
}
$stream = New-Object System.IO.MemoryStream
$i = 0
foreach ($f in $files) {
  $i++
  $b = [IO.File]::ReadAllBytes($f)
  $stream.Write($b, 0, $b.Length)
  '{0,2}. {1}' -f $i, ($f.Substring($root.Length + 5))
}
$out = Join-Path $root 'dist\PracticalComplete.luau'
New-Item -ItemType Directory -Force -Path (Split-Path $out) | Out-Null
[IO.File]::WriteAllBytes($out, $stream.ToArray())
''
"Built $out"
"$($files.Count) files, $($stream.Length) bytes"
# Sanity: the UI string must be opened once and closed once.
$text = [Text.Encoding]::UTF8.GetString($stream.ToArray())
$open  = ([regex]::Matches($text, [regex]::Escape('loadstring([====['))).Count
$close = ([regex]::Matches($text, [regex]::Escape(']====]'))).Count
if ($open -ne 1 -or $close -ne 1) { "WARNING: UI string markers look wrong (open=$open close=$close). A file may contain ]====] or be misplaced."; exit 1 }
'Structure check OK.'
if ($Verify) {
  if ((Get-FileHash -Algorithm SHA256 $Verify).Hash -eq (Get-FileHash -Algorithm SHA256 $out).Hash) { 'Identical to original.' } else { 'Differs from original (expected if you edited something).' }
}
