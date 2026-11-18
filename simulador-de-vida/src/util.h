// Pequenas ajudas usadas em vários arquivos do jogo.
#pragma once
#include <random>
#include <string>
#include "jogo.h"

// direções, de 0 (norte) a 7 (noroeste), no sentido do relógio
constexpr int DX[8] = {0, 1, 1, 1, 0, -1, -1, -1}, DY[8] = {1, 1, 0, -1, -1, -1, 0, 1};

inline double acaso(double a, double b) {
    static std::mt19937 sorte{std::random_device{}()};
    return std::uniform_real_distribution<double>(a, b)(sorte);
}
inline std::string goles(int n) { return n == 1 ? "1 gole" : std::to_string(n) + " goles"; }
inline std::string maiuscula(std::string s) {
    // só a primeira letra; também as minúsculas com acento do UTF-8 (á, é, ó, ç...: 0xC3 0xA0 a 0xBE)
    if (!s.empty() && s[0] >= 'a' && s[0] <= 'z') s[0] = (char)(s[0] - 32);
    else if (s.size() >= 2 && (unsigned char)s[0] == 0xC3 && (unsigned char)s[1] >= 0xA0 && (unsigned char)s[1] <= 0xBE && (unsigned char)s[1] != 0xB7) s[1] = (char)(s[1] - 0x20);
    return s;
}
std::string nomeProduto(const std::string& t, bool plural);  // colheita e sementes (roca.cpp)
int tipoDaSemente(const std::string& t);

inline std::string plural(const Coisa& c, int qtd, const std::string& singular) {
    if (qtd == 1) return singular;
    const std::string& t = c.tipo;
    if (!nomeProduto(t, true).empty()) return nomeProduto(t, true);
    if (t == "barra") return "barras de ração embrulhadas";
    if (t == "barra-aberta") return "barras de ração abertas";
    if (t == "antibiotico") return "comprimidos de antibiótico";
    if (t == "atadura") return "ataduras";
    if (t == "remedio") return "comprimidos para dor";
    if (t == "ovo") return "ovos";
    if (t == "ovo-frito") return "ovos fritos";
    if (t == "ovo-quebrado") return "ovos quebrados";
    if (t == "meio-queijo") return "metades de queijo";
    if (t == "omelete") return "omeletes de queijo";
    if (t == "faca-cozinha") return "facas de cozinha";
    if (t == "frigideira") return "frigideiras";
    if (t == "trufa") return "trufas";
    if (t == "tigela") return "tigelas";
    if (t == "gesso") return "pacotes de gesso em pó";
    if (t == "camisa") return "camisas";
    if (t == "calca") return "calças";
    if (t == "chapeu") return "chapéus";
    if (t == "semente-milho") return "sementes de milho";
    if (t == "semente-feijao") return "sementes de feijão de corda";
    if (t == "semente-arroz") return "sementes de arroz";
    if (t == "milho") return "espigas de milho";
    if (t == "feijao") return "punhados de feijão de corda";
    if (t == "arroz") return "feixes de arroz";
    if (t == "po-cafe") return "pacotes de pó de café";
    if (t == "acucar") return "sacos de açúcar";
    if (t == "farinha") return "sacos de farinha de trigo";
    if (t == "gas") return "botijões de gás";
    if (t == "cerveja") return "garrafas de cerveja";
    if (t == "carne") return "pedaços de carne crua";
    if (t == "carne-assada") return "pedaços de carne assada";
    if (t == "faca") return "facas de abate";
    if (t == "panela") return "panelas de alumínio";
    if (t == "picareta") return "picaretas";
    if (t == "cafe-torrado") return "punhados de café torrado";
    if (t == "garapa") return "copos de garapa";
    if (t == "queijo") return "queijos minas";
    if (t == "expresso") return "xícaras de café expresso";
    if (t == "cappuccino") return "xícaras de café com leite vaporizado";
    if (t == "bosta") return "bostas de vaca secas";
    if (t == "adubo") return "punhados de adubo";
    if (t == "lenha") return "toras de lenha";
    if (t == "pedra") return "pedras";
    if (t == "maca") return "maçãs";
    if (t == "cereja") return "cerejas";
    if (t == "manga") return "mangas";
    if (t == "leite") return "garrafas de leite";
    if (t == "couro") return "couros de vaca";
    if (t == "cinto") return "cintos de couro";
    if (t == "bolsa") return "bolsas de couro";
    if (t == "chapeu-couro") return "chapéus de couro";
    if (t == "botas") return "pares de botas de couro";
    if (t == "sela") return "selas de couro";
    if (t == "bolo") return "fatias de bolo";
    if (t == "brigadeiro" || t == "beijinho" || t == "camafeu" || t == "risole" || t == "coxinha" || t == "empada") return t == "camafeu" ? "camafeus" : t + "s";
    if (t == "torta") return "pedaços de torta de carne moída";
    if (t == "refrigerante") return "copos de refrigerante";
    if (t == "vinho") return "taças de vinho";
    if (t == "uisque") return "doses de uísque";
    if (t == "salada") return "pratinhos de salada";
    if (t == "maionese") return "pratinhos de maionese";
    if (t == "isqueiro") return "isqueiros";
    if (t == "gas-isqueiro") return "latinhas de gás para isqueiro";
    if (t == "tampa") return "tampas de garrafa";
    if (t == "garrafa" || t == "garrafa-aberta") {
        std::string estado = t == "garrafa" ? "tampadas" : "abertas";
        return c.carga >= 4 ? "garrafas de água " + estado + ", cheias"
             : c.carga == 0 ? "garrafas vazias, " + estado
             : "garrafas de água " + estado + ", com " + goles(c.carga);
    }
    return singular;
}

// primeira letra sem acento, minúscula (para pular com a letra em listas)
char inicialSemAcento(const std::string& s);
