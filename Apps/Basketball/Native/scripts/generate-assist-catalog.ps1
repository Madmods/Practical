$ErrorActionPreference='Stop'
$native=Split-Path $PSScriptRoot -Parent
$source=Join-Path $native 'src\ff3_assist_catalog.json'
$target=Join-Path $native 'src\ff3_assist_catalog.hpp'
$definitions=Get-Content -LiteralPath $source -Raw -Encoding UTF8 | ConvertFrom-Json
$ids=@{}
$parts=@('L"["')
foreach($definition in $definitions){
 if($definition.id -notmatch '^ff3_[a-z]+$' -or $ids.ContainsKey($definition.id)){throw 'Invalid or duplicate FF3 feature id.'}
 $ids[$definition.id]=$true
 $json=$definition | ConvertTo-Json -Depth 16 -Compress
 if($json.Contains(')YF"')){throw 'Invalid feature text delimiter.'}
 if($parts.Count -gt 1){$json=','+$json}
 $parts+='LR"YF('+$json+')YF"'
}
$parts+='L"]"'
$content="#pragma once`nnamespace ff3Assist { inline constexpr wchar_t CatalogJson[]=`n"+($parts -join "`n")+"; }`n"
if(!(Test-Path -LiteralPath $target) -or (Get-Content -LiteralPath $target -Raw -Encoding UTF8) -ne $content){[IO.File]::WriteAllText($target,$content,[Text.UTF8Encoding]::new($false))}
Write-Output ('Validated '+$definitions.Count+' FF3 feature definitions.')
