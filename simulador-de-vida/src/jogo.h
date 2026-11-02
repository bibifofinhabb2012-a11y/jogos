// O jogo em si, sem janela nem teclado: recebe teclas e o tempo que passou, e fala e toca sons.
// Assim dá para testar tudo por roteiro, sem abrir janela.
#pragma once
#include <map>
#include <string>
#include <vector>
#include "mapa.h"

enum class Tecla { Cima, Baixo, Esquerda, Direita, I, U, S, E, T, L, H, Q, R, B, V, Um, Dois, Tres, Enter, Espaco, Esc, F1, F2 };
enum class Postura { EmPe, Sentado, Agachado, Deitado };

// Uma coisa: o tipo e uma carga (goles numa garrafa, moedas na carteira).
struct Coisa {
    std::string tipo;
    int carga = 0;
    bool operator==(const Coisa& o) const { return tipo == o.tipo && carga == o.carga; }
};

struct Pilha { Coisa coisa; int qtd; };

class Jogo {
public:
    Jogo();
    void novo();                                  // começa do zero, com a cena do pai
    bool carregar(const std::string& arquivo);    // true: havia um jogo salvo
    void salvar(const std::string& arquivo) const;
    void tecla(Tecla t, bool shift, bool ctrl);   // tecla apertada (não repetição)
    void soltar(Tecla t);                         // tecla solta
    void atualizar(double segundos);              // tempo real que passou
    bool querSair() const { return sair; }
    void abertura(bool novoJogo);                 // o que se ouve ao abrir o jogo

private:
    Mapa mapa;
    // onde está e para onde olha (0 norte, 1 nordeste, ... 7 noroeste)
    int x = 500, y = 9, rumo = 0;
    Postura postura = Postura::EmPe;
    double intestino = 30, bexiga = 30;  // de 0 (vazio) a 100 (não segura mais)
    int excretando = 0;                   // 1 xixi, 2 cocô, segurando a tecla
    double excretaT = 0, proximoPlop = 0;
    int somXixi = 0;
    // necessidades, de 0 a 100: 100 é satisfeito
    double fome = 85, sede = 85, energia = 90, saude = 100;
    double minutos = 7 * 60;  // relógio do jogo, em minutos desde a meia-noite do dia 1
    std::vector<Pilha> mochila;
    Coisa maoDireita, maoEsquerda;  // tipo vazio = mão vazia
    bool mochilaAberta = false;
    int itemMochila = 0;
    bool dormindo = false;
    bool sair = false, confirmarSair = false;
    double relogioSalvar = 0, ultimoAviso[5] = {100, 100, 100, 100, 100};
    // andar: tecla segurada e tempo até o próximo passo
    bool andandoFrente = false, andandoTras = false, correndo = false;
    int andandoLado = 0;   // -1 esquerda, 1 direita (passo de lado)
    int maoSel = 0;        // mão selecionada: 0 direita, 1 esquerda
    int somSono = 0;
    double esperaPasso = 0;
    std::string ultimoObstaculo;
    double toqueNum[2] = {-10, -10}, agora = 0;
    // sons do ambiente
    int somCorrego = 0, somVento = 0, somNoite = 0, somDia = 0, carroPai = 0;
    double carroT = -1, proximoPassaro = 5;
    std::string lugarAtual;

    // mochila e mãos
    std::string nomeCoisa(const Coisa& c, bool comArtigo = false) const;
    void guardar(const Coisa& c, int qtd = 1);
    bool tirar(const Coisa& c);
    void falarItemMochila(const std::string& antes = "");
    void colocarNaMao(bool direita);
    void verMao(int mao);
    Coisa& mao(bool direita) { return direita ? maoDireita : maoEsquerda; }
    void combinar();
    void usar(bool direita);
    // corpo
    void dizerFome(); void dizerSede(); void dizerEnergia(); void dizerSaude();
    void avisosDoCorpo();
    void dormir();
    void acordar(const std::string& motivo);
    // mundo
    void passo(int sentido);
    void passoPara(int r);
    void virar(int lado);
    void bussola();
    void mudarPostura(Postura p);
    void abrirPorta();
    void anunciarPortas();
    void comecarExcretar(int tipo);
    void pararExcretar(bool cedo);
    void dizerIntestino();
    void dizerBexiga();
    void verMaos();
    std::string relativo(int dirAbs) const;
    void olhar();
    void hora();
    void ajuda();
    std::string passagemPerto(int ox, int oy, int r) const;
    void ambiente(double dt);
    std::string direcao(int r) const;
    int hora24() const { return ((int)minutos / 60) % 24; }
    int diaDoJogo() const { return 1 + (int)minutos / 1440; }
};
