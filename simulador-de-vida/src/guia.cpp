// Achar as coisas: Ç e Shift Ç trocam a categoria (móveis, portas, lugares), K e L andam pelos itens,
// do mais perto para o mais longe. O liga o rastreio: um bip soa de onde a coisa está, mais rápido
// quanto mais perto. Alt O anda sozinho até lá, abrindo as portas no caminho.
#include <algorithm>
#include <cmath>
#include <queue>
#include "audio.h"
#include "fala.h"
#include "jogo.h"
#include "util.h"

static const char* CATEGORIAS[3] = {"Móveis", "Portas e porteiras", "Lugares"};

// "a sala", "o corredor"
static std::string com(const std::string& l) {
    static const char* fem[] = {"sala", "cozinha", "varanda da casa", "calçada da casa", "horta", "mata", "estrada da fazenda", "ponte do córrego"};
    for (const char* f : fem) if (l == f) return "a " + l;
    return "o " + l;
}
static bool fora(const std::string& l) { return l == "terreiro da sede" || l == "pasto" || l == "calçada da casa" || l == "varanda da casa" || l.rfind("pasto", 0) == 0; }

static int distancia(int ax, int ay, int bx, int by) { return std::max(std::abs(ax - bx), std::abs(ay - by)); }

void Jogo::montarAlvos() {
    alvos.clear();
    if (categoria == 0) {
        for (size_t i = 0; i < moveis.size(); i++) {
            std::string n = nomeMovel((int)i);
            std::string l = mapa.nomeLugar(moveis[i].x, moveis[i].y);
            alvos.push_back({maiuscula(n) + ", " + (l == "calçada da casa" ? "na casa" : l == "box do chuveiro" ? "no box" : "no " + l), moveis[i].x, moveis[i].y, true});
            if (l == "sala" || l == "cozinha" || l == "varanda da casa") alvos.back().nome = maiuscula(n) + ", na " + l;
        }
    } else if (categoria == 1) {
        for (auto [px, py] : mapa.passagens()) {
            Obstaculo o = mapa.em(px, py).obs;
            if (mapa.valido(px - 1, py) && mapa.em(px - 1, py).obs == o) continue;  // porteira larga: conta uma vez
            // nomeia pelos lugares dos dois lados
            std::vector<std::string> lados;
            for (int d = 0; d < 8; d += 2) {
                int qx = px + DX[d], qy = py + DY[d];
                if (!mapa.valido(qx, qy) || mapa.em(qx, qy).obs != Obstaculo::Nada) continue;
                std::string l = mapa.nomeLugar(qx, qy);
                if (std::find(lados.begin(), lados.end(), l) == lados.end()) lados.push_back(l);
            }
            std::string n = maiuscula(Mapa::nome(o));
            if (px == 500 && py == 109) n = "Porta da frente da casa, entre a varanda e a sala";
            else if (px == 500 && py == 308) n = "Porta dos fundos da casa";
            else if (o == Obstaculo::Porteira && py == 5) n = "Porteira principal da fazenda";
            else if (lados.size() >= 2 && fora(lados[0]) != fora(lados[1])) { std::string d = fora(lados[0]) ? lados[1] : lados[0]; n += " d" + com(d); }
            else if (lados.size() >= 2) n += " entre " + com(lados[0]) + " e " + com(lados[1]);
            else if (lados.size() == 1) n += " d" + com(lados[0]);
            alvos.push_back({n, px, py, true});
        }
    } else {
        for (const Lugar& l : mapa.listaLugares()) {
            if (l.nome == "pasto" || l.nome.rfind("estrada municipal", 0) == 0) continue;
            int cx = (l.x0 + l.x1) / 2, cy = (l.y0 + l.y1) / 2, bx = -1, by = -1;
            for (int r = 0; r < 60 && bx < 0; r++)  // quadrado livre mais perto do meio
                for (int dy = -r; dy <= r && bx < 0; dy++)
                    for (int dx = -r; dx <= r && bx < 0; dx++)
                        if (mapa.passa(cx + dx, cy + dy) && mapa.nomeLugar(cx + dx, cy + dy).rfind(l.nome, 0) == 0) { bx = cx + dx; by = cy + dy; }
            bool repetido = std::any_of(alvos.begin(), alvos.end(), [&](const Alvo& a) { return a.nome == maiuscula(l.nome); });
            if (bx >= 0 && !repetido) alvos.push_back({maiuscula(l.nome), bx, by, false});
        }
    }
    std::sort(alvos.begin(), alvos.end(), [this](const Alvo& a, const Alvo& b) { return distancia(x, y, a.x, a.y) < distancia(x, y, b.x, b.y); });
    itemAlvo = 0;
}

std::string Jogo::descreverAlvo(const Alvo& a) const {
    int d = distancia(x, y, a.x, a.y);
    if (d == 0) return a.nome + ", aqui.";
    double ang = std::atan2((double)(a.x - x), (double)(a.y - y)) * 180 / 3.14159265;  // 0 norte, 90 leste
    int dir = ((int)std::lround(ang / 45.0) % 8 + 8) % 8;
    return a.nome + ", a " + std::to_string(d) + (d == 1 ? " metro, " : " metros, ") + relativo(dir) + ".";
}

void Jogo::trocarCategoria(int lado) {
    categoria = categoria < 0 ? (lado > 0 ? 0 : 2) : (categoria + lado + 3) % 3;
    montarAlvos();
    audio::tocar("gota", 0.6f, 0.8f);
    if (alvos.empty()) { fala::dizer(std::string(CATEGORIAS[categoria]) + ". Nada aqui."); return; }
    fala::dizer(std::string(CATEGORIAS[categoria]) + ", " + std::to_string(alvos.size()) + " itens. " + descreverAlvo(alvos[0]));
    evento("categoria");
}

void Jogo::trocarItem(int lado) {
    if (categoria < 0) { fala::dizer("Escolha primeiro uma categoria com Ç."); return; }
    if (alvos.empty()) { fala::dizer("Nada nesta categoria."); return; }
    itemAlvo = (itemAlvo + lado + (int)alvos.size()) % (int)alvos.size();
    audio::tocar("gota", 0.5f);
    fala::dizer(descreverAlvo(alvos[itemAlvo]));
    evento("item");
}

bool Jogo::chegou(const Alvo& a) const {
    if (a.chegarAoLado) return distancia(x, y, a.x, a.y) <= 1;
    std::string n = a.nome;
    if (!n.empty() && n[0] >= 'A' && n[0] <= 'Z') n[0] = (char)(n[0] + 32);  // o lugar foi guardado com maiúscula
    std::string aqui = mapa.nomeLugar(x, y);
    return aqui.rfind(n, 0) == 0 && (aqui.size() == n.size() || aqui[n.size()] == ',');  // dentro do lugar, não do vizinho
}

void Jogo::rastrear() {
    if (rastreando) { rastreando = false; caminho.clear(); fala::dizer("Rastreio desligado."); return; }
    if (categoria < 0 || alvos.empty()) { fala::dizer("Escolha primeiro o que procurar: Ç para a categoria, K e L para o item."); return; }
    rastreado = alvos[itemAlvo];
    if (chegou(rastreado)) { audio::tocar("chegou", 0.8f); fala::dizer("Você já está em " + rastreado.nome + "."); evento("chegou"); return; }
    rastreando = true; proximoBip = 0;
    fala::dizer("Rastreando: " + descreverAlvo(rastreado) + " Siga o bip. O de novo desliga; Alt O leva você até lá.");
    evento("rastrear");
}

// Caminho mais curto até o alvo, contando portas fechadas como passagem (abre ao chegar nelas).
void Jogo::irAteAlvo() {
    if (!rastreando) { rastrear(); if (!rastreando) return; }
    std::vector<int> veio(LADO * LADO, -1);
    std::queue<int> fila;
    int ini = y * LADO + x, fim = -1;
    veio[ini] = ini; fila.push(ini);
    while (!fila.empty() && fim < 0) {
        int c = fila.front(); fila.pop();
        int cx = c % LADO, cy = c / LADO;
        Alvo a = rastreado;
        if (a.chegarAoLado ? distancia(cx, cy, a.x, a.y) <= 1 : (cx == a.x && cy == a.y)) { fim = c; break; }
        for (int d = 0; d < 8; d++) {
            int nx = cx + DX[d], ny = cy + DY[d];
            if (!mapa.valido(nx, ny)) continue;
            int k = ny * LADO + nx;
            if (veio[k] >= 0) continue;
            auto livre = [this](int px, int py) { return mapa.passa(px, py) || (mapa.ehPassagem(px, py) && mapa.em(px, py).movel < 0); };
            if (!livre(nx, ny)) continue;
            if (d % 2 == 1 && (!mapa.passa(cx + DX[d], cy) || !mapa.passa(cx, cy + DY[d]))) continue;  // sem raspar quina
            veio[k] = c; fila.push(k);
        }
    }
    if (fim < 0) { fala::dizer("Não achei um caminho até lá."); return; }
    caminho.clear();
    for (int c = fim; c != ini; c = veio[c]) {
        int p = veio[c];
        int dx = c % LADO - p % LADO, dy = c / LADO - p / LADO;
        for (int d = 0; d < 8; d++) if (DX[d] == dx && DY[d] == dy) { caminho.push_back(d); break; }
    }
    std::reverse(caminho.begin(), caminho.end());
    if (postura != Postura::EmPe) { postura = Postura::EmPe; sentadoEm = -1; }
    segurando = -1;
    esperaCaminho = 0;
    fala::dizer("Indo até " + rastreado.nome + ", " + std::to_string(caminho.size()) + " passos. Qualquer tecla para.");
}

void Jogo::atualizarGuia(double dt) {
    if (!rastreando) return;
    if (chegou(rastreado)) {
        rastreando = false; caminho.clear();
        // fica de frente para a coisa
        double ang = std::atan2((double)(rastreado.x - x), (double)(rastreado.y - y)) * 180 / 3.14159265;
        if (rastreado.x != x || rastreado.y != y) rumo = ((int)std::lround(ang / 45.0) % 8 + 8) % 8;
        audio::tocar("chegou", 0.8f);
        fala::dizer("Você chegou: " + rastreado.nome + ", " + relativo(rumo) + ".");
        evento("chegou");
        return;
    }
    proximoBip -= dt;
    if (proximoBip <= 0) {
        int d = distancia(x, y, rastreado.x, rastreado.y);
        proximoBip = std::clamp(d / 12.0, 0.25, 1.3);
        audio::tocar3D("bip", rastreado.x + 0.5f, rastreado.y + 0.5f, 1.2f, 0.7f, 6);
    }
    if (!caminho.empty()) {
        esperaCaminho -= dt;
        if (esperaCaminho <= 0) {
            esperaCaminho = 0.18;
            int d = caminho.front();
            int nx = x + DX[d], ny = y + DY[d];
            if (mapa.ehPassagem(nx, ny) && !mapa.em(nx, ny).aberta) {  // abre a porta no caminho
                mapa.em(nx, ny).aberta = true;
                bool porteira = mapa.em(nx, ny).obs == Obstaculo::Porteira;
                audio::tocar3D(porteira ? "porteira" : "porta-abrir", nx + 0.5f, ny + 0.5f, 1.0f, 0.8f, 3);
            }
            rumo = d;
            int ax = x, ay = y;
            passoPara(d);
            if (x == ax && y == ay) { caminho.clear(); fala::dizer("O caminho está bloqueado."); return; }
            caminho.erase(caminho.begin());
        }
    }
}
