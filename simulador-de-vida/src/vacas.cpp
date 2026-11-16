// Vacas do curral: andam devagar, mugem, e se abatem com a faca. Na hora do abate, a vaca pode dar coice.
// As vacas ficam no fim da lista de móveis da fazenda, para não mudar o número dos outros móveis no salvo.
#include <algorithm>
#include <cmath>
#include "audio.h"
#include "fala.h"
#include "jogo.h"
#include "util.h"

static const int CX0 = 683, CY0 = 153, CX1 = 757, CY1 = 227;  // dentro da cerca do curral
static const int MAX_VACAS = 5;

int Jogo::contarVacas() const {
    return (int)std::count_if(moveis.begin(), moveis.end(), [](const Movel& m) { return m.tipo == "vaca"; });
}

bool Jogo::poeVaca() {
    for (int t = 0; t < 200; t++) {
        int vx = (int)acaso(CX0, CX1 + 0.99), vy = (int)acaso(CY0, CY1 + 0.99);
        if (!mapa.passa(vx, vy) || (vx == x && vy == y)) continue;
        Movel v; v.tipo = "vaca"; v.x = vx; v.y = vy; v.ciclo = acaso(3, 15);
        moveis.push_back(v);
        mapa.em(vx, vy).movel = (int16_t)(moveis.size() - 1);
        return true;
    }
    return false;
}

void Jogo::ajustarVacas(int n) {
    n = std::clamp(n, 0, MAX_VACAS);
    while (contarVacas() > n) {
        for (int i = (int)moveis.size() - 1; i >= 0; i--) if (moveis[i].tipo == "vaca") { moveis.erase(moveis.begin() + i); break; }
    }
    ocupar();
    while (contarVacas() < n && poeVaca()) {}
}

void Jogo::nascerBezerro() {
    if (mundoAtual != "fazenda" || contarVacas() >= MAX_VACAS || acaso(0, 1) > 0.35) return;
    if (!poeVaca()) return;
    if (dormindo) aoAcordar.push_back("Nasceu um bezerro no curral."); else fala::dizer("Do curral vem um mugido fino: nasceu um bezerro.", false);
}

void Jogo::atualizarVacas(double seg) {
    if (mundoAtual != "fazenda") return;
    for (size_t i = 0; i < moveis.size(); i++) {
        Movel& v = moveis[i];
        if (v.tipo != "vaca") continue;
        bool fugir = v.ciclo < -0.5;  // acabou de dar coice: sai de perto
        v.ciclo -= seg;
        if (v.ciclo > 0) continue;
        v.ciclo = acaso(6, 20);
        // com gente do lado, a vaca fica parada, para dar tempo de pegar a faca ou fazer carinho
        if (!fugir && std::max(std::abs(v.x - x), std::abs(v.y - y)) <= 2) continue;
        if (!fugir && rastreando && rastreado.x == v.x && rastreado.y == v.y) continue;  // a que você está seguindo com O espera
        if (acaso(0, 1) < 0.3) audio::tocar3D("mugido", v.x + 0.5f, v.y + 0.5f, 1.2f, 0.9f, 25, (float)acaso(0.9, 1.1));
        int d = (int)acaso(0, 7.99);
        int nx = v.x + DX[d], ny = v.y + DY[d];
        if (nx < CX0 || nx > CX1 || ny < CY0 || ny > CY1 || !mapa.passa(nx, ny) || (nx == x && ny == y)) continue;
        mapa.em(v.x, v.y).movel = -1;
        v.x = nx; v.y = ny;
        mapa.em(nx, ny).movel = (int16_t)i;
    }
}

void Jogo::carinhoVaca(int i) {
    audio::tocar3D("mugido", moveis[i].x + 0.5f, moveis[i].y + 0.5f, 1.2f, 0.8f, 25);
    fala::dizer("Você passa a mão no lombo da vaca, quente e de pelo curto. Ela abana o rabo e muge baixinho.");
}

void Jogo::usarFaca() {
    int i = movelFrente();
    if (i < 0 || moveis[i].tipo != "vaca") {
        fala::dizer("Uma faca de abate, afiada. Para abater: de frente para uma vaca, no curral, Espaço. Cuidado com o coice.");
        return;
    }
    if (mundoAtual != "fazenda") { fala::dizer("A vaca não é sua. Quem abate é o dono."); return; }
    if (energia < 10) { fala::dizer("Sem força para segurar uma vaca. Descanse antes."); return; }
    Movel& v = moveis[i];
    energia = std::max(0.0, energia - 3);
    if (acaso(0, 1) < 0.4) {  // coice
        int dano = (int)acaso(80, 220.99);  // pontos de saúde, de 0 a 1000
        saude = std::max(0.0, saude - dano / 10.0);
        audio::tocar("coice", 1.0f);
        audio::tocar3D("mugido", v.x + 0.5f, v.y + 0.5f, 1.2f, 1.0f, 25, 1.15f);
        static const char* ONDE[] = {"na coxa", "na barriga", "no joelho", "na canela"};
        fala::dizer("A vaca se assusta com a faca, vira o corpo e te acerta um coice " + std::string(ONDE[(int)acaso(0, 3.99)]) +
                    "! Você perde " + std::to_string(dano) + " pontos de saúde. Saúde: " + std::to_string((int)std::lround(saude * 10)) + " de 1000." +
                    (saude < 30 ? " Mais um desses e você não levanta." : ""));
        v.ciclo = -1;  // ela sai de perto
        return;
    }
    audio::tocar("faca", 0.8f);
    audio::tocar3D("baque", v.x + 0.5f, v.y + 0.5f, 0.3f, 1.0f, 15);
    mapa.em(v.x, v.y).movel = -1;
    moveis.erase(moveis.begin() + i);
    ocupar();
    guardar({"carne", 0}, 12);
    higiene = std::max(0.0, higiene - 20);
    energia = std::max(0.0, energia - 7);
    int resta = contarVacas();
    fala::dizer("Você segura a vaca firme pelo cabresto e faz o corte certeiro. Ela cai de lado, com um baque. "
                "Você limpa e corta a carne: 12 pedaços de carne crua, na mochila. A roupa ficou suja de sangue. " +
                std::string(resta ? "Ainda " + std::string(resta == 1 ? "resta 1 vaca" : "restam " + std::to_string(resta) + " vacas") + " no curral."
                              : "O curral ficou vazio. De vez em quando nasce um bezerro."));
}
