// Cozinha e paiol: torrar café e ferver garapa no fogão (com panela), moedor de café, cafeteira expresso
// com vaporizador de leite, prensa de queijo (cozinha), moenda de cana e moinho de trigo (paiol).
#include <algorithm>
#include <cmath>
#include "audio.h"
#include "fala.h"
#include "jogo.h"
#include "util.h"

// Quantas coisas desse tipo a pessoa tem (mãos e mochila); com "cheia", só garrafas de leite cheias.
int Jogo::contarTipo(const std::string& tipo, int carga) const {
    int n = 0;
    for (const Coisa* m : {&maoDireita, &maoEsquerda}) if (m->tipo == tipo && (carga < 0 || m->carga == carga)) n++;
    for (const Pilha& p : mochila) if (p.coisa.tipo == tipo && (carga < 0 || p.coisa.carga == carga)) n += p.qtd;
    return n;
}

// Gasta n coisas desse tipo: primeiro da mochila, depois das mãos. "troca" vira o que sobra (garrafa vazia).
void Jogo::gastarTipo(const std::string& tipo, int n, int carga, const Coisa& troca) {
    for (size_t k = 0; k < mochila.size() && n > 0;) {
        Pilha& p = mochila[k];
        if (p.coisa.tipo == tipo && (carga < 0 || p.coisa.carga == carga)) {
            int usa = std::min(n, p.qtd);
            Coisa c = p.coisa;
            for (int j = 0; j < usa; j++) tirar(c);
            n -= usa;
            if (!troca.tipo.empty()) guardar(troca, usa);
            continue;  // a pilha pode ter sumido: olha de novo a mesma posição
        }
        k++;
    }
    for (Coisa* m : {&maoDireita, &maoEsquerda})
        if (n > 0 && m->tipo == tipo && (carga < 0 || m->carga == carga)) { *m = troca; n--; }
}



// Coisa na mão, de frente para o fogão ou para uma máquina. true: respondeu.
bool Jogo::usarMaquina(Coisa& c) {
    int f = movelFrente();
    if (f < 0) return false;
    Movel& m = moveis[f];
    const std::string& t = c.tipo;
    if (m.tipo == "fogao" && t == "ovo") {
        if (!m.ligado) { fala::dizer("O fogão está apagado. Acenda com Espaço, de mão vazia, e tente de novo."); return true; }
        if (!temCoisa("panela")) { fala::dizer("Precisa de uma panela: seu Zé vende, por 25 moedas."); return true; }
        Coisa usado = c; c = {};
        if (tirar(usado)) c = usado;
        guardar({"ovo-frito", 0});
        audio::tocar("fritura", 0.7f);
        fala::dizer("Você quebra o ovo na beira da panela e ele cai chiando no óleo quente: a clara borbulha e fica branca, a gema mole no meio. Um ovo frito, na mochila.");
        return true;
    }
    if (m.tipo == "fogao" && (t == "cafe" || t == "garapa")) {
        if (!m.ligado) { fala::dizer("O fogão está apagado. Acenda com Espaço, de mão vazia, e tente de novo."); return true; }
        if (!temCoisa("panela")) { fala::dizer("Precisa de uma panela: seu Zé vende, por 25 moedas."); return true; }
        if (t == "cafe") {
            Coisa usado = c; c = {};
            if (tirar(usado)) c = usado;
            guardar({"cafe-torrado", 0});
            audio::tocar("fritura", 0.6f, 0.8f);
            fala::dizer("Você joga os grãos na panela seca e mexe sem parar com a colher de pau. Estalam, escurecem, e a cozinha inteira cheira a café. "
                        "Um punhado de café torrado, na mochila. Três punhados enchem um pacote no moedor.");
            return true;
        }
        int n = contarTipo("garapa");
        if (n < 3) { fala::dizer("Para dar açúcar, precisa de 3 copos de garapa na panela. Você tem " + std::to_string(n) + "."); return true; }
        gastarTipo("garapa", 3);
        guardar({"acucar", 0});
        audio::tocar("fritura", 0.5f, 0.6f);
        fala::dizer("Você despeja a garapa na panela e deixa ferver, mexendo. A água vai embora, a calda engrossa, escurece, e no fim seca em açúcar mascavo, "
                    "que você esfarela com a colher. Um saco de açúcar, na mochila.");
        return true;
    }
    if (m.tipo == "moedor") {
        if (t == "cafe") { fala::dizer("Café cru não se mói: torre antes, na panela, no fogo. Grão na mão, de frente para o fogão aceso, Espaço."); return true; }
        if (t != "cafe-torrado") { fala::dizer("O moedor é para café torrado."); return true; }
        int n = contarTipo("cafe-torrado");
        if (n < 3) { fala::dizer("Para encher um pacote, precisa de 3 punhados de café torrado. Você tem " + std::to_string(n) + "."); return true; }
        gastarTipo("cafe-torrado", 3);
        guardar({"po-cafe", 0});
        audio::tocar3D("moedor", m.x + 0.5f, m.y + 0.5f, 1.0f, 0.9f, 4);
        fala::dizer("Você despeja os grãos no funil e gira a manivela: o moedor range e mói, e o pó cai cheiroso na gavetinha. Um pacote de pó de café, na mochila.");
        return true;
    }
    if (m.tipo == "cafeteira") {
        if (t != "po-cafe") return false;
        if (m.carga >= 30) { fala::dizer("O reservatório de pó da cafeteira já está cheio: 30 doses."); return true; }
        Coisa usado = c; c = {};
        if (tirar(usado)) c = usado;
        m.carga = std::min(30, m.carga + 10);
        audio::tocar("embalagem", 0.5f);
        fala::dizer("Você abre a tampinha do compartimento de pó, em cima da cafeteira, e despeja o pacote. Dá para " + std::to_string(m.carga) + " doses.");
        return true;
    }
    if (m.tipo == "prensa") {
        if (t != "leite") return false;
        if (m.pronto > 0) { prensaPronta(f); return true; }
        int n = contarTipo("leite", 4);
        if (n < 2) { fala::dizer("Para um queijo, precisa de 2 garrafas de leite cheias. Você tem " + std::to_string(n) + "."); return true; }
        gastarTipo("leite", 2, 4, {"garrafa", 0});
        m.pronto = minutos + 12 * 60;
        audio::tocar3D("prensa", m.x + 0.5f, m.y + 0.5f, 0.8f, 0.9f, 4);
        fala::dizer("Você aquece o leite, pinga o coalho e espera firmar. Corta a coalhada, põe na forma com o pano, encaixa na prensa e aperta o fuso, "
                    "e o soro escorre. Fica prensando: o queijo fica pronto em 12 horas do jogo. As garrafas vazias voltam para a mochila.");
        return true;
    }
    if (m.tipo == "moenda") {
        if (t != "cana") { fala::dizer("A moenda é para cana-de-açúcar."); return true; }
        Coisa usado = c; c = {};
        if (tirar(usado)) c = usado;
        guardar({"garapa", 0}, 2);
        energia = std::max(0.0, energia - 0.3);
        audio::tocar3D("moenda", m.x + 0.5f, m.y + 0.5f, 0.8f, 1.0f, 6);
        fala::dizer("Você enfia a cana entre os rolos e gira a roda: a moenda range, a cana estala e sai achatada do outro lado, e a garapa escorre verdinha "
                    "na bica. Dois copos de garapa, na mochila. Três copos, fervidos na panela, viram açúcar.");
        return true;
    }
    if (m.tipo == "moinho") {
        if (t != "trigo") { fala::dizer("O moinho é para trigo."); return true; }
        int n = contarTipo("trigo");
        if (n < 2) { fala::dizer("Para um saco de farinha, precisa de 2 feixes de trigo. Você tem " + std::to_string(n) + "."); return true; }
        gastarTipo("trigo", 2);
        guardar({"farinha", 0});
        energia = std::max(0.0, energia - 0.3);
        audio::tocar3D("moenda", m.x + 0.5f, m.y + 0.5f, 0.8f, 0.9f, 6, 0.8f);
        fala::dizer("Você debulha os feixes, joga os grãos no funil e gira a pedra do moinho: ela roda pesada, chiando, e a farinha cai branquinha. "
                    "Um saco de farinha de trigo, na mochila.");
        return true;
    }
    return false;
}

void Jogo::prensaPronta(int i) {
    Movel& m = moveis[i];
    if (m.pronto <= 0) {
        fala::dizer("Prensa de queijo, de madeira, com o fuso de rosca em cima. Vazia. Para fazer queijo: garrafa de leite cheia na mão, e outra na mochila, Espaço.");
        return;
    }
    if (minutos < m.pronto) {
        int h = std::max(1, (int)std::lround((m.pronto - minutos) / 60.0));
        fala::dizer("O queijo ainda está na prensa, pingando soro. Fica pronto em " + std::to_string(h) + (h == 1 ? " hora" : " horas") + " do jogo.");
        return;
    }
    m.pronto = 0;
    guardar({"queijo", 0});
    audio::tocar3D("prensa", m.x + 0.5f, m.y + 0.5f, 0.8f, 0.7f, 4, 1.2f);
    fala::dizer("Você afrouxa o fuso, tira a forma e desenforma: um queijo minas branquinho, firme, cheirando a leite. Na mochila.");
}

// Espaço de mão vazia na cafeteira: o menu dela.
void Jogo::menuCafeteira(int i) {
    Menu mn;
    mn.titulo = "Cafeteira expresso. Pó no compartimento: " + std::to_string(moveis[i].carga) + (moveis[i].carga == 1 ? " dose" : " doses");
    mn.opcoes = [this, i]() {
        std::vector<Opcao> v;
        v.push_back({"Tirar um café expresso", [this, i]() {
            Movel& m = moveis[i];
            if (m.carga <= 0) { fala::dizer("Sem pó no compartimento. Pacote de pó de café na mão, de frente para a cafeteira, Espaço."); return; }
            m.carga--;
            guardar({"expresso", 0});
            audio::tocar3D("expresso", m.x + 0.5f, m.y + 0.5f, 1.0f, 0.9f, 5);
            fala::dizer("Você tira o porta-filtro, enche com uma dose de pó, aperta com o socador e encaixa girando. Põe a xícara embaixo e aperta o botão: "
                        "a bomba zune, e o café escorre fininho, grosso, com o creme dourado por cima. Uma xícara de expresso, na mochila. Restam " +
                        std::to_string(m.carga) + (m.carga == 1 ? " dose." : " doses."));
        }, nullptr, nullptr});
        v.push_back({"Expresso com leite vaporizado", [this, i]() {
            Movel& m = moveis[i];
            if (m.carga <= 0) { fala::dizer("Sem pó no compartimento. Pacote de pó de café na mão, de frente para a cafeteira, Espaço."); return; }
            // um gole de leite de qualquer garrafa de leite: primeiro a da mão, senão uma da mochila
            bool gastou = false;
            Coisa* g = maoDireita.tipo == "leite" ? &maoDireita : maoEsquerda.tipo == "leite" ? &maoEsquerda : nullptr;
            if (g) { g->carga--; if (g->carga <= 0) *g = {"garrafa", 0}; gastou = true; }
            for (size_t k = 0; k < mochila.size() && !gastou; k++)
                if (mochila[k].coisa.tipo == "leite") {
                    Coisa l = mochila[k].coisa; tirar(l); l.carga--;
                    guardar(l.carga > 0 ? l : Coisa{"garrafa", 0});
                    gastou = true;
                }
            if (!gastou) { fala::dizer("Precisa de leite: uma garrafa de leite na mão ou na mochila. As vacas do curral de leite dão."); return; }
            m.carga--;
            guardar({"cappuccino", 0});
            audio::tocar3D("expresso", m.x + 0.5f, m.y + 0.5f, 1.0f, 0.9f, 5);
            marcar(3.0, [this, i]() { audio::tocar3D("vaporizador", moveis[i].x + 0.5f, moveis[i].y + 0.5f, 1.0f, 0.9f, 5); });
            fala::dizer("Primeiro o expresso: a bomba zune e o café escorre na xícara grande. Depois você despeja um gole de leite na jarrinha de inox, "
                        "mergulha o bico do vaporizador, aquele caninho do lado da máquina, e gira o botão do vapor: chia alto, o leite gira, esquenta e cresce "
                        "numa espuma cremosa. Você despeja por cima do café, devagar. Uma xícara de café com leite vaporizado, na mochila.");
        }, nullptr, nullptr});
        return v;
    };
    abrirMenu(mn);
}
