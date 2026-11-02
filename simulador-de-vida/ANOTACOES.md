# Simulador de Vida: anotações do projeto

Audiogame de sobrevivência em 3D para computador (Windows), em C++. Quem pede e testa é cega e joga com NVDA. Leia também o `CLAUDE.md` da raiz.

## A história
Aos 18 anos, o pai larga a pessoa na porteira da fazenda dele, parada há anos, numa cidadezinha do interior de Minas: "acabou a mamata". Sem aprender a coletar, plantar e cuidar de bicho, não sobrevive. Não suponha o gênero de quem joga nos textos.

## Como é feito
- `src/main.cpp`: janela SDL2, teclado, laço principal, salvar a cada minuto. `--roteiro arquivo` roda sem janela nem som e escreve as falas no terminal: é como se testa aqui.
- `src/jogo.cpp`: mochila, mãos, combinações (T), uso (Enter), fome, sede, energia, saúde, sono, relógio, passos, bússola, sons do ambiente, abertura com o pai.
- `src/mapa.cpp`: 500 por 500 metros (x leste, y norte). Porteira principal no sul (x 248 a 252, y 5), início em (245, 9) virado para o norte, casa sede em x 235 a 265, y 205 a 225 com cinco cômodos, curral, galinheiro, paiol, horta, pomar, córrego serpenteando por y 330 com ponte na estrada, mata no noroeste, cerca de arame em volta.
- `src/audio.cpp`: OpenAL Soft com HRTF (som binaural no fone). Sons em `sons/*.mp3`, mono; `nome-1.mp3`, `nome-2.mp3` são variações sorteadas.
- `src/fala.cpp`: Tolk no Windows (NVDA, JAWS, SAPI); no Linux, imprime.
- Relógio: 1 segundo real = 12 segundos do jogo (um dia = 2 horas reais). Dormindo, 1 segundo real = 10 minutos.

## Compilar
- Windows: o fluxo `.github/workflows/simulador-windows.yml` compila em todo push que mexe em `simulador-de-vida/` e entrega o zip `simulador-de-vida-windows`. SDL2, OpenAL Soft e Tolk são baixados e compilados pelo CMake.
- Linux, para testar: `apt-get install libsdl2-dev libopenal-dev`, depois `cmake -S simulador-de-vida -B /tmp/sv-build -G Ninja && cmake --build /tmp/sv-build`, e `/tmp/sv-build/simulador-de-vida --roteiro roteiro.txt`.

## Sons
Do Freesound, pelo fluxo "Buscar sons no Freesound" (grupos com prefixo `sv-` na branch `sons-candidatos`). `ferramentas/preparar_sons.py <pasta sons-candidatos>` corta os passos um a um, nivela e grava em `sons/`, com `sons/CREDITOS.txt`. Passos foram conferidos com o classificador do Meu Bebê nas gravações inteiras.

## Teclas (definidas por ela)
I mochila (setas escolhem, direita e esquerda põem nas mãos). 1 e 2 selecionam a mão e dizem o que tem; duas vezes esvaziam. Espaço usa a coisa da mão selecionada (come, bebe, bússola diz a direção, dorme). T combina: barra fechada + mão vazia abre a barra na mesma mão e a embalagem some; garrafa + mão vazia abre, e a tampa vai para a outra mão; garrafa aberta + tampa tampa, e a garrafa fica na mão em que estava; mãos vazias batem palmas. U fome, S sede, E energia (dormindo: "energia em N por cento, você está dormindo"). Setas cima e baixo andam; esquerda e direita andam de lado; Control + lado vira 45 graus; Shift + lado vira 90; Shift + cima ou baixo vira 180. Dormindo toca ruído branco.
Escolhidas por Claude: Control + cima corre, L olha em volta e dá a posição, Q hora, H saúde, R repete, F1 ajuda, Esc duas vezes sai. A bússola só funciona na mão (não tem mais tecla B).

## Ainda não feito (próximos passos)
Pegar coisas do chão e de lugares; plantar na horta; animais; ferramentas no paiol; ir à cidade comprar; ossos quebrados (o gesso já combina com água, mas não há fratura ainda); infecção (antibiótico); água do córrego sem ferver dando dor de barriga; reverberação dentro de casa; mobília.

## Download para ela
Sessões do Claude não podem criar Releases no GitHub. O zip pronto fica em `downloads/simulador-de-vida-windows.zip` (baixar o item do Actions com `gh run download`, compactar a pasta "Simulador de Vida" e substituir). Link direto, sem login: https://github.com/bibifofinhabb2012-a11y/jogos/raw/main/downloads/simulador-de-vida-windows.zip
