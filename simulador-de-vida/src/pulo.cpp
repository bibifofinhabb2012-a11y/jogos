// Pulo: segure Z para carregar (um bip por nível, até 10 de altura) e solte para pular.
// Com Z apertado, uma seta escolhe a direção; cada toque a mais na mesma seta é um passo a mais (até 10).
// Sem seta, pula no lugar. O pulo passa por cima das plantas da roça, mas não cai em cima delas.
// Dentro de casa, o teto segura o pulo em 2 de altura.
#include <algorithm>
#include <cmath>
#include "audio.h"
#include "fala.h"
#include "jogo.h"
#include "util.h"

static const double SEG_POR_NIVEL = 0.2;
static const int ALTURA_MAX = 10;

void Jogo::comecarPulo() {
    if (carregandoPulo || noAr > 0) return;
    if (postura != Postura::EmPe) { fala::dizer("Para pular, fique de pé: aperte R."); return; }
    carregandoPulo = true; cargaPulo = 0; nivelPulo = 1; dirPulo = -1; passosPulo = 0;
    andandoFrente = andandoTras = false; andandoLado = 0;
    audio::tocar("bip", 0.4f, 0.75f);
}

void Jogo::direcaoPulo(Tecla t) {
    int rel = t == Tecla::Cima ? 0 : t == Tecla::Direita ? 2 : t == Tecla::Baixo ? 4 : 6;
    int d = (rumo + rel) % 8;
    if (d != dirPulo) { dirPulo = d; passosPulo = 1; }
    else passosPulo = std::min(10, passosPulo + 1);
    audio::tocar("gota", 0.5f, 0.9f + 0.05f * passosPulo);
    fala::dizer(std::string(rel == 0 ? "Para a frente" : rel == 2 ? "Para a direita" : rel == 4 ? "Para trás" : "Para a esquerda") +
                ", " + std::to_string(passosPulo) + (passosPulo == 1 ? " passo." : " passos."));
}

void Jogo::pular() {
    if (!carregandoPulo) return;
    carregandoPulo = false;
    if (postura != Postura::EmPe) return;
    if (energia < 3) { fala::dizer("Sem força nas pernas para pular."); return; }
    int h = std::clamp(nivelPulo, 1, ALTURA_MAX);
    bool teto = mapa.dentro(x, y) && h > 2;
    if (teto) h = 2;
    alturaPulo = h;
    tempoNoAr = noAr = 2.0 * std::sqrt(2.0 * h / 9.8);  // subir e descer, com a gravidade de verdade
    pousoX = x; pousoY = y; bateuEm.clear();
    plantaNoPouso.clear();
    if (dirPulo >= 0) {
        int dist = std::max(1, passosPulo);
        int d = dirPulo;
        // planta não segura o pulo: passa por cima; só não dá para cair em cima dela
        auto livre = [this](int px, int py) { return mapa.valido(px, py) && (mapa.passa(px, py) || (mapa.em(px, py).planta && mapa.em(px, py).obs == Obstaculo::Nada && mapa.em(px, py).movel < 0)); };
        int cx = x, cy = y;
        for (int k = 0; k < dist; k++) {
            int nx = cx + DX[d], ny = cy + DY[d];
            bool quina = d % 2 == 1 && (!livre(cx + DX[d], cy) || !livre(cx, cy + DY[d]));
            if (!livre(nx, ny) || quina) {
                int i = mapa.valido(nx, ny) ? mapa.em(nx, ny).movel : -1;
                bateuEm = i >= 0 ? nomeMovel(i, true) : quina ? "a quina" : !mapa.valido(nx, ny) ? "o fim do mapa" : "a " + std::string(Mapa::nome(mapa.em(nx, ny).obs));
                break;
            }
            cx = nx; cy = ny;
            if (mapa.passa(cx, cy)) { pousoX = cx; pousoY = cy; plantaNoPouso.clear(); }
            else plantaNoPouso = nomePlanta(mapa.em(cx, cy).planta);  // caiu em planta: volta para o último chão livre
        }
    }
    energia = std::max(0.0, energia - 0.3 * h);
    sede = std::max(0.0, sede - 0.05 * h);
    audio::tocar(Mapa::somPasso(mapa.em(x, y).chao), 1.0f, 0.85f);  // o impulso dos pés no chão
    evento("pulou");
    if (teto) {
        saude = std::max(0.0, saude - 1.5);
        audio::tocar("parede", 0.9f, 0.8f);
        fala::dizer("Você pula e bate a cabeça no teto! Perde 15 pontos de saúde. Dentro de casa, o pulo vai só até 2 de altura.");
    }
}

void Jogo::atualizarPulo(double seg) {
    if (carregandoPulo) {
        if (postura != Postura::EmPe || dormindo) { carregandoPulo = false; return; }
        cargaPulo += seg;
        int n = std::min(ALTURA_MAX, 1 + (int)(cargaPulo / SEG_POR_NIVEL));
        if (n > nivelPulo) {
            nivelPulo = n;
            audio::tocar("bip", n == ALTURA_MAX ? 0.7f : 0.4f, 0.75f + 0.08f * n);
        }
    }
    if (noAr <= 0) return;
    noAr -= seg;
    if (noAr > 0) return;
    noAr = 0;
    int andou = std::max(std::abs(pousoX - x), std::abs(pousoY - y));
    x = pousoX; y = pousoY;
    audio::tocar("aterrissar", std::min(1.0f, 0.4f + 0.06f * alturaPulo));
    audio::tocar(Mapa::somPasso(mapa.em(x, y).chao), 0.7f);
    std::string msg = "Pulo de " + std::to_string(alturaPulo) + " de altura.";
    if (andou > 0) {
        int rel = ((dirPulo - rumo) % 8 + 8) % 8;
        const char* para = rel == 0 ? "à frente" : rel == 2 ? "à direita" : rel == 4 ? "para trás" : rel == 6 ? "à esquerda" : "na diagonal";
        msg += " Você cai " + std::to_string(andou) + (andou == 1 ? " passo " : " passos ") + para + ".";
    }
    if (!plantaNoPouso.empty())
        msg += " Não dá para cair em cima do " + plantaNoPouso + ": você cai antes dele. Aperte a seta mais vezes para pular mais longe.";
    if (!bateuEm.empty()) {
        audio::tocar("parede", 0.6f);
        msg += (andou == 0 ? " Você bate em " : " Parou ao bater em ") + bateuEm + (andou == 0 ? " e cai no mesmo lugar." : ".");
    }
    std::string l = mapa.nomeLugar(x, y);
    if (andou > 0 && l != lugarAtual) { lugarAtual = l; msg += " " + maiuscula(l) + "."; }
    fala::dizer(msg, false);
}

// Altura da cabeça acima do normal, durante o pulo (para o som em 3D).
float Jogo::alturaNoAr() const {
    if (noAr <= 0 || tempoNoAr <= 0) return 0;
    double p = 1.0 - noAr / tempoNoAr;
    return (float)(4.0 * alturaPulo * p * (1.0 - p));
}
