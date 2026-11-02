#include "jogo.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <random>
#include <sstream>
#include "audio.h"
#include "fala.h"

static std::mt19937 sorte{std::random_device{}()};
static double acaso(double a, double b) { return std::uniform_real_distribution<double>(a, b)(sorte); }

// Relógio: 1 segundo de verdade = 12 segundos do jogo (um dia dura 2 horas de verdade).
// Dormindo, 1 segundo de verdade = 10 minutos do jogo.
constexpr double MIN_POR_SEG = 0.2, MIN_POR_SEG_DORMINDO = 10.0;
constexpr int DX[8] = {0, 1, 1, 1, 0, -1, -1, -1}, DY[8] = {1, 1, 0, -1, -1, -1, 0, 1};

Jogo::Jogo() { novo(); }

void Jogo::novo() {
    x = 245; y = 9; rumo = 0;
    fome = 85; sede = 85; energia = 90; saude = 100;
    minutos = 7 * 60;
    mochila = {
        {{"barra", 0}, 100}, {{"garrafa", 4}, 10}, {{"antibiotico", 0}, 50}, {{"atadura", 0}, 10},
        {{"gesso", 0}, 10}, {{"saco-dormir", 0}, 1}, {{"carteira", 100}, 1}, {{"bussola", 0}, 1},
    };
    maoDireita = maoEsquerda = {};
    mochilaAberta = false; itemMochila = 0; dormindo = false;
    for (double& a : ultimoAviso) a = 100;
    lugarAtual = mapa.nomeLugar(x, y);
}

// ---------- nomes ----------
static std::string goles(int n) { return n == 1 ? "1 gole" : std::to_string(n) + " goles"; }

std::string Jogo::nomeCoisa(const Coisa& c, bool comArtigo) const {
    const std::string& t = c.tipo;
    std::string n;
    if (t == "barra") n = "barra de ração embrulhada";
    else if (t == "barra-aberta") n = "barra de ração aberta";
    else if (t == "garrafa" || t == "garrafa-aberta") {
        std::string estado = t == "garrafa" ? "tampada" : "aberta";
        n = c.carga >= 4 ? "garrafa de água " + estado + ", cheia"
          : c.carga == 0 ? "garrafa vazia, " + estado
          : "garrafa de água " + estado + ", com " + goles(c.carga);
    }
    else if (t == "antibiotico") n = "comprimido de antibiótico";
    else if (t == "atadura") n = "atadura";
    else if (t == "gesso") n = "pacote de gesso em pó";
    else if (t == "saco-dormir") n = "saco de dormir";
    else if (t == "carteira") n = "carteira, com " + std::to_string(c.carga) + (c.carga == 1 ? " moeda" : " moedas");
    else if (t == "bussola") n = "bússola";
    else if (t == "tampa") n = "tampa de garrafa";
    else n = t;
    if (!comArtigo) return n;
    bool fem = t != "antibiotico" && t != "pacote" && t != "gesso" && t != "saco-dormir";
    return std::string(fem ? "a " : "o ") + n;
}

static std::string plural(const Coisa& c, int qtd, const std::string& singular) {
    if (qtd == 1) return singular;
    const std::string& t = c.tipo;
    if (t == "barra") return "barras de ração embrulhadas";
    if (t == "barra-aberta") return "barras de ração abertas";
    if (t == "antibiotico") return "comprimidos de antibiótico";
    if (t == "atadura") return "ataduras";
    if (t == "gesso") return "pacotes de gesso em pó";
    if (t == "tampa") return "tampas de garrafa";
    if (t == "garrafa" || t == "garrafa-aberta") {
        std::string estado = t == "garrafa" ? "tampadas" : "abertas";
        return c.carga >= 4 ? "garrafas de água " + estado + ", cheias"
             : c.carga == 0 ? "garrafas vazias, " + estado
             : "garrafas de água " + estado + ", com " + goles(c.carga);
    }
    return singular;
}

static std::string maiuscula(std::string s) {
    // só a primeira letra, e só se for ASCII (as palavras do jogo não começam com acento)
    if (!s.empty() && s[0] >= 'a' && s[0] <= 'z') s[0] = (char)(s[0] - 32);
    return s;
}

// ---------- mochila e mãos ----------
void Jogo::guardar(const Coisa& c0, int qtd) {
    if (c0.tipo.empty()) return;
    Coisa c = c0;
    if (c.tipo == "garrafa-aberta" && tirar({"tampa", 0})) c.tipo = "garrafa";  // tem tampa na mochila: guarda tampada
    for (Pilha& p : mochila) if (p.coisa == c) { p.qtd += qtd; return; }
    mochila.push_back({c, qtd});
}

bool Jogo::tirar(const Coisa& c) {
    for (size_t i = 0; i < mochila.size(); i++)
        if (mochila[i].coisa == c) {
            if (--mochila[i].qtd <= 0) mochila.erase(mochila.begin() + (long)i);
            return true;
        }
    return false;
}

void Jogo::falarItemMochila(const std::string& antes) {
    if (mochila.empty()) { fala::dizer(antes + "A mochila está vazia."); return; }
    itemMochila = std::clamp(itemMochila, 0, (int)mochila.size() - 1);
    const Pilha& p = mochila[itemMochila];
    std::string n = plural(p.coisa, p.qtd, nomeCoisa(p.coisa));
    fala::dizer(antes + maiuscula(n) + (p.qtd > 1 ? ": " + std::to_string(p.qtd) : "") + ". " +
                std::to_string(itemMochila + 1) + " de " + std::to_string(mochila.size()) + ".");
}

void Jogo::colocarNaMao(bool direita) {
    if (mochila.empty()) { fala::dizer("A mochila está vazia."); return; }
    itemMochila = std::clamp(itemMochila, 0, (int)mochila.size() - 1);
    Coisa pega = mochila[itemMochila].coisa;
    Coisa& m = mao(direita);
    std::string lado = direita ? "direita" : "esquerda", antes;
    tirar(pega);
    if (!m.tipo.empty()) {
        Coisa& outra = mao(!direita);
        if (m.tipo == "garrafa-aberta" && outra.tipo == "tampa") { m.tipo = "garrafa"; outra = {}; antes = "Você tampa a garrafa com a tampa da outra mão. "; }
        antes += maiuscula(nomeCoisa(m, true)) + " volta para a mochila. "; guardar(m);
    }
    m = pega;
    maoSel = direita ? 0 : 1;
    audio::tocar("mochila", 0.5f);
    fala::dizer(antes + "Na mão " + lado + ": " + nomeCoisa(m) + ".");
    // o item que estava em foco pode ter acabado: fica no mesmo lugar da lista
    if (itemMochila >= (int)mochila.size()) itemMochila = std::max(0, (int)mochila.size() - 1);
}

void Jogo::verMao(int qual) {
    bool direita = qual == 0;
    Coisa& m = mao(direita);
    std::string lado = direita ? "direita" : "esquerda";
    bool duplo = agora - toqueNum[qual] < 0.6;
    toqueNum[qual] = agora;
    maoSel = qual;
    if (duplo) {
        toqueNum[qual] = -10;
        if (m.tipo.empty()) { fala::dizer("A mão " + lado + " já está vazia."); return; }
        std::string n = nomeCoisa(m, true);
        Coisa& outra = mao(!direita);
        std::string extra;
        if (m.tipo == "garrafa-aberta" && outra.tipo == "tampa") {  // tampa com a tampa da outra mão antes de guardar
            m.tipo = "garrafa"; outra = {}; n = nomeCoisa(m, true);
            extra = "Você tampa a garrafa com a tampa da outra mão. ";
            audio::tocar("tampa");
        }
        guardar(m); m = {};
        audio::tocar("mochila", 0.5f);
        fala::dizer(extra + "Mão " + lado + " vazia. " + maiuscula(n) + " voltou para a mochila.");
        return;
    }
    fala::dizer(m.tipo.empty() ? "Mão " + lado + " vazia." : "Mão " + lado + ": " + nomeCoisa(m) + ".");
}

// T: combina o que está nas duas mãos.
void Jogo::combinar() {
    Coisa& d = maoDireita; Coisa& e = maoEsquerda;
    if (d.tipo.empty() && e.tipo.empty()) {
        bool dentro = mapa.dentro(x, y);
        audio::tocar("palmas", 0.9f);
        fala::dizer(dentro ? "Você bate palmas. O som ecoa pela casa vazia." : "Você bate palmas. O som se perde no campo.");
        return;
    }
    if (d.tipo.empty() || e.tipo.empty()) {
        Coisa& c = d.tipo.empty() ? e : d;
        std::string lado = d.tipo.empty() ? "esquerda" : "direita";
        Coisa& vazia = d.tipo.empty() ? d : e;
        std::string outroLado = d.tipo.empty() ? "direita" : "esquerda";
        if (c.tipo == "barra") {
            c.tipo = "barra-aberta";
            audio::tocar("embalagem");
            fala::dizer("Você segura a barra com a mão " + lado + " e rasga a embalagem com a outra. A barra fica aberta na mão " + lado + ". A embalagem você joga fora.");
        } else if (c.tipo == "garrafa") {
            c.tipo = "garrafa-aberta";
            vazia = {"tampa", 0};
            audio::tocar("tampa");
            fala::dizer("Você gira a tampa e abre a garrafa. A garrafa fica na mão " + lado + ", e a tampa, na mão " + outroLado + ".");
        } else if (c.tipo == "garrafa-aberta") {
            if (tirar({"tampa", 0})) {
                c.tipo = "garrafa";
                audio::tocar("tampa");
                fala::dizer("Você pega uma tampa na mochila e tampa a garrafa. Ela fica na mão " + lado + ".");
            } else fala::dizer("Para tampar, segure a tampa na outra mão e aperte T.");
        } else if (c.tipo == "tampa") {
            fala::dizer("É só a tampa. Para tampar, segure a garrafa aberta na outra mão.");
        } else {
            fala::dizer("Com " + nomeCoisa(c, true) + " e a outra mão vazia, não tem o que fazer.");
        }
        return;
    }
    auto par = [&](const char* a, const char* b) { return (d.tipo == a && e.tipo == b) || (d.tipo == b && e.tipo == a); };
    if (par("garrafa-aberta", "tampa")) {
        Coisa& g = d.tipo == "garrafa-aberta" ? d : e;
        Coisa& tp = d.tipo == "tampa" ? d : e;
        g.tipo = "garrafa"; tp = {};
        audio::tocar("tampa");
        fala::dizer(std::string("Você tampa a garrafa. Ela fica tampada na mão ") + (&g == &d ? "direita" : "esquerda") + ", e a outra mão fica vazia.");
        return;
    }
    if (par("gesso", "garrafa-aberta")) {
        Coisa& g = d.tipo == "garrafa-aberta" ? d : e;
        if (g.carga == 0) { fala::dizer("A garrafa está vazia. Gesso precisa de água."); return; }
        fala::dizer("Você não tem nada quebrado. Gesso misturado com água endurece em poucos minutos e não serve mais: guarde para quando precisar.");
    } else if (par("gesso", "garrafa")) {
        fala::dizer("A garrafa está tampada. Abra primeiro: segure a garrafa com uma mão e aperte T com a outra vazia.");
    } else {
        fala::dizer(maiuscula(nomeCoisa(d, true)) + " e " + nomeCoisa(e, true) + " não combinam.");
    }
}

// Enter: usa o que está na mão direita (ou na esquerda, se a direita estiver vazia). Shift+Enter: a esquerda.
void Jogo::usar(bool direita) {
    Coisa& c = mao(direita);
    std::string lado = direita ? "direita" : "esquerda";
    const std::string& t = c.tipo;
    if (t.empty()) { fala::dizer("A mão " + lado + " está vazia."); return; }
    if (t == "barra-aberta") {
        if (fome > 92) { fala::dizer("Você está sem fome. A barra fica na mão."); return; }
        fome = std::min(100.0, fome + 25);
        c = {};
        audio::tocar("mastigar");
        fala::dizer("Você come a barra de ração. Seca, com gosto de aveia e papelão, mas enche a barriga.");
    } else if (t == "barra") {
        fala::dizer("A barra está embrulhada. Para abrir: com a outra mão vazia, aperte T.");
    } else if (t == "garrafa-aberta" && c.carga > 0) {
        if (sede > 97) { fala::dizer("Você está sem sede."); return; }
        sede = std::min(100.0, sede + 12);
        c.carga--;
        audio::tocar("beber");
        fala::dizer(c.carga == 0 ? "Você bebe o último gole. A garrafa ficou vazia. Na beira do córrego, dá para encher de novo."
                                 : "Você bebe um gole. Ainda tem " + goles(c.carga) + ".");
    } else if (t == "garrafa-aberta") {
        int ax, ay;
        if (mapa.aguaMaisPerto(x, y, 2, ax, ay)) {
            c.carga = 4;
            audio::tocar3D("passos-agua", ax + 0.5f, ay + 0.5f, 0, 1.0f, 3);
            fala::dizer("Você se agacha na beira do córrego e enche a garrafa. A água é fria e um pouco turva.");
        } else fala::dizer("A garrafa está vazia. Para encher, vá até a beira do córrego.");
    } else if (t == "garrafa") {
        fala::dizer("A garrafa está tampada. Para abrir: com a outra mão vazia, aperte T.");
    } else if (t == "antibiotico") {
        fala::dizer("Você não tem infecção nenhuma. Antibiótico à toa faz mal e deixa a bactéria mais forte. O comprimido fica na mão.");
    } else if (t == "atadura") {
        fala::dizer("Não tem nenhum machucado para enfaixar.");
    } else if (t == "gesso") {
        fala::dizer("Gesso em pó precisa ser misturado com água: segure uma garrafa aberta na outra mão e aperte T. E só serve para osso quebrado.");
    } else if (t == "saco-dormir") {
        dormir();
    } else if (t == "carteira") {
        audio::tocar("moedas", 0.7f);
        fala::dizer("Você abre a carteira e conta: " + std::to_string(c.carga) + (c.carga == 1 ? " moeda." : " moedas."));
    } else if (t == "bussola") {
        bussola();
    } else if (t == "tampa") {
        fala::dizer("É a tampa da garrafa. Com a garrafa aberta na outra mão, aperte T para tampar.");
    }
}

// ---------- corpo ----------
static std::string porCento(double v) { return std::to_string((int)std::lround(v)) + " de 100"; }

void Jogo::dizerFome() {
    const char* t = fome >= 85 ? "Barriga cheia." : fome >= 60 ? "Sem fome." : fome >= 40 ? "Começando a dar fome."
                  : fome >= 20 ? "Com fome: o estômago ronca." : fome >= 8 ? "Fome forte: as mãos tremem e a cabeça dói."
                  : "Fome de doer. Sem comer, o corpo começa a falhar.";
    fala::dizer(std::string(t) + " Barriga: " + porCento(fome) + ".");
}
void Jogo::dizerSede() {
    const char* t = sede >= 85 ? "Sem sede nenhuma." : sede >= 60 ? "Sem sede." : sede >= 40 ? "Começando a dar sede."
                  : sede >= 20 ? "Com sede: a boca está seca." : sede >= 8 ? "Sede forte: a garganta arranha e a cabeça lateja."
                  : "Sede de desmaiar. Sem água, o corpo começa a falhar.";
    fala::dizer(std::string(t) + " Água no corpo: " + porCento(sede) + ".");
}
void Jogo::dizerEnergia() {
    if (dormindo) { fala::dizer("Energia em " + std::to_string((int)std::lround(energia)) + " por cento. Você está dormindo."); return; }
    const char* t = energia >= 80 ? "Com disposição." : energia >= 50 ? "Um pouco de cansaço."
                  : energia >= 25 ? "Cansaço: as pernas pesam." : energia >= 10 ? "Exaustão: os olhos fecham sozinhos."
                  : "No limite. A qualquer momento o corpo desliga.";
    fala::dizer(std::string(t) + " Energia: " + porCento(energia) + ".");
}
void Jogo::dizerSaude() {
    const char* t = saude >= 90 ? "Saúde boa." : saude >= 60 ? "O corpo está um pouco abatido, mas de pé." : saude >= 30 ? "A saúde está fraca."
                  : "A saúde está por um fio.";
    fala::dizer(std::string(t) + " Saúde: " + porCento(saude) + ".");
}

// Avisa quando fome, sede ou energia caem de um patamar.
void Jogo::avisosDoCorpo() {
    struct { double v; int i; double limites[3]; const char* txt[3]; } itens[] = {
        {fome, 0, {40, 20, 8}, {"Começa a dar fome.", "O estômago ronca alto. Hora de comer.", "Fome de doer. Coma alguma coisa já."}},
        {sede, 1, {40, 20, 8}, {"Começa a dar sede.", "A boca está seca. Hora de beber água.", "Sede de desmaiar. Beba água já."}},
        {energia, 2, {30, 15, 6}, {"O cansaço começa a pesar.", "Exaustão. Procure um lugar para dormir.", "Você mal para em pé. Durma já, ou o corpo desliga sozinho."}},
    };
    for (auto& it : itens) {
        double& marca = ultimoAviso[it.i];
        for (int k = 0; k < 3; k++)
            if (it.v < it.limites[k] && marca >= it.limites[k]) { fala::dizer(it.txt[k], false); marca = it.limites[k] - 0.01; }
        if (it.v > marca + 10) marca = std::min(100.0, it.v);
    }
}

void Jogo::dormir() {
    if (dormindo) return;
    if (energia > 75) { fala::dizer("Você ainda não está com sono."); return; }
    dormindo = true;
    audio::tocar("saco-dormir");
    if (!somSono) somSono = audio::laco("ruido", false, 0.35f);
    fala::dizer("Você estende o saco de dormir no chão de " + std::string(Mapa::nome(mapa.em(x, y).chao)) + ", entra e fecha o zíper até o queixo. " +
                (mapa.dentro(x, y) ? "Dentro de casa, o silêncio ajuda." : "Ao ar livre, o vento passa por cima do saco.") +
                " Você dorme. E diz como a energia está se recuperando. Qualquer outra tecla, menos U, S e Q, acorda antes da hora.");
}

void Jogo::acordar(const std::string& motivo) {
    if (!dormindo) return;
    dormindo = false;
    if (somSono) { audio::pararFonte(somSono); somSono = 0; }
    char h[64]; std::snprintf(h, sizeof h, "%d e %02d", hora24(), (int)minutos % 60);
    fala::dizer(motivo + " São " + h + ", dia " + std::to_string(diaDoJogo()) + ". Energia: " + porCento(energia) + ".");
}

// ---------- mundo ----------
std::string Jogo::direcao(int r) const {
    static const char* n[8] = {"norte", "nordeste", "leste", "sudeste", "sul", "sudoeste", "oeste", "noroeste"};
    return n[(r % 8 + 8) % 8];
}

void Jogo::virar(int lado) {
    rumo = (rumo + lado + 8) % 8;
    fala::dizer(maiuscula(direcao(rumo)) + ".");
}

void Jogo::bussola() {
    bool tem = maoDireita.tipo == "bussola" || maoEsquerda.tipo == "bussola" ||
               std::any_of(mochila.begin(), mochila.end(), [](const Pilha& p) { return p.coisa.tipo == "bussola"; });
    if (!tem) { fala::dizer("Você está sem a bússola."); return; }
    audio::tocar("bussola", 0.8f);
    fala::dizer("A agulha treme, gira um pouco e para: " + direcao(rumo) + ".");
}

void Jogo::olhar() {
    std::string txt = maiuscula(mapa.nomeLugar(x, y)) + ", no chão de " + Mapa::nome(mapa.em(x, y).chao) + ", a " + std::to_string(x) +
                      " metros da divisa oeste e " + std::to_string(y) + " da divisa sul. ";
    int dx = DX[rumo], dy = DY[rumo];
    std::string aqui = mapa.nomeLugar(x, y), mudanca;
    for (int i = 1; i <= 25; i++) {
        int px = x + dx * i, py = y + dy * i;
        if (!mapa.valido(px, py)) { txt += "À frente, a " + std::to_string(i) + " metros, acaba o mundo conhecido."; return fala::dizer(txt); }
        const Celula& k = mapa.em(px, py);
        if (mudanca.empty() && k.obs == Obstaculo::Nada) {
            std::string l = mapa.nomeLugar(px, py);
            if (l != aqui) mudanca = "A " + std::to_string(i) + (i == 1 ? " metro" : " metros") + ", começa: " + l + ". ";
        }
        if (k.obs != Obstaculo::Nada) {
            txt += mudanca + "À frente, a " + std::to_string(i) + (i == 1 ? " metro: " : " metros: ") + Mapa::nome(k.obs) +
                   (k.obs == Obstaculo::Porta || k.obs == Obstaculo::Porteira ? ", dá para passar." : "." + passagemPerto(px, py, rumo));
            return fala::dizer(txt);
        }
    }
    fala::dizer(txt + mudanca + "À frente, caminho livre por mais de 25 metros.");
}

void Jogo::hora() {
    int h = hora24(), m = (int)minutos % 60;
    std::string periodo = h < 5 ? "da madrugada" : h < 12 ? "da manhã" : h < 18 ? "da tarde" : "da noite";
    int h12 = h % 12 == 0 ? 12 : h % 12;
    fala::dizer("Dia " + std::to_string(diaDoJogo()) + ", " + std::to_string(h12) + (m ? " e " + std::to_string(m) : " em ponto") + " " + periodo + ".");
}

void Jogo::passo(int sentido) { passoPara(sentido > 0 ? rumo : (rumo + 4) % 8); }

void Jogo::passoPara(int r) {
    int nx = x + DX[r], ny = y + DY[r];
    if (!mapa.passa(nx, ny)) {
        Obstaculo o = mapa.valido(nx, ny) ? mapa.em(nx, ny).obs : Obstaculo::Cerca;
        const char* som = o == Obstaculo::Corrego ? "passos-agua" : (o == Obstaculo::Parede || o == Obstaculo::Arvore) ? "parede" : "cerca";
        audio::tocar3D(som, nx + 0.5f, ny + 0.5f, 1.0f, 0.9f, 2, o == Obstaculo::Arvore ? 0.8f : 1.0f);
        std::string n = mapa.valido(nx, ny) ? Mapa::nome(o) : "fim do mapa";
        andandoFrente = andandoTras = false; andandoLado = 0;
        fala::dizer(maiuscula(n) + "." + passagemPerto(nx, ny, r));
        ultimoObstaculo = n;
        return;
    }
    ultimoObstaculo.clear();
    Obstaculo o = mapa.em(nx, ny).obs;
    if (o == Obstaculo::Porta) audio::tocar3D("porta", nx + 0.5f, ny + 0.5f, 1.0f, 0.8f, 3);
    if (o == Obstaculo::Porteira) audio::tocar3D("porteira", nx + 0.5f, ny + 0.5f, 1.0f, 0.9f, 4);
    x = nx; y = ny;
    energia -= correndo ? 0.05 : 0.015;
    sede -= correndo ? 0.03 : 0.008;
    audio::tocar(Mapa::somPasso(mapa.em(x, y).chao), (float)acaso(0.55, 0.75) * (correndo ? 1.2f : 1.0f), (float)acaso(0.94, 1.06));
    std::string l = mapa.nomeLugar(x, y);
    if (o == Obstaculo::Porta || o == Obstaculo::Porteira) fala::dizer(maiuscula(Mapa::nome(o)) + ".");
    if (l != lugarAtual) { lugarAtual = l; fala::dizer(maiuscula(l) + ".", o != Obstaculo::Porta && o != Obstaculo::Porteira); }
}

// Diante de parede ou cerca, procura ao longo dela a porta ou porteira mais perto.
std::string Jogo::passagemPerto(int ox, int oy, int r) const {
    Obstaculo o = mapa.valido(ox, oy) ? mapa.em(ox, oy).obs : Obstaculo::Nada;
    if (o != Obstaculo::Parede && o != Obstaculo::Cerca && o != Obstaculo::CercaMadeira && o != Obstaculo::Tela) return "";
    int esq = (r + 6) % 8, dir = (r + 2) % 8;  // perpendiculares ao rumo
    for (int d = 1; d <= 30; d++)
        for (int lado : {esq, dir}) {
            int px = ox + DX[lado] * d, py = oy + DY[lado] * d;
            if (!mapa.valido(px, py)) continue;
            Obstaculo q = mapa.em(px, py).obs;
            if (q == Obstaculo::Porta || q == Obstaculo::Porteira)
                return " A " + std::string(Mapa::nome(q)) + " mais perto fica " + std::to_string(d) + (d == 1 ? " metro" : " metros") +
                       " à sua " + (lado == esq ? "esquerda" : "direita") + ".";
        }
    return "";
}

void Jogo::ajuda() {
    fala::dizer("Teclas. Seta para cima: andar para a frente; segure para continuar; com Control, correr. Seta para baixo: andar de costas. "
                "Setas para a esquerda e para a direita: andar de lado. Control mais seta para o lado: virar 45 graus. "
                "Shift mais seta para o lado: virar 90 graus. Shift mais seta para cima ou para baixo: dar meia-volta. "
                "L: onde você está e o que tem à frente. Q: dia e hora. "
                "I: abre e fecha a mochila. Com a mochila aberta, setas para cima e para baixo escolhem o item, seta para a direita põe na mão direita, "
                "seta para a esquerda põe na mão esquerda. 1: seleciona a mão direita e diz o que tem nela; apertado duas vezes, esvazia. "
                "2: o mesmo para a mão esquerda. Espaço: usa o que está na mão selecionada: comer, beber, olhar a bússola, dormir. "
                "T: combina o que está nas duas mãos; com as duas vazias, bate palmas. "
                "U: fome. S: sede. E: energia. H: saúde. R: repete a última fala. Esc duas vezes: salva e sai. F1: esta ajuda.");
}

// ---------- teclas ----------
void Jogo::tecla(Tecla t, bool shift, bool ctrl) {
    (void)ctrl;
    if (carroT < 0 && carroPai) carroT = 0;  // primeira tecla: o pai vai embora
    if (t != Tecla::Esc) confirmarSair = false;
    if (dormindo) {
        if (t == Tecla::U) return dizerFome();
        if (t == Tecla::S) return dizerSede();
        if (t == Tecla::E) return dizerEnergia();
        if (t == Tecla::Q) return hora();
        return acordar("Você acorda e se espreguiça dentro do saco de dormir.");
    }
    if (mochilaAberta) {
        switch (t) {
            case Tecla::Cima: itemMochila = mochila.empty() ? 0 : (itemMochila - 1 + (int)mochila.size()) % (int)mochila.size(); falarItemMochila(); return;
            case Tecla::Baixo: itemMochila = mochila.empty() ? 0 : (itemMochila + 1) % (int)mochila.size(); falarItemMochila(); return;
            case Tecla::Direita: colocarNaMao(true); return;
            case Tecla::Esquerda: colocarNaMao(false); return;
            case Tecla::I: case Tecla::Esc:
                mochilaAberta = false; audio::tocar("mochila", 0.6f, 0.9f); fala::dizer("Mochila fechada."); return;
            default: break;
        }
    }
    switch (t) {
        case Tecla::Cima:
            if (shift) { virar(4); break; }
            andandoFrente = true; correndo = ctrl; passo(1); esperaPasso = correndo ? 0.3 : 0.45; break;
        case Tecla::Baixo:
            if (shift) { virar(4); break; }
            andandoTras = true; correndo = false; passo(-1); esperaPasso = 0.6; break;
        case Tecla::Esquerda: case Tecla::Direita: {
            int lado = t == Tecla::Esquerda ? -1 : 1;
            if (ctrl) { virar(lado); break; }
            if (shift) { virar(2 * lado); break; }
            andandoLado = lado; passoPara((rumo + 2 * lado + 8) % 8); esperaPasso = 0.55; break;
        }
        case Tecla::I:
            mochilaAberta = true; audio::tocar("mochila", 0.6f);
            falarItemMochila("Mochila aberta. ");
            break;
        case Tecla::Um: verMao(0); break;
        case Tecla::Dois: verMao(1); break;
        case Tecla::T: combinar(); break;
        case Tecla::Espaco: usar(maoSel == 0); break;
        case Tecla::U: dizerFome(); break;
        case Tecla::S: dizerSede(); break;
        case Tecla::E: dizerEnergia(); break;
        case Tecla::H: dizerSaude(); break;

        case Tecla::Q: hora(); break;
        case Tecla::L: olhar(); break;
        case Tecla::R: fala::dizer(fala::ultima()); break;
        case Tecla::F1: ajuda(); break;
        case Tecla::Enter: break;
        case Tecla::Esc:
            if (confirmarSair) { sair = true; fala::dizer("Jogo salvo. Até logo."); }
            else { confirmarSair = true; fala::dizer("Aperte Esc de novo para salvar e sair."); }
            break;
    }
}

void Jogo::soltar(Tecla t) {
    if ((t == Tecla::Esquerda && andandoLado < 0) || (t == Tecla::Direita && andandoLado > 0)) andandoLado = 0;
    if (t == Tecla::Cima) andandoFrente = false;
    if (t == Tecla::Baixo) andandoTras = false;
}

// ---------- tempo ----------
void Jogo::atualizar(double seg) {
    agora += seg;
    double dm = seg * (dormindo ? MIN_POR_SEG_DORMINDO : MIN_POR_SEG), h = dm / 60.0;
    minutos += dm;
    if (dormindo) { fome -= 1.1 * h; sede -= 2.0 * h; energia += 12.0 * h; }
    else { fome -= 2.2 * h; sede -= 4.0 * h; energia -= 3.5 * h; }
    if (fome <= 0) saude -= 5 * h;
    if (sede <= 0) saude -= 8 * h;
    if (fome > 30 && sede > 30 && energia > 20) saude += 1 * h;
    fome = std::clamp(fome, 0.0, 100.0); sede = std::clamp(sede, 0.0, 100.0);
    energia = std::clamp(energia, 0.0, 100.0); saude = std::clamp(saude, 0.0, 100.0);

    if (dormindo) {
        if (energia >= 100) acordar("Você acorda com o corpo leve e a cabeça descansada.");
        else if (hora24() == 6 && energia >= 70) acordar("O galo canta longe e a claridade entra. Você acorda.");
        else if ((fome < 10 || sede < 10) && energia >= 30) acordar(fome < sede ? "A fome te acorda." : "A sede te acorda.");
    } else if (energia <= 0) {
        dormindo = true;
        if (!somSono) somSono = audio::laco("ruido", false, 0.35f);
        fala::dizer("As pernas cedem. Você apaga ali mesmo, no chão, de puro cansaço.");
    }
    if (saude <= 0) {
        fala::dizer("O corpo não aguenta mais. Você desmaia e não acorda. Fim de jogo. Um jogo novo começa do início, na porteira.");
        novo();
        return;
    }
    if (!dormindo) avisosDoCorpo();

    // andar segurando a tecla
    if ((andandoFrente || andandoTras || andandoLado) && !mochilaAberta && !dormindo) {
        esperaPasso -= seg;
        if (esperaPasso <= 0) {
            if (andandoFrente) { passo(1); esperaPasso += correndo ? 0.3 : 0.45; }
            else if (andandoTras) { passo(-1); esperaPasso += 0.6; }
            else { passoPara((rumo + 2 * andandoLado + 8) % 8); esperaPasso += 0.55; }
        }
    }
    ambiente(seg);
}

// Sons do lugar e da hora: córrego, vento, pássaros de dia, grilos à noite, galo de madrugada.
void Jogo::ambiente(double dt) {
    audio::ouvinte(x + 0.5f, y + 0.5f, 1.6f, rumo * 45.0f);
    if (!audio::ativo()) return;
    bool dentro = mapa.dentro(x, y);
    float abafa = dentro ? 0.3f : 1.0f;
    int h = hora24();
    bool dia = h >= 6 && h < 18, noite = h >= 19 || h < 5;
    if (!somVento) somVento = audio::laco("vento", false, 0.2f);
    if (!somCorrego) somCorrego = audio::laco("corrego", true, 0.0f, 6);
    if (!somNoite) somNoite = audio::laco("grilos", false, 0.0f);
    if (!somDia) somDia = audio::laco("cigarra", false, 0.0f);
    audio::lacoVolume(somVento, 0.18f * abafa);
    audio::lacoVolume(somNoite, noite ? 0.3f * abafa : 0.0f);
    audio::lacoVolume(somDia, (h >= 11 && h < 17) ? 0.16f * abafa : 0.0f);
    static double revisaAgua = 0;
    revisaAgua -= dt;
    if (revisaAgua <= 0) {
        revisaAgua = 0.25;
        int ax, ay;
        if (mapa.aguaMaisPerto(x, y, 45, ax, ay)) { audio::lacoPosicao(somCorrego, ax + 0.5f, ay + 0.5f, 0.0f); audio::lacoVolume(somCorrego, 0.9f * abafa); }
        else audio::lacoVolume(somCorrego, 0.0f);
    }
    proximoPassaro -= dt;
    if (proximoPassaro <= 0) {
        proximoPassaro = acaso(5, 14);
        float px = x + (float)acaso(-25, 25), py = y + (float)acaso(-25, 25);
        if (dia) audio::tocar3D("passaros", px, py, 6, 0.6f * abafa, 6);
        else if (noite) {
            int ax, ay;
            if (mapa.aguaMaisPerto(x, y, 40, ax, ay)) audio::tocar3D("sapos", ax + (float)acaso(-6, 6), ay + 0.5f, 0, 0.6f * abafa, 5);
        }
        if (h == 5 || (h == 6 && (int)minutos % 60 < 30)) audio::tocar3D("galo", 480, -40, 2, 0.8f * abafa, 60);
    }
    // a caminhonete do pai vai embora pela estrada, para o oeste
    if (carroPai) {
        if (carroT >= 0) {
            carroT += dt;
            float cx = 253 - (float)(carroT * carroT * 1.2 + carroT * 2);
            audio::moverFonte(carroPai, cx, 2, 0.6f);
            audio::lacoTom(carroPai, 1.0f + (float)std::min(0.6, carroT * 0.06));  // o motor acelera
            if (carroT > 22) { audio::pararFonte(carroPai); carroPai = 0; }
        } else if (agora > 40) carroT = 0;
    }
}

void Jogo::abertura(bool novoJogo) {
    if (!novoJogo) {
        char h[16]; std::snprintf(h, sizeof h, "%d e %02d", hora24(), (int)minutos % 60);
        fala::dizer("Simulador de Vida. Que bom ter você de volta. Dia " + std::to_string(diaDoJogo()) + ", " + h + ". Você está em: " +
                    mapa.nomeLugar(x, y) + ", virado para o " + direcao(rumo) + ". F1 para ouvir as teclas.");
        return;
    }
    carroPai = audio::fonteMovel("carro", 253, 2, 0.6f, 0.8f, 6, true);
    carroT = -1;
    fala::dizer("Simulador de Vida. F1, a qualquer momento, para ouvir as teclas.");
    fala::dizer("Hoje você completa 18 anos. Seu pai para a caminhonete na estrada de terra, em frente à porteira da fazenda, e desce sem desligar o motor. "
                "Ele joga uma mochila velha de lona no gramado, aos seus pés, e fala sem rodeio:", false);
    fala::dizer("Pois é. Dezoito anos. Acabou a mamata: daqui pra frente eu não te sustento mais, não. Essa fazenda aqui tá parada faz anos, "
                "e agora é sua pra cuidar. Tem casa, tem córrego, tem terra boa. Quer comer, vai ter que plantar. Quer leite, vai ter que cuidar de bicho. "
                "Na mochila tem o que dá pra começar. O resto é com você. Boa sorte, viu?", false);
    fala::dizer("Ele te dá um tapinha no ombro, volta para a caminhonete e vai embora. O motor vai sumindo na estrada, e fica só o vento no capim. "
                "Você está no gramado da porteira principal, virado para o norte. A estrada da fazenda segue em frente até a casa sede, "
                "a uns 190 metros. Aperte I para abrir a mochila.", false);
}

// ---------- salvar ----------
void Jogo::salvar(const std::string& arquivo) const {
    std::ofstream f(arquivo);
    if (!f) return;
    f << "versao 1\n" << "pos " << x << ' ' << y << ' ' << rumo << '\n'
      << "corpo " << fome << ' ' << sede << ' ' << energia << ' ' << saude << '\n'
      << "relogio " << minutos << '\n';
    for (const Pilha& p : mochila) f << "item " << p.coisa.tipo << ' ' << p.coisa.carga << ' ' << p.qtd << '\n';
    f << "maoD " << (maoDireita.tipo.empty() ? "-" : maoDireita.tipo) << ' ' << maoDireita.carga << '\n';
    f << "maoE " << (maoEsquerda.tipo.empty() ? "-" : maoEsquerda.tipo) << ' ' << maoEsquerda.carga << '\n';
}

bool Jogo::carregar(const std::string& arquivo) {
    std::ifstream f(arquivo);
    if (!f) return false;
    std::vector<Pilha> m;
    std::string linha;
    while (std::getline(f, linha)) {
        std::istringstream s(linha);
        std::string k; s >> k;
        if (k == "pos") s >> x >> y >> rumo;
        else if (k == "corpo") s >> fome >> sede >> energia >> saude;
        else if (k == "relogio") s >> minutos;
        else if (k == "item") { Pilha p; s >> p.coisa.tipo >> p.coisa.carga >> p.qtd; if (p.qtd > 0) m.push_back(p); }
        else if (k == "maoD" || k == "maoE") {
            Coisa c; s >> c.tipo >> c.carga; if (c.tipo == "-") c = {};
            (k == "maoD" ? maoDireita : maoEsquerda) = c;
        }
    }
    mochila = m;
    if (!mapa.valido(x, y) || !mapa.passa(x, y)) { x = 245; y = 9; }
    rumo = (rumo % 8 + 8) % 8;
    lugarAtual = mapa.nomeLugar(x, y);
    return true;
}
