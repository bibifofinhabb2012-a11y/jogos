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
    // só a primeira letra, e só se for ASCII (as palavras do jogo não começam com acento)
    if (!s.empty() && s[0] >= 'a' && s[0] <= 'z') s[0] = (char)(s[0] - 32);
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
