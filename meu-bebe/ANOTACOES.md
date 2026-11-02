# Meu Bebê: anotações do projeto

Tudo o que uma sessão nova precisa para continuar o trabalho sem a conversa original.

## Quem joga e como conversar

- Quem pediu e testa o jogo é cega e joga com leitor de tela (TalkBack no Android). Acessibilidade vem antes de tudo: cada coisa visual precisa de equivalente em texto ou som, e todo texto passa pelo leitor de tela.
- Ela testa no celular de verdade e é o ouvido do projeto: Claude não ouve áudio. Sons são conferidos com o classificador (ver Ferramentas), mas a palavra final é dela.
- Conversa em português do Brasil. Ela está viajando e manda pedidos pelo celular.
- Não suponha o gênero de quem joga nos textos do jogo (já foi corrigido um "você mesma").

## Onde está cada coisa

- `meu-bebe/web/meu-bebe.html`: o jogo inteiro num arquivo só (HTML, CSS, JavaScript e os sons embutidos). É a fonte da verdade.
- `meu-bebe/app/src/main/assets/index.html`: a mesma página, para o app. Só muda a meta viewport (`viewport-fit=cover`). Toda mudança vai nas duas.
- `meu-bebe/app/src/main/java/br/meubebe/app/`: o app Android, que só abre a página numa WebView e faz a ponte com o aparelho:
  - `MainActivity.java`: WebView, botão voltar, sensor de movimento (dança), avisa o jogo ao sair (`aoSair`) e ao voltar (`aoVoltar`).
  - `Ponte.java`: o objeto `window.Android` (agendar e cancelar avisos, vibrar, testar notificação, copiar, colar, compartilhar).
  - `Avisos.java`: canais de notificação, alarmes e notificações. `AlarmeReceiver` e `ReiniciarReceiver` mostram e reagendam.
- `meu-bebe/sons/`: os sons originais em mp3. `ferramentas/embutir_sons.py` copia todos para dentro das duas páginas.
- `meu-bebe/ferramentas/`: scripts que fabricaram as músicas e alguns sons, e o classificador de sons.
- Versão web publicada (Artifact do claude.ai): https://claude.ai/artifact/SyFsVK9JE6sXH1y64fmVcJ
  - O Artifact recebe a página sem `<!doctype>`, `<html>`, `<head>` e `<body>`: tire essas linhas do `web/meu-bebe.html` antes de publicar.
  - Para atualizar o mesmo link a partir de outra sessão, publique passando a `url` acima (leia o artifact antes).

## Como publicar uma versão nova do app

1. Edite `web/meu-bebe.html` e copie para `app/src/main/assets/index.html`, trocando a meta viewport.
2. Suba `versionCode` e `versionName` em `meu-bebe/app/build.gradle` (última: versionCode 14, versão 2.3).
3. Commit e push na `main`. O fluxo `.github/workflows/meu-bebe-apk.yml` compila sozinho em uns 2 minutos.
4. O APK fica na execução do Actions, no item `meu-bebe-apk`. Mande o link da execução para ela.
5. Lembre a ela: antes de desinstalar a versão antiga, copiar o código do bebê (Opções). Cada compilação usa uma chave de depuração nova, então o Android não instala por cima; o código do bebê resolve. A solução definitiva (chave fixa guardada como segredo do GitHub, que só ela pode criar) ficou para depois.

## Restrições do ambiente (aprendidas na prática)

- A rede da sessão depende do ambiente que ela escolheu. Mesmo com acesso completo, o Freesound responde 403 vazio para a sessão. O APK e a busca de sons rodam no GitHub Actions.
- Não rode programas binários de pacotes de terceiros (aapt2, d8 de npm): o sistema de segurança bloqueia, com razão.
- Não ponha chaves, senhas ou keystores no repositório (é público).
- Freesound: a busca roda no GitHub Actions, fluxo "Buscar sons no Freesound" (`.github/workflows/freesound.yml`), com o segredo `FREESOUND_API_KEY` do repositório (ela já criou). Dispare com `gh api -X POST repos/bibifofinhabb2012-a11y/jogos/actions/workflows/freesound.yml/dispatches -f ref=main -f "inputs[buscas]=nome | termos"`. Os sons caem na branch `sons-candidatos`, com `creditos.json` (autor, licença, link). Filtra só CC0 e CC BY. Passe no classificador e mostre a ela só os melhores, numa página com um botão por som (ela não quer ouvir dezenas um por um).

## Como o jogo funciona por dentro (resumo)

- Estado em `S`, criado por `novoEstado()`. Bebês antigos recebem os campos novos com `Object.assign(novoEstado(...), salvo)`. Para um campo novo, basta pôr o valor padrão em `novoEstado`.
- Tempo: segue o relógio real. `passo()` roda a cada segundo; `minuto()` avança um minuto do jogo. Ritmo normal: 1 minuto do jogo a cada 10 segundos reais (`VEL`). Um dia do jogo = um mês de idade.
- App fechado: `recuperar()` simula o tempo perdido em silêncio (`silencio` desliga falas e sons e conta os acontecimentos com `marca()`), e `textoAusencia()` conta o que aconteceu.
- Avisos no celular: `planejarAvisos()` simula o futuro sem sorteios e marca quando o bebê precisará de atenção (aviso a cada 10 minutos) e quando a situação fica crítica (chamado com bipes e vibração).
- Choro: `motivos()` lista o que incomoda, em ordem de prioridade; `avaliarChoro()` anuncia. Cada motivo tem um choro gravado (`arquivoChoro`).
- Falas: `falar()` usa a região polida; `alertar()` a assertiva; `acrescentar()` junta frases à fala atual; `avisos` (estoque, moedas) entram no fim da narração da ação, em `depois()`.
- Moedas: cada vez que os pontos de cuidado sobem, mais 10 moedas (`conferirMoedas`). À meia-noite, `avaliarDia()` dá 50 ou tira 20.
- Menus: objeto `MENUS`, com itens dinâmicos. Navegação por setas, letra inicial, Esc; no Android o botão voltar chama `voltarAndroid()`.
- Banho e troca de fralda interativos: tela cheia, grade de células com sujeira, vibração e som posicional onde ainda falta limpar.

## O que o jogo já tem

Versão 2.3: babá Dona Rosa (`babaCuida`, roda em `minuto()` enquanto `S.babaAte > S.min`; 10 moedas por hora real, convertida em minutos do jogo pela velocidade; com ela, `conferirMoedas` não paga e `avaliarDia` não dá bônus nem desconto; faltas de suprimento em `S.babaFaltas`, avisadas no jogo e, com o app fechado, como notificação agendada por `planejarAvisos`, que também avisa o fim do turno). Conversa pelo microfone só pelo tom: `Voz.java` grava em 16 kHz e manda a cada 64 ms volume, altura (autocorrelação), clareza e cruzamentos de zero para `window.vozQuadro`; `analisarVoz()` compara com `S.vozBase` (voz normal gravada na primeira vez) e classifica em grito, voz dura, áspera, fala de bebê, sussurro, baixinho ou normal. O áudio não é guardado. Testado só com vozes simuladas: os limites (8 dB para grito, 2,5 semitons para fala de bebê) podem precisar de ajuste com o ouvido dela.

Versão 2.2: doenças graves (hipotermia no frio, desidratação no calor, diarreia de guloseima) que pioram rápido e levam ao pronto-socorro; soro de reidratação oral; Doceria com guloseimas e consequências (agitação de açúcar, sede, gases, engasgo com pirulito, recusa de papinha, diarreia); roupas novas e a parte `maos` (luvinhas); brinquedos novos com preferido e rejeitado sorteados por bebê (`S.favorito`, `S.desgosto`); menu Conversar (chamar pelo nome, imitar, nariz, ensinar palavras, que viram fala a partir dos 9 meses); balbucio contínuo quando acordado e contente (`atualizarBalbucio`, desliga em Opções); choro de bebê maior a partir dos 4 meses (`choro-maior-forte|irritado|manha-N.mp3`); riso, arroto, goles, fralda e respiração trocados por sons do Freesound que ela escolheu. Corrigido: havia duas funções `estadoTexto`, a do banho virou `estadoParteTexto`.


Alimentação (mamadeira, arroto, papinha e fruta a partir dos 6 meses, golfada pequena ou grande), fralda (troca rápida, troca passo a passo com o dedo, sem fralda, xixi e cocô na cama, lençol), banho rápido e banho com o dedo, sono (ninar, canção, berço, respiração dormindo), colo e brincadeiras (chocalho, mordedor, pelúcia, móbile, dança balançando o celular), saúde (febre, resfriado, cólica, dentes com febrinha, assadura, gases, intestino preso, "Ajudar a soltar"), remédios com doses e intervalo do antitérmico, vacinas do calendário do posto, pediatra por telefone, pronto-socorro, roupas com clima e conforto, inventário, loja (mercado, farmácia, roupas, brinquedos), moedas, marcos de crescimento até 12 meses, perigos de quem engatinha, cocô que muda com a comida, dicas (automáticas, a pedido e do dia), código do bebê para guardar e recuperar, música de caixinha (Nuvem de Algodão, original em mi bemol; Rock-a-bye Baby; Brahms), notificações e chamados com o app fechado.

## Ideias combinadas para depois

- O bebê ouvir pelo microfone: virar a cabeça ao ouvir o nome, acalmar com canto, balbuciar de volta.
- Voz guiando as instruções, com legenda em tempo real: voz do Android (TextToSpeech, via ponte) para frases dinâmicas e, se ela quiser, gravações humanas nas instruções fixas. Cuidado para não falar por cima do TalkBack.
- Marcos depois de 1 ano.
- Outras ideias sugeridas e ainda não pedidas: passeios, visitas da avó, consulta de rotina, conquistas, madrugada com mamada.
- Chave de assinatura fixa no cofre do GitHub, para o app instalar por cima.

## Próximos jogos (conversados, não começados)

- Simulador de motorista de aplicativo (sem usar a marca Uber): passageiros com personalidade, dinheiro, carros melhores, celular como volante (inclinação) e botões de volume para acelerar e frear, som 3D em estéreo, acidentes sem mortes. Ela quer amigos dublando os passageiros (frases curtas, gravadas no celular; Claude escreve o roteiro e trata os áudios). Humor adulto e palavrão liberados; sem sexo explícito, sem piada contra grupos, sem difamar pessoas reais.
- Audiogame de sobrevivência em 3D pelo som (andar, caçar, coletar). Mais difícil no celular; começar pelo carro, que testa a mesma base.

## Créditos e licenças dos sons

Estão no próprio jogo, em "Como jogar". Resumo: choros até 4 meses, balbucio, resmungo e tosse do Donate-a-Cry Corpus (ODbL e DbCL); riso, arroto, goles, fralda, respiração, choros de bebê maior e balbucios extras do Freesound (CC0 e CC BY, com autor e link nos créditos); água da golfada do ESC-50 (CC BY-NC 3.0, o único som de uso não comercial que sobrou); pum do TerminalFart (Apache 2.0); músicas, chocalho, moeda, bipes e sons do banho sintetizados. Se o jogo um dia for vendido, falta trocar a água da golfada.
