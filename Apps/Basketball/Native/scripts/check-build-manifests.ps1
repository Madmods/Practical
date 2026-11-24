$ErrorActionPreference='Stop'
$native=Split-Path $PSScriptRoot -Parent
[xml]$production=Get-Content -LiteralPath (Join-Path $native 'build\webui\production.manifest') -Raw
[xml]$test=Get-Content -LiteralPath (Join-Path $native 'build\webui-test\test.manifest') -Raw
$request=$production.SelectSingleNode("//*[local-name()='requestedExecutionLevel']")
$testRequest=$test.SelectSingleNode("//*[local-name()='requestedExecutionLevel']")
if($request.level -ne 'requireAdministrator' -or $request.uiAccess -ne 'false'){throw 'Production Vanta must request administrator permission with uiAccess disabled.'}
if($testRequest.level -ne 'asInvoker' -or $testRequest.uiAccess -ne 'false'){throw 'The isolated test host must use the caller permissions.'}
Write-Output 'Embedded manifests verified: administrator startup for Vanta; caller permissions for the isolated test host.'
