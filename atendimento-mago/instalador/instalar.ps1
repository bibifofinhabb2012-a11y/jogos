# Instalador do Atendimento do Mago para Windows 10 e 11.
#
# Como usar: abra o PowerShell (tecla Windows, escreva powershell, Enter), cole a linha abaixo e tecle Enter.
#   irm https://raw.githubusercontent.com/bibifofinhabb2012-a11y/jogos/atendimento-mago/atendimento-mago/instalador/instalar.ps1 | iex
#
# Não precisa de administrador. Tudo fica em C:\Users\<você>\AtendimentoMago.
# Rodar de novo atualiza o programa e mantém pedidos, ajustes e a conexão do WhatsApp.

function Instalar-AtendimentoMago {
  $ErrorActionPreference = 'Stop'
  $ProgressPreference = 'SilentlyContinue'   # barra de progresso deixa o download lento e confunde o leitor de tela
  [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
  try { [Console]::OutputEncoding = [Text.Encoding]::UTF8 } catch { }

  $repo = 'bibifofinhabb2012-a11y/jogos'
  $ramo = 'atendimento-mago'
  $base = Join-Path $env:USERPROFILE 'AtendimentoMago'
  $temp = Join-Path $env:TEMP ('atendimento-mago-' + [guid]::NewGuid().ToString('N'))
  $totalPassos = 6

  function Dizer($texto) { Write-Host ''; Write-Host $texto }
  function Passo($n, $texto) { Write-Host ''; Write-Host "Passo $n de ${totalPassos}: $texto" }

  function Perguntar($pergunta, $padrao) {
    $texto = if ($padrao) { "$pergunta [se deixar em branco: $padrao]" } else { $pergunta }
    while ($true) {
      $r = Read-Host $texto
      if (-not [string]::IsNullOrWhiteSpace($r)) { return $r.Trim() }
      if ($padrao) { return $padrao }
      Write-Host 'Essa resposta é obrigatória.'
    }
  }

  function SimOuNao($pergunta) {
    while ($true) {
      $r = (Read-Host "$pergunta Responda s para sim ou n para não").Trim().ToLower()
      if ($r.StartsWith('s')) { return $true }
      if ($r.StartsWith('n')) { return $false }
    }
  }

  function PerguntarTelefone($pergunta) {
    while ($true) {
      $d = (Read-Host "$pergunta Com DDD, por exemplo 51 99999 0000") -replace '\D', ''
      if ($d.Length -eq 10 -or $d.Length -eq 11) { $d = '55' + $d }
      if ($d.Length -eq 12 -or $d.Length -eq 13) { return $d }
      Write-Host 'Número inválido. Escreva o DDD e o número, só com algarismos.'
    }
  }

  function PerguntarSenha {
    while ($true) {
      $a = Read-Host 'Crie uma senha para o painel, com pelo menos 8 caracteres. Ela não aparece enquanto você digita' -AsSecureString
      $b = Read-Host 'Digite a mesma senha de novo' -AsSecureString
      $sa = [Runtime.InteropServices.Marshal]::PtrToStringBSTR([Runtime.InteropServices.Marshal]::SecureStringToBSTR($a))
      $sb = [Runtime.InteropServices.Marshal]::PtrToStringBSTR([Runtime.InteropServices.Marshal]::SecureStringToBSTR($b))
      if ($sa.Length -lt 8) { Write-Host 'A senha tem menos de 8 caracteres. Tente de novo.'; continue }
      if ($sa -ne $sb) { Write-Host 'As duas senhas ficaram diferentes. Tente de novo.'; continue }
      return $sa
    }
  }

  # Usa o curl.exe do Windows 10 e 11 (rápido, com tempo-limite); se não houver, o Invoke-WebRequest.
  function Baixar($url, $destino) {
    $curl = Join-Path $env:SystemRoot 'System32\curl.exe'
    for ($i = 1; $i -le 3; $i++) {
      try {
        if (Test-Path $curl) {
          & $curl -sSfL --connect-timeout 30 --speed-limit 1000 --speed-time 60 -o $destino $url
          if ($LASTEXITCODE -eq 0) { return }
          throw "curl saiu com código $LASTEXITCODE"
        }
        Invoke-WebRequest -Uri $url -OutFile $destino -UseBasicParsing -TimeoutSec 600
        return
      } catch {
        if ($i -eq 3) { throw "Não consegui baixar $url. Confira a internet e rode o instalador de novo." }
        Write-Host "O download falhou. Tentando de novo, tentativa $($i + 1) de 3."
        Start-Sleep -Seconds (3 * $i)
      }
    }
  }

  # O tar.exe do Windows 10 e 11 descompacta muito mais rápido que o Expand-Archive.
  function Extrair($zip, $destino) {
    New-Item -ItemType Directory -Force -Path $destino | Out-Null
    $tar = Join-Path $env:SystemRoot 'System32\tar.exe'
    if (Test-Path $tar) {
      & $tar -xf $zip -C $destino
      if ($LASTEXITCODE -eq 0) { return }
    }
    Expand-Archive -Path $zip -DestinationPath $destino -Force
  }

  function Parar {
    Get-CimInstance Win32_Process -ErrorAction SilentlyContinue |
      Where-Object { ($_.Name -eq 'powershell.exe' -or $_.Name -eq 'pwsh.exe') -and $_.CommandLine -like "*$base\iniciar.ps1*" } |
      ForEach-Object { Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue }
    Get-Process node, cloudflared -ErrorAction SilentlyContinue |
      Where-Object { $_.Path -and $_.Path.StartsWith($base) } |
      Stop-Process -Force -ErrorAction SilentlyContinue
    Start-Sleep -Seconds 2
  }

  function PortaLivre {
    foreach ($p in 3000, 3010, 3020, 3030, 3040) {
      $ocupada = Get-NetTCPConnection -LocalPort $p -State Listen -ErrorAction SilentlyContinue
      if (-not $ocupada) { return $p }
    }
    throw 'Não achei uma porta livre entre 3000 e 3040.'
  }

  # ------------------------------------------------------------------
  Write-Host 'Instalador do Atendimento do Mago.'
  Write-Host 'Importante: não clique dentro desta janela. Se clicar, o Windows pausa o instalador. Se acontecer, tecle Esc para continuar.'
  $configArq = Join-Path $base 'config.json'
  $atualizando = Test-Path $configArq
  if ($atualizando) {
    Dizer 'O Atendimento do Mago já está instalado neste computador. Vou atualizar o programa.'
    Write-Host 'Pedidos, ajustes, usuários e a conexão do WhatsApp continuam como estão.'
    Parar
  } else {
    Write-Host 'Vou fazer algumas perguntas, depois baixo e configuro tudo sozinho. Leva uns 5 minutos.'
  }

  New-Item -ItemType Directory -Force -Path $base, $temp | Out-Null

  # ---------- perguntas (só na primeira instalação) ----------
  if (-not $atualizando) {
    Passo 1 'perguntas'
    Dizer 'Primeiro, quem vai cuidar do painel.'
    $nome = Perguntar 'Seu nome' $null
    $usuario = (Perguntar 'Escolha um nome de usuário para entrar no painel, sem espaços' ($nome.Split(' ')[0].ToLower())) -replace '\s', ''
    $senha = PerguntarSenha
    $telefone = PerguntarTelefone 'Seu número de WhatsApp, para receber links de acesso ao painel.'

    Dizer 'Agora, o Mago.'
    $nomeMago = Perguntar 'Nome do Mago' 'Mago'
    $telMago = PerguntarTelefone 'Número do WhatsApp do Mago, o mesmo que vai atender os clientes.'

    Dizer 'Dados do Pix que o cliente vai receber. Dá para mudar depois no painel.'
    $pixTipo = Perguntar 'Tipo de chave Pix: CPF, celular, e-mail ou aleatória' 'CPF'
    $pixChave = Perguntar 'Chave Pix' $null
    $pixTitular = Perguntar 'Nome do titular da conta' $null
    $pixBanco = Perguntar 'Banco' $null

    Dizer 'O Mago vai entrar no painel pelo celular, fora de casa?'
    Write-Host 'Se sim, eu crio um endereço seguro na internet para o painel, de graça, pelo serviço Cloudflare.'
    Write-Host 'Se não, o painel só abre em aparelhos ligados no mesmo Wi-Fi deste computador.'
    $tunel = SimOuNao 'Criar o endereço na internet? Recomendado.'

    Dizer 'O computador não pode dormir, senão o assistente para de responder.'
    $semSono = SimOuNao 'Posso desligar a suspensão automática enquanto ele estiver na tomada?'

    $config = @{ porta = (PortaLivre); tunel = $tunel; usuario = $usuario }
  } else {
    $config = Get-Content $configArq -Raw -Encoding UTF8 | ConvertFrom-Json
  }

  # ---------- Node.js portátil ----------
  Passo 2 'baixando o Node.js, que roda o programa'
  $nodeExe = Join-Path $base 'node\node.exe'
  $precisaNode = $true
  if (Test-Path $nodeExe) {
    $maior = [int](((& $nodeExe --version) -replace '^v', '').Split('.')[0])
    $precisaNode = $maior -lt 22
  }
  if ($precisaNode) {
    $arq = if ($env:PROCESSOR_ARCHITECTURE -eq 'ARM64') { 'arm64' } else { 'x64' }
    Write-Host 'Procurando a versão mais nova.'
    Baixar 'https://nodejs.org/dist/index.json' "$temp\versoes.json"
    $versoes = Get-Content "$temp\versoes.json" -Raw | ConvertFrom-Json
    $v = $versoes | Where-Object { $_.lts -and ($_.files -contains "win-$arq-zip") -and ([int]($_.version.TrimStart('v').Split('.')[0]) -ge 22) } | Select-Object -First 1
    if (-not $v) { throw 'Não achei uma versão do Node.js para este computador.' }
    $nomeZip = "node-$($v.version)-win-$arq"
    Write-Host "Versão $($v.version)."
    Write-Host 'Baixando, uns 35 megabytes.'
    Baixar "https://nodejs.org/dist/$($v.version)/$nomeZip.zip" "$temp\node.zip"
    Write-Host 'Descompactando. Pode levar alguns minutos, a tela fica parada enquanto isso.'
    Extrair "$temp\node.zip" "$temp\node"
    Remove-Item (Join-Path $base 'node') -Recurse -Force -ErrorAction SilentlyContinue
    Move-Item "$temp\node\$nomeZip" (Join-Path $base 'node')
  } else {
    Write-Host 'Já estava instalado.'
  }
  $env:Path = (Join-Path $base 'node') + ';' + $env:Path

  # ---------- programa ----------
  Passo 3 'baixando o programa do assistente'
  Baixar "https://codeload.github.com/$repo/zip/refs/heads/$ramo" "$temp\programa.zip"
  Extrair "$temp\programa.zip" "$temp\programa"
  $origem = Get-ChildItem "$temp\programa" -Directory | Select-Object -First 1 | ForEach-Object { Join-Path $_.FullName 'atendimento-mago' }
  if (-not (Test-Path (Join-Path $origem 'package.json'))) { throw 'O pacote baixado não tem o programa. Avise quem fez o instalador.' }
  $programa = Join-Path $base 'programa'
  Remove-Item $programa -Recurse -Force -ErrorAction SilentlyContinue
  Move-Item $origem $programa
  foreach ($f in 'iniciar.ps1', 'parar.ps1') { Copy-Item (Join-Path $programa "instalador\$f") (Join-Path $base $f) -Force }

  Write-Host 'Instalando as peças do programa. Pode levar alguns minutos, a tela fica parada enquanto isso.'
  Push-Location $programa
  try {
    & (Join-Path $base 'node\npm.cmd') ci --omit=dev --no-audit --no-fund --loglevel=error
    if ($LASTEXITCODE -ne 0) { throw 'A instalação das peças do programa falhou. Rode o instalador de novo.' }
  } finally { Pop-Location }

  # ---------- túnel ----------
  Passo 4 'endereço do painel na internet'
  $cloudflared = Join-Path $base 'cloudflared.exe'
  if ($config.tunel) {
    if (-not (Test-Path $cloudflared)) {
      Baixar 'https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-windows-amd64.exe' $cloudflared
    }
    Write-Host 'Pronto.'
  } else {
    Write-Host 'Pulado: o painel vai funcionar só no Wi-Fi de casa.'
  }

  # ---------- usuários, Pix e ajustes do computador ----------
  Passo 5 'configurando'
  $env:PASTA_DADOS = Join-Path $base 'dados'
  $node = Join-Path $base 'node\node.exe'
  $scripts = Join-Path $programa 'scripts'
  if (-not $atualizando) {
    $env:SENHA_PAINEL = $senha
    & $node --disable-warning=ExperimentalWarning "$scripts\usuario.js" --nome $nome --usuario $usuario --numero $telefone
    $env:SENHA_PAINEL = ''
    if ($LASTEXITCODE -ne 0) { throw 'Não consegui criar o seu usuário do painel.' }
    & $node --disable-warning=ExperimentalWarning "$scripts\usuario.js" --nome $nomeMago --numero $telMago
    if ($LASTEXITCODE -ne 0) { throw 'Não consegui criar o usuário do Mago.' }
    & $node --disable-warning=ExperimentalWarning "$scripts\ajustes.js" "pix_tipo=$pixTipo" "pix_chave=$pixChave" "pix_titular=$pixTitular" "pix_banco=$pixBanco"
    if ($LASTEXITCODE -ne 0) { throw 'Não consegui salvar a configuração.' }
    $config | ConvertTo-Json | Set-Content -Path $configArq -Encoding UTF8

    if ($semSono) {
      try { powercfg /change standby-timeout-ac 0 | Out-Null; powercfg /change hibernate-timeout-ac 0 | Out-Null } catch { }
      Write-Host 'Suspensão automática desligada quando o computador está na tomada.'
    }
  }

  # Atalhos: ligar sozinho ao entrar no Windows, e três na área de trabalho.
  $bat = Join-Path $base 'iniciar.bat'
  Set-Content -Path $bat -Encoding ASCII -Value "@echo off`r`nstart `"Atendimento do Mago`" /min powershell -NoProfile -ExecutionPolicy Bypass -WindowStyle Minimized -File `"%~dp0iniciar.ps1`""
  $batParar = Join-Path $base 'parar.bat'
  Set-Content -Path $batParar -Encoding ASCII -Value "@echo off`r`npowershell -NoProfile -ExecutionPolicy Bypass -File `"%~dp0parar.ps1`"`r`ntimeout /t 3 >nul"

  $shell = New-Object -ComObject WScript.Shell
  function Atalho($pasta, $nomeAtalho, $alvo, $janela) {
    $a = $shell.CreateShortcut((Join-Path $pasta "$nomeAtalho.lnk"))
    $a.TargetPath = $alvo
    $a.WorkingDirectory = $base
    $a.WindowStyle = $janela
    $a.Save()
  }
  $areaDeTrabalho = [Environment]::GetFolderPath('Desktop')
  Atalho ([Environment]::GetFolderPath('Startup')) 'Atendimento do Mago' $bat 7
  Atalho $areaDeTrabalho 'Atendimento do Mago - ligar' $bat 7
  Atalho $areaDeTrabalho 'Atendimento do Mago - desligar' $batParar 1
  Set-Content -Path (Join-Path $areaDeTrabalho 'Atendimento do Mago - painel.url') -Encoding ASCII -Value "[InternetShortcut]`r`nURL=http://localhost:$($config.porta)/"

  # ---------- ligar ----------
  Passo 6 'ligando o assistente'
  Start-Process -FilePath $bat -WindowStyle Minimized
  $url = "http://localhost:$($config.porta)/entrar"
  $ok = $false
  for ($i = 0; $i -lt 90; $i++) {
    Start-Sleep -Seconds 1
    try { Invoke-WebRequest -Uri $url -UseBasicParsing -TimeoutSec 2 | Out-Null; $ok = $true; break } catch { }
  }
  Remove-Item $temp -Recurse -Force -ErrorAction SilentlyContinue
  if (-not $ok) {
    throw "O assistente não ligou. O registro do erro está em $base\registro.txt"
  }

  Dizer 'Pronto! O Atendimento do Mago está ligado.'
  if ($atualizando) {
    Write-Host 'Programa atualizado.'
    return
  }
  Dizer 'Falta conectar o WhatsApp do Mago. Vou abrir o painel no navegador agora.'
  Write-Host "1. Entre com o usuário $($config.usuario) e a senha que você criou."
  Write-Host '2. No menu do painel, abra WhatsApp. Digite o número do Mago e escolha Gerar código.'
  Write-Host '3. No celular do Mago, abra o WhatsApp: Mais opções, Dispositivos conectados, Conectar dispositivo, Conectar com número de telefone. Digite o código de 8 letras.'
  Write-Host '4. No painel, abra Contatos e marque como pessoais a família e os amigos do Mago, para eles não receberem o menu.'
  Dizer 'Depois que o WhatsApp estiver conectado, o Mago entra no painel pelo celular quando quiser:'
  Write-Host 'basta escrever ou ditar a palavra painel na conversa dele com ele mesmo no WhatsApp. Chega um link, ele toca e entra.'
  Dizer 'Na área de trabalho ficaram três atalhos: Atendimento do Mago - painel, - ligar e - desligar.'
  Write-Host 'O assistente liga sozinho sempre que alguém entra no Windows deste computador.'
  Write-Host "Se der algum problema, o registro fica em $base\registro.txt"
  Start-Process $url
}

try {
  Instalar-AtendimentoMago
} catch {
  Write-Host ''
  Write-Host "Erro: $($_.Exception.Message)"
  Write-Host 'Nada foi apagado. Pode rodar o instalador de novo.'
}
Write-Host ''
Read-Host 'Tecle Enter para fechar esta mensagem'
