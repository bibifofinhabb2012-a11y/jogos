#include "rede.h"
#include <enet/enet.h>
#include <cstring>
#include <algorithm>
#include <set>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#endif

namespace rede {
enum Tipo : unsigned char { OLA = 1, BEMVINDO, JOGADOR, SAIU, POS, CHAT, RELOGIO, PEDIR, FAZENDA, VOZ, MUNDO, APP };
constexpr char SEP = '\x1f';

static bool ligada = false, souAnfitriao = false, conectado = false;
static ENetHost* hostEnet = nullptr;
static ENetPeer* servidor = nullptr;                 // no cliente: o anfitrião
static std::map<ENetPeer*, int> idDoPeer;             // no anfitrião: quem é quem
static std::map<int, Remoto> outros;                   // todo mundo menos eu
static int meuId = -1, proximoId = 1;
static std::string nome, meuMundo = "fazenda";

static std::vector<std::string> partes(const std::string& s) {
    std::vector<std::string> v; std::string a;
    for (char c : s) { if (c == SEP) { v.push_back(a); a.clear(); } else a += c; }
    v.push_back(a);
    return v;
}
static std::string juntar(std::initializer_list<std::string> l) {
    std::string s; bool primeiro = true;
    for (const auto& p : l) { if (!primeiro) s += SEP; s += p; primeiro = false; }
    return s;
}
static ENetPacket* pacote(Tipo t, const std::string& corpo, bool confiavel) {
    std::string d(1, (char)t); d += corpo;
    return enet_packet_create(d.data(), d.size(), confiavel ? ENET_PACKET_FLAG_RELIABLE : 0);
}
static void mandar(ENetPeer* p, Tipo t, const std::string& corpo, int canal, bool confiavel) {
    if (p) enet_peer_send(p, (enet_uint8)canal, pacote(t, corpo, confiavel));
}
// anfitrião: manda para todos os clientes, menos um
static void espalhar(Tipo t, const std::string& corpo, int canal, bool confiavel, ENetPeer* menos = nullptr, const std::string& soMundo = "") {
    for (auto& [p, id] : idDoPeer) {
        if (p == menos) continue;
        if (!soMundo.empty() && outros.count(id) && outros[id].mundo != soMundo) continue;
        mandar(p, t, corpo, canal, confiavel);
    }
}
static ENetPeer* peerDe(int id) { for (auto& [p, i] : idDoPeer) if (i == id) return p; return nullptr; }

bool iniciar() { if (!ligada) ligada = enet_initialize() == 0; return ligada; }
void encerrar() { sair(); if (ligada) enet_deinitialize(); ligada = false; }
bool online() { return conectado; }
bool anfitriao() { return conectado && souAnfitriao; }
const std::string& meuNome() { return nome; }
const std::map<int, Remoto>& remotos() { return outros; }
std::vector<std::string> outrosJogadores() { std::vector<std::string> v; for (auto& [i, r] : outros) v.push_back(r.nome); return v; }

bool hospedar(const std::string& n, std::string& erro) {
    if (!iniciar()) { erro = "A rede não iniciou."; return false; }
    sair();
    ENetAddress a; a.host = ENET_HOST_ANY; a.port = PORTA;
    hostEnet = enet_host_create(&a, 16, 3, 0, 0);
    if (!hostEnet) { erro = "Não deu para abrir a porta " + std::to_string(PORTA) + ". Outro programa está usando, ou já tem um jogo hospedado aqui."; return false; }
    souAnfitriao = true; conectado = true; meuId = 0; nome = n; proximoId = 1; meuMundo = "fazenda:" + n;
    return true;
}

bool entrar(const std::string& ip, const std::string& n, std::string& erro) {
    if (!iniciar()) { erro = "A rede não iniciou."; return false; }
    sair();
    hostEnet = enet_host_create(nullptr, 1, 3, 0, 0);
    if (!hostEnet) { erro = "A rede não iniciou."; return false; }
    ENetAddress a;
    if (enet_address_set_host(&a, ip.c_str()) != 0) { erro = "Endereço inválido: " + ip; enet_host_destroy(hostEnet); hostEnet = nullptr; return false; }
    a.port = PORTA;
    servidor = enet_host_connect(hostEnet, &a, 3, 0);
    ENetEvent ev;
    if (!servidor || enet_host_service(hostEnet, &ev, 5000) <= 0 || ev.type != ENET_EVENT_TYPE_CONNECT) {
        erro = "Não achei jogo hospedado em " + ip + ". Confira o endereço, se a Radmin está ligada nos dois computadores, e se o anfitrião já hospedou.";
        if (servidor) enet_peer_reset(servidor);
        enet_host_destroy(hostEnet); hostEnet = nullptr; servidor = nullptr;
        return false;
    }
    souAnfitriao = false; conectado = true; nome = n; meuMundo = "fazenda:" + n;
    mandar(servidor, OLA, nome, 0, true);
    enet_host_flush(hostEnet);
    return true;
}

void sair() {
    if (hostEnet) {
        if (servidor) { enet_peer_disconnect(servidor, 0); enet_host_flush(hostEnet); }
        for (auto& [p, id] : idDoPeer) enet_peer_disconnect(p, 0);
        enet_host_flush(hostEnet);
        enet_host_destroy(hostEnet);
    }
    hostEnet = nullptr; servidor = nullptr; idDoPeer.clear(); outros.clear(); conectado = false; souAnfitriao = false; meuId = -1;
}

// ---------- envio ----------
void enviarPosicao(const std::string& mundo, int x, int y, int rumo) {
    if (!conectado) return;
    meuMundo = mundo;
    std::string c = juntar({std::to_string(meuId), mundo, std::to_string(x), std::to_string(y), std::to_string(rumo)});
    if (souAnfitriao) espalhar(POS, c, 1, false); else mandar(servidor, POS, c, 1, false);
}
void mudouDeMundo(const std::string& mundo) {
    meuMundo = mundo;
    if (!conectado) return;
    std::string c = juntar({std::to_string(meuId), mundo});
    if (souAnfitriao) espalhar(MUNDO, c, 0, true); else mandar(servidor, MUNDO, c, 0, true);
}
void enviarChat(const std::string& texto) {
    if (!conectado) return;
    std::string c = juntar({nome, texto});
    if (souAnfitriao) espalhar(CHAT, c, 0, true); else mandar(servidor, CHAT, c, 0, true);
}
void enviarRelogio(double minutos) { if (anfitriao()) espalhar(RELOGIO, std::to_string(minutos), 0, true); }
void enviarVoz(const unsigned char* d, int n) {
    if (!conectado || n <= 0) return;
    std::string c = std::to_string(meuId) + SEP + std::string((const char*)d, (size_t)n);
    if (souAnfitriao) espalhar(VOZ, c, 2, false, nullptr, meuMundo); else mandar(servidor, VOZ, c, 2, false);
}
void pedirFazenda(const std::string& dono) {
    if (!conectado) return;
    if (souAnfitriao) {  // eu, anfitrião, visitando um cliente: peço direto ao dono
        for (auto& [id, r] : outros) if (r.nome == dono) mandar(peerDe(id), PEDIR, juntar({"0", dono}), 0, true);
    } else mandar(servidor, PEDIR, juntar({"-1", dono}), 0, true);
}

void enviarApp(const std::string& tipo, const std::string& para, const std::string& dados) {
    if (!conectado) return;
    std::string c = juntar({tipo, nome, para, dados});
    if (!souAnfitriao) { mandar(servidor, APP, c, 0, true); return; }
    if (para == "*") espalhar(APP, c, 0, true);
    else for (auto& [id, r] : outros) if (r.nome == para) mandar(peerDe(id), APP, c, 0, true);
}

// ---------- recebimento ----------
static void receber(ENetPeer* de, const std::string& d, const Ganchos& g) {
    if (d.empty()) return;
    Tipo t = (Tipo)(unsigned char)d[0];
    std::string corpo = d.substr(1);
    if (t == VOZ) {  // binário: id, separador, dados
        size_t p = corpo.find(SEP);
        if (p == std::string::npos) return;
        int id = souAnfitriao ? idDoPeer[de] : std::atoi(corpo.substr(0, p).c_str());
        std::string dados = corpo.substr(p + 1);
        if (souAnfitriao) {  // repassa para quem está no mesmo lugar que o dono da voz
            std::string mundo = outros.count(id) ? outros[id].mundo : "";
            espalhar(VOZ, std::to_string(id) + SEP + dados, 2, false, de, mundo);
            if (mundo != meuMundo) return;
        }
        if (g.voz) g.voz(id, (const unsigned char*)dados.data(), (int)dados.size());
        return;
    }
    auto f = partes(corpo);
    switch (t) {
        case OLA: if (souAnfitriao) {
            int id = proximoId++;
            std::string n = f[0].empty() ? "Jogador " + std::to_string(id) : f[0];
            for (auto& [i, r] : outros) if (r.nome == n) n += " " + std::to_string(id);
            if (n == nome) n += " " + std::to_string(id);
            idDoPeer[de] = id;
            outros[id] = Remoto{id, n, "fazenda:" + n, 0, 0, 0};
            mandar(de, BEMVINDO, juntar({std::to_string(id), nome, n}), 0, true);
            mandar(de, JOGADOR, juntar({"0", nome, meuMundo}), 0, true);
            for (auto& [i, r] : outros) if (i != id) mandar(de, JOGADOR, juntar({std::to_string(i), r.nome, r.mundo}), 0, true);
            espalhar(JOGADOR, juntar({std::to_string(id), n, "fazenda:" + n}), 0, true, de);
            if (g.entrou) g.entrou(n);
        } break;
        case BEMVINDO: meuId = std::atoi(f[0].c_str()); if (f.size() > 2) nome = f[2]; break;
        case JOGADOR: {
            int id = std::atoi(f[0].c_str());
            if (id == meuId || f.size() < 3) break;
            bool novo = !outros.count(id);
            outros[id].id = id; outros[id].nome = f[1]; outros[id].mundo = f[2];
            if (novo && g.entrou) g.entrou(f[1]);
        } break;
        case SAIU: {
            int id = std::atoi(f[0].c_str());
            if (outros.count(id)) { std::string n = outros[id].nome; outros.erase(id); if (g.saiu) g.saiu(n); }
        } break;
        case POS: case MUNDO: {
            int id = souAnfitriao ? idDoPeer[de] : std::atoi(f[0].c_str());
            if (id == meuId) break;
            Remoto& r = outros[id];
            r.id = id;
            if (f.size() > 1) r.mundo = f[1];
            if (t == POS && f.size() >= 5) {
                int nx = std::atoi(f[2].c_str()), ny = std::atoi(f[3].c_str());
                bool andou = nx != r.x || ny != r.y;
                r.x = nx; r.y = ny; r.rumo = std::atoi(f[4].c_str());
                if (andou && g.andou) g.andou(id);
            }
            if (souAnfitriao) {
                f[0] = std::to_string(id);
                std::string c = f[0]; for (size_t k = 1; k < f.size(); k++) c += SEP + f[k];
                espalhar(t, c, t == POS ? 1 : 0, t == MUNDO, de);
            }
        } break;
        case CHAT:
            if (souAnfitriao) espalhar(CHAT, corpo, 0, true, de);
            if (g.chat && f.size() >= 2) g.chat(f[0], f[1]);
            break;
        case RELOGIO: if (g.relogio) g.relogio(std::atof(f[0].c_str())); break;
        case APP: {  // tipo, de, para, dados (os dados podem ter separadores: junta o resto)
            if (f.size() < 4) break;
            std::string dados = f[3]; for (size_t k = 4; k < f.size(); k++) dados += SEP + f[k];
            if (souAnfitriao) {
                if (f[2] == "*") espalhar(APP, corpo, 0, true, de);
                else if (f[2] != nome) { for (auto& [id, r] : outros) if (r.nome == f[2]) mandar(peerDe(id), APP, corpo, 0, true); break; }
            }
            if (g.app && (f[2] == "*" || f[2] == nome)) g.app(f[0], f[1], dados);
        } break;
        case PEDIR: {  // f[0] quem pediu (id), f[1] dono da fazenda
            int quem = std::atoi(f[0].c_str());
            if (souAnfitriao && quem == -1) quem = idDoPeer[de];
            if (f.size() < 2) break;
            if (f[1] == nome) {  // é a minha fazenda: mando o estado
                std::string est = g.estadoFazenda ? g.estadoFazenda() : "";
                std::string c = juntar({std::to_string(quem), est});
                if (souAnfitriao) mandar(peerDe(quem), FAZENDA, c, 0, true); else mandar(servidor, FAZENDA, c, 0, true);
            } else if (souAnfitriao) {  // repasso ao dono
                for (auto& [id, r] : outros) if (r.nome == f[1]) mandar(peerDe(id), PEDIR, juntar({std::to_string(quem), f[1]}), 0, true);
            }
        } break;
        case FAZENDA: {
            int para = std::atoi(f[0].c_str());
            std::string est = f.size() > 1 ? f[1] : "";
            if (souAnfitriao && para != 0) mandar(peerDe(para), FAZENDA, corpo, 0, true);
            else if (g.fazendaChegou) g.fazendaChegou(est);
        } break;
        default: break;
    }
}

void servico(const Ganchos& g) {
    if (!hostEnet) return;
    ENetEvent ev;
    while (hostEnet && enet_host_service(hostEnet, &ev, 0) > 0) {
        switch (ev.type) {
            case ENET_EVENT_TYPE_RECEIVE:
                receber(ev.peer, std::string((const char*)ev.packet->data, ev.packet->dataLength), g);
                enet_packet_destroy(ev.packet);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                if (souAnfitriao) {
                    auto it = idDoPeer.find(ev.peer);
                    if (it != idDoPeer.end()) {
                        int id = it->second; std::string n = outros[id].nome;
                        idDoPeer.erase(it); outros.erase(id);
                        espalhar(SAIU, std::to_string(id), 0, true);
                        if (g.saiu) g.saiu(n);
                    }
                } else {
                    servidor = nullptr; enet_host_destroy(hostEnet); hostEnet = nullptr;
                    conectado = false; outros.clear();
                    if (g.aviso) g.aviso("A conexão com o anfitrião caiu. Você está jogando sozinho de novo.");
                    return;
                }
                break;
            default: break;
        }
    }
}

std::vector<std::string> enderecosLocais() {
    std::vector<std::string> v;
#ifdef _WIN32
    char nomeMaquina[256];
    if (gethostname(nomeMaquina, sizeof nomeMaquina) == 0) {
        addrinfo dica{}, *res = nullptr;
        dica.ai_family = AF_INET;
        if (getaddrinfo(nomeMaquina, nullptr, &dica, &res) == 0) {
            for (addrinfo* p = res; p; p = p->ai_next) {
                char b[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &((sockaddr_in*)p->ai_addr)->sin_addr, b, sizeof b);
                v.push_back(b);
            }
            freeaddrinfo(res);
        }
    }
#else
    ifaddrs* l = nullptr;
    if (getifaddrs(&l) == 0) {
        for (ifaddrs* p = l; p; p = p->ifa_next)
            if (p->ifa_addr && p->ifa_addr->sa_family == AF_INET) {
                char b[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &((sockaddr_in*)p->ifa_addr)->sin_addr, b, sizeof b);
                if (std::string(b) != "127.0.0.1") v.push_back(b);
            }
        freeifaddrs(l);
    }
#endif
    std::set<std::string> vistos; std::vector<std::string> u;
    for (auto& s : v) if (vistos.insert(s).second) u.push_back(s);
    std::stable_partition(u.begin(), u.end(), [](const std::string& s) { return s.rfind("26.", 0) == 0; });  // Radmin primeiro
    return u;
}
}
