param([string]$PythonPath='')
$ErrorActionPreference='Stop'
$native=Split-Path $PSScriptRoot -Parent
$version='1.0.4314.35'
$download=Join-Path $native 'third_party\.downloads\webview2.zip'
New-Item -ItemType Directory -Force -Path (Split-Path $download) | Out-Null
if(!(Test-Path -LiteralPath $download)){Invoke-WebRequest -Uri "https://api.nuget.org/v3-flatcontainer/microsoft.web.webview2/$version/microsoft.web.webview2.$version.nupkg" -OutFile $download}
if((Get-FileHash -LiteralPath $download -Algorithm SHA256).Hash -ne '90778DEB1B77E36292828B4B5833C9255C27533F40E0DDD55891AE718944EDC4'){throw 'WebView2 SDK hash mismatch.'}
Expand-Archive -LiteralPath $download -DestinationPath (Join-Path $native 'third_party\webview2') -Force
$venv=Join-Path $native '.venv'
if(!(Test-Path -LiteralPath (Join-Path $venv 'Scripts\python.exe'))){
 if($PythonPath){& $PythonPath -m venv $venv}else{& py -3.12 -m venv $venv}
 if($LASTEXITCODE){throw 'Python 3.12 environment creation failed.'}
}
& (Join-Path $venv 'Scripts\python.exe') -m pip install -r (Join-Path $native 'requirements-build.txt')
if($LASTEXITCODE){throw 'Python build dependency installation failed.'}
Write-Output 'WebView2 SDK and Python build dependencies are ready. No driver is installed.'
