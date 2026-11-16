// Vacas: o curral de corte (tipo "vaca", abate com faca, dá carne e couro) e o curral de leite
// (tipo "vaca-leite", ordenha uma vez por dia com garrafa vazia). Qualquer uma pode dar coice.
// As vacas ficam no fim da lista de móveis da fazenda; no salvo, cada uma é uma linha "bicho".
#include <algorithm>
#include <cmath>
#include <sstream>
#include "audio.h"
#include "fala.h"
#include "jogo.h"
#include "util.h"

struct Curral { const char* tipo; int x0, y0, x1, y1, maximo; };
static const Curral CORTE = {"vaca", 683, 153, 757, 227, 5};        // dentro da cerca do curral de corte
static const Curral LEITE = {"vaca-leite", 683, 243, 757, 317, 4};  // dentro da cerca do curral de leite
static const Curral& curralDe(const std::string& t) { return t == "vaca-leite" ? LEITE : CORTE; }
bool ehVaca(const std::string& t) { return t == "vaca" || t == "vaca-leite"; }

int Jogo::contarVacas(const std::string& tipo) const {
    return (int)std::count_if(moveis.begin(), moveis.end(), [&](const Movel& m) { return m.tipo == tipo; });
}

bool Jogo::poeVaca(const std::string& tipo, int vx, int vy) {
    const Curral& c = curralDe(tipo);
    for (int t = 0; t < 200 && vx < 0; t++) {
        int ax = (int)acaso(c.x0, c.x1 + 0.99), ay = (int)acaso(c.y0, c.y1 + 0.99);
        if (mapa.passa(ax, ay) && !(ax == x && ay == y)) { vx = ax; vy = ay; }
    }
    if (vx < 0 || !mapa.valido(vx, vy)) return false;
    Movel v; v.tipo = tipo; v.x = vx; v.y = vy; v.ciclo = acaso(3, 15);
    moveis.push_back(v);
    mapa.em(vx, vy).movel = (int16_t)(moveis.size() - 1);
    return true;
}

void Jogo::ajustarVacas(int n) {  // salvos antigos: "vacas N", só as de corte
    n = std::clamp(n, 0, CORTE.maximo);
    while (contarVacas("vaca") > n) {
        for (int i = (int)moveis.size() - 1; i >= 0; i--) if (moveis[i].tipo == "vaca") { moveis.erase(moveis.begin() + i); break; }
    }
    ocupar();
    while (contarVacas("vaca") < n && poeVaca("vaca")) {}
}

std::string Jogo::linhasBichos() const {
    std::ostringstream f;
    f << "bichos\n";  // marca: a lista abaixo substitui as vacas de fábrica
    for (const Movel& m : moveis)
        if (ehVaca(m.tipo)) f << "bicho " << m.tipo << ' ' << m.x << ' ' << m.y << ' ' << m.ordenha << '\n';
    return f.str();
}

void Jogo::lerBicho(const std::string& linha) {
    std::istringstream s(linha);
    std::string k, tipo; s >> k;
    if (k == "bichos") {
        moveis.erase(std::remove_if(moveis.begin(), moveis.end(), [](const Movel& m) { return ehVaca(m.tipo); }), moveis.end());
        ocupar();
        return;
    }
    int bx, by, o = -1; s >> tipo >> bx >> by >> o;
    if (!ehVaca(tipo) || !mapa.valido(bx, by) || mapa.em(bx, by).movel >= 0) return;
    if (poeVaca(tipo, bx, by)) moveis.back().ordenha = o;
}

void Jogo::nascerBezerro() {
    if (mundoAtual != "fazenda") return;
    for (const Curral* c : {&CORTE, &LEITE}) {
        if (contarVacas(c->tipo) >= c->maximo || acaso(0, 1) > 0.35 || !poeVaca(c->tipo)) continue;
        std::string onde = c == &LEITE ? "no curral de leite" : "no curral de corte";
        if (dormindo) aoAcordar.push_back("Nasceu um bezerro " + onde + ".");
        else fala::dizer("Vem um mugido fino " + std::string(c == &LEITE ? "do curral de leite" : "do curral de corte") + ": nasceu um bezerro.", false);
    }
}

void Jogo::atualizarVacas(double seg) {
    if (mundoAtual != "fazenda") return;
    for (size_t i = 0; i < moveis.size(); i++) {
        Movel& v = moveis[i];
        if (!ehVaca(v.tipo)) continue;
        const Curral& c = curralDe(v.tipo);
        bool fugir = v.ciclo < -0.5;  // acabou de dar coice: sai de perto
        v.ciclo -= seg;
        if (v.ciclo > 0) continue;
        v.ciclo = acaso(6, 20);
        // com gente do lado, a vaca fica parada, para dar tempo de pegar a faca, a garrafa ou fazer carinho
        if (!fugir && std::max(std::abs(v.x - x), std::abs(v.y - y)) <= 2) continue;
        if (!fugir && rastreando && rastreado.x == v.x && rastreado.y == v.y) continue;  // a que você está seguindo com O espera
        if (acaso(0, 1) < 0.3) audio::tocar3D("mugido", v.x + 0.5f, v.y + 0.5f, 1.2f, 0.9f, 25, (float)acaso(0.9, 1.1));
        int d = (int)acaso(0, 7.99);
        int nx = v.x + DX[d], ny = v.y + DY[d];
        if (nx < c.x0 || nx > c.x1 || ny < c.y0 || ny > c.y1 || !mapa.passa(nx, ny) || (nx == x && ny == y)) continue;
        mapa.em(v.x, v.y).movel = -1;
        v.x = nx; v.y = ny;
        mapa.em(nx, ny).movel = (int16_t)i;
    }
}

void Jogo::carinhoVaca(int i) {
    audio::tocar3D("mugido", moveis[i].x + 0.5f, moveis[i].y + 0.5f, 1.2f, 0.8f, 25);
    bool leite = moveis[i].tipo == "vaca-leite";
    fala::dizer(leite ? "Você passa a mão no pescoço da vaca de leite, malhada e mansa. Ela vira a cabeça e cheira a sua mão."
                      : "Você passa a mão no lombo da vaca de corte, quente e de pelo curto. Ela abana o rabo e muge baixinho.");
}

// Coice: tira pontos de saúde (0 a 1000) e a vaca sai de perto.
void Jogo::coice(Movel& v, int minimo, int maximo, const std::string& susto) {
    int dano = (int)acaso(minimo, maximo + 0.99);
    saude = std::max(0.0, saude - dano / 10.0);
    audio::tocar("coice", 1.0f);
    audio::tocar3D("mugido", v.x + 0.5f, v.y + 0.5f, 1.2f, 1.0f, 25, 1.15f);
    static const char* ONDE[] = {"na coxa", "na barriga", "no joelho", "na canela"};
    fala::dizer(susto + " e te acerta um coice " + std::string(ONDE[(int)acaso(0, 3.99)]) +
                "! Você perde " + std::to_string(dano) + " pontos de saúde. Saúde: " + std::to_string((int)std::lround(saude * 10)) + " de 1000." +
                (saude < 30 ? " Mais um desses e você não levanta." : ""));
    v.ciclo = -1;
}

void Jogo::usarFaca() {
    int i = movelFrente();
    if (i < 0 || !ehVaca(moveis[i].tipo)) {
        fala::dizer("Uma faca de abate, afiada. Para abater: de frente para uma vaca do curral de corte, Espaço. Cuidado com o coice. "
                    "Com o couro numa mão e a faca na outra, T faz objetos de couro.");
        return;
    }
    if (moveis[i].tipo == "vaca-leite") { fala::dizer("Essa é vaca de leite: não se abate. Ela dá leite todo dia; traga uma garrafa vazia."); return; }
    if (mundoAtual != "fazenda") { fala::dizer("A vaca não é sua. Quem abate é o dono."); return; }
    if (energia < 10) { fala::dizer("Sem força para segurar uma vaca. Descanse antes."); return; }
    Movel& v = moveis[i];
    energia = std::max(0.0, energia - 3);
    if (acaso(0, 1) < 0.4) { coice(v, 80, 220, "A vaca se assusta com a faca, vira o corpo"); return; }
    audio::tocar("faca", 0.8f);
    audio::tocar3D("baque", v.x + 0.5f, v.y + 0.5f, 0.3f, 1.0f, 15);
    mapa.em(v.x, v.y).movel = -1;
    moveis.erase(moveis.begin() + i);
    ocupar();
    guardar({"carne", 0}, 12);
    guardar({"couro", 0}, 1);
    higiene = std::max(0.0, higiene - 20);
    energia = std::max(0.0, energia - 7);
    int resta = contarVacas("vaca");
    fala::dizer("Você segura a vaca firme pelo cabresto e faz o corte certeiro. Ela cai de lado, com um baque. "
                "Você tira o couro inteiro e corta a carne: 12 pedaços de carne crua e um couro de vaca, na mochila. A roupa ficou suja de sangue. " +
                std::string(resta ? "No curral de corte " + std::string(resta == 1 ? "resta 1 vaca." : "restam " + std::to_string(resta) + " vacas.")
                              : "O curral de corte ficou vazio. De vez em quando nasce um bezerro."));
}

// Garrafa vazia (aberta ou fechada) de frente para a vaca de leite: ordenha, uma vez por dia cada vaca.
bool Jogo::ordenhar(Coisa& c) {
    int i = movelFrente();
    if (i < 0 || !ehVaca(moveis[i].tipo)) return false;
    if ((c.tipo != "garrafa" && c.tipo != "garrafa-aberta") || c.carga != 0) {
        fala::dizer("Para tirar leite, a garrafa precisa estar vazia.");
        return true;
    }
    Movel& v = moveis[i];
    if (v.tipo == "vaca") { fala::dizer("Essa é vaca de corte: não está dando leite. As de leite ficam no curral de leite."); return true; }
    if (mundoAtual.rfind("visita:", 0) == 0 && !moradorDe(mundoAtual.substr(7))) { fala::dizer("A vaca não é sua. Quem tira o leite é o dono."); return true; }
    int hoje = (int)(minutos / 1440);
    if (v.ordenha == hoje) { fala::dizer("Essa vaca já foi ordenhada hoje. Amanhã ela dá de novo; tente outra."); return true; }
    if (acaso(0, 1) < 0.1) { coice(v, 40, 100, "A vaca se incomoda com a mão fria, levanta a pata"); return true; }
    v.ordenha = hoje;
    c = {"leite", 4};
    audio::tocar("leite", 0.8f);
    energia = std::max(0.0, energia - 2);
    fala::dizer("Você agacha do lado da vaca, segura as tetas e puxa, num ritmo: o leite sai morno e espumado, chiando na garrafa. Garrafa cheia de leite.");
    return true;
}

// Couro numa mão e faca na outra, T: escolhe o que fazer. Cada objeto gasta um couro.
void Jogo::menuCouro() {
    Menu m;
    m.titulo = "Fazer com o couro. Cada objeto gasta um couro";
    m.opcoes = [this]() {
        static const std::pair<const char*, const char*> OBJ[] = {
            {"cinto", "Cinto de couro"}, {"bolsa", "Bolsa de couro"}, {"chapeu-couro", "Chapéu de couro"},
            {"botas", "Par de botas de couro"}, {"sela", "Sela de couro, para cavalo"},
        };
        std::vector<Opcao> v;
        for (const auto& [tipo, nome] : OBJ)
            v.push_back({nome, [this, tipo = std::string(tipo), nome = std::string(nome)]() {
                Coisa* couro = maoDireita.tipo == "couro" ? &maoDireita : maoEsquerda.tipo == "couro" ? &maoEsquerda : nullptr;
                if (!couro) { fala::dizer("Cadê o couro? Segure o couro numa mão e a faca na outra."); return; }
                if (energia < 5) { fala::dizer("Cansaço demais para trabalhar o couro. Descanse antes."); return; }
                Coisa usado = *couro; *couro = {};
                if (tirar(usado)) *couro = usado;  // pega outro couro da mochila, se tiver
                guardar({tipo, 0});
                energia = std::max(0.0, energia - 5);
                audio::tocar("faca", 0.8f);
                while (!menus.empty()) fecharMenu();
                fala::dizer("Você estica o couro, risca o molde, corta com a faca e costura com tento. Pronto: " + nomeCoisa({tipo, 0}, true) +
                            ", na mochila. Vende na caixa de remessa." + std::string(couro->tipo == "couro" ? "" : " O couro acabou."));
            }, nullptr, nullptr});
        return v;
    };
    abrirMenu(m);
}
