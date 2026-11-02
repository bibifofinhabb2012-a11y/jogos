#include "mapa.h"
#include <cmath>
#include <random>

// Fazenda do pai, no interior de Minas. Porteira principal no sul, casa sede no meio,
// córrego cortando de oeste a leste, pasto e mata ao norte.
Mapa::Mapa() : c(LADO * LADO) {
    std::mt19937 sorte(18);  // sempre o mesmo mapa
    std::uniform_real_distribution<float> u(0, 1);

    // fora da cerca, ao sul: a estrada municipal de terra
    retangulo(0, 0, LADO - 1, 4, Chao::Terra);
    // gramado em volta da sede e da porteira
    retangulo(200, 178, 300, 262, Chao::Grama);
    retangulo(230, 6, 270, 22, Chao::Grama);
    // estrada da fazenda: da porteira até a casa, e da casa até o fundo do pasto
    retangulo(248, 6, 252, 197, Chao::Terra);
    retangulo(248, 228, 252, 488, Chao::Terra);
    // horta e pomar
    retangulo(205, 150, 230, 175, Chao::TerraArada);
    retangulo(275, 150, 320, 185, Chao::Grama);
    for (int y = 152; y <= 183; y += 6)
        for (int x = 277; x <= 318; x += 6) em(x, y).obs = Obstaculo::Arvore;
    // mata no noroeste: chão de folhas e árvores
    for (int y = 262; y <= 492; y++)
        for (int x = 6; x <= 150; x++) {
            em(x, y).chao = Chao::Folhas;
            if (u(sorte) < 0.16f) em(x, y).obs = Obstaculo::Arvore;
        }
    // árvores soltas no pasto
    for (int i = 0; i < 260; i++) {
        int x = 10 + (int)(u(sorte) * 480), y = 30 + (int)(u(sorte) * 460);
        Celula& k = em(x, y);
        if (k.chao == Chao::Pasto && k.obs == Obstaculo::Nada && std::abs(x - 250) > 4) k.obs = Obstaculo::Arvore;
    }
    // córrego: serpenteia por volta de y = 330, com 3 metros de largura
    for (int x = 0; x < LADO; x++) {
        int yc = 330 + (int)std::lround(12 * std::sin(x / 40.0));
        for (int y = yc - 1; y <= yc + 1; y++) {
            em(x, y).chao = Chao::Agua;
            em(x, y).obs = (x >= 248 && x <= 252) ? Obstaculo::Nada : Obstaculo::Corrego;
            if (x >= 248 && x <= 252) em(x, y).chao = Chao::Madeira;  // ponte
        }
    }
    // casa sede: calçada em volta, varanda, paredes e cômodos
    retangulo(232, 198, 268, 228, Chao::Calcada);
    retangulo(235, 205, 265, 225, Chao::Taco);
    retangulo(250, 206, 264, 214, Chao::Ceramica);   // cozinha
    retangulo(250, 216, 256, 224, Chao::Ceramica);   // banheiro
    contorno(235, 205, 265, 225, Obstaculo::Parede);
    for (int y = 206; y <= 224; y++) em(249, y).obs = Obstaculo::Parede;   // sala | cozinha, quarto | banheiro
    for (int x = 236; x <= 264; x++) em(x, 215).obs = Obstaculo::Parede;   // frente | fundos
    for (int y = 216; y <= 224; y++) em(257, y).obs = Obstaculo::Parede;   // banheiro | segundo quarto
    abrir(244, 205, Obstaculo::Porta, Chao::Taco);       // porta da frente, na sala
    abrir(249, 210, Obstaculo::Porta, Chao::Taco);       // sala -> cozinha
    abrir(242, 215, Obstaculo::Porta, Chao::Taco);       // sala -> quarto
    abrir(253, 215, Obstaculo::Porta, Chao::Ceramica);   // cozinha -> banheiro
    abrir(261, 215, Obstaculo::Porta, Chao::Taco);       // cozinha -> segundo quarto
    abrir(258, 225, Obstaculo::Porta, Chao::Taco);       // porta dos fundos, no segundo quarto
    // curral, galinheiro e paiol
    retangulo(300, 210, 340, 250, Chao::Terra);
    contorno(300, 210, 340, 250, Obstaculo::CercaMadeira);
    abrir(300, 230, Obstaculo::Porteira, Chao::Terra);
    retangulo(280, 235, 288, 241, Chao::Terra);
    contorno(280, 235, 288, 241, Obstaculo::Tela);
    abrir(284, 235, Obstaculo::Porta, Chao::Terra);
    retangulo(200, 230, 214, 244, Chao::Terra);
    contorno(200, 230, 214, 244, Obstaculo::Parede);
    abrir(207, 230, Obstaculo::Porta, Chao::Terra);
    // cerca de arame em volta da fazenda, com a porteira principal no sul
    contorno(5, 5, LADO - 6, LADO - 6, Obstaculo::Cerca);
    for (int x = 248; x <= 252; x++) abrir(x, 5, Obstaculo::Porteira, Chao::Terra);

    // do mais específico para o mais geral
    lugares = {
        {"sala da casa", 236, 206, 248, 214, true},
        {"cozinha", 250, 206, 264, 214, true},
        {"quarto", 236, 216, 248, 224, true},
        {"banheiro", 250, 216, 256, 224, true},
        {"segundo quarto", 258, 216, 264, 224, true},
        {"varanda da casa", 232, 198, 268, 204, false},
        {"calçada da casa", 232, 198, 268, 228, false},
        {"galinheiro", 281, 236, 287, 240, true},
        {"paiol", 201, 231, 213, 243, true},
        {"curral", 301, 211, 339, 249, false},
        {"horta", 205, 150, 230, 175, false},
        {"pomar", 275, 150, 320, 185, false},
        {"ponte do córrego", 248, 300, 252, 360, false},
        {"gramado da porteira principal", 230, 6, 270, 22, false},
        {"terreiro da sede", 200, 178, 300, 262, false},
        {"estrada da fazenda", 248, 6, 252, 488, false},
        {"mata", 6, 262, 150, 492, false},
        {"estrada municipal, do lado de fora da fazenda", 0, 0, LADO - 1, 4, false},
        {"pasto", 0, 0, LADO - 1, LADO - 1, false},
    };
}

void Mapa::retangulo(int x0, int y0, int x1, int y1, Chao ch) {
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) { em(x, y).chao = ch; em(x, y).obs = Obstaculo::Nada; }
}
void Mapa::contorno(int x0, int y0, int x1, int y1, Obstaculo o) {
    for (int x = x0; x <= x1; x++) { em(x, y0).obs = o; em(x, y1).obs = o; }
    for (int y = y0; y <= y1; y++) { em(x0, y).obs = o; em(x1, y).obs = o; }
}
void Mapa::abrir(int x, int y, Obstaculo o, Chao ch) { em(x, y).obs = o; em(x, y).chao = ch; }

bool Mapa::passa(int x, int y) const {
    if (!valido(x, y)) return false;
    Obstaculo o = em(x, y).obs;
    return o == Obstaculo::Nada || o == Obstaculo::Porta || o == Obstaculo::Porteira;
}

const Lugar* Mapa::lugar(int x, int y) const {
    // a beira do córrego não é um retângulo: é qualquer ponto a até 3 metros da água
    for (const Lugar& l : lugares)
        if (x >= l.x0 && x <= l.x1 && y >= l.y0 && y <= l.y1) {
            if (l.nome == "ponte do córrego" && em(x, y).chao != Chao::Madeira) continue;
            return &l;
        }
    return nullptr;
}

std::string Mapa::nomeLugar(int x, int y) const {
    const Lugar* l = lugar(x, y);
    std::string n = l ? l->nome : "pasto";
    int ax, ay;
    if ((n == "pasto" || n == "mata" || n == "estrada da fazenda") && aguaMaisPerto(x, y, 3, ax, ay)) n += ", na beira do córrego";
    return n;
}

bool Mapa::dentro(int x, int y) const { const Lugar* l = lugar(x, y); return l && l->dentro; }

bool Mapa::aguaMaisPerto(int x, int y, int raio, int& ax, int& ay) const {
    int melhor = raio * raio + 1;
    for (int dy = -raio; dy <= raio; dy++)
        for (int dx = -raio; dx <= raio; dx++) {
            int px = x + dx, py = y + dy;
            if (!valido(px, py) || em(px, py).obs != Obstaculo::Corrego) continue;
            int d = dx * dx + dy * dy;
            if (d < melhor) { melhor = d; ax = px; ay = py; }
        }
    return melhor <= raio * raio;
}

const char* Mapa::nome(Chao ch) {
    switch (ch) {
        case Chao::Grama: return "grama baixa";
        case Chao::Pasto: return "capim do pasto";
        case Chao::Terra: return "terra batida";
        case Chao::Cascalho: return "cascalho";
        case Chao::Calcada: return "calçada de cimento";
        case Chao::Taco: return "piso de taco de madeira";
        case Chao::Ceramica: return "piso de cerâmica";
        case Chao::Madeira: return "tábuas da ponte";
        case Chao::Agua: return "água";
        case Chao::TerraArada: return "terra fofa da horta";
        case Chao::Folhas: return "folhas secas da mata";
    }
    return "chão";
}

const char* Mapa::somPasso(Chao ch) {
    switch (ch) {
        case Chao::Grama: return "passos-grama";
        case Chao::Pasto: return "passos-mato";
        case Chao::Terra: case Chao::TerraArada: return "passos-terra";
        case Chao::Cascalho: return "passos-cascalho";
        case Chao::Calcada: return "passos-calcada";
        case Chao::Taco: case Chao::Madeira: return "passos-madeira";
        case Chao::Ceramica: return "passos-ceramica";
        case Chao::Agua: return "passos-agua";
        case Chao::Folhas: return "passos-folhas";
    }
    return "passos-terra";
}

const char* Mapa::nome(Obstaculo o) {
    switch (o) {
        case Obstaculo::Nada: return "";
        case Obstaculo::Cerca: return "cerca de arame farpado";
        case Obstaculo::CercaMadeira: return "cerca de madeira do curral";
        case Obstaculo::Tela: return "tela do galinheiro";
        case Obstaculo::Parede: return "parede";
        case Obstaculo::Arvore: return "árvore";
        case Obstaculo::Corrego: return "córrego";
        case Obstaculo::Porteira: return "porteira";
        case Obstaculo::Porta: return "porta";
    }
    return "obstáculo";
}
