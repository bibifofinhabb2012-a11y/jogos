// Tutorial interativo: cada passo diz o que fazer e espera a pessoa fazer.
// Quando o jogo vê que foi feito, toca um sinal, diz "muito bem" e passa para o próximo.
// Para acrescentar passos: um texto e uma condição. Os eventos vêm de evento("nome") pelo jogo.
#include <algorithm>
#include "audio.h"
#include "fala.h"
#include "jogo.h"
#include "util.h"


void Jogo::evento(const std::string& e) {
    if (passoTutorial < 0) return;
    eventos.push_back(e);
    if (eventos.size() > 20) eventos.erase(eventos.begin());
}


void Jogo::comecarTutorial() {
    passoTutorial = 0;
    eventos.clear();
    esperaTutorial = 0.3;
    fala::dizer("Tutorial. Em cada passo eu digo o que fazer e espero você fazer. F2 repete a última fala. Para parar, Tab e Parar o tutorial.");
}

void Jogo::atualizarTutorial(double dt) {
    if (passoTutorial < 0) return;
    auto tem = [this](const char* e) { return std::find(eventos.begin(), eventos.end(), e) != eventos.end(); };
    struct P { const char* texto; std::function<bool()> feito; };
    static int passosNoInicio = 0;
    std::vector<P> lista = {
        {"Primeiro passo: aperte B para saber onde você está.", [&] { return tem("b"); }},
        {"Agora Shift B: o lugar, a posição e o que tem à sua frente.", [&] { return tem("olhar"); }},
        {"Vire 90 graus para a direita: Shift mais seta para a direita.", [&] { return tem("virar"); }},
        {"Agora vire 90 graus para a esquerda, de volta: Shift mais seta para a esquerda.", [&] { return tem("virar"); }},
        {"Ande uns passos para a frente: segure a seta para cima.", [&] { return contadorPassos - passosNoInicio >= 5; }},
        {"Abra a mochila com I.", [&] { return mochilaAberta; }},
        {"Com a mochila aberta, aperte a letra B. Cada vez que você aperta, ela pula para a próxima coisa que começa com B: bússola, barra.", [&] { return tem("letra"); }},
        {"Aperte B até ouvir a barra de ração, e então seta para a direita, para pôr na mão direita.", [&] { return maoDireita.tipo == "barra"; }},
        {"Feche a mochila com Esc.", [&] { return !mochilaAberta; }},
        {"Aperte 3 para ouvir o que tem nas duas mãos.", [&] { return tem("maos"); }},
        {"Abra a barra: aperte T. A outra mão está vazia e rasga a embalagem.", [&] { return maoDireita.tipo == "barra-aberta"; }},
        {"Selecione a mão direita com 2, e coma com Espaço.", [&] { return tem("comeu"); }},
        {"Aperte U para saber como está a fome.", [&] { return tem("fome"); }},
        {"Sente no chão: aperte R.", [&] { return postura == Postura::Sentado; }},
        {"Levante: R de novo.", [&] { return postura == Postura::EmPe; }},
        {"Agora vamos achar a casa. Aperte Ç até ouvir Portas e porteiras.", [&] { return categoria == 2; }},
        {"Com L para a frente e K para trás, escolha a porta da frente da casa.", [&] { return !alvos.empty() && alvos[itemAlvo].x == 500 && alvos[itemAlvo].y == 109; }},
        {"Aperte O para rastrear. Um bip vai soar de onde a porta está, mais rápido quanto mais perto. Ande na direção do bip. Se quiser, Alt O leva você até lá.", [&] { return tem("chegou"); }},
        {"Você chegou na porta. Abra com Shift Enter.", [&] { return mapa.em(500, 109).aberta; }},
        {"Entre na casa: dois passos para a frente. Depois, aperte B para confirmar que está na sala.", [&] { return tem("b") && mapa.nomeLugar(x, y) == "sala"; }},
        {"Por fim, aperte W para saber como está a limpeza do corpo, e se precisa de banho.", [&] { return tem("higiene"); }},
    };
    if (passoTutorial >= (int)lista.size()) {
        passoTutorial = -1;
        audio::tocar("tutorial-ok", 0.8f);
        fala::dizer("Fim do tutorial. Muito bem! Agora explore: com Ç, K e L você acha os móveis. A geladeira abre com Espaço e a mão vazia. "
                    "O sofá, a cama e o chuveiro estão esperando. Bom jogo.", false);
        return;
    }
    if (esperaTutorial > 0) {
        esperaTutorial -= dt;
        if (esperaTutorial <= 0) {
            if (passoTutorial == 4) passosNoInicio = contadorPassos;
            eventos.clear();
            fala::dizer(lista[passoTutorial].texto, false);
        }
        return;
    }
    if (lista[passoTutorial].feito()) {
        passoTutorial++;
        audio::tocar("tutorial-ok", 0.8f);
        fala::dizer("Muito bem!", false);
        esperaTutorial = 0.8;
    }
}
