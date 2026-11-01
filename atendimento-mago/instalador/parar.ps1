# Desliga o Atendimento do Mago (o programa, o túnel e a janela que os mantém ligados).
$base = $PSScriptRoot
try { [Console]::OutputEncoding = [Text.Encoding]::UTF8 } catch { }

Get-CimInstance Win32_Process -ErrorAction SilentlyContinue |
  Where-Object { ($_.Name -eq 'powershell.exe' -or $_.Name -eq 'pwsh.exe') -and $_.CommandLine -like "*$base\iniciar.ps1*" } |
  ForEach-Object { Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue }

Get-Process node, cloudflared -ErrorAction SilentlyContinue |
  Where-Object { $_.Path -and $_.Path.StartsWith($base) } |
  Stop-Process -Force -ErrorAction SilentlyContinue

Write-Host 'Atendimento do Mago desligado. Para ligar de novo, use o atalho Atendimento do Mago - ligar.'
