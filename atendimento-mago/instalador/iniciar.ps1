# Liga o Atendimento do Mago e o mantém ligado: se o programa cair, liga de novo sozinho.
# Também cria o endereço do painel na internet (túnel gratuito da Cloudflare), se foi escolhido na instalação.
$ErrorActionPreference = 'Continue'
try { [Console]::OutputEncoding = [Text.Encoding]::UTF8 } catch { }
$base = $PSScriptRoot

$trava = New-Object System.Threading.Mutex($false, 'Local\AtendimentoMago')
if (-not $trava.WaitOne(0)) {
  Write-Host 'O Atendimento do Mago já está ligado.'
  Start-Sleep -Seconds 3
  return
}

try { $Host.UI.RawUI.WindowTitle = 'Atendimento do Mago. Não feche esta janela.' } catch { }
$config = Get-Content (Join-Path $base 'config.json') -Raw -Encoding UTF8 | ConvertFrom-Json
$porta = [string]$config.porta
$registro = Join-Path $base 'registro.txt'
$env:Path = (Join-Path $base 'node') + ';' + $env:Path
$env:PASTA_DADOS = Join-Path $base 'dados'
$env:PORTA = $porta

function Escrever($texto) {
  $linha = (Get-Date -Format 'dd/MM HH:mm:ss') + ' ' + $texto
  Write-Host $linha
  Add-Content -Path $registro -Value $linha -Encoding UTF8
}

function Ligar-Tunel {
  $log = Join-Path $base 'tunel.txt'
  Remove-Item $log -ErrorAction SilentlyContinue
  $p = Start-Process -FilePath (Join-Path $base 'cloudflared.exe') `
    -ArgumentList @('tunnel', '--no-autoupdate', '--url', "http://127.0.0.1:$porta") `
    -RedirectStandardError $log -RedirectStandardOutput (Join-Path $base 'tunel-saida.txt') `
    -WindowStyle Hidden -PassThru
  for ($i = 0; $i -lt 60; $i++) {
    Start-Sleep -Seconds 1
    if (Test-Path $log) {
      $achado = Select-String -Path $log -Pattern 'https://[a-z0-9-]+\.trycloudflare\.com' | Select-Object -First 1
      if ($achado) { return @{ processo = $p; endereco = $achado.Matches[0].Value } }
    }
    if ($p.HasExited) { break }
  }
  return @{ processo = $p; endereco = $null }
}

function IpLocal {
  $ip = Get-NetIPAddress -AddressFamily IPv4 -ErrorAction SilentlyContinue |
    Where-Object { $_.IPAddress -notlike '127.*' -and $_.IPAddress -notlike '169.254.*' -and $_.PrefixOrigin -ne 'WellKnown' } |
    Select-Object -First 1
  if ($ip) { return $ip.IPAddress } else { return 'localhost' }
}

while ($true) {
  # Registro grande demais vira registro-antigo.txt
  if ((Test-Path $registro) -and (Get-Item $registro).Length -gt 5MB) {
    Move-Item $registro (Join-Path $base 'registro-antigo.txt') -Force
  }

  $tunel = $null
  if ($config.tunel) {
    Escrever 'Criando o endereço do painel na internet.'
    $tunel = Ligar-Tunel
    $env:HOST = '127.0.0.1'
    if ($tunel.endereco) {
      $env:ENDERECO_PUBLICO = $tunel.endereco
    } else {
      Escrever 'Não consegui criar o endereço na internet. Por enquanto, o painel abre só neste computador.'
      $env:ENDERECO_PUBLICO = "http://localhost:$porta"
    }
  } else {
    $env:HOST = '0.0.0.0'
    $env:ENDERECO_PUBLICO = "http://$(IpLocal):$porta"
  }
  Set-Content -Path (Join-Path $base 'endereco-do-painel.txt') -Value $env:ENDERECO_PUBLICO -Encoding UTF8
  Escrever "Endereço do painel: $env:ENDERECO_PUBLICO"
  Escrever 'Assistente ligado. Esta janela pode ficar minimizada.'

  & (Join-Path $base 'node\node.exe') --disable-warning=ExperimentalWarning (Join-Path $base 'programa\src\index.js') 2>&1 |
    ForEach-Object { $l = $_.ToString(); Write-Host $l; Add-Content -Path $registro -Value $l -Encoding UTF8 }

  if ($tunel -and $tunel.processo -and -not $tunel.processo.HasExited) {
    Stop-Process -Id $tunel.processo.Id -Force -ErrorAction SilentlyContinue
  }
  Escrever 'O assistente parou. Ligando de novo em 10 segundos.'
  Start-Sleep -Seconds 10
}
