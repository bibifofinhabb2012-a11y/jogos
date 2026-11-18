// Menus: geladeira, guarda-roupa, menu do Tab, configurações.
// Setas para cima e para baixo escolhem, Enter ativa, Shift Enter ativa "tudo" (pegar todas),
// setas para os lados ajustam (volume), uma letra pula para a opção com aquela inicial, Esc volta.
#include <algorithm>
#include <cctype>
#include <cmath>
#include <map>
#include "audio.h"
#include "fala.h"
#include "jogo.h"
#include "rede.h"
#include "util.h"

void Jogo::abrirMenu(Menu m) {
    m.sel = 0;
    menus.push_back(m);
    audio::tocar("menu-abrir", 0.5f);
    const std::string& t = menus.back().titulo;
    falarOpcao(t + (!t.empty() && (t.back() == '?' || t.back() == '.') ? " " : ". "));
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
    m.pausa = !rede::online();  // no online o mundo não para
    if (rede::online()) m.titulo = "Menu. O jogo online não pausa";
    m.opcoes = [this]() {
        std::vector<Opcao> v;
        v.push_back({passoTutorial >= 0 ? "Tutorial: recomeçar do início" : "Tutorial", [this]() {
            while (!menus.empty()) fecharMenu();
            comecarTutorial();
        }, nullptr, nullptr});
        if (passoTutorial >= 0)
            v.push_back({"Parar o tutorial", [this]() { passoTutorial = -1; while (!menus.empty()) fecharMenu(); fala::dizer("Tutorial parado. Para voltar, Tab e Tutorial.", false); }, nullptr, nullptr});
        if (!rede::online()) {
            v.push_back({"Hospedar online", [this]() { while (!menus.empty()) fecharMenu(); hospedarOnline(); }, nullptr, nullptr});
            v.push_back({"Entrar em jogo online", [this]() { while (!menus.empty()) fecharMenu(); entrarOnline(); }, nullptr, nullptr});
        } else {
            auto outros = rede::outrosJogadores();
            v.push_back({"Jogadores online: " + std::to_string(outros.size() + 1), [this, outros]() {
                std::string l = rede::meuNome() + ", você" + (rede::anfitriao() ? ", hospedando" : "");
                for (const auto& [id, r] : rede::remotos()) {
                    std::string onde = r.mundo == "cidade" ? ", na cidade" : r.mundo == "fazenda:" + r.nome ? ", na própria fazenda"
                                     : r.mundo.rfind("fazenda:", 0) == 0 ? ", visitando a fazenda de " + r.mundo.substr(8) : "";
                    l += ". " + r.nome + onde;
                }
                fala::dizer(l + ".");
            }, nullptr, nullptr});
            v.push_back({rede::anfitriao() ? "Parar de hospedar" : "Sair do jogo online", [this]() {
                rede::sair(); while (!menus.empty()) fecharMenu();
                if (mundoAtual.rfind("visita:", 0) == 0) chegarEm("fazenda");
                fala::dizer("Fora do online. Agora é jogo sozinho de novo.", false);
            }, nullptr, nullptr});
        }
        if (!evento_.tipo.empty() && agora < evento_.fim)
            v.push_back({std::string("Teleportar para o evento: ") + (evento_.tipo == "casamento" ? "casamento de " : "festa de ") + evento_.a + " e " + evento_.b,
                         [this]() { irAoEvento(); }, nullptr, nullptr});
        if (!conjuge.empty() && moraCom.empty() && rede::online())
            v.push_back({"Conversar com " + conjuge + " sobre a fazenda", [this]() { while (!menus.empty()) fecharMenu(); conversarSobreFazenda(); }, nullptr, nullptr});
        if (!rede::online()) v.push_back({"Começar um jogo novo", [this]() { confirmarJogoNovo(false); }, nullptr, nullptr});
        v.push_back({chatAtivo ? "Desativar chat" : "Ativar chat", [this]() {
            chatAtivo = !chatAtivo; if (!arquivoConfig.empty()) salvarConfig(arquivoConfig);
            fala::dizer(chatAtivo ? "Chat ativado. Dois-pontos abre o campo de mensagem." : "Chat desativado. As mensagens não serão lidas.");
        }, nullptr, nullptr});
        v.push_back({"Histórico do chat", [this]() {
            Menu h; h.titulo = "Histórico do chat, da mais nova para a mais velha";
            h.opcoes = [this]() {
                std::vector<Opcao> hv;
                for (auto it = historicoChat.rbegin(); it != historicoChat.rend(); ++it) hv.push_back({*it, [] {}, nullptr, nullptr});
                if (hv.empty()) hv.push_back({"Nenhuma mensagem ainda", [] {}, nullptr, nullptr});
                return hv;
            };
            abrirMenu(h);
        }, nullptr, nullptr});
        v.push_back({vozAtiva ? "Desativar voz" : "Ativar voz", [this]() { ligarVoz(!vozAtiva); }, nullptr, nullptr});
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
            {"voz", "Volume da voz dos outros jogadores"},
            {"musica", "Volume da música de fundo, que ainda não existe"},
        };
        v.push_back({"Volume de cada som, um por um", [this]() { menuVolumeSons(); }, nullptr, nullptr});
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

// Volume de cada som, separado: setas para os lados mudam e tocam o som.
static std::string nomeDoSom(const std::string& n) {
    static const std::map<std::string, std::string> N = {
        {"aplausos", "aplausos do casamento"}, {"arrastar", "arrastar móvel"}, {"arvore", "árvore caindo"}, {"aterrissar", "aterrissar do pulo"},
        {"baque", "baque da vaca caindo"}, {"bar", "movimento do bar"}, {"beber", "beber água"}, {"bip", "bip do rastreio e do radar"},
        {"energia", "bip descendente da energia baixa"}, {"ronco", "barriga roncando de fome"}, {"ofegar", "ofegar de sede"}, {"box", "porta do box"}, {"bussola", "bússola"}, {"cafe", "café"}, {"cama", "cama"},
        {"carro", "caminhonete do pai"}, {"cavar", "cavar e plantar"}, {"cerca", "esbarrar na cerca"}, {"cerveja", "abrir cerveja"},
        {"chama", "chama do fogão"}, {"chat", "mensagem do chat"}, {"chegou", "chegou no lugar"}, {"chuveiro", "chuveiro"}, {"cigarra", "cigarras"},
        {"coco", "cocô"}, {"coice", "coice da vaca"}, {"colher", "colher"}, {"corrego", "córrego"}, {"descarga", "descarga"},
        {"embalagem", "embalagem"}, {"faca", "faca"}, {"festa", "festa"}, {"fogao", "acender o fogão"}, {"fogao-desligar", "apagar o fogão"},
        {"fritura", "fritura"}, {"galo", "galo"}, {"geladeira", "motor da geladeira"}, {"geladeira-porta", "porta da geladeira"},
        {"gole", "gole"}, {"gota", "gota do Ç, do K e do L"}, {"grilos", "grilos"}, {"isqueiro", "isqueiro"}, {"leite", "ordenha"},
        {"machado", "machado"}, {"mastigar", "mastigar"}, {"menu-abrir", "abrir menu"}, {"menu-fechar", "fechar menu"}, {"mochila", "mochila"},
        {"moedas", "moedas"}, {"mugido", "mugido das vacas"}, {"palmas", "palmas"}, {"parede", "esbarrar na parede"}, {"passaros", "pássaros"},
        {"passos-agua", "passos na água"}, {"passos-calcada", "passos na calçada"}, {"passos-cascalho", "passos no cascalho e nas pedras"},
        {"passos-ceramica", "passos na cerâmica"}, {"passos-folhas", "passos nas folhas"}, {"passos-grama", "passos na grama"},
        {"passos-madeira", "passos na madeira"}, {"passos-mato", "passos no mato"}, {"passos-terra", "passos na terra"}, {"peido", "pum"},
        {"picareta", "picareta"}, {"porta", "porta"}, {"porta-abrir", "abrir porta"}, {"porta-fechar", "fechar porta"}, {"porteira", "porteira"},
        {"regador", "regador"}, {"ruido", "ruído do sono"}, {"saco-dormir", "saco de dormir"}, {"sapos", "sapos"}, {"sementes", "sementes"},
        {"sinos", "sinos"}, {"sofa", "sofá"}, {"tampa", "tampa"}, {"torneira", "torneira"}, {"tosse", "tosse"}, {"tragada", "tragada do cigarro"},
        {"trator", "trator"}, {"moedor", "moedor de café"}, {"expresso", "cafeteira expresso"}, {"vaporizador", "vaporizador de leite"},
        {"moenda", "moenda e moinho"}, {"prensa", "prensa de queijo"},
        {"galinha", "galinhas"}, {"porco", "porcos"}, {"coelho", "coelhos"}, {"martelo", "martelo"}, {"serra", "serra"}, {"fogueira", "fogo do forno a lenha"}, {"fungar", "porquinho trufeiro fungando"}, {"tutorial-ok", "acerto do tutorial"}, {"vento", "vento"}, {"vomito", "vômito"}, {"xixi", "xixi"},
    };
    auto it = N.find(n);
    if (it != N.end()) return it->second;
    std::string r = n; for (char& c : r) if (c == '-') c = ' ';
    return r;
}

void Jogo::menuVolumeSons() {
    Menu m;
    m.titulo = "Volume de cada som. Setas para os lados mudam e tocam o som; uma letra pula";
    m.opcoes = [this]() {
        std::vector<Opcao> v;
        std::vector<std::pair<std::string, std::string>> lista;
        for (const std::string& n : audio::sons()) lista.push_back({nomeDoSom(n), n});
        std::sort(lista.begin(), lista.end());
        for (const auto& [rotulo, n] : lista) {
            int pct = (int)std::lround(audio::volumeSom(n) * 100);
            v.push_back({maiuscula(rotulo) + ": " + std::to_string(pct) + " por cento",
                [this, n]() { audio::tocar(n, 0.8f); }, nullptr,
                [this, n](int lado) {
                    float nv = std::clamp(audio::volumeSom(n) + 0.1f * lado, 0.0f, 2.0f);
                    audio::definirVolumeSom(n, nv);
                    audio::tocar(n, 0.8f);
                    fala::dizer(std::to_string((int)std::lround(nv * 100)) + " por cento.");
                    if (!arquivoConfig.empty()) salvarConfig(arquivoConfig);
                }});
        }
        if (v.empty()) v.push_back({"Nenhum som carregado: o som do computador não abriu", [this]() {}, nullptr, nullptr});
        return v;
    };
    abrirMenu(m);
}

// Ao abrir o jogo com um salvo: continuar ou começar de novo.
void Jogo::menuInicio() {
    Menu m;
    m.titulo = "Simulador de Vida. Tem um jogo salvo: dia " + std::to_string(diaDoJogo()) + ". Setas para escolher, Enter para confirmar";
    m.pausa = true; m.obrigatorio = true;
    m.opcoes = [this]() {
        std::vector<Opcao> v;
        v.push_back({"Continuar o jogo salvo", [this]() { while (!menus.empty()) fecharMenu(); abertura(false); }, nullptr, nullptr});
        v.push_back({"Começar um jogo novo", [this]() { confirmarJogoNovo(true); }, nullptr, nullptr});
        return v;
    };
    abrirMenu(m);
}

void Jogo::confirmarJogoNovo(bool doInicio) {
    Menu m;
    m.titulo = "Começar um jogo novo apaga o jogo salvo: a fazenda, a mochila, o dinheiro, tudo. Tem certeza?";
    m.pausa = true;
    m.obrigatorio = doInicio;
    m.opcoes = [this, doInicio]() {
        std::vector<Opcao> v;
        v.push_back({"Não, voltar", [this, doInicio]() { fecharMenu(); if (!doInicio) fala::dizer("Nada mudou.", false); }, nullptr, nullptr});
        v.push_back({"Sim, apagar e começar de novo", [this]() {
            while (!menus.empty()) fecharMenu();
            novo();
            if (!arquivoSalvo.empty()) salvar(arquivoSalvo);
            abertura(true);
        }, nullptr, nullptr});
        return v;
    };
    abrirMenu(m);
}
