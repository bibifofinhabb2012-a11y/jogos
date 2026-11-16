// Itens no chão (categoria Itens do Ç): frutas que caíram da árvore derrubada, o que a pessoa largou com Shift Espaço,
// bosta das vacas. Espaço de mão vazia, de frente ou em cima, pega. Bosta seca vira adubo (T, com a outra mão vazia).
#include <algorithm>
#include <cmath>
#include "audio.h"
#include "fala.h"
#include "jogo.h"
#include "util.h"

// Põe a coisa num quadrado livre perto de (cx, cy), até "raio" passos; falso se não achou lugar.
bool Jogo::espalhar(const Coisa& c, int cx, int cy, int raio) {
    for (int t = 0; t < 60; t++) {
        int px = cx + (int)std::lround(acaso(-raio, raio)), py = cy + (int)std::lround(acaso(-raio, raio));
        if ((px == cx && py == cy) || !mapa.passa(px, py)) continue;
        chao.push_back({c, px, py});
        return true;
    }
    return false;
}

// O item do chão na frente, ou, se não tiver, debaixo dos pés.
int Jogo::itemChaoPerto() const {
    int fx = x + DX[rumo], fy = y + DY[rumo];
    for (size_t i = 0; i < chao.size(); i++) if (chao[i].x == fx && chao[i].y == fy) return (int)i;
    for (size_t i = 0; i < chao.size(); i++) if (chao[i].x == x && chao[i].y == y) return (int)i;
    return -1;
}

void Jogo::pegarDoChao(int i) {
    if (mundoAtual.rfind("visita:", 0) == 0 && !souMorador()) { fala::dizer("Isso está no chão da fazenda de outra pessoa. Melhor deixar."); return; }
    Coisa c = chao[i].coisa;
    chao.erase(chao.begin() + i);
    guardar(c);
    audio::tocar("mochila", 0.6f);
    fala::dizer("Você se abaixa e pega " + nomeCoisa(c, true) + " do chão. Na mochila." +
                (c.tipo == "bosta" ? " Com a outra mão vazia, T esfarela e vira adubo." : ""));
    if (categoria == 7) { montarAlvos(); itemAlvo = alvos.empty() ? 0 : (int)alvos.size() - 1; }
}

// Shift Espaço: larga no chão, na frente (ou debaixo dos pés), o que está na mão selecionada.
void Jogo::largar() {
    Coisa& m = mao(maoSel == 0);
    if (m.tipo.empty()) { fala::dizer("A mão selecionada está vazia: não tem o que largar."); return; }
    if (m.tipo == "carteira") { fala::dizer("Largar a carteira no chão? Melhor não."); return; }
    int fx = x + DX[rumo], fy = y + DY[rumo];
    bool frente = mapa.passa(fx, fy);
    chao.push_back({m, frente ? fx : x, frente ? fy : y});
    std::string n = nomeCoisa(m, true);
    m = {};
    audio::tocar(Mapa::somPasso(mapa.em(frente ? fx : x, frente ? fy : y).chao), 0.5f, 1.3f);
    fala::dizer("Você larga " + n + (frente ? " no chão, na sua frente." : " no chão, aos seus pés.") + " Fica em Itens, no Ç.");
    if (categoria == 7) montarAlvos();
}
