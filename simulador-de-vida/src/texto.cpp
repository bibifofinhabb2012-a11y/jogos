// Campo de escrita, chat de texto e as opções do jogo online (hospedar, entrar, sair).
#include <cmath>
#include "audio.h"
#include "fala.h"
#include "jogo.h"
#include "util.h"
#include "voz.h"

// nome falado de um caractere digitado
static std::string falaDe(const std::string& c) {
    if (c == " ") return "espaço";
    if (c == ".") return "ponto";
    if (c == ",") return "vírgula";
    if (c == "?") return "interrogação";
    if (c == "!") return "exclamação";
    if (c == ":") return "dois-pontos";
    return c;
}

void Jogo::abrirCampo(const std::string& titulo, const std::string& inicial, std::function<void(const std::string&)> ok) {
    campo.ativo = true; campo.titulo = titulo; campo.texto = inicial; campo.ok = std::move(ok);
    audio::tocar("menu-abrir", 0.5f);
    fala::dizer(titulo + ". Campo de edição" + (inicial.empty() ? "" : ", com: " + inicial) + ". Digite e aperte Enter; Esc cancela.");
}

void Jogo::texto(const std::string& utf8) {
    if (campo.ativo) {
        campo.texto += utf8;
        fala::dizer(falaDe(utf8));
        return;
    }
    if (utf8 == ":" && menus.empty()) abrirChat();
}

void Jogo::teclaCampo(Tecla t) {
    if (t == Tecla::Enter) {
        campo.ativo = false;
        audio::tocar("menu-fechar", 0.5f);
        auto ok = campo.ok; std::string txt = campo.texto;
        if (ok) ok(txt);
    } else if (t == Tecla::Esc) {
        campo.ativo = false;
        audio::tocar("menu-fechar", 0.5f);
        fala::dizer("Cancelado.");
    } else if (t == Tecla::Backspace) {
        if (campo.texto.empty()) { fala::dizer("Vazio."); return; }
        size_t p = campo.texto.size() - 1;
        while (p > 0 && ((unsigned char)campo.texto[p] & 0xC0) == 0x80) p--;  // volta um caractere inteiro (acentos têm 2 bytes)
        std::string c = campo.texto.substr(p);
        campo.texto.erase(p);
        fala::dizer("Apagou " + falaDe(c) + ".");
    } else if (t == Tecla::Cima || t == Tecla::Baixo) {
        fala::dizer(campo.texto.empty() ? campo.titulo + ": vazio." : campo.titulo + ": " + campo.texto);
    }
}

void Jogo::abrirChat() {
    if (!rede::online()) { fala::dizer("O chat funciona no jogo online. Hospede ou entre pelo menu do Tab."); return; }
    if (!chatAtivo) { fala::dizer("O chat está desativado. Ative no menu do Tab."); return; }
    abrirCampo("Mensagem para todos", "", [this](const std::string& t) {
        if (t.empty()) { fala::dizer("Mensagem vazia, não foi."); return; }
        rede::enviarChat(t);
        historicoChat.push_back("Você: " + t);
        audio::tocar("chat", 0.5f);
        fala::dizer("Enviada.");
    });
}

rede::Ganchos Jogo::ganchos() {
    rede::Ganchos g;
    g.chat = [this](const std::string& nome, const std::string& t) {
        historicoChat.push_back(nome + ": " + t);
        if (historicoChat.size() > 50) historicoChat.erase(historicoChat.begin());
        if (!chatAtivo) return;
        audio::tocar("chat", 0.6f);
        fala::dizer(nome + ": " + t, false);
    };
    g.entrou = [this](const std::string& n) { audio::tocar("chegou", 0.6f); avisar(n + " entrou no jogo."); };
    g.saiu = [this](const std::string& n) { audio::tocar("menu-fechar", 0.6f); avisar(n + " saiu do jogo."); };
    g.relogio = [this](double m) { if (!rede::anfitriao() && std::fabs(m - minutos) > 0.3) minutos = m; };
    g.estadoFazenda = [this]() { return estadoFazenda(); };
    g.fazendaChegou = [this](const std::string& t) { aplicarEstado(t); };
    g.voz = [this](int id, const unsigned char* d, int n) {
        pacotesVoz++;
        auto it = rede::remotos().find(id);
        if (it == rede::remotos().end() || it->second.mundo != mundoCanonico()) return;
        voz::receber(id, d, n, it->second.x + 0.5f, it->second.y + 0.5f);
    };
    g.andou = [this](int id) {
        auto it = rede::remotos().find(id);
        if (it == rede::remotos().end() || it->second.mundo != mundoCanonico() || !mapa.valido(it->second.x, it->second.y)) return;
        const rede::Remoto& r = it->second;
        audio::tocar3D(Mapa::somPasso(mapa.em(r.x, r.y).chao), r.x + 0.5f, r.y + 0.5f, 0.0f, 0.7f, 3, (float)acaso(0.94, 1.06));
    };
    g.aviso = [this](const std::string& m) { avisar(m); };
    return g;
}

// Todo quadro: recebe da rede, manda a posição quando muda, e o anfitrião manda o relógio.
void Jogo::servicoRede(double dt) {
    rede::servico(ganchos());
    if (!rede::online()) return;
    posicaoRede -= dt; relogioRede -= dt;
    std::string mc = mundoCanonico();
    bool mudou = x != enviadoX || y != enviadoY || rumo != enviadoRumo || mc != enviadoMundo;
    if ((mudou && posicaoRede <= 0) || posicaoRede < -2) {
        rede::enviarPosicao(mc, x, y, rumo);
        enviadoX = x; enviadoY = y; enviadoRumo = rumo; enviadoMundo = mc;
        posicaoRede = 0.08;
    }
    if (rede::anfitriao() && relogioRede <= 0) { rede::enviarRelogio(minutos); relogioRede = 5; }
    // a voz dos outros sai de onde eles estão; a minha vai para a rede
    for (const auto& [id, r] : rede::remotos()) voz::posicionar(id, r.x + 0.5f, r.y + 0.5f, r.mundo == mc);
    if (vozAtiva) voz::capturar([](const unsigned char* d, int n) { rede::enviarVoz(d, n); });
}

void Jogo::pedirNome(std::function<void()> depois) {
    if (!nomeOnline.empty()) { depois(); return; }
    abrirCampo("Seu nome no jogo online, como os outros vão te chamar", "", [this, depois](const std::string& n) {
        if (n.empty()) { fala::dizer("Sem nome não dá. Tente de novo pelo menu do Tab."); return; }
        nomeOnline = n;
        if (!arquivoConfig.empty()) salvarConfig(arquivoConfig);
        depois();
    });
}

void Jogo::hospedarOnline() {
    pedirNome([this]() {
        std::string erro;
        if (!rede::hospedar(nomeOnline, erro)) { fala::dizer("Não deu para hospedar. " + erro); return; }
        std::string ends;
        for (const std::string& e : rede::enderecosLocais()) ends += (ends.empty() ? "" : ", ") + e;
        enviadoMundo.clear();
        fala::dizer("Hospedando o jogo online, com o nome " + nomeOnline + ". Endereços deste computador: " + (ends.empty() ? "nenhum encontrado" : ends) +
                    ". O da Radmin começa com 26: passe esse número para seus amigos. Eles entram pelo menu do Tab, em Entrar em jogo online.");
        audio::tocar("tutorial-ok", 0.6f);
    });
}

void Jogo::entrarOnline() {
    pedirNome([this]() {
        abrirCampo("Endereço de quem hospeda, o número da Radmin", ultimoIp, [this](const std::string& ip) {
            if (ip.empty()) { fala::dizer("Sem endereço não dá para entrar."); return; }
            ultimoIp = ip;
            if (!arquivoConfig.empty()) salvarConfig(arquivoConfig);
            fala::dizer("Conectando a " + ip + "...");
            std::string erro;
            if (!rede::entrar(ip, nomeOnline, erro)) { fala::dizer(erro); return; }
            enviadoMundo.clear();
            rede::mudouDeMundo(mundoCanonico());
            audio::tocar("tutorial-ok", 0.6f);
            fala::dizer("Conectado! Agora o relógio é o de quem hospeda, e dormir não pula o dia. O trator leva você à fazenda dos outros e à cidade. "
                        "Dois-pontos abre o chat.", false);
        });
    });
}

void Jogo::ligarVoz(bool ligar) {
    if (ligar && !voz::iniciarCaptura()) { fala::dizer("Não consegui abrir o microfone. Confira se ele está ligado e liberado para programas no Windows."); return; }
    if (!ligar) voz::pararCaptura();
    vozAtiva = ligar;
    fala::dizer(ligar ? "Voz ativada. Quem estiver perto de você ouve o que você fala, vindo de onde o seu personagem está. Use fone de ouvido, senão vira eco."
                      : "Voz desativada. Seu microfone está fechado.");
}
