// O mapa: 500 por 500 quadrados de 1 metro. x cresce para o leste, y para o norte.
// Cada quadrado tem um chão (o que se pisa, com seu som de passo) e pode ter um obstáculo.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

constexpr int LADO = 500;

enum class Chao : uint8_t { Grama, Pasto, Terra, Cascalho, Calcada, Taco, Ceramica, Madeira, Agua, TerraArada, Folhas };
enum class Obstaculo : uint8_t { Nada, Cerca, CercaMadeira, Tela, Parede, Arvore, Corrego, Porteira, Porta };

struct Celula {
    Chao chao = Chao::Pasto;
    Obstaculo obs = Obstaculo::Nada;
};

struct Lugar {
    std::string nome;
    int x0, y0, x1, y1;  // retângulo, inclusive
    bool dentro;         // dentro de uma construção: abafa o som de fora
};

class Mapa {
public:
    Mapa();
    bool valido(int x, int y) const { return x >= 0 && y >= 0 && x < LADO && y < LADO; }
    const Celula& em(int x, int y) const { return c[y * LADO + x]; }
    Celula& em(int x, int y) { return c[y * LADO + x]; }
    bool passa(int x, int y) const;                // dá para pisar ali?
    const Lugar* lugar(int x, int y) const;        // o lugar mais específico que contém o ponto
    std::string nomeLugar(int x, int y) const;
    bool dentro(int x, int y) const;
    // ponto de água do córrego mais perto de (x, y), procurando até "raio" metros
    bool aguaMaisPerto(int x, int y, int raio, int& ax, int& ay) const;

    static const char* nome(Chao ch);           // "grama", "piso de taco"...
    static const char* somPasso(Chao ch);       // nome do grupo de sons de passo
    static const char* nome(Obstaculo o);       // "cerca de arame farpado"...

private:
    std::vector<Celula> c;
    std::vector<Lugar> lugares;  // do mais específico para o mais geral
    void retangulo(int x0, int y0, int x1, int y1, Chao ch);
    void contorno(int x0, int y0, int x1, int y1, Obstaculo o);
    void abrir(int x, int y, Obstaculo o, Chao ch);
};
