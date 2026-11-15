$ErrorActionPreference='Stop'
$native=Split-Path $PSScriptRoot -Parent
$exe=Join-Path $native 'build\Vanta-Basketball.exe'
$started=[DateTime]::UtcNow
$process=Start-Process -FilePath $exe -ArgumentList '--hud-test' -WorkingDirectory $native -WindowStyle Hidden -PassThru
if(-not $process.WaitForExit(60000)){$process.Kill();throw 'Isolated QB HUD test timed out.'}
$report=Join-Path $native 'build\qb-html-test.json'
if(!(Test-Path -LiteralPath $report) -or (Get-Item -LiteralPath $report).LastWriteTimeUtc -lt $started){throw 'HUD test did not produce a fresh report.'}
$result=Get-Content -LiteralPath $report -Raw|ConvertFrom-Json
if($process.ExitCode -ne 0 -or !$result.ok){throw 'QB HUD test failed.'}
Write-Output "$(@($result.checks).Count) HTML HUD checks passed; aim/result previews saved in build."
