param([switch]$Frosted)
$ErrorActionPreference='Stop'
$native=Split-Path $PSScriptRoot -Parent
$exe=Join-Path $native 'build\Vanta-Basketball.exe'
$argsList=@('--ui-test','--scale=1')
if(-not $Frosted){$argsList+='--no-blur'}
$started=[DateTime]::UtcNow
$process=Start-Process -FilePath $exe -ArgumentList $argsList -WindowStyle Hidden -PassThru
if(-not $process.WaitForExit(60000)){
    # Only the isolated test process created above is terminated on timeout.
    $process.Kill()
    throw 'WebView test exceeded 60 seconds.'
}
$report=Join-Path $native 'build\webui-test.json'
if(!(Test-Path -LiteralPath $report) -or (Get-Item -LiteralPath $report).LastWriteTimeUtc -lt $started){throw 'UI run did not produce a fresh report.'}
Get-Content -LiteralPath $report
if($process.ExitCode -ne 0){throw "WebView integration test failed: $($process.ExitCode)"}
if(!(Get-Content -LiteralPath $report -Raw | ConvertFrom-Json).ok){throw 'UI report did not pass.'}
Write-Output 'WebView integration tests passed. Screenshots: Native\build\webui-*.png'
