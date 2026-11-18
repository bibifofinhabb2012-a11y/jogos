#!/bin/bash
# Roda os roteiros de teste e confere frases que precisam aparecer.
# Uso, de dentro de simulador-de-vida/:  bash testes/rodar.sh /tmp/sv-build/simulador-de-vida
# Os roteiros que carregam jogo salvo usam caminhos relativos a simulador-de-vida/ (testes/salvos/...).
# Alguns testes têm sorte no meio (cigarro que não acende, coice da vaca): se falhar uma vez, rode de novo antes de procurar erro.
cd "$(dirname "$0")/.." || exit 1
BIN="${1:-/tmp/sv-build/simulador-de-vida}"
falhas=0
confere() {  # confere roteiro "frase" ["outra frase" ...]
    local rot="$1"; shift
    local saida; saida=$(timeout 300 "$BIN" --roteiro "testes/$rot" 2>&1)
    for frase in "$@"; do
        if grep -qF -- "$frase" <<< "$saida"; then echo "ok    $rot: $frase"; else echo "FALHA $rot: $frase"; falhas=$((falhas + 1)); fi
    done
}
confere tutorial.txt "Fim do tutorial"
confere maquinas.txt "Um punhado de café torrado" "Um pacote de pó de café, na mochila" "Dá para 10 doses" "Uma xícara de café com leite vaporizado, na mochila" \
    "queijo minas branquinho" "Dois copos de garapa" "Um saco de farinha de trigo" "Um saco de açúcar"
confere pulo-plantas.txt "Não dá para cair em cima do pé de milho" "Você cai 2 passos à frente"
confere faca.txt "faz o corte certeiro"
confere portas-e-terra.txt "Lista das portas de dentro" "Terra pronta para plantar."
confere objetos-roca.txt "Terra pronta para plantar, na roça, aqui"
confere porteira.txt "5 metros à frente" "5 metros atrás"
confere arvore-pomar.txt "Bananeira não dá madeira" "a macieira cai com um estrondo" "Toco de macieira"
confere obras.txt "está pronto: chiqueiro" "está pronto: galinheiro de luxo"
confere pecuarista.txt "3 bichos novos no lugar deles"
confere galinha.txt "Você segura a galinha velha pelas asas"
confere remedio.txt "Abaixo de 500: precisa de remédio ou atadura" "mais 250 pontos de saúde"
echo; [ $falhas -eq 0 ] && echo "Tudo certo." || echo "$falhas falha(s)."
exit $falhas
