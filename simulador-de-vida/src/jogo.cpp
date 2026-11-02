#include "jogo.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <random>
#include <sstream>
#include "audio.h"
#include "fala.h"
#include "util.h"


// Relógio: um dia do jogo dura 1 hora de verdade (1 segundo de verdade = 24 segundos do jogo).
// Dormindo, 1 segundo de verdade = 10 minutos do jogo. Para mudar a duração do dia, só esta linha.
constexpr double HORAS_REAIS_POR_DIA = 1.0;
constexpr double MIN_POR_SEG = 1440.0 / (HORAS_REAIS_POR_DIA * 3600.0), MIN_POR_SEG_DORMINDO = 10.0;

Jogo::Jogo() { novo(); }

void Jogo::novo() {
    x = 500; y = 9; rumo = 0; postura = Postura::EmPe; intestino = 30; bexiga = 30; excretando = 0;
    fome = 85; sede = 85; energia = 90; saude = 100; higiene = 90;
    minutos = 7 * 60;
    mochila = {
        {{"barra", 0}, 100}, {{"garrafa", 4}, 10}, {{"antibiotico", 0}, 50}, {{"atadura", 0}, 10},
        {{"gesso", 0}, 10}, {{"saco-dormir", 0}, 1}, {{"carteira", 100}, 1}, {{"bussola", 0}, 1},
    };
    maoDireita = maoEsquerda = {};
    mapa = Mapa();
    plantas.clear(); limpos.clear(); cenaTiao = false; diaCafe = 0; agenda.clear();
    genero = '?'; cartas.clear(); proximaColeta = 22 * 60; aoAcordar.clear(); acordarEm = -1;
    criarMoveis();
    segurando = sentadoEm = -1; tomandoBanho = 0;
    menus.clear(); categoria = -1; alvos.clear(); rastreando = false; caminho.clear();
    mochilaAberta = false; itemMochila = 0; dormindo = false;
    for (double& a : ultimoAviso) a = 100;
    lugarAtual = mapa.nomeLugar(x, y);
}

// ---------- nomes ----------

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
    else if (t == "camisa") n = "camisa de flanela xadrez, do seu pai";
    else if (t == "calca") n = "calça jeans gasta";
    else if (t == "chapeu") n = "chapéu de palha";
    else if (t == "po-cafe") n = "pacote de pó de café";
    else if (t == "acucar") n = "saco de açúcar";
    else if (t == "farinha") n = "saco de farinha de trigo";
    else if (t == "gas") n = "botijão de gás";
    else if (t == "cigarro") n = c.carga == 0 ? "maço de cigarro vazio" : "maço de cigarro, com " + std::to_string(c.carga);
    else if (t == "semente-milho") n = "semente de milho";
    else if (t == "semente-feijao") n = "semente de feijão de corda";
    else if (t == "semente-arroz") n = "semente de arroz";
    else if (t == "machado") n = "machado";
    else if (t == "regador") n = c.carga == 0 ? "regador vazio" : "regador com água para " + std::to_string(c.carga) + (c.carga == 1 ? " rega" : " regas");
    else if (t == "garrafa-cafe") n = c.carga == 0 ? "garrafa térmica vazia" : "garrafa térmica de café, com " + goles(c.carga);
    else if (t == "milho") n = "espiga de milho";
    else if (t == "feijao") n = "punhado de feijão de corda";
    else if (t == "arroz") n = "feixe de arroz";
    else if (t == "tampa") n = "tampa de garrafa";
    else n = t;
    if (!comArtigo) return n;
    bool fem = t != "antibiotico" && t != "pacote" && t != "gesso" && t != "saco-dormir" && t != "chapeu" && t != "machado" && t != "regador" && t != "feixe" && t != "arroz" && t != "feijao" && t != "po-cafe" && t != "acucar" && t != "farinha" && t != "gas" && t != "cigarro";
    return std::string(fem ? "a " : "o ") + n;
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
    if (mundoAtual.rfind("visita:", 0) == 0 && (t == "machado" || t == "regador" || t.rfind("semente-", 0) == 0)) { fala::dizer("Na roça dos outros, só o dono planta e colhe."); return; }
    if (usarNaRoca(c)) return;
    if (t == "cigarro") {
        if (c.carga <= 0) { fala::dizer("O maço está vazio."); return; }
        c.carga--;
        audio::tocar("isqueiro", 0.8f);
        marcar(2.2, [this]() { audio::tocar("tosse", 0.8f); });
        saude = std::max(0.0, saude - 0.5);
        fala::dizer("Você acende um cigarro e dá uma tragada. A fumaça arranha a garganta, e vem a tosse." + std::string(c.carga ? " Restam " + std::to_string(c.carga) + " no maço." : " Era o último do maço."));
        return;
    }
    if (t == "po-cafe" || t == "acucar" || t == "farinha" || t == "gas") { fala::dizer(maiuscula(nomeCoisa(c, true)) + ". Serve para cozinhar, quando a cozinha estiver pronta para isso."); return; }
    if (t == "garrafa-cafe") {
        if (c.carga <= 0) { fala::dizer("A garrafa térmica está vazia. Dona Nezinha enche uma vez por dia."); return; }
        c.carga--;
        energia = std::min(100.0, energia + 12); sede = std::min(100.0, sede + 3); bexiga = std::min(100.0, bexiga + 6);
        audio::tocar("gole", 0.9f);
        fala::dizer("Você toma um gole de café da garrafa térmica, ainda quente. A energia volta." + std::string(c.carga ? " Restam " + goles(c.carga) + "." : " Acabou o café."));
        return;
    }
    if (t == "milho" || t == "feijao" || t == "arroz") { fala::dizer("Isso é colheita para vender. Seu Tião compra: fale com ele com Espaço e a mão vazia."); return; }
    if (t == "barra-aberta") {
        if (fome > 92) { fala::dizer("Você está sem fome. A barra fica na mão."); return; }
        fome = std::min(100.0, fome + 25);
        intestino = std::min(100.0, intestino + 12);
        c = {};
        audio::tocar("mastigar");
        evento("comeu");
        fala::dizer("Você come a barra de ração. Seca, com gosto de aveia e papelão, mas enche a barriga.");
    } else if (t == "barra") {
        fala::dizer("A barra está embrulhada. Para abrir: com a outra mão vazia, aperte T.");
    } else if (t == "garrafa-aberta" && movelFrente() >= 0 && moveis[movelFrente()].tipo == "pia" && c.carga < 4) {
        c.carga = 4;
        audio::tocar3D("torneira", moveis[movelFrente()].x + 0.5f, moveis[movelFrente()].y + 0.5f, 0.9f, 0.9f, 2);
        fala::dizer("Você abre a torneira e enche a garrafa. Água limpa, da caixa d'água.");
    } else if (t == "garrafa-aberta" && c.carga > 0) {
        if (sede > 97) { fala::dizer("Você está sem sede."); return; }
        sede = std::min(100.0, sede + 12);
        bexiga = std::min(100.0, bexiga + 10);
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
        if (postura != Postura::Deitado) { fala::dizer("Para entrar no saco de dormir, deite primeiro: Control R. Depois, Espaço."); return; }
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
        {100 - intestino, 3, {30, 15, 6}, {"Começa a dar vontade de fazer cocô.", "A barriga aperta: precisa dar um cagão.", "Não vai dar para segurar muito mais. Sente e aperte Control Q."}},
        {100 - bexiga, 4, {30, 15, 6}, {"Começa a dar vontade de fazer xixi.", "A bexiga está cheia: precisa fazer xixi.", "Vai escapar. Agache e aperte Shift Q."}},
        {higiene, 5, {40, 20, 8}, {"O suor começa a pesar e a dar cheiro.", "A sujeira já incomoda: precisa de um banho.", "O cheiro já está forte. Banho, já."}},
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
    bool naCama = sentadoEm >= 0 && moveis[sentadoEm].tipo == "cama";
    if (energia > (naCama ? 85 : 75)) { fala::dizer("Você ainda não está com sono."); return; }
    dormindo = true;
    postura = Postura::Deitado;
    if (!somSono) somSono = audio::laco("ruido", false, 0.35f);
    if (sentadoEm >= 0 && moveis[sentadoEm].tipo == "cama") {
        double hojeSeis = std::floor(minutos / 1440) * 1440 + 6 * 60;
        acordarEm = minutos < hojeSeis - 60 ? hojeSeis : hojeSeis + 1440;  // as 6 da manhã que vêm (de madrugada, as de hoje)
        if (rede::online()) acordarEm = -1;  // online: o dia não pula; acorda quando a energia encher
        fala::dizer("Você puxa o lençol, afunda a cabeça no travesseiro e dorme. E diz como a energia está se recuperando. Qualquer outra tecla, menos U, S, V e Q, acorda.");
        evento("dormiu");
        return;
    }
    audio::tocar("saco-dormir");
    evento("dormiu");
    fala::dizer("Você entra no saco de dormir, no chão de " + std::string(Mapa::nome(mapa.em(x, y).chao)) + ", e fecha o zíper até o queixo. " +
                (mapa.dentro(x, y) ? "Dentro de casa, o silêncio ajuda." : "Ao ar livre, o vento passa por cima do saco.") +
                " Você dorme. E diz como a energia está se recuperando. Qualquer outra tecla, menos U, S e Q, acorda antes da hora.");
}

void Jogo::acordar(const std::string& motivo) {
    if (!dormindo) return;
    dormindo = false;
    acordarEm = -1;
    if (somSono) { audio::pararFonte(somSono); somSono = 0; }
    char h[64]; std::snprintf(h, sizeof h, "%d e %02d", hora24(), (int)minutos % 60);
    fala::dizer(motivo + " São " + h + ", dia " + std::to_string(diaDoJogo()) + ". Energia: " + porCento(energia) + ".");
    for (const std::string& t : aoAcordar) fala::dizer(t, false);  // o que aconteceu de noite
    aoAcordar.clear();
}

// ---------- mundo ----------
std::string Jogo::direcao(int r) const {
    static const char* n[8] = {"norte", "nordeste", "leste", "sudeste", "sul", "sudoeste", "oeste", "noroeste"};
    return n[(r % 8 + 8) % 8];
}

void Jogo::virar(int lado) {
    if (segurando >= 0) { fala::dizer("Você está segurando " + nomeMovel(segurando, true) + ". Para virar, solte primeiro: Control Espaço."); return; }
    evento("virar");
    rumo = (rumo + lado + 8) % 8;
    fala::dizer(maiuscula(direcao(rumo)) + ".");
}

// Direção de uma coisa em relação a quem olha: dirAbs de 0 (norte) a 7 (noroeste).
std::string Jogo::relativo(int dirAbs) const {
    static const char* n[8] = {"à frente", "à frente, pela direita", "à direita", "atrás, pela direita", "atrás", "atrás, pela esquerda", "à esquerda", "à frente, pela esquerda"};
    return n[(dirAbs - rumo + 16) % 8];
}

// Ao pisar num quadrado colado numa porta ou porteira, avisa onde ela está.
void Jogo::anunciarPortas() {
    for (int d = 0; d < 8; d += 2) {
        int px = x + DX[d], py = y + DY[d];
        if (!mapa.ehPassagem(px, py)) continue;
        const Celula& k = mapa.em(px, py);
        std::string n = Mapa::nome(k.obs);
        if (k.aberta && relativo(d) == "atrás") continue;  // a que acabou de atravessar
        fala::dizer(maiuscula(n) + " " + relativo(d) + (k.aberta ? ", aberta." : ", fechada. Aperte Shift Enter para abrir."), false);
    }
}

// Shift Enter: abre ou fecha a porta à frente, ou, se não houver, a que estiver colada em você.
void Jogo::abrirPorta() {
    int alvo = -1;
    if (mapa.ehPassagem(x + DX[rumo], y + DY[rumo])) alvo = rumo;
    for (int d = 0; d < 8 && alvo < 0; d += 2) if (mapa.ehPassagem(x + DX[d], y + DY[d])) alvo = d;
    if (alvo < 0) { fala::dizer("Não tem porta nem porteira encostada em você."); return; }
    if (postura != Postura::EmPe) { fala::dizer("Para abrir, levante primeiro: aperte R."); return; }
    Celula& k = mapa.em(x + DX[alvo], y + DY[alvo]);
    k.aberta = !k.aberta;
    bool porteira = k.obs == Obstaculo::Porteira, box = k.obs == Obstaculo::PortaBox;
    audio::tocar3D(porteira ? "porteira" : box ? "box" : (k.aberta ? "porta-abrir" : "porta-fechar"), x + DX[alvo] + 0.5f, y + DY[alvo] + 0.5f, 1.0f, 0.9f, 3);
    std::string n = porteira ? "a porteira" : box ? "a porta de vidro do box" : "a porta";
    fala::dizer(std::string(k.aberta ? "Você abre " : "Você fecha ") + n + ", " + relativo(alvo) + ".");
}

void Jogo::mudarPostura(Postura p) {
    if (postura == p) { fala::dizer("Você já está nessa posição."); return; }
    std::string chao = Mapa::nome(mapa.em(x, y).chao);
    bool vaso = mapa.nomeLugar(x, y) == "banheiro";
    andandoFrente = andandoTras = false; andandoLado = 0;
    postura = p;
    if (p == Postura::EmPe && sentadoEm >= 0) { sentadoEm = -1; fala::dizer("Você se levanta."); evento("levantar"); return; }
    sentadoEm = -1;
    switch (p) {
        case Postura::EmPe: fala::dizer("Você se levanta."); break;
        case Postura::Sentado: fala::dizer(vaso ? "Você senta no vaso sanitário." : "Você senta no chão de " + chao + "."); break;
        case Postura::Agachado: fala::dizer("Você se agacha."); break;
        case Postura::Deitado: fala::dizer("Você se deita no chão de " + chao + "."); break;
    }
}

void Jogo::verMaos() {
    auto n = [&](const Coisa& c) { return c.tipo.empty() ? std::string("vazia") : nomeCoisa(c); };
    fala::dizer("Mão esquerda: " + n(maoEsquerda) + ". Mão direita: " + n(maoDireita) + ".");
}

void Jogo::dizerIntestino() {
    const char* t = intestino < 30 ? "Intestino tranquilo." : intestino < 60 ? "Nada urgente no intestino ainda."
                  : intestino < 85 ? "Já dá vontade de fazer cocô." : "Intestino cheio: precisando dar um cagão, e logo.";
    fala::dizer(std::string(t) + " Intestino: " + std::to_string((int)std::lround(intestino)) + " de 100.");
}
void Jogo::dizerBexiga() {
    const char* t = bexiga < 30 ? "Bexiga tranquila." : bexiga < 60 ? "Nada urgente na bexiga ainda."
                  : bexiga < 85 ? "Já dá vontade de fazer xixi." : "Bexiga cheia: precisando fazer xixi, e logo.";
    fala::dizer(std::string(t) + " Bexiga: " + std::to_string((int)std::lround(bexiga)) + " de 100.");
}

// Segurar Control Q (cocô, sentado) ou Shift Q (xixi, agachado) até acabar.
void Jogo::comecarExcretar(int tipo) {
    if (excretando) return;
    if (tipo == 2 && postura != Postura::Sentado) { fala::dizer("Cocô se faz sentando. Aperte R para sentar, e depois segure Control Q."); return; }
    if (tipo == 1 && postura != Postura::Agachado) { fala::dizer("Xixi se faz de cócoras. Aperte Shift R para agachar, e depois segure Shift Q."); return; }
    if (tipo == 2 && intestino < 10) { fala::dizer("Você faz força, mas não tem cocô nenhum para sair."); return; }
    if (tipo == 1 && bexiga < 8) { fala::dizer("Não tem xixi para sair."); return; }
    excretando = tipo; excretaT = 0; proximoPlop = 1.0;
    if (tipo == 2) audio::tocar("peido", 0.9f);
    else somXixi = audio::laco("xixi", false, 0.7f);
}

void Jogo::pararExcretar(bool cedo) {
    if (!excretando) return;
    int t = excretando;
    excretando = 0;
    if (somXixi) { audio::pararFonte(somXixi); somXixi = 0; }
    if (cedo) fala::dizer(t == 2 ? "Ainda tem cocô pra sair!" : "Ainda tem xixi pra sair!");
    else fala::dizer(t == 2 ? "Pronto. Intestino vazio. Que alívio." : "Pronto. Bexiga vazia. Que alívio.");
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
        if (k.planta) {
            txt += mudanca + "À frente, a " + std::to_string(i) + (i == 1 ? " metro: " : " metros: ") + nomePlanta(k.planta) + ", " + estadoPlanta(plantas[py * LADO + px]) + ".";
            return fala::dizer(txt);
        }
        if (k.movel >= 0) {
            txt += mudanca + "À frente, a " + std::to_string(i) + (i == 1 ? " metro: " : " metros: ") + nomeMovel(k.movel) + ".";
            return fala::dizer(txt);
        }
        if (k.obs != Obstaculo::Nada) {
            txt += mudanca + "À frente, a " + std::to_string(i) + (i == 1 ? " metro: " : " metros: ") + Mapa::nome(k.obs) +
                   (k.obs == Obstaculo::Porta || k.obs == Obstaculo::Porteira ? (k.aberta ? ", aberta." : ", fechada.") : "." + passagemPerto(px, py, rumo));
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
    if (segurando >= 0) {  // arrastando um móvel: empurra para a frente, puxa para trás, leva de lado
        Movel& m = moveis[segurando];
        int mx = m.x + DX[r], my = m.y + DY[r];
        bool frente = r == rumo, tras = r == (rumo + 4) % 8;
        if (frente) {
            if (!moverMovel(segurando, mx, my)) { andandoFrente = false; fala::dizer(maiuscula(nomeMovel(segurando, true)) + " não passa: tem coisa no caminho."); return; }
        } else if (tras) {
            if (!mapa.passa(nx, ny)) { andandoTras = false; fala::dizer("Atrás de você tem obstáculo. Não dá para puxar."); return; }
            int ox = x, oy = y;
            x = nx; y = ny;
            moverMovel(segurando, ox, oy);
            audio::tocar(Mapa::somPasso(mapa.em(x, y).chao), 0.5f);
            return;
        } else {
            if (!mapa.passa(nx, ny) || !moverMovel(segurando, mx, my)) { andandoLado = 0; fala::dizer("Não dá para levar " + nomeMovel(segurando, true) + " para esse lado."); return; }
        }
    }
    // diagonal só com os dois lados do canto livres: não dá para raspar quina de parede, vidro ou porta fechada
    if (r % 2 == 1 && segurando < 0 && mapa.passa(nx, ny) && (!mapa.passa(x + DX[r], y) || !mapa.passa(x, y + DY[r]))) {
        andandoFrente = andandoTras = false; andandoLado = 0;
        audio::tocar3D("parede", nx + 0.5f, ny + 0.5f, 1.0f, 0.5f, 2);
        fala::dizer("Quina. Na diagonal não passa; vá reto.");
        return;
    }
    if (mapa.valido(nx, ny) && mapa.em(nx, ny).planta) {
        andandoFrente = andandoTras = false; andandoLado = 0;
        audio::tocar3D("passos-mato", nx + 0.5f, ny + 0.5f, 0.3f, 0.5f, 2);
        fala::dizer(maiuscula(nomePlanta(mapa.em(nx, ny).planta)) + ". Plantas não se pisam: dê a volta.");
        return;
    }
    if (mapa.valido(nx, ny) && mapa.em(nx, ny).movel >= 0) {
        int i = mapa.em(nx, ny).movel;
        andandoFrente = andandoTras = false; andandoLado = 0;
        audio::tocar3D("parede", nx + 0.5f, ny + 0.5f, 0.8f, 0.5f, 2, 1.2f);
        fala::dizer(maiuscula(nomeMovel(i)) + ".");
        return;
    }
    if (!mapa.passa(nx, ny)) {
        Obstaculo o = mapa.valido(nx, ny) ? mapa.em(nx, ny).obs : Obstaculo::Cerca;
        const char* som = o == Obstaculo::Corrego ? "passos-agua" : (o == Obstaculo::Parede || o == Obstaculo::Arvore) ? "parede" : "cerca";
        audio::tocar3D(som, nx + 0.5f, ny + 0.5f, 1.0f, 0.9f, 2, o == Obstaculo::Arvore ? 0.8f : 1.0f);
        std::string n = mapa.valido(nx, ny) ? Mapa::nome(o) : "fim do mapa";
        andandoFrente = andandoTras = false; andandoLado = 0;
        if (o == Obstaculo::Porta || o == Obstaculo::Porteira || o == Obstaculo::PortaBox) { audio::tocar3D("parede", nx + 0.5f, ny + 0.5f, 1.0f, 0.5f, 2, 1.3f); fala::dizer(maiuscula(n) + " fechada. Aperte Shift Enter para abrir."); return; }
        fala::dizer(maiuscula(n) + "." + passagemPerto(nx, ny, r));
        ultimoObstaculo = n;
        return;
    }
    ultimoObstaculo.clear();
    Obstaculo o = mapa.em(nx, ny).obs;
    x = nx; y = ny;
    contadorPassos++;
    higiene -= 0.004;
    energia -= correndo ? 0.05 : 0.015;
    sede -= correndo ? 0.03 : 0.008;
    audio::tocar(Mapa::somPasso(mapa.em(x, y).chao), (float)acaso(0.55, 0.75) * (correndo ? 1.2f : 1.0f), (float)acaso(0.94, 1.06));
    std::string l = mapa.nomeLugar(x, y);
    if (o == Obstaculo::Nada) {  // no vão da porta não anuncia lugar: a parede fica no meio de dois
        if (l != lugarAtual) { lugarAtual = l; fala::dizer(maiuscula(l) + "."); }
        anunciarPortas();
    }
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
    fala::dizer("Teclas. Seta para cima: andar para a frente; segure para continuar; com Control, correr. Seta para baixo: andar de costas. Setas para a esquerda e para a direita: andar de lado. Control mais seta para o lado: virar 45 graus. Shift mais seta para o lado: virar 90 graus. Shift mais seta para cima ou para baixo: dar meia-volta. R: sentar, no chão ou no sofá, na poltrona, no vaso ou na cama que estiver na sua frente; em outra posição, levantar. Control mais R: deitar; de frente para a cama, deita nela e dorme. Shift mais R: agachar. Shift mais Enter: abrir ou fechar a porta, a porteira ou a porta do box encostada em você. Espaço: usa o que está na mão selecionada: comer, beber, tomar café, olhar a bússola, entrar no saco de dormir (deitado), contar o dinheiro. Na roça: machado limpa o matagal da frente, semente planta na terra limpa, regador rega a planta da frente e enche na beira do córrego. Com a mão vazia: de frente para um móvel, mexe nele; de frente para seu Tião, dona Nezinha, seu Zé (no balcão do bar) ou o prefeito, conversa e compra; de frente para o trator, escolhe para onde ir; de frente para uma planta pronta, colhe (o arroz se colhe com o machado). Control mais Espaço: segura o móvel da frente para arrastar; ande para empurrar ou puxar; de novo, solta. Alt mais Espaço: liga e desliga o chuveiro, de dentro do box. B: onde você está. Shift mais B: onde você está, a que distância das divisas e o que tem à frente. Ç: próxima categoria: móveis, pessoas (também os outros jogadores, no online), portas e porteiras, lugares, plantação. Shift mais Ç: categoria anterior. L: próximo item da categoria, do mais perto para o mais longe. K: item anterior. O: rastreia o item escolhido: um bip soa de onde ele está; O de novo desliga. Alt mais O: anda sozinho até o item, abrindo as portas; qualquer tecla para. Q: dia e hora. I: abre e fecha a mochila; com ela aberta, setas para cima e para baixo escolhem o item, uma letra pula para o próximo item com aquela inicial, seta para a direita põe na mão direita, seta para a esquerda põe na mão esquerda, Esc fecha. 1: seleciona a mão esquerda e diz o que tem nela; apertado duas vezes, esvazia. 2: o mesmo para a mão direita. 3: diz o que tem nas duas mãos. T: combina o que está nas duas mãos; com as duas vazias, bate palmas. U: fome. S: sede. E: energia. H: saúde. W: limpeza: se precisa de banho. V: intestino. Shift mais V: bexiga. Control mais Q, segurando: fazer cocô, sentando, até acabar. Shift mais Q, segurando: fazer xixi, de cócoras, até acabar. Dois-pontos: no jogo online, abre o campo de mensagem do chat; Enter envia, Esc cancela, Backspace apaga; o jogo fala cada letra que você digita. Tab: menu: tutorial, hospedar online, entrar em jogo online, jogadores, ativar e desativar chat, histórico do chat, ativar e desativar voz, configurações de volume, sair; Tab de novo volta ao jogo; sozinho, o menu pausa o jogo, no online não. Nos menus: setas para cima e para baixo escolhem, Enter faz, Shift Enter pega, guarda ou vende tudo, setas para os lados mudam volume, uma letra pula, Esc volta. F2: repete a última fala. F1: esta ajuda. Esc duas vezes: salva e sai.");
}

// ---------- teclas ----------
void Jogo::tecla(Tecla t, bool shift, bool ctrl, bool alt, char letra) {
    if (campo.ativo) { teclaCampo(t); return; }  // escrevendo: as letras chegam por texto(); aqui só Enter, Esc, Backspace
    if (carroT < 0 && carroPai) carroT = 0;  // primeira tecla: o pai vai embora
    if (viajando && t != Tecla::U && t != Tecla::S && t != Tecla::E && t != Tecla::Q) { fala::dizer("Em viagem, no trator. Já, já chega."); return; }
    if (t != Tecla::Esc) confirmarSair = false;
    // andando sozinho até um alvo: qualquer tecla para
    if (!caminho.empty() && !(t == Tecla::O && alt)) { caminho.clear(); fala::dizer("Parou. O rastreio continua: siga o bip, ou Alt O para continuar."); return; }
    // menus abertos (geladeira, guarda-roupa, Tab) recebem as teclas primeiro
    if (!menus.empty()) { teclaMenu(t, shift, letra); return; }
    if (t == Tecla::Tab) { menuPrincipal(); return; }
    if (dormindo) {
        if (t == Tecla::U) return dizerFome();
        if (t == Tecla::S) return dizerSede();
        if (t == Tecla::E) return dizerEnergia();
        if (t == Tecla::Q) return hora();
        if (t == Tecla::V) return shift ? dizerBexiga() : dizerIntestino();
        if (t == Tecla::W) return dizerHigiene();
        return acordar(sentadoEm >= 0 ? "Você acorda e se espreguiça na cama. Para levantar, aperte R." : "Você acorda e se espreguiça dentro do saco de dormir. Para levantar, aperte R.");
    }
    if (mochilaAberta) {
        switch (t) {
            case Tecla::Cima: itemMochila = mochila.empty() ? 0 : (itemMochila - 1 + (int)mochila.size()) % (int)mochila.size(); audio::tocar("gota", 0.4f); falarItemMochila(); return;
            case Tecla::Baixo: itemMochila = mochila.empty() ? 0 : (itemMochila + 1) % (int)mochila.size(); audio::tocar("gota", 0.4f); falarItemMochila(); return;
            case Tecla::Direita: colocarNaMao(true); return;
            case Tecla::Esquerda: colocarNaMao(false); return;
            case Tecla::I: case Tecla::Esc:
                mochilaAberta = false; audio::tocar("mochila", 0.6f, 0.9f); fala::dizer("Mochila fechada."); return;
            default: break;
        }
        if (letra && !mochila.empty()) {  // letra: pula para o próximo item que começa com ela
            char l = (char)std::tolower((unsigned char)letra);
            int n = (int)mochila.size();
            for (int k = 1; k <= n; k++) {
                int j = (itemMochila + k) % n;
                if (inicialSemAcento(nomeCoisa(mochila[j].coisa)) == l) { itemMochila = j; audio::tocar("gota", 0.4f); falarItemMochila(); evento("letra"); return; }
            }
            fala::dizer(std::string("Nada na mochila com a letra ") + l + ".");
            return;
        }
    }
    bool andar = (t == Tecla::Cima || t == Tecla::Baixo || t == Tecla::Esquerda || t == Tecla::Direita) && !shift && !ctrl;
    if (andar && postura != Postura::EmPe) { fala::dizer("Para andar, levante primeiro: aperte R."); return; }
    switch (t) {
        case Tecla::Cima:
            if (shift) { virar(4); break; }
            andandoFrente = true; correndo = ctrl && segurando < 0; passo(1); esperaPasso = correndo ? 0.3 : 0.45; break;
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
        case Tecla::Um: verMao(1); break;
        case Tecla::Dois: verMao(0); break;
        case Tecla::Tres: verMaos(); evento("maos"); break;
        case Tecla::T: combinar(); break;
        case Tecla::U: dizerFome(); evento("fome"); break;
        case Tecla::S: dizerSede(); break;
        case Tecla::E: dizerEnergia(); break;
        case Tecla::H: dizerSaude(); break;
        case Tecla::W: dizerHigiene(); evento("higiene"); break;
        case Tecla::Q: if (ctrl) comecarExcretar(2); else if (shift) comecarExcretar(1); else hora(); break;
        case Tecla::B:
            if (shift) { olhar(); evento("olhar"); }
            else { fala::dizer(maiuscula(mapa.nomeLugar(x, y)) + "."); evento("b"); }
            break;
        case Tecla::V: if (shift) dizerBexiga(); else dizerIntestino(); break;
        case Tecla::Cedilha: trocarCategoria(shift ? -1 : 1); break;
        case Tecla::K: trocarItem(-1); break;
        case Tecla::L: trocarItem(1); break;
        case Tecla::O: if (alt) irAteAlvo(); else rastrear(); break;
        case Tecla::Espaco: {
            if (ctrl) { arrastar(); break; }
            if (alt) { chuveiro(); break; }
            Coisa& m = mao(maoSel == 0);
            int f = movelFrente();
            for (const auto& [id, r] : rede::remotos())  // outro jogador na frente
                if (m.tipo.empty() && r.mundo == mundoCanonico() && r.x == x + DX[rumo] && r.y == y + DY[rumo]) {
                    fala::dizer(r.nome + " está na sua frente. Com gente de verdade, a conversa é pelo chat, com dois-pontos, ou pela voz, que se ativa no Tab.");
                    return;
                }
            if (m.tipo.empty() && f >= 0) interagir(f);
            else if (m.tipo.empty() && plantaFrente() >= 0) {
                if (mundoAtual.rfind("visita:", 0) == 0) fala::dizer(maiuscula(nomePlanta(plantas[plantaFrente()].tipo)) + " de outra pessoa: " + estadoPlanta(plantas[plantaFrente()]) + ".");
                else colher(plantaFrente(), false);
            }
            else usar(maoSel == 0);
            break;
        }
        case Tecla::R: {
            int f = movelFrente();
            if (postura == Postura::EmPe && f >= 0 && !shift) {
                const std::string& tp = moveis[f].tipo;
                if (ctrl && tp == "cama") { sentarEm(f, Postura::Deitado); break; }
                if (!ctrl && (tp == "sofa" || tp == "poltrona" || tp == "vaso" || tp == "cama")) { sentarEm(f, Postura::Sentado); break; }
            }
            if (ctrl) mudarPostura(Postura::Deitado);
            else if (shift) mudarPostura(Postura::Agachado);
            else mudarPostura(postura == Postura::EmPe ? Postura::Sentado : Postura::EmPe);
            break;
        }
        case Tecla::F2: fala::dizer(fala::ultima()); break;
        case Tecla::F1: ajuda(); break;
        case Tecla::Enter: if (shift) abrirPorta(); break;
        case Tecla::Esc:
            if (confirmarSair) { sair = true; fala::dizer("Jogo salvo. Até logo."); }
            else { confirmarSair = true; fala::dizer("Aperte Esc de novo para salvar e sair."); }
            break;
        default: break;
    }
}

void Jogo::soltar(Tecla t) {
    if (t == Tecla::Q && excretando) pararExcretar(true);
    if ((t == Tecla::Esquerda && andandoLado < 0) || (t == Tecla::Direita && andandoLado > 0)) andandoLado = 0;
    if (t == Tecla::Cima) andandoFrente = false;
    if (t == Tecla::Baixo) andandoTras = false;
}

// ---------- tempo ----------
void Jogo::atualizar(double seg) {
    agora += seg;
    servicoRede(seg);
    if (pausado()) { ambiente(0); return; }  // menu do Tab aberto: o tempo para
    bool online = rede::online();  // no online, o relógio é de todos: dormir não pula o tempo
    double dm = seg * (dormindo && !online ? MIN_POR_SEG_DORMINDO : MIN_POR_SEG), h = dm / 60.0;
    minutos += dm;
    if (dormindo) { fome -= 1.1 * h; sede -= 2.0 * h; energia += (online ? 120.0 : sentadoEm >= 0 ? 15.0 : 12.0) * h; higiene -= 0.5 * h; }
    else { fome -= 2.2 * h; sede -= 4.0 * h; energia -= 3.5 * h; higiene -= 1.5 * h; }
    intestino += 2.5 * h; bexiga += (dormindo ? 2.0 : 4.0) * h;
    if (excretando == 1) {
        bexiga -= 12 * seg;
        if (bexiga <= 0) { bexiga = 0; pararExcretar(false); }
    } else if (excretando == 2) {
        excretaT += seg;
        if (excretaT > 0.9) intestino -= 9 * seg;
        proximoPlop -= seg;
        if (proximoPlop <= 0 && excretaT > 0.9) { proximoPlop = acaso(0.9, 1.6); audio::tocar3D("coco", x + 0.5f, y + 0.2f, 0.1f, 0.9f, 2, (float)acaso(0.9, 1.1)); }
        if (intestino <= 0) { intestino = 0; pararExcretar(false); }
    }
    if (bexiga >= 100 && excretando != 1) {
        bexiga = 0; higiene -= 30; audio::tocar("xixi", 0.6f);
        fala::dizer("Não deu para segurar: o xixi escorre pela perna e molha a calça inteira. Da próxima vez, agache e aperte Shift Q a tempo.");
    }
    if (intestino >= 100 && excretando != 2) {
        intestino = 0; higiene -= 45; audio::tocar("peido"); audio::tocar("coco", 0.7f);
        fala::dizer("Não deu para segurar: o cocô sai ali mesmo, na roupa. Que fedor. Da próxima vez, sente e aperte Control Q a tempo.");
    }
    if (fome <= 0) saude -= 5 * h;
    if (sede <= 0) saude -= 8 * h;
    if (fome > 30 && sede > 30 && energia > 20) saude += 1 * h;
    fome = std::clamp(fome, 0.0, 100.0); sede = std::clamp(sede, 0.0, 100.0);
    intestino = std::clamp(intestino, 0.0, 100.0); bexiga = std::clamp(bexiga, 0.0, 100.0); higiene = std::clamp(higiene, 0.0, 100.0);
    energia = std::clamp(energia, 0.0, 100.0); saude = std::clamp(saude, 0.0, 100.0);

    if (dormindo && acordarEm >= 0) {  // na cama: dorme até amanhecer
        if (minutos >= acordarEm) { energia = 100; acordar("Amanhece. O galo canta longe, a claridade entra pela janela, e você acorda " + gen("descansado", "descansada") + " na cama."); }
        else if (bexiga >= 95 || intestino >= 95) acordar(bexiga >= 95 ? "A vontade de fazer xixi te acorda no meio da noite." : "A barriga aperta e te acorda no meio da noite: precisa fazer cocô.");
    } else if (dormindo) {
        if (energia >= 100) acordar("Você acorda com o corpo leve e a cabeça descansada.");
        else if (hora24() == 6 && energia >= 70) acordar("O galo canta longe e a claridade entra. Você acorda.");
        else if (bexiga >= 95 || intestino >= 95) acordar(bexiga >= 95 ? "A vontade de fazer xixi te acorda." : "A barriga aperta e te acorda: precisa fazer cocô.");
        else if ((fome < 10 || sede < 10) && energia >= 30) acordar(fome < sede ? "A fome te acorda." : "A sede te acorda.");
    } else if (energia <= 0) {
        dormindo = true;
        postura = Postura::Deitado;
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
    atualizarRoca(dm);
    if (minutos >= 1440 + 6 * 60) chegouCarta("manel1");  // no dia seguinte à chegada
    while (minutos >= proximaColeta) { coletarRemessa(); proximaColeta += 1440; }  // seu Manel passa às 22h
    for (size_t k = 0; k < agenda.size();) {  // coisas marcadas (a cena com seu Tião)
        if (agenda[k].first <= agora) { auto f = agenda[k].second; agenda.erase(agenda.begin() + (long)k); f(); }
        else k++;
    }
    if (!cenaTiao && mundoAtual == "fazenda") {  // chegou perto de seu Tião pela primeira vez
        int t = idx("tiao");
        if (t >= 0 && std::max(std::abs(x - moveis[t].x), std::abs(y - moveis[t].y)) <= 3 && !dormindo) cenaChegada();
    }
    atualizarMoveis(seg);
    atualizarGuia(seg);
    atualizarTutorial(seg);
}

// Sons do lugar e da hora: córrego, vento, pássaros de dia, grilos à noite, galo de madrugada.
void Jogo::ambiente(double dt) {
    float altura = postura == Postura::EmPe ? 1.6f : postura == Postura::Sentado ? 0.9f : postura == Postura::Agachado ? 0.7f : 0.2f;
    audio::ouvinte(x + 0.5f, y + 0.5f, altura, rumo * 45.0f);
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
    if (novoJogo && genero == '?') { escolherGenero(true); return; }
    if (!novoJogo) {
        char h[16]; std::snprintf(h, sizeof h, "%d e %02d", hora24(), (int)minutos % 60);
        fala::dizer("Simulador de Vida. Que bom ter você de volta. Dia " + std::to_string(diaDoJogo()) + ", " + h + ". Você está em: " +
                    mapa.nomeLugar(x, y) + ", de frente para o " + direcao(rumo) + ". F1 para ouvir as teclas.");
        if (genero == '?') escolherGenero(false);  // jogo salvo antes da escolha existir
        return;
    }
    carroPai = audio::fonteMovel("carro", 253, 2, 0.6f, 0.8f, 6, true);
    carroT = -1;
    fala::dizer("Simulador de Vida. F1, a qualquer momento, para ouvir as teclas.");
    fala::dizer("Hoje você completa 18 anos. Seu pai para a caminhonete na estrada de terra, em frente à porteira da fazenda, e desce sem desligar o motor. "
                "Ele joga uma mochila velha de lona no gramado, aos seus pés, e fala sem rodeio:", false);
    fala::dizer("Pois é. Dezoito anos. Acabou a mamata: daqui pra frente eu não te sustento mais, não. Essa fazenda aqui tá parada faz anos, "
                "e agora é sua pra cuidar. Tem casa, tem córrego, tem terra boa. Quer comer, vai ter que plantar. Quer leite, vai ter que cuidar de bicho. "
                "Na mochila tem o que dá pra começar. O resto é com você. Boa sorte, viu? Ah, e vá até a casa do Tião, ele tem uma coisinha pra te dar.", false);
    fala::dizer("Ele te dá um tapinha no ombro, volta para a caminhonete e vai embora. O motor vai sumindo na estrada, e fica só o vento no capim. "
                "Você está no gramado da porteira principal, de frente para o norte. A porta da casa sede fica bem em frente, "
                "a 100 passos, no meio da parede da varanda. Seu Tião é o caseiro: mora com dona Nezinha numa casinha perto da casa sede, a oeste. "
                "Para achar: Ç até Lugares, L até a casinha, e O para seguir o bip. Aperte I para abrir a mochila. Se é a primeira vez, aperte Tab e escolha Tutorial: ele ensina o básico, passo a passo.", false);
}

// ---------- configurações (volume) ----------
void Jogo::carregarConfig(const std::string& arquivo) {
    arquivoConfig = arquivo;
    std::ifstream f(arquivo);
    std::string linha;
    while (std::getline(f, linha)) {
        std::istringstream s(linha);
        std::string k; s >> k;
        if (k == "volume") { std::string cat; float v; if (s >> cat >> v) audio::definirVolume(cat, v); }
        else if (k == "nome") { std::getline(s >> std::ws, nomeOnline); }
        else if (k == "ip") s >> ultimoIp;
        else if (k == "chat") { int c = 1; s >> c; chatAtivo = c != 0; }
    }
}
void Jogo::salvarConfig(const std::string& arquivo) const {
    std::ofstream f(arquivo);
    for (const char* c : {"ambiente", "passos", "efeitos", "interface", "musica", "voz"}) f << "volume " << c << ' ' << audio::volume(c) << '\n';
    if (!nomeOnline.empty()) f << "nome " << nomeOnline << '\n';
    if (!ultimoIp.empty()) f << "ip " << ultimoIp << '\n';
    f << "chat " << (chatAtivo ? 1 : 0) << '\n';
}

// ---------- salvar ----------
void Jogo::salvar(const std::string& arquivo) {
    std::ofstream f(arquivo);
    if (!f) return;
    bool fora = (bool)fazendaGuardada;  // fora de casa: salva a fazenda guardada, e você volta para ela
    if (fora) trocarComGuardada();
    f << "versao 2\n" << "pos " << x << ' ' << y << ' ' << rumo << '\n'
      << "corpo " << fome << ' ' << sede << ' ' << energia << ' ' << saude << '\n'
      << "relogio " << minutos << '\n'
      << "corpo2 " << (int)postura << ' ' << intestino << ' ' << bexiga << ' ' << higiene << '\n';
    f << "historia " << (cenaTiao ? 1 : 0) << ' ' << diaCafe << '\n';
    f << "genero " << genero << '\n' << "coleta " << proximaColeta << '\n';
    for (const Carta& c : cartas) f << "carta " << c.id << ' ' << (c.lida ? 1 : 0) << '\n';
    for (int c : limpos) f << "limpo " << c << '\n';
    for (const auto& [c, p] : plantas) f << "planta " << c << ' ' << p.tipo << ' ' << p.horas << ' ' << p.molhadaAte << '\n';
    for (size_t i = 0; i < moveis.size(); i++) {
        f << "movel " << i << ' ' << moveis[i].x << ' ' << moveis[i].y << '\n';
        for (size_t p = 0; p < moveis[i].partes.size(); p++)
            for (const Pilha& q : moveis[i].partes[p]) f << "dentro " << i << ' ' << p << ' ' << q.coisa.tipo << ' ' << q.coisa.carga << ' ' << q.qtd << '\n';
    }
    for (auto [px, py] : mapa.passagens()) f << "porta " << px << ' ' << py << ' ' << (mapa.em(px, py).aberta ? 1 : 0) << '\n';
    for (const Pilha& p : mochila) f << "item " << p.coisa.tipo << ' ' << p.coisa.carga << ' ' << p.qtd << '\n';
    f << "maoD " << (maoDireita.tipo.empty() ? "-" : maoDireita.tipo) << ' ' << maoDireita.carga << '\n';
    f << "maoE " << (maoEsquerda.tipo.empty() ? "-" : maoEsquerda.tipo) << ' ' << maoEsquerda.carga << '\n';
    if (fora) trocarComGuardada();
}

bool Jogo::carregar(const std::string& arquivo) {
    std::ifstream f(arquivo);
    if (!f) return false;
    menus.clear();
    std::vector<Pilha> m;
    std::string linha;
    int versao = 1;
    bool movelLido = false;
    while (std::getline(f, linha)) {
        std::istringstream s(linha);
        std::string k; s >> k;
        if (k == "versao") s >> versao;
        else if (k == "pos") s >> x >> y >> rumo;
        else if (k == "corpo") s >> fome >> sede >> energia >> saude;
        else if (k == "relogio") s >> minutos;
        else if (k == "corpo2") { int p = 0; s >> p >> intestino >> bexiga; if (!(s >> higiene)) higiene = 90; postura = (Postura)std::clamp(p, 0, 3); }
        else if (k == "movel") {
            size_t i; int mx, my; s >> i >> mx >> my;
            if (i < moveis.size() && mapa.valido(mx, my)) {
                if (!movelLido) { for (Movel& mv : moveis) for (auto& pt : mv.partes) pt.clear(); movelLido = true; }
                moveis[i].x = mx; moveis[i].y = my;
            }
        }
        else if (k == "historia") { int c = 0; s >> c >> diaCafe; cenaTiao = c != 0; }
        else if (k == "genero") { s >> genero; if (genero != 'h' && genero != 'm') genero = '?'; }
        else if (k == "coleta") s >> proximaColeta;
        else if (k == "carta") { Carta c; int l = 0; s >> c.id >> l; c.lida = l != 0; cartas.push_back(c); }
        else if (k == "limpo") { int c; s >> c; if (c >= 0 && c < LADO * LADO) { limpos.push_back(c); mapa.em(c % LADO, c / LADO).chao = Chao::TerraArada; } }
        else if (k == "planta") {
            int c; Planta p; s >> c >> p.tipo >> p.horas >> p.molhadaAte;
            if (c >= 0 && c < LADO * LADO && p.tipo >= 1 && p.tipo <= 3) { plantas[c] = p; mapa.em(c % LADO, c / LADO).planta = (uint8_t)p.tipo; }
        }
        else if (k == "dentro") {
            size_t i, p; Pilha q; s >> i >> p >> q.coisa.tipo >> q.coisa.carga >> q.qtd;
            if (i < moveis.size() && p < moveis[i].partes.size() && q.qtd > 0) moveis[i].partes[p].push_back(q);
        }
        else if (k == "porta") { int px, py, a; s >> px >> py >> a; if (mapa.ehPassagem(px, py)) mapa.em(px, py).aberta = a != 0; }
        else if (k == "item") { Pilha p; s >> p.coisa.tipo >> p.coisa.carga >> p.qtd; if (p.qtd > 0) m.push_back(p); }
        else if (k == "maoD" || k == "maoE") {
            Coisa c; s >> c.tipo >> c.carga; if (c.tipo == "-") c = {};
            (k == "maoD" ? maoDireita : maoEsquerda) = c;
        }
    }
    mochila = m;
    // jogo salvo no mapa antigo, de 500 por 500: volta para a porteira, com a mochila e o corpo como estavam
    if (versao < 2 || !mapa.valido(x, y) || !mapa.passa(x, y)) { x = 500; y = 9; rumo = 0; }
    rumo = (rumo % 8 + 8) % 8;
    ocupar();
    if (!mapa.passa(x, y) && mapa.em(x, y).movel >= 0) { x = 500; y = 9; }
    if (postura == Postura::Deitado || postura == Postura::Sentado) sentadoEm = -1;
    lugarAtual = mapa.nomeLugar(x, y);
    return true;
}
