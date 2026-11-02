// Menus: geladeira, guarda-roupa, menu do Tab, configurações.
// Setas para cima e para baixo escolhem, Enter ativa, Shift Enter ativa "tudo" (pegar todas),
// setas para os lados ajustam (volume), uma letra pula para a opção com aquela inicial, Esc volta.
#include <algorithm>
#include <cctype>
#include "audio.h"
#include "fala.h"
#include "jogo.h"
#include "util.h"

void Jogo::abrirMenu(Menu m) {
    m.sel = 0;
    menus.push_back(m);
    audio::tocar("menu-abrir", 0.5f);
    falarOpcao(menus.back().titulo + ". ");
}

void Jogo::fecharMenu() {
    if (menus.empty()) return;
    Menu m = menus.back();
    menus.pop_back();
    audio::tocar("menu-fechar", 0.5f);
    if (m.aoFechar) m.aoFechar();
    if (!menus.empty()) falarOpcao(menus.back().titulo + ". ");
}

void Jogo::falarOpcao(const std::string& antes) {
    if (menus.empty()) return;
    Menu& m = menus.back();
    auto ops = m.opcoes();
    if (ops.empty()) { fala::dizer(antes + "Vazio."); return; }
    m.sel = std::clamp(m.sel, 0, (int)ops.size() - 1);
    fala::dizer(antes + ops[m.sel].texto + ". " + std::to_string(m.sel + 1) + " de " + std::to_string(ops.size()) + ".");
}

static char inicial(const std::string& s) {
    // primeira letra, sem acento (á, é, í, ó, ú, â, ê, ô, ã, õ, ç viram a, e, i, o, u, c)
    if (s.empty()) return 0;
    unsigned char c = (unsigned char)s[0];
    if (c < 128) return (char)std::tolower(c);
    if (c == 0xC3 && s.size() > 1) {
        unsigned char d = (unsigned char)s[1];
        if (d >= 0x80 && d <= 0x85) return 'a';
        if (d >= 0xA0 && d <= 0xA5) return 'a';
        if ((d >= 0x88 && d <= 0x8B) || (d >= 0xA8 && d <= 0xAB)) return 'e';
        if ((d >= 0x8C && d <= 0x8F) || (d >= 0xAC && d <= 0xAF)) return 'i';
        if ((d >= 0x92 && d <= 0x96) || (d >= 0xB2 && d <= 0xB6)) return 'o';
        if ((d >= 0x99 && d <= 0x9C) || (d >= 0xB9 && d <= 0xBC)) return 'u';
        if (d == 0x87 || d == 0xA7) return 'c';
    }
    return 0;
}
char inicialSemAcento(const std::string& s) { return inicial(s); }

bool Jogo::teclaMenu(Tecla t, bool shift, char letra) {
    if (menus.empty()) return false;
    Menu& m = menus.back();
    auto ops = m.opcoes();
    int n = (int)ops.size();
    switch (t) {
        case Tecla::Cima: if (n) { m.sel = (m.sel - 1 + n) % n; audio::tocar("gota", 0.5f); falarOpcao(); } return true;
        case Tecla::Baixo: if (n) { m.sel = (m.sel + 1) % n; audio::tocar("gota", 0.5f); falarOpcao(); } return true;
        case Tecla::Enter: case Tecla::Espaco:
            if (!n) return true;
            if (shift && ops[m.sel].tudo) ops[m.sel].tudo();
            else if (ops[m.sel].ativar) ops[m.sel].ativar();
            return true;
        case Tecla::Esquerda: case Tecla::Direita:
            if (n && ops[m.sel].ajustar) ops[m.sel].ajustar(t == Tecla::Direita ? 1 : -1);
            return true;
        case Tecla::Esc: if (!m.obrigatorio) fecharMenu(); else fala::dizer("Escolha uma opção com as setas e aperte Enter."); return true;
        case Tecla::Tab:
            if (m.obrigatorio) { fala::dizer("Escolha uma opção com as setas e aperte Enter."); return true; }
            if (pausado()) { while (!menus.empty()) fecharMenu(); fala::dizer("De volta ao jogo."); }
            else { while (!menus.empty()) fecharMenu(); menuPrincipal(); }
            return true;
        case Tecla::F2: fala::dizer(fala::ultima()); return true;
        default: break;
    }
    if (letra && n) {
        char l = (char)std::tolower((unsigned char)letra);
        for (int k = 1; k <= n; k++) {
            int j = (m.sel + k) % n;
            if (inicial(ops[j].texto) == l) { m.sel = j; audio::tocar("gota", 0.5f); falarOpcao(); return true; }
        }
        fala::dizer(std::string("Nada com a letra ") + l + ".");
    }
    return true;  // com menu aberto, as outras teclas não mexem no jogo
}

// Tab: tutorial, configurações, sair. O jogo fica parado enquanto este menu está aberto.
void Jogo::menuPrincipal() {
    Menu m;
    m.titulo = "Menu";
    m.pausa = true;
    m.opcoes = [this]() {
        std::vector<Opcao> v;
        v.push_back({passoTutorial >= 0 ? "Tutorial: recomeçar do início" : "Tutorial", [this]() {
            while (!menus.empty()) fecharMenu();
            comecarTutorial();
        }, nullptr, nullptr});
        if (passoTutorial >= 0)
            v.push_back({"Parar o tutorial", [this]() { passoTutorial = -1; while (!menus.empty()) fecharMenu(); fala::dizer("Tutorial parado. Para voltar, Tab e Tutorial.", false); }, nullptr, nullptr});
        v.push_back({"Configurações", [this]() { menuConfiguracoes(); }, nullptr, nullptr});
        v.push_back({"Sair do jogo", [this]() { sair = true; fala::dizer("Jogo salvo. Até logo."); }, nullptr, nullptr});
        return v;
    };
    abrirMenu(m);
}

void Jogo::menuConfiguracoes() {
    Menu m;
    m.titulo = "Configurações. Setas para os lados mudam o volume";
    m.opcoes = [this]() {
        std::vector<Opcao> v;
        struct { const char* cat; const char* nome; } cats[] = {
            {"ambiente", "Volume do ambiente: vento, córrego, bichos, geladeira"},
            {"passos", "Volume dos passos"},
            {"efeitos", "Volume dos efeitos: portas, objetos, corpo"},
            {"interface", "Volume dos sons de navegação e do rastreio"},
            {"musica", "Volume da música de fundo, que ainda não existe"},
        };
        for (auto& c : cats) {
            std::string cat = c.cat;
            int pct = (int)std::lround(audio::volume(cat) * 100);
            v.push_back({std::string(c.nome) + ": " + std::to_string(pct) + " por cento",
                [this]() { fala::dizer("Use as setas para a esquerda e para a direita para mudar."); }, nullptr,
                [this, cat](int lado) {
                    float nv = std::clamp(audio::volume(cat) + 0.1f * lado, 0.0f, 1.5f);
                    audio::definirVolume(cat, nv);
                    if (cat == "passos") audio::tocar("passos-grama", 0.7f);
                    else if (cat == "interface") audio::tocar("gota", 0.6f);
                    else if (cat == "efeitos") audio::tocar("palmas", 0.8f);
                    fala::dizer(std::to_string((int)std::lround(nv * 100)) + " por cento.");
                    if (!arquivoConfig.empty()) salvarConfig(arquivoConfig);
                }});
        }
        return v;
    };
    abrirMenu(m);
}
