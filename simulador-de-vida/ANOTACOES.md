# Simulador de Vida: anotações do projeto

Audiogame de sobrevivência em 3D para computador (Windows), em C++. Quem pede e testa é cega e joga com NVDA. Leia também o `CLAUDE.md` da raiz.

## A história
Aos 18 anos, o pai larga a pessoa na porteira da fazenda dele, parada há anos, numa cidadezinha do interior de Minas: "acabou a mamata". Sem aprender a coletar, plantar e cuidar de bicho, não sobrevive. Não suponha o gênero de quem joga nos textos.

## Como é feito
- `src/main.cpp`: janela SDL2, teclado, laço principal, salvar a cada minuto. `--roteiro arquivo` roda sem janela nem som e escreve as falas no terminal: é como se testa aqui.
- `src/jogo.cpp`: mochila, mãos, combinações (T), uso (Enter), fome, sede, energia, saúde, sono, relógio, passos, bússola, sons do ambiente, abertura com o pai.
- `src/mapa.cpp`: 1000 por 1000 metros (x leste, y norte). Veja a seção Mapa. Córrego por y 620 com ponte na estrada, mata no noroeste, curral, galinheiro e paiol no terreiro, cerca de arame em volta.
- `src/audio.cpp`: OpenAL Soft com HRTF (som binaural no fone). Sons em `sons/*.mp3`, mono; `nome-1.mp3`, `nome-2.mp3` são variações sorteadas.
- `src/fala.cpp`: Tolk no Windows (NVDA, JAWS, SAPI); no Linux, imprime.
- Relógio: um dia do jogo = 1 hora real (`HORAS_REAIS_POR_DIA` em `jogo.cpp`; antes eram 2 horas, ela pediu 1 pensando no online). Dormindo, 1 segundo real = 10 minutos. Fome, sede e o resto seguem o relógio do jogo.

## Compilar
- Windows: o fluxo `.github/workflows/simulador-windows.yml` compila em todo push que mexe em `simulador-de-vida/` e entrega o zip `simulador-de-vida-windows`. SDL2, OpenAL Soft e Tolk são baixados e compilados pelo CMake.
- Linux, para testar: `apt-get install libsdl2-dev libopenal-dev`, depois `cmake -S simulador-de-vida -B /tmp/sv-build -G Ninja && cmake --build /tmp/sv-build`, e `/tmp/sv-build/simulador-de-vida --roteiro roteiro.txt`.

## Sons
Do Freesound, pelo fluxo "Buscar sons no Freesound" (grupos com prefixo `sv-` na branch `sons-candidatos`). `ferramentas/preparar_sons.py <pasta sons-candidatos>` corta os passos um a um, nivela e grava em `sons/`, com `sons/CREDITOS.txt`. Passos foram conferidos com o classificador do Meu Bebê nas gravações inteiras.

## Teclas
A lista completa (a mesma do F1) fica em `ferramentas/ajuda.py`, que reescreve `Jogo::ajuda()` e o LEIA-ME. Mexeu em tecla, atualize lá e rode. Pedidos dela que definem teclas: 1 esquerda, 2 direita, 3 as duas; Espaço usa a mão selecionada; T combina; I mochila (letra pula, Esc fecha); U, S, E, W, V, Shift V; R senta e levanta, Control R deita, Shift R agacha; Control Q e Shift Q segurando para cocô e xixi; Shift Enter portas e box; Control Espaço arrasta móvel; Alt Espaço chuveiro; Ç e Shift Ç categorias, K e L itens, O rastreia, Alt O vai sozinho; Tab menu. Escolhas de Claude: Shift B olha em volta, Control seta para cima corre, Q hora, H saúde, F2 repete, F1 ajuda, Esc duas vezes sai.

## História, gente e roça (versão de novembro)
- Pai se chama Alceu; manda ir à casa do Tião. Seu Tião (caseiro) e dona Nezinha moram na casinha (x 355 a 385, y 190 a 210, porta em (370, 190)). Tião fica em (370, 187); Nezinha começa dentro, em (372, 200), e vem até (372, 187) na cena. Os dois são entradas em `moveis` com tipo "tiao" e "nezinha" (categoria Pessoas do Ç, não arrastáveis).
- A cena (`cenaChegada`, em `roca.cpp`) dispara a 3 metros do Tião, uma vez (`cenaTiao`, salvo em "historia"). As falas são as que ela escreveu, com o sotaque dela ("Aôôôba!", "tu tá um homem", "se aprochegue"). Ela escreveu o personagem como homem ("menino do Alceu"); mantive o texto dela nas falas dos dois. Fora dessas falas, o jogo segue sem gênero.
- Roça: x 510 a 570, y 590 até 3 quadrados antes do córrego; chão `Mato` vira `TerraArada` com o machado (lista `limpos`). Plantas em `plantas` (chave y*LADO+x) e em `Celula::planta`; crescem só com a terra molhada (rega vale 24 horas do jogo); prontas com 72 horas. Colheita: milho 3 espigas, feijão 4 punhados, arroz 3 feixes (com machado), e 2 sementes de volta. O prefeito compra pela caixa de remessa.
- Lugares grandes (casa da sede, casinha, roça) viram alvo com retângulo: chegar é entrar.
- Roteiros de teste aceitam `ir x y rumo` (teleporte só de teste), `salvar arquivo` e `carregar arquivo`.

## Correio, remessa, prefeito e gênero
- Escolha de homem ou mulher no jogo novo (`escolherGenero`, menu obrigatório e com pausa); salva em "genero". Jogo salvo sem escolha pergunta ao abrir. `gen("homem", "mulher")` monta as falas de Tião, Nezinha e das cartas (menino, menina; "rapaz, tu tá um homem" ou "guria, tu tá uma mulher"). Fora dessas falas, o jogo segue sem gênero.
- Caixa de correio em (502, 108), à direita da porta (olhando de fora). Cartas por id, texto montado na hora (`textoCarta`). A primeira, "manel1", chega no dia 2 às 6h.
- Caixa de remessa em (505, 8). O prefeito, Manel do Bacanal, recolhe às 22h de todo dia (`proximaColeta`, salvo em "coleta"), acordado ou dormindo; o que acontece dormindo é dito ao acordar (`aoAcordar`). Preços em `preco()` de `correio.cpp` (milho 4, feijão 3, arroz 5; já previstos pedra 1, adubo 2, leite 6). Tião não compra mais.
- Estrada para a cidade sai do terreiro para o leste até a porteira leste (994, 198 a 202, fechada). A prefeitura fica fora do mapa, por enquanto.
- Cama: dorme até as 6 da manhã seguinte (`acordarEm`), com energia cheia; só acorda antes por bexiga ou intestino no limite.

## Organização do código
- `jogo.cpp`: corpo, mochila, mãos, teclas, andar, salvar. `moveis.cpp`: móveis (tabela TIPOS), arrastar, sentar e deitar, chuveiro, geladeira e guarda-roupa (menus de recipiente). `menu.cpp`: sistema de menus (pilha de `Menu` com opções geradas na hora), menu do Tab e configurações. `guia.cpp`: categorias, K e L, rastreio com bip 3D, caminho por busca em largura (sem cortar quina). `roca.cpp`: Tião e Nezinha, cena, roça. `correio.cpp`: gênero, cartas, remessa, coleta do prefeito. `tutorial.cpp`: lista de passos com texto e condição; eventos vêm de `evento("nome")`.
- Volume por categoria em `audio.cpp` (`categoriaDe` decide pelo nome do som); salvo em `config.txt`, ao lado do `salvo.txt`.
- Móveis ocupam um quadrado e ficam em `Celula::movel`. Salvo versão 2 guarda posição dos móveis e o que tem dentro.

## Mapa (versão 2 do salvo)
1000 por 1000. Início em (500, 9), no gramado da porteira, virado para o norte. Porta da frente da casa em (500, 109), no meio da parede da varanda, a 100 quadrados; começa fechada. Casa de 200 por 200 (x 400 a 599, y 109 a 308): sala na frente, corredor no meio, cozinha e quarto a oeste, banheiro e segundo quarto a leste, porta dos fundos em (500, 308). Ela pediu "100 por 100" para o mapa com uma casa de 200 por 200; entendi 1000 por 1000 e avisei.

## Ainda não feito (próximos passos)
Pegar coisas do chão; plantar na horta; animais; ferramentas no paiol; ir à cidade comprar; ossos quebrados (o gesso já combina com água, mas não há fratura ainda); infecção (antibiótico); água do córrego sem ferver dando dor de barriga; reverberação dentro de casa; mobília.

## Download para ela
Sessões do Claude não podem criar Releases no GitHub. O zip pronto fica em `downloads/simulador-de-vida-windows.zip` (baixar o item do Actions com `gh run download`, compactar a pasta "Simulador de Vida" e substituir). Link direto, sem login: https://github.com/bibifofinhabb2012-a11y/jogos/raw/main/downloads/simulador-de-vida-windows.zip

## Online (feito em novembro, testado aqui com dois jogos na mesma máquina)
- `rede.cpp` com ENet (UDP, porta 27960). Quem hospeda é servidor e jogador (id 0) e repassa tudo. Mensagens: OLA, BEMVINDO, JOGADOR, SAIU, POS, MUNDO, CHAT, RELOGIO, PEDIR, FAZENDA, VOZ (canal 0 confiável, 1 posições, 2 voz).
- Mundos com nome igual para todos (`mundoCanonico`): "fazenda:Dono" ou "cidade". `mundoAtual` local é "fazenda" (a sua), "cidade" ou "visita:Dono". A sua fazenda fica em `fazendaGuardada` enquanto você está fora (trocada com `trocarComGuardada`): plantas crescem, prefeito recolhe, salvar grava ela.
- Visitar: o visitante gera o mesmo mapa e pede o estado ao dono (`estadoFazenda`: limpos, plantas, móveis, portas); só leitura (não planta, não colhe, não mexe nos móveis).
- Chat: `texto()` recebe SDL_TEXTINPUT; dois-pontos abre o campo; o jogo fala cada caractere. Voz: `voz.cpp`, captura OpenAL a 16 kHz, Opus 16 kbps em quadros de 20 ms, só manda quando tem som; cada jogador tem uma fonte OpenAL na posição dele (referência 3 m, máximo 40 m).
- Relógio: anfitrião manda a cada 5 s; no online dormir não acelera o tempo (energia +120 por hora do jogo dormindo). Tab não pausa no online.
- Testes: roteiros com `real N` (espera de verdade), `digitar texto`, `vozteste`, `contarvoz`. Para testar, roda um anfitrião em segundo plano e um cliente com 127.0.0.1.
- Não testado aqui: voz com microfone de verdade e o som saindo; rede entre dois computadores pela Radmin; Firewall do Windows.
- Ideias para depois: plantar e colher na fazenda dos outros com permissão; mapa da cidade maior (mercado, igreja); Tião e Nezinha na cidade.

## Bar, bebida, cigarro e casamento (novembro)
- Seu Zé (`mundo.cpp`, `PRODUTOS`): botijão (troca o gás do fogão, `Movel::gas` em segundos de chama), cerveja, maço, isqueiro, latinha de gás, pacotes de 10 sementes.
- `ESPECIES` em `roca.cpp`: milho, feijão, arroz, laranja, banana, abóbora, cenoura, cacau. Perenes (laranja, banana, cacau) voltam a dar 48 h depois de colher.
- Cigarro: `combinar` isqueiro+cigarro (40% de acender), `fumando` com tragadas; isqueiro+latinha enche. Álcool: `beberAlcool`, `alcool` cai 0,4 por hora do jogo; acima de 2,5 cambaleia e vomita.
- Casamento em `casamento.cpp`, por mensagens APP: pedido, resposta, evento-casamento, proposta-fazenda, fazenda-ok, fazenda-nao, acao, estado. Festa dura 30 min de verdade; mesa só funciona durante a festa, 5 de cada por jogador.
- Morador (`moraCom`): só roça e portas (`acaoNaFazenda` manda a ação ao dono, que aplica e devolve o estado). A fazenda antiga do morador é apagada. Morador que abre o jogo sozinho começa na cidade.
- Ainda não feito: fazer chocolate com cacau; o morador usar geladeira e guarda-roupa; divórcio.

## Pontos de 0 a 1000 e vacas (novembro)
- Fome, sede, energia e saúde continuam guardadas de 0 a 100 (double) no código e no salvo; só a fala multiplica por 10 (`porMil` em jogo.cpp). Assim nenhum limiar mudou.
- `vacas.cpp`: vacas são móveis do tipo "vaca", sempre no fim da lista (não mexe no número dos outros móveis do salvo). Salvo e estado online têm a linha `vacas N` antes das linhas `movel`. Andam um passo a cada 6 a 20 s dentro do curral; ficam paradas a 2 passos da pessoa ou quando são o alvo do O; depois de coice, saem de perto.
- Abate: faca na mão, Espaço; 40% de coice (80 a 220 pontos de saúde); senão 12 de carne crua. Carne assa no fogão aceso. Bezerro: 35% de chance por noite, até 5 vacas, só com a pessoa na própria fazenda.
- Categoria do Ç: 5 = Animais (`Alvo::movel` acompanha a vaca).
- Ainda não feito: tirar leite; vaca na fazenda de quem foi morar junto (lá só carinho).

## Dois currais, leite e couro (novembro)
- Curral de corte (681..759 × 151..229, tipo "vaca", 5) e curral de leite (681..759 × 241..319, porteira em 680,280, tipo "vaca-leite", 4). Tabela `Curral` em `vacas.cpp`.
- Salvo: as vacas não têm mais linha `movel`; vão como `bichos` (marca: apaga as vacas de fábrica) e `bicho tipo x y diaOrdenha`. Salvo antigo com `vacas N` ainda funciona.
- Ordenha: garrafa vazia (aberta ou tampada), uma vez por dia do jogo por vaca (`Movel::ordenha`), 10% de coice de 40 a 100. Vira `{"leite", 4}` (goles); bebida vazia volta a ser garrafa vazia.
- Couro: o abate dá 1 `couro`; T com couro e faca abre `menuCouro` (cinto, bolsa, chapeu-couro, botas, sela; 1 couro cada). Preços em `correio.cpp`. O mercado da cidade ainda não existe: quando existir, vender lá também.
- Ajuste: coice da vaca de corte de 60 a 130 pontos; a faca alcança a vaca de corte mais perto até 5 passos (`ALCANCE_FACA`). Outras formas de abate ficam para depois.
- Panela (`"panela"`, 25 moedas no seu Zé): por enquanto só existe; vai servir para as receitas que ela vai criar.

## Pulo, sono, chuveiro e saco de dormir (novembro)
- `pulo.cpp`: Z segurado carrega (0,2 s por nível, até 10, bip mais agudo a cada nível); seta com Z segurado escolhe a direção; soltar pula. Tempo no ar pela gravidade (2·√(2h/9,8)); distância = metade da altura (mínimo 1), para antes de obstáculo. Dentro de casa, teto: altura 2 e 15 pontos de saúde. O ouvinte 3D sobe junto (`alturaNoAr`). Roteiro de teste: `apertar z`, `largar z`.
- Dormindo, tecla não acorda (só U, S, E, H, V, W, Q, F1, F2, Esc respondem). Acorda com energia 100 (1000 na fala), de manhã na cama, ou por bexiga, intestino, fome ou sede. Dá para dormir com energia até 90 (900).
- Chuveiro: Espaço de mão vazia de frente para ele (Alt Espaço continua funcionando).

## Objetos e pomar (novembro)
- `Mapa::objetos` (struct `Objeto`): coisas fixas do mapa, achadas no Ç na categoria 6, Objetos. Por enquanto, as 5 árvores do pomar (macieira, laranjeira, cerejeira, bananeira, mangueira), sorteadas com semente fixa (2026) dentro do pomar, a pelo menos 8 passos uma da outra. Não vão no salvo: o mapa gera sempre igual.
- A grade antiga de árvores sem nome do pomar saiu. Árvore bloqueia o passo e é nomeada ao bater e no Shift B; Espaço descreve (`olharArvore`, roca.cpp).
- Ideia para depois: colher fruta das árvores.

## Picareta, pedras, lenha e frutas (novembro)
- `Chao::Pedras`: 30% do mato da roça (sorteio fixo 77). Picareta arranca (vira terra limpa, entra em `limpos`, dá 1 pedra). Seu Tião dá machado e picareta na cena; quem não tem ganha ao conversar.
- Objetos (categoria 6) lista as árvores do pomar e cada quadrado de matagal ou pedra da roça; depois de limpar, a lista se refaz (L vai para o mais perto).
- Machado em árvore (fora do pomar): 5 golpes (`golpes`), 4 de lenha; `cortadas` vai no salvo, no estado online e na troca de fazenda; ação de morador "cortada".
- Pomar (`arvores.cpp`): `EstadoArvore` por objeto (dia, bits das frutas em 6 galhos), salvo como `fruta i dia bits`. Na virada do dia, 1 a 4 frutas em galhos sorteados. Radar: Alt K/L (0 tronco, 1 a 6 galhos, som 3D no lado e na altura), Alt O mira, Espaço pega. O radar vale enquanto a pessoa não se mexe nem vira.
- `maiuscula` agora sobe também letra com acento (á, é...).

## Itens no chão, árvores que brotam, jogo novo (novembro)
- Categoria 7, Itens: `chao` (vector de `ItemChao`), no salvo como `chao tipo carga x y`, trocado junto com a fazenda (`Mundo::chao`). Não vai para quem visita. Shift Espaço larga; Espaço de mão vazia pega (frente, depois debaixo dos pés).
- Bosta: cada vaca, em média uma a cada 40 minutos de verdade, até 25 no chão. T com a outra mão vazia vira adubo; adubo na planta: +24 horas de crescimento.
- Objetos com nome: 5 do pomar + 30 no pasto (carvalho, bétula, ipê, eucalipto, cedro; sorteio fixo 31, 40 passos entre eles). Derrubados: `EstadoArvore::renasce` (1200 s de verdade, conta mesmo longe da fazenda), salvo no 4º campo de `fruta`. Bananeira não se corta. Frutas do dia caem em volta (`espalhar`).
- Objetos também lista "terra pronta para plantar" (terra arada sem planta) na roça.
- Menu de início (`menuInicio`, menu.cpp) quando há salvo; "Começar um jogo novo" no Tab (fora do online). `novo()` agora zera também casamento, viagem, fazenda guardada, chão, álcool, cigarro, tutorial. Roteiro: comando `inicio`.
- Dormindo sem energia, bexiga e intestino não acordam mais (evitava acordar e desmaiar sem parar).
