// Casamento no jogo online: aliança comprada na prefeitura, pedido (teleporta até a pessoa),
// evento para todos (Tab, Teleportar para o evento), cerimônia do prefeito, festa no salão com a
// mesa de comes e bebes (5 de cada por pessoa), e a escolha da fazenda onde o casal vai morar.
#include <algorithm>
#include <cmath>
#include <sstream>
#include "audio.h"
#include "fala.h"
#include "jogo.h"
#include "util.h"

static const char* COMES[] = {"bolo", "brigadeiro", "beijinho", "camafeu", "empada", "torta", "risole", "coxinha",
                              "refrigerante", "cerveja", "vinho", "uisque", "agua", "salada", "maionese"};
static const int ALTAR_A[2] = {548, 152}, ALTAR_B[2] = {552, 152}, PLATEIA[2] = {550, 140}, SALAO[2] = {455, 194};

static std::string casal(const std::string& a, char ga, const std::string& b, char gb) {
    (void)ga; (void)gb;
    return a + " e " + b;
}

void Jogo::menuPrefeito() {
    Menu m;
    m.titulo = "Seu Manel do Bacanal, prefeito";
    m.opcoes = [this]() {
        std::vector<Opcao> v;
        v.push_back({"Conversar", [this]() { conversarPrefeito(); }, nullptr, nullptr});
        v.push_back({"Como funciona o casamento", [this]() {
            fala::dizer("Seu Manel do Bacanal: Casamento eu faço aqui mesmo, na prefeitura! Compra as alianças comigo. Depois é só segurar a aliança "
                        "e apertar Espaço: você vai até a pessoa amada e faz o pedido. Se ela aceitar, todo mundo é avisado, e quem quiser assistir "
                        "vem pelo Tab, em Teleportar para o evento. Depois da cerimônia, festa no salão, com comes e bebes. Só funciona no jogo online, com outro jogador.");
        }, nullptr, nullptr});
        if (conjuge.empty())
            v.push_back({"Comprar alianças de casamento: 50 moedas", [this]() {
                int* m = carteira();
                if (!m || *m < 50) { fala::dizer("Seu Manel do Bacanal: Aliança é ouro, não é bijuteria! Volta com 50 moedas."); return; }
                *m -= 50; guardar({"alianca", 0});
                audio::tocar("moedas", 0.7f);
                fala::dizer("Seu Manel do Bacanal tira do cofre uma caixinha de veludo: Aqui estão. Boa sorte no pedido! Restam " + std::to_string(*m) + " moedas.");
            }, nullptr, nullptr});
        return v;
    };
    abrirMenu(m);
}

void Jogo::teleportarPara(const std::string& mundoCan, int tx, int ty) {
    std::string destino = mundoCan == "cidade" ? "cidade" : mundoCan == "fazenda:" + rede::meuNome() ? "fazenda"
                        : mundoCan.rfind("fazenda:", 0) == 0 ? "visita:" + mundoCan.substr(8) : "cidade";
    if (destino != mundoAtual) { chegadaCalada = true; chegarEm(destino); chegadaCalada = false; }
    // um quadrado livre colado no ponto, de frente para ele
    for (int raio = 0; raio <= 3; raio++)
        for (int d = 0; d < 8; d++) {
            int px = tx + DX[d] * raio, py = ty + DY[d] * raio;
            if (raio && mapa.passa(px, py) && !(px == tx && py == ty)) {
                x = px; y = py; rumo = (d + 4) % 8;
                lugarAtual = mapa.nomeLugar(x, y);
                return;
            }
            if (!raio && mapa.passa(px, py)) { x = px; y = py; lugarAtual = mapa.nomeLugar(x, y); return; }
        }
}

void Jogo::usarAlianca() {
    if (!rede::online()) { fala::dizer("Casamento é no jogo online, com outro jogador."); return; }
    if (!conjuge.empty()) { fala::dizer("Você já é casad" + gen("o", "a") + " com " + conjuge + "."); return; }
    auto outros = rede::outrosJogadores();
    if (outros.empty()) { fala::dizer("Não tem mais ninguém online para pedir em casamento."); return; }
    Menu m;
    m.titulo = "Pedir quem em casamento";
    m.opcoes = [this, outros]() {
        std::vector<Opcao> v;
        for (const std::string& n : outros)
            v.push_back({n, [this, n]() {
                while (!menus.empty()) fecharMenu();
                for (const auto& [id, r] : rede::remotos()) if (r.nome == n) teleportarPara(r.mundo, r.x, r.y);
                rede::enviarApp("pedido", n, std::string(1, genero));
                audio::tocar("chegou", 0.6f);
                fala::dizer("Num piscar de olhos você está diante de " + n + ". Você se ajoelha e abre a caixinha com a aliança. Agora é esperar a resposta.");
            }, nullptr, nullptr});
        return v;
    };
    abrirMenu(m);
}

void Jogo::receberApp(const std::string& tipo, const std::string& de, const std::string& dados) {
    if (tipo == "pedido") {
        char gde = dados.empty() ? 'h' : dados[0];
        audio::tocar("chegou", 0.7f);
        Menu m;
        m.titulo = de + " está de joelhos na sua frente, com uma aliança na mão, e pergunta: quer casar comigo?";
        m.opcoes = [this, de, gde]() {
            return std::vector<Opcao>{
                {"Sim, aceito", [this, de, gde]() {
                    while (!menus.empty()) fecharMenu();
                    rede::enviarApp("resposta", de, std::string("sim") + genero);
                    std::string dadosEv = de + "|" + gde + "|" + rede::meuNome() + "|" + genero;
                    rede::enviarApp("evento-casamento", "*", dadosEv);
                    comecarCasamento(de, gde, rede::meuNome(), genero);
                }, nullptr, nullptr},
                {"Não, obrigad" + gen("o", "a"), [this, de]() {
                    while (!menus.empty()) fecharMenu();
                    rede::enviarApp("resposta", de, "nao");
                    fala::dizer("Você recusa, com jeito.");
                }, nullptr, nullptr},
            };
        };
        abrirMenu(m);
    } else if (tipo == "resposta") {
        if (dados.rfind("sim", 0) == 0) fala::dizer(de + " disse sim! O casamento vai ser na prefeitura.");
        else fala::dizer(de + " recusou o pedido. A aliança continua com você.");
    } else if (tipo == "evento-casamento") {
        std::istringstream s(dados); std::string a, ga, b, gb;
        std::getline(s, a, '|'); std::getline(s, ga, '|'); std::getline(s, b, '|'); std::getline(s, gb, '|');
        comecarCasamento(a, ga.empty() ? 'h' : ga[0], b, gb.empty() ? 'h' : gb[0]);
    } else if (tipo == "proposta-fazenda") {
        std::string dono = dados;
        Menu m;
        m.titulo = de + " quer que vocês morem " + (dono == rede::meuNome() ? "na sua fazenda" : generoConjuge == 'm' ? "na fazenda dela" : "na fazenda dele") + ". Aceita?";
        m.opcoes = [this, de, dono]() {
            return std::vector<Opcao>{
                {"Sim", [this, de, dono]() {
                    while (!menus.empty()) fecharMenu();
                    rede::enviarApp("fazenda-ok", de, dono);
                    receberApp("fazenda-ok", de, dono);
                }, nullptr, nullptr},
                {"Não", [this, de]() { while (!menus.empty()) fecharMenu(); rede::enviarApp("fazenda-nao", de, ""); fala::dizer("Vocês ficam cada um na sua, por enquanto."); }, nullptr, nullptr},
            };
        };
        abrirMenu(m);
    } else if (tipo == "fazenda-nao") {
        fala::dizer(de + " preferiu não decidir agora.");
    } else if (tipo == "fazenda-ok") {
        moraCom = dados;
        if (dados == rede::meuNome()) {
            fala::dizer("Decidido: o casal mora aqui, na sua fazenda, que agora é a fazenda de " + rede::meuNome() + " e " + conjuge + ". " + conjuge + " pode ajudar a cuidar dela.");
        } else {  // quem se muda: a fazenda antiga desaparece
            if (mundoAtual == "fazenda") chegarEm("cidade");
            if (fazendaGuardada) {
                fazendaGuardada->mapa = Mapa(); fazendaGuardada->plantas.clear(); fazendaGuardada->limpos.clear();
                std::swap(moveis, fazendaGuardada->moveis); criarMoveis(); std::swap(moveis, fazendaGuardada->moveis); ocupar();
            }
            fala::dizer("Decidido: o casal mora na fazenda de " + dados + ", que agora é a fazenda de " + dados + " e " + rede::meuNome() + ". "
                        "A sua fazenda antiga não existe mais. Para ir para casa: Espaço no trator. Lá você pode limpar, plantar, regar, colher e abrir portas. "
                        "Ela só abre quando " + dados + " estiver online.");
        }
        evento("fazenda-casal");
    } else if (tipo == "acao") {
        if (de == conjuge && moraCom == rede::meuNome()) aplicarAcao(dados);  // só quem mora junto mexe na minha fazenda
    } else if (tipo == "estado") {
        auto p = dados.find('\n');
        std::string dono = dados.substr(0, p);
        if (mundoAtual == "visita:" + dono && p != std::string::npos) aplicarEstado(dados.substr(p + 1));
    }
}

void Jogo::comecarCasamento(const std::string& a, char ga, const std::string& b, char gb) {
    evento_ = {"casamento", a, b, ga, gb, agora + 120};
    bool noivo = a == rede::meuNome() || b == rede::meuNome();
    audio::tocar("sinos", 0.8f);
    if (noivo) {
        bool souA = a == rede::meuNome();
        teleportarPara("cidade", souA ? ALTAR_A[0] : ALTAR_B[0], souA ? ALTAR_A[1] : ALTAR_B[1]);
        x = souA ? ALTAR_A[0] : ALTAR_B[0]; y = souA ? ALTAR_A[1] : ALTAR_B[1]; rumo = 0;
        fala::dizer("Num instante você está na prefeitura, na frente da mesa do prefeito, ao lado de " + (souA ? b : a) + ". A cerimônia começa em meio minuto, esperando os convidados.");
    } else {
        fala::dizer("Evento! Casamento de " + casal(a, ga, b, gb) + ", na prefeitura da cidade, daqui a meio minuto. Para assistir: Tab, Teleportar para o evento.", false);
    }
    marcar(30, [this]() { cerimonia(); });
}

void Jogo::cerimonia() {
    const Evento e = evento_;
    bool presente = mundoAtual == "cidade" && mapa.nomeLugar(x, y) == "prefeitura";
    bool noivo = e.a == rede::meuNome() || e.b == rede::meuNome();
    if (noivo) {  // os dois casam, estejam onde estiverem
        conjuge = e.a == rede::meuNome() ? e.b : e.a;
        generoConjuge = e.a == rede::meuNome() ? e.gb : e.ga;
        for (Coisa* m : {&maoDireita, &maoEsquerda}) if (m->tipo == "alianca") *m = {};
        tirar({"alianca", 0});
        guardar({"alianca-casado", 0});
    }
    if (!presente && !noivo) {
        avisar("Na prefeitura, " + e.a + " e " + e.b + " se casaram. A festa é no salão de festas da cidade: Tab, Teleportar para o evento.");
        marcar(40, [this]() { comecarFesta(); });
        return;
    }
    auto tratamento = [](char g) { return g == 'm' ? "sua legítima esposa" : "seu legítimo esposo"; };
    std::string declara = e.ga == 'h' && e.gb == 'h' ? "marido e marido" : e.ga == 'm' && e.gb == 'm' ? "esposa e esposa" : "marido e mulher";
    audio::tocar("sinos", 0.9f);
    fala::dizer("Seu Manel do Bacanal ajeita a gravata, pigarreia e abre o livro de registros. Estamos aqui reunidos, nesta prefeitura, para unir em casamento "
                + e.a + " e " + e.b + ". Que a terra dê fartura, que a chuva venha na hora certa, e que o amor de vocês cresça mais que milho em ano bom.", false);
    fala::dizer(e.a + ", você aceita " + e.b + " como " + tratamento(e.gb) + ", na seca e na chuva, na colheita boa e na ruim? " + e.a + " responde: Aceito!", false);
    fala::dizer(e.b + ", você aceita " + e.a + " como " + tratamento(e.ga) + ", na seca e na chuva, na colheita boa e na ruim? " + e.b + " responde: Aceito!", false);
    fala::dizer("Pelo poder que me foi dado por esta cidade, eu vos declaro " + declara + "! Pode beijar! Agora, todo mundo para o salão de festas, que os comes e bebes estão na mesa!", false);
    marcar(28, [this]() { audio::tocar("aplausos", 0.9f); });
    marcar(36, [this]() { comecarFesta(); });
    evento("casou");
}

void Jogo::comecarFesta() {
    bool presente = mundoAtual == "cidade" && mapa.nomeLugar(x, y) == "prefeitura";
    evento_.tipo = "festa";
    evento_.fim = agora + 30 * 60;  // a festa dura meia hora de verdade
    pegosNaFesta.clear();
    if (presente) {
        teleportarPara("cidade", SALAO[0], SALAO[1]);
        x = SALAO[0]; y = SALAO[1]; rumo = 0;
        fala::dizer("Todo mundo atravessa a rua para o salão de festas. Música, conversa, e a mesa de comes e bebes logo à sua frente.");
    }
}

void Jogo::irAoEvento() {
    if (evento_.tipo.empty() || agora > evento_.fim) { fala::dizer("Não tem evento acontecendo agora."); return; }
    while (!menus.empty()) fecharMenu();
    if (evento_.tipo == "casamento") { teleportarPara("cidade", PLATEIA[0], PLATEIA[1]); x = PLATEIA[0]; y = PLATEIA[1]; rumo = 0; fala::dizer("Você aparece na prefeitura, entre os convidados, de frente para o altar."); }
    else { teleportarPara("cidade", SALAO[0], SALAO[1]); x = SALAO[0]; y = SALAO[1]; rumo = 0; fala::dizer("Você aparece no salão de festas, de frente para a mesa de comes e bebes."); }
}

void Jogo::menuMesaFesta() {
    if (evento_.tipo != "festa" || agora > evento_.fim) { fala::dizer("A mesa está vazia, só com a toalha de renda. Comes e bebes, só quando tem festa de casamento."); return; }
    Menu m;
    m.titulo = "Mesa de comes e bebes. Cada pessoa pode pegar até 5 de cada";
    m.opcoes = [this]() {
        std::vector<Opcao> v;
        for (const char* t : COMES) {
            std::string tp = t;
            Coisa c = tp == "agua" ? Coisa{"garrafa", 4} : Coisa{tp, 0};
            int n = pegosNaFesta[tp];
            v.push_back({maiuscula(nomeCoisa(c)) + (n ? ", você já pegou " + std::to_string(n) : ""), [this, tp, c]() {
                int& n = pegosNaFesta[tp];
                if (n >= 5) { fala::dizer("Já pegou 5. Deixa para os outros!"); return; }
                n++; guardar(c);
                audio::tocar("mochila", 0.4f);
                fala::dizer("Você pega " + nomeCoisa(c, true) + ". Está na mochila.");
            }, nullptr, nullptr});
        }
        return v;
    };
    abrirMenu(m);
}

void Jogo::conversarSobreFazenda() {
    if (conjuge.empty()) { fala::dizer("Isso é para quem é casado."); return; }
    bool online = std::find(rede::outrosJogadores().begin(), rede::outrosJogadores().end(), conjuge) != rede::outrosJogadores().end();
    if (!online) { fala::dizer(conjuge + " precisa estar online para vocês conversarem sobre a fazenda."); return; }
    Menu m;
    m.titulo = "Conversar com " + conjuge + " sobre onde morar";
    m.opcoes = [this]() {
        return std::vector<Opcao>{
            {"Morar na minha fazenda", [this]() { while (!menus.empty()) fecharMenu(); rede::enviarApp("proposta-fazenda", conjuge, rede::meuNome()); fala::dizer("Proposta enviada. Esperando " + conjuge + " responder."); }, nullptr, nullptr},
            {"Morar na fazenda de " + conjuge, [this]() { while (!menus.empty()) fecharMenu(); rede::enviarApp("proposta-fazenda", conjuge, conjuge); fala::dizer("Proposta enviada. Esperando " + conjuge + " responder."); }, nullptr, nullptr},
        };
    };
    abrirMenu(m);
}

// ---------- quem mora junto ajuda na fazenda do casal ----------
void Jogo::acaoNaFazenda(const std::string& acao) {
    if (mundoAtual.rfind("visita:", 0) == 0) rede::enviarApp("acao", mundoAtual.substr(7), acao);
}

void Jogo::aplicarAcao(const std::string& acao) {
    bool fora = (bool)fazendaGuardada;  // a fazenda é minha: aplico nela, esteja eu onde estiver
    if (fora) trocarComGuardada();
    std::istringstream s(acao);
    std::string k; s >> k;
    int c = -1; s >> c;
    if (c >= 0 && c < LADO * LADO) {
        int cx = c % LADO, cy = c / LADO;
        if (k == "limpo" && mapa.em(cx, cy).chao == Chao::Mato) { mapa.em(cx, cy).chao = Chao::TerraArada; limpos.push_back(c); }
        else if (k == "planta") { int tp = 1; s >> tp; plantas[c] = Planta{tp, 0, 0}; mapa.em(cx, cy).planta = (uint8_t)tp; }
        else if (k == "rega" && plantas.count(c)) plantas[c].molhadaAte = minutos + 24 * 60;
        else if (k == "colhe" && plantas.count(c)) { double h; s >> h; if (h >= 0) plantas[c].horas = h; else { plantas.erase(c); mapa.em(cx, cy).planta = 0; } }
        else if (k == "porta" && mapa.ehPassagem(cx, cy)) { int a = 0; s >> a; mapa.em(cx, cy).aberta = a != 0; }
    }
    if (fora) trocarComGuardada();
    rede::enviarApp("estado", "*", rede::meuNome() + "\n" + estadoFazenda());
}
