#include "mapa.h"
#include <cmath>
#include <random>

// Fazenda do pai, no interior de Minas. Porteira principal no sul, no meio da divisa (x 498 a 502).
// Quem chega fica no gramado, em (500, 9), virado para o norte. Bem em frente, a 100 quadrados,
// em (500, 109), está a porta da casa sede, no meio da parede da varanda. A casa tem 200 por 200.
Mapa::Mapa() : c(LADO * LADO) {
    std::mt19937 sorte(18);  // sempre o mesmo mapa
    std::uniform_real_distribution<float> u(0, 1);

    // fora da cerca, ao sul: a estrada municipal de terra
    retangulo(0, 0, LADO - 1, 4, Chao::Terra);
    // terreiro gramado em volta da sede, e o gramado da porteira
    retangulo(340, 80, 660, 380, Chao::Grama);
    retangulo(460, 6, 540, 40, Chao::Grama);
    // estrada da fazenda: do gramado até a varanda, e dos fundos da casa até o fim do pasto
    retangulo(498, 41, 502, 100, Chao::Terra);
    retangulo(498, 314, 502, 985, Chao::Terra);
    // horta e pomar, na frente da casa, dos lados da estrada
    retangulo(360, 30, 420, 75, Chao::TerraArada);
    retangulo(580, 30, 650, 75, Chao::Grama);
    for (int y = 33; y <= 73; y += 7)
        for (int x = 583; x <= 648; x += 7) em(x, y).obs = Obstaculo::Arvore;
    // mata no noroeste: chão de folhas e árvores
    for (int y = 520; y <= 992; y++)
        for (int x = 6; x <= 300; x++) {
            em(x, y).chao = Chao::Folhas;
            if (u(sorte) < 0.16f) em(x, y).obs = Obstaculo::Arvore;
        }
    // árvores soltas no pasto
    for (int i = 0; i < 1100; i++) {
        int x = 10 + (int)(u(sorte) * 980), y = 10 + (int)(u(sorte) * 980);
        Celula& k = em(x, y);
        if (k.chao == Chao::Pasto && k.obs == Obstaculo::Nada && std::abs(x - 500) > 6) k.obs = Obstaculo::Arvore;
    }
    // córrego: serpenteia por volta de y = 620, com 3 metros de largura; ponte de tábuas na estrada
    for (int x = 0; x < LADO; x++) {
        int yc = 620 + (int)std::lround(20 * std::sin(x / 70.0));
        for (int y = yc - 1; y <= yc + 1; y++) {
            bool ponte = x >= 498 && x <= 502;
            em(x, y).chao = ponte ? Chao::Madeira : Chao::Agua;
            em(x, y).obs = ponte ? Obstaculo::Nada : Obstaculo::Corrego;
        }
    }
    // casa sede: calçada em volta, varanda na frente, 200 por 200 de paredes (x 400 a 599, y 109 a 308)
    retangulo(395, 101, 604, 313, Chao::Calcada);
    retangulo(400, 109, 599, 308, Chao::Taco);
    retangulo(401, 161, 493, 229, Chao::Ceramica);   // cozinha
    retangulo(507, 161, 598, 199, Chao::Ceramica);   // banheiro
    contorno(400, 109, 599, 308, Obstaculo::Parede);
    for (int x = 401; x <= 598; x++) em(x, 160).obs = Obstaculo::Parede;     // sala | resto da casa
    for (int y = 161; y <= 307; y++) { em(494, y).obs = Obstaculo::Parede; em(506, y).obs = Obstaculo::Parede; }  // corredor
    for (int x = 401; x <= 493; x++) em(x, 230).obs = Obstaculo::Parede;     // cozinha | quarto
    for (int x = 507; x <= 598; x++) em(x, 200).obs = Obstaculo::Parede;     // banheiro | segundo quarto
    // suíte: banheiro dentro do quarto, no canto sudoeste
    retangulo(401, 231, 425, 250, Chao::Ceramica);
    for (int y = 231; y <= 251; y++) em(426, y).obs = Obstaculo::Parede;
    for (int x = 401; x <= 426; x++) em(x, 251).obs = Obstaculo::Parede;
    abrir(426, 241, Obstaculo::Porta, Chao::Ceramica);       // quarto -> banheiro da suíte
    box(595, 198);   // box do banheiro social
    box(403, 249);   // box da suíte
    abrir(500, 109, Obstaculo::Porta, Chao::Taco, false);   // porta da frente, no meio da varanda: fechada
    abrir(500, 308, Obstaculo::Porta, Chao::Taco, false);   // porta dos fundos: fechada
    abrir(500, 160, Obstaculo::Porta, Chao::Taco);           // sala -> corredor
    abrir(494, 195, Obstaculo::Porta, Chao::Ceramica);       // corredor -> cozinha
    abrir(494, 269, Obstaculo::Porta, Chao::Taco);           // corredor -> quarto
    abrir(506, 180, Obstaculo::Porta, Chao::Ceramica);       // corredor -> banheiro
    abrir(506, 254, Obstaculo::Porta, Chao::Taco);           // corredor -> segundo quarto
    // casinha de seu Tião e dona Nezinha, a oeste da sede, porta para o sul
    retangulo(355, 190, 385, 210, Chao::Cimento);
    contorno(355, 190, 385, 210, Obstaculo::Parede);
    abrir(370, 190, Obstaculo::Porta, Chao::Cimento);
    // roça: depois da ponte, a leste da estrada, perto do córrego; tomada de matagal
    for (int ry = 590; ry <= 640; ry++)
        for (int rx = 510; rx <= 570; rx++) {
            int yc = 620 + (int)std::lround(20 * std::sin(rx / 70.0));
            if (ry < yc - 3) { em(rx, ry).chao = Chao::Mato; em(rx, ry).obs = Obstaculo::Nada; }
        }
    // estrada para a cidade: sai do terreiro para o leste, até a porteira leste; lá fora fica a prefeitura
    retangulo(661, 198, 993, 202, Chao::Terra);
    retangulo(995, 195, LADO - 1, 205, Chao::Terra);
    // curral, galinheiro e paiol, no terreiro
    retangulo(680, 150, 760, 230, Chao::Terra);
    contorno(680, 150, 760, 230, Obstaculo::CercaMadeira);
    abrir(680, 190, Obstaculo::Porteira, Chao::Terra, false);
    retangulo(620, 340, 634, 350, Chao::Terra);
    contorno(620, 340, 634, 350, Obstaculo::Tela);
    abrir(627, 340, Obstaculo::Porta, Chao::Terra, false);
    retangulo(330, 330, 350, 350, Chao::Terra);
    contorno(330, 330, 350, 350, Obstaculo::Parede);
    abrir(340, 330, Obstaculo::Porta, Chao::Terra, false);
    // cerca de arame em volta da fazenda, com a porteira principal no sul, aberta
    contorno(5, 5, LADO - 6, LADO - 6, Obstaculo::Cerca);
    for (int x = 498; x <= 502; x++) abrir(x, 5, Obstaculo::Porteira, Chao::Terra);
    for (int y = 198; y <= 202; y++) abrir(LADO - 6, y, Obstaculo::Porteira, Chao::Terra, false);

    // do mais específico para o mais geral
    lugares = {
        {"box do chuveiro", 595, 198, 595, 198, true},
        {"box do chuveiro da suíte", 403, 249, 403, 249, true},
        {"banheiro da suíte", 401, 231, 425, 250, true},
        {"sala", 401, 110, 598, 159, true},
        {"casinha de seu Tião e dona Nezinha", 356, 191, 384, 209, true},
        {"corredor", 495, 161, 505, 307, true},
        {"cozinha", 401, 161, 493, 229, true},
        {"quarto", 401, 231, 493, 307, true},
        {"banheiro", 507, 161, 598, 199, true},
        {"segundo quarto", 507, 201, 598, 307, true},
        {"varanda da casa", 395, 101, 604, 108, false},
        {"calçada da casa", 395, 101, 604, 313, false},
        {"galinheiro", 621, 341, 633, 349, true},
        {"paiol", 331, 331, 349, 349, true},
        {"curral", 681, 151, 759, 229, false},
        {"roça", 510, 590, 570, 640, false},
        {"horta", 360, 30, 420, 75, false},
        {"pomar", 580, 30, 650, 75, false},
        {"ponte do córrego", 498, 560, 502, 680, false},
        {"casa da sede", 400, 109, 599, 308, true},
        {"gramado da porteira principal", 460, 6, 540, 40, false},
        {"terreiro da sede", 340, 80, 660, 380, false},
        {"estrada da fazenda", 498, 41, 502, 985, false},
        {"estrada para a cidade, do lado de fora da fazenda", 995, 195, LADO - 1, 205, false},
        {"estrada para a cidade", 661, 198, 993, 202, false},
        {"mata", 6, 520, 300, 992, false},
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
void Mapa::box(int ix, int iy) {
    for (int dx : {-1, 1}) { em(ix + dx, iy).obs = Obstaculo::Vidro; em(ix + dx, iy + 1).obs = Obstaculo::Vidro; }
    em(ix, iy).chao = Chao::Ceramica;
    abrir(ix, iy - 1, Obstaculo::PortaBox, Chao::Ceramica, false);
}

void Mapa::abrir(int x, int y, Obstaculo o, Chao ch, bool aberta) { em(x, y).obs = o; em(x, y).chao = ch; em(x, y).aberta = aberta; }

bool Mapa::passa(int x, int y) const {
    if (!valido(x, y)) return false;
    const Celula& k = em(x, y);
    if (k.movel >= 0 || k.planta) return false;
    return k.obs == Obstaculo::Nada || (ehPassagem(x, y) && k.aberta);
}

std::vector<std::pair<int, int>> Mapa::passagens() const {
    std::vector<std::pair<int, int>> v;
    for (int y = 0; y < LADO; y++)
        for (int x = 0; x < LADO; x++) if (ehPassagem(x, y)) v.push_back({x, y});
    return v;
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
    if ((n == "pasto" || n == "mata" || n == "estrada da fazenda" || n == "roça") && aguaMaisPerto(x, y, 3, ax, ay)) n += ", na beira do córrego";
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
        case Chao::TerraArada: return "terra fofa, limpa";
        case Chao::Folhas: return "folhas secas da mata";
        case Chao::Mato: return "matagal fechado";
        case Chao::Cimento: return "cimento queimado";
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
        case Chao::Mato: return "passos-mato";
        case Chao::Cimento: return "passos-calcada";
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
        case Obstaculo::Vidro: return "vidro do box";
        case Obstaculo::PortaBox: return "porta do box";
    }
    return "obstáculo";
}
