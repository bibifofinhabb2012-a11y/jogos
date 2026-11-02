// Jogo online: um jogador hospeda (o jogo dele vira o servidor) e os outros entram pelo endereço,
// por exemplo o da Radmin VPN (26.x.x.x). Usa ENet (UDP). O anfitrião repassa tudo: posições,
// chat, voz, pedidos de fazenda, e manda o relógio, que é um só para todos.
#pragma once
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace rede {
constexpr int PORTA = 27960;

struct Remoto {
    int id = 0;
    std::string nome, mundo;
    int x = 0, y = 0, rumo = 0;
};

// o que a rede avisa ao jogo
struct Ganchos {
    std::function<void(const std::string& nome, const std::string& texto)> chat;
    std::function<void(const std::string& nome)> entrou, saiu;
    std::function<void(double minutos)> relogio;
    std::function<std::string()> estadoFazenda;             // alguém quer visitar a sua fazenda
    std::function<void(const std::string& texto)> fazendaChegou;
    std::function<void(int id, const unsigned char* dados, int n)> voz;
    std::function<void(int id)> andou;                      // um jogador mudou de quadrado: tocar o passo dele
    std::function<void(const std::string& msg)> aviso;      // conexão caiu, recusada etc.
};

bool iniciar();                // liga a biblioteca; false se não deu
void encerrar();
bool hospedar(const std::string& nome, std::string& erro);
bool entrar(const std::string& ip, const std::string& nome, std::string& erro);
void sair();
bool online();
bool anfitriao();
const std::string& meuNome();
void servico(const Ganchos& g);       // chamar todo quadro: recebe e repassa
void enviarPosicao(const std::string& mundo, int x, int y, int rumo);
void enviarChat(const std::string& texto);
void enviarVoz(const unsigned char* dados, int n);
void enviarRelogio(double minutos);   // só o anfitrião
void pedirFazenda(const std::string& dono);
void mudouDeMundo(const std::string& mundo);
std::vector<std::string> outrosJogadores();
const std::map<int, Remoto>& remotos();
std::vector<std::string> enderecosLocais();   // IPv4 deste computador (o da Radmin começa com 26)
}
