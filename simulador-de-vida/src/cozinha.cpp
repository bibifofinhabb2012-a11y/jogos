// Cozinha com as mãos: frigideira e panela levam o que se põe nelas (T), vão ao fogo (Espaço de frente para o fogão aceso),
// fritam com tempo e saem de lá (Espaço de mão vazia de frente para o fogão). T com a outra mão vazia tira a comida.
// Primeira receita: omelete de queijo (3 ovos quebrados na panela, meio queijo, fogo).
#include <algorithm>
#include <cmath>
#include "audio.h"
#include "fala.h"
#include "jogo.h"
#include "util.h"

struct Receita { const char* crua; const char* pronta; const char* queimada; double segundos; const char* aviso; };
static const Receita NO_FOGO[] = {
    {"frigideira-ovo", "frigideira-ovo-frito", "frigideira-queimada", 8, "O ovo fritou: a clara está branquinha e a gema ainda mole."},
    {"panela-omelete-crua", "panela-omelete", "panela-queimada", 20, "A omelete dourou por baixo e firmou por cima, com o queijo derretendo."},
};
static const Receita* receitaDe(const std::string& t) { for (const Receita& r : NO_FOGO) if (t == r.crua) return &r; return nullptr; }
static const double QUEIMA = 40;  // segundos depois de pronto, queima

// T com as duas mãos ocupadas: o que se junta na cozinha. true: respondeu.
bool Jogo::combinarCozinha(Coisa& d, Coisa& e) {
    auto par = [&](const char* a, const char* b) { return (d.tipo == a && e.tipo == b) || (d.tipo == b && e.tipo == a); };
    auto qual = [&](const char* a) -> Coisa& { return d.tipo == a ? d : e; };
    if (par("frigideira", "ovo")) {
        Coisa& f = qual("frigideira"); Coisa& o = qual("ovo");
        o = {};
        f.tipo = "frigideira-ovo";
        audio::tocar("tampa", 0.5f, 1.6f);
        fala::dizer("Você bate o ovo na beirada da frigideira e abre: cai inteiro, com a gema no meio. Frigideira com ovo cru. Agora leve ao fogão aceso: Espaço de frente para ele.");
        return true;
    }
    if (par("panela", "ovo-quebrado")) {
        int n = contarTipo("ovo-quebrado");
        if (n < 3) { fala::dizer("Para a omelete, precisa de 3 ovos quebrados. Você tem " + std::to_string(n) + ". Ovo inteiro numa mão e a outra vazia, T, quebra."); return true; }
        Coisa& p = qual("panela");
        gastarTipo("ovo-quebrado", 3);
        p.tipo = "panela-ovos";
        audio::tocar("tampa", 0.5f, 1.3f);
        fala::dizer("Você despeja os 3 ovos quebrados na panela e bate com o garfo até ficar tudo amarelinho. Agora acrescente meio queijo: queijo e faca de cozinha, T, corta.");
        return true;
    }
    if (par("panela", "meio-queijo")) { fala::dizer("Primeiro os ovos: 3 ovos quebrados na panela. O queijo vem depois."); return true; }
    if (par("queijo", "faca-cozinha")) {
        Coisa& q = qual("queijo");
        q = {};
        guardar({"meio-queijo", 0}, 2);
        audio::tocar("faca", 0.6f, 1.2f);
        fala::dizer("Você apoia o queijo na tábua e corta ao meio com a faca de cozinha. Duas metades de queijo, na mochila.");
        return true;
    }
    if (par("queijo", "faca")) { fala::dizer("Faca de abate não é para cozinha. Use a faca de cozinha: seu Zé vende."); return true; }
    if (par("panela-ovos", "meio-queijo")) {
        Coisa& p = qual("panela-ovos"); Coisa& q = qual("meio-queijo");
        q = {};
        p.tipo = "panela-omelete-crua";
        audio::tocar("cavar", 0.4f, 1.5f);
        fala::dizer("Você esfarela o meio queijo por cima dos ovos batidos e mistura. Omelete pronta para fritar: leve a panela ao fogão aceso, Espaço de frente para ele.");
        return true;
    }
    return false;
}

// T com uma mão vazia: quebrar ovo, tirar a comida da frigideira ou da panela, jogar fora o queimado. true: respondeu.
bool Jogo::separarCozinha(Coisa& c, Coisa& vazia) {
    if (c.tipo == "ovo") {
        c = {};
        if (tirar({"ovo", 0})) c = {"ovo", 0};
        guardar({"ovo-quebrado", 0});
        int n = contarTipo("ovo-quebrado");
        audio::tocar("tampa", 0.6f, 1.7f);
        fala::dizer("Você quebrou o ovo. " + std::to_string(n) + (n == 1 ? " ovo quebrado." : " ovos quebrados.") +
                    " Para fazer omelete, precisa de 3: depois, ovo quebrado e panela, T. Para um bolo, quebre os ovos numa tigela e misture açúcar.");
        return true;
    }
    if (c.tipo == "frigideira-ovo-frito" || c.tipo == "panela-omelete") {
        bool ovo = c.tipo == "frigideira-ovo-frito";
        vazia = {ovo ? "ovo-frito" : "omelete", 0};
        c.tipo = ovo ? "frigideira" : "panela";
        audio::tocar("colher", 0.5f, 1.3f);
        fala::dizer(ovo ? "Você solta o ovo frito com a espátula e passa para a outra mão. A frigideira fica vazia."
                        : "Você vira a omelete de queijo no prato, inteira e dourada: na outra mão. A panela fica vazia.");
        return true;
    }
    if (c.tipo == "frigideira-queimada" || c.tipo == "panela-queimada") {
        c.tipo = c.tipo == "frigideira-queimada" ? "frigideira" : "panela";
        audio::tocar("cavar", 0.5f);
        fala::dizer("Você raspa o queimado com a colher e joga fora. Fica a lição: tirar do fogo assim que o jogo avisar.");
        return true;
    }
    return false;
}

// Recipiente com comida crua na mão, de frente para o fogão aceso: vai para o fogo.
bool Jogo::levarAoFogo(Coisa& c, int f) {
    const Receita* r = receitaDe(c.tipo);
    if (!r) return false;
    Movel& m = moveis[f];
    if (!m.ligado) { fala::dizer("O fogão está apagado. Acenda com Espaço, de mão vazia, e ponha de novo."); return true; }
    if (!m.cozinhando.empty()) { fala::dizer("Já tem coisa no fogo. Tire antes: mão vazia, Espaço de frente para o fogão."); return true; }
    m.cozinhando = c.tipo; m.cozinhaFalta = r->segundos; m.avisouCozinha = 0;
    std::string n = nomeCoisa(c, true);
    c = {};
    if (!m.somFrito) m.somFrito = audio::fonteMovel("fritura", m.x + 0.5f, m.y + 0.5f, 0.95f, 0.6f, 3, true);
    fala::dizer("Você põe " + n + " na boca acesa: começa a chiar. Fica pronta em uns " + std::to_string((int)r->segundos) +
                " segundos; o jogo avisa. Não deixe passar muito, que queima.");
    return true;
}

// Espaço de mão vazia de frente para o fogão com algo no fogo: tira.
void Jogo::tirarDoFogo(int f) {
    Movel& m = moveis[f];
    const Receita* r = receitaDe(m.cozinhando);
    std::string t = m.cozinhando;
    if (r && m.cozinhaFalta > 0) t = m.cozinhando;  // ainda cru: sai como entrou
    else if (r) t = m.cozinhaFalta > -QUEIMA ? r->pronta : r->queimada;
    Coisa& mao1 = mao(maoSel == 0);
    mao1 = {t, 0};
    bool cru = r && m.cozinhaFalta > 0;
    m.cozinhando.clear();
    if (m.somFrito) { audio::pararFonte(m.somFrito); m.somFrito = 0; }
    audio::tocar3D("tampa", m.x + 0.5f, m.y + 0.5f, 0.9f, 0.6f, 2, 0.8f);
    fala::dizer("Você tira do fogo: " + nomeCoisa({t, 0}, true) + "." +
                (cru ? " Ainda estava cru: ponha de volta para terminar." : t.find("queimada") != std::string::npos ? " Queimou. T com a outra mão vazia joga fora." :
                 " T com a outra mão vazia tira a comida."));
}

// O tempo no fogo: avisa quando fica pronto e quando queima. A chama apagada para o cozimento.
void Jogo::atualizarFogo(Movel& m, double dt) {
    if (m.cozinhando.empty()) return;
    const Receita* r = receitaDe(m.cozinhando);
    if (!r) return;
    if (!m.ligado) {
        if (m.somFrito) { audio::pararFonte(m.somFrito); m.somFrito = 0; }
        return;
    }
    if (!m.somFrito) m.somFrito = audio::fonteMovel("fritura", m.x + 0.5f, m.y + 0.5f, 0.95f, 0.6f, 3, true);
    m.cozinhaFalta -= dt;
    if (m.cozinhaFalta <= 0 && m.avisouCozinha == 0) {
        m.avisouCozinha = 1;
        audio::tocar("tutorial-ok", 0.7f);
        fala::dizer(std::string(r->aviso) + " Tire do fogo: mão vazia, Espaço de frente para o fogão.", false);
    }
    if (m.cozinhaFalta <= -QUEIMA / 2 && m.avisouCozinha == 1) {
        m.avisouCozinha = 2;
        audio::tocar("energia", 0.8f, 1.5f);
        fala::dizer("Cheiro de queimado começando! Tire do fogo agora.", false);
    }
    if (m.cozinhaFalta <= -QUEIMA && m.avisouCozinha == 2) {
        m.avisouCozinha = 3;
        fala::dizer("Queimou. Fumaça preta saindo do fogão.", false);
    }
}
