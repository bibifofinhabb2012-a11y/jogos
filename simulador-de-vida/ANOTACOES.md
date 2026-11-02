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
- Relógio: 1 segundo real = 12 segundos do jogo (um dia = 2 horas reais). Dormindo, 1 segundo real = 10 minutos.

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
