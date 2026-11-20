param([Parameter(Mandatory=$true)][ValidateRange(0,500)][int]$DelayMs,
      [Parameter(Mandatory=$true)][string]$ResultFile)
$ErrorActionPreference='Stop'
try {
    if(-not ([Security.Principal.WindowsPrincipal]::new([Security.Principal.WindowsIdentity]::GetCurrent())).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)){throw 'Run this script as administrator'}
    $exe=(Resolve-Path (Join-Path $PSScriptRoot '..\..\..\..\Tools\LatencyTest\bin\clumsy.exe')).Path
    $players=@(Get-Process RobloxPlayerBeta -ErrorAction Stop)
    if($players.Count -ne 1){throw 'Expected one Roblox process'}
    $all=@(Get-NetUDPEndpoint)
    $ports=@($all | Where-Object {$_.OwningProcess -eq $players[0].Id -and $_.LocalAddress -notin @('127.0.0.1','::1')} | Select-Object -ExpandProperty LocalPort -Unique)
    if(!$ports){throw 'No active game UDP port'}
    foreach($port in $ports){if($all | Where-Object {$_.LocalPort -eq $port -and $_.OwningProcess -ne $players[0].Id}){throw 'Shared UDP port; cancelled'}}
    $existing=@(Get-Process clumsy -ErrorAction SilentlyContinue)
    if($existing.Count -gt 1){throw 'Multiple Clumsy instances; cancelled'}
    foreach($old in $existing){
        if($old.Path -ne $exe){throw 'Unexpected Clumsy executable; cancelled'}
        if(-not $old.CloseMainWindow() -or -not $old.WaitForExit(3000)){throw 'Existing Clumsy did not close'}
    }
    $filter='outbound and udp and !loopback and ('+(($ports | ForEach-Object {'udp.SrcPort == '+$_}) -join ' or ')+')'
    $arguments='--filter "'+$filter+'" --lag on --lag-outbound on --lag-inbound off --lag-time '+$DelayMs
    $tool=Start-Process -FilePath $exe -WorkingDirectory (Split-Path $exe) -ArgumentList $arguments -WindowStyle Normal -PassThru
    [ordered]@{ok=$true;pid=$tool.Id;delay_ms=$DelayMs;filter=$filter;automatic_timeout=$false} | ConvertTo-Json | Set-Content -LiteralPath $ResultFile
} catch {
    [ordered]@{ok=$false;error=$_.Exception.Message} | ConvertTo-Json | Set-Content -LiteralPath $ResultFile
    exit 1
}
