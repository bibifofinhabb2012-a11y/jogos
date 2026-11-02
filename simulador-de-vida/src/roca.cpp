// Roça e gente: seu Tião e dona Nezinha, a cena da chegada, limpar matagal, plantar, regar, colher, vender.
#include <algorithm>
#include <cmath>
#include "audio.h"
#include "fala.h"
#include "jogo.h"
#include "util.h"

constexpr double HORAS_PARA_COLHER = 72;  // 3 dias do jogo, sempre com a terra molhada
static const char* PRODUTO[4] = {"", "milho", "feijao", "arroz"};
static const char* SEMENTE[4] = {"", "semente-milho", "semente-feijao", "semente-arroz"};

void Jogo::marcar(double segundos, std::function<void()> f) { agenda.push_back({agora + segundos, std::move(f)}); }

int Jogo::idx(const std::string& tipo) const {
    for (size_t i = 0; i < moveis.size(); i++) if (moveis[i].tipo == tipo) return (int)i;
    return -1;
}

int Jogo::plantaFrente() const {
    int fx = x + DX[rumo], fy = y + DY[rumo];
    if (!mapa.valido(fx, fy) || !mapa.em(fx, fy).planta) return -1;
    return fy * LADO + fx;
}

std::string Jogo::nomePlanta(int tipo, bool plural) const {
    if (tipo == 1) return plural ? "pés de milho" : "pé de milho";
    if (tipo == 2) return plural ? "pés de feijão de corda" : "pé de feijão de corda";
    return plural ? "touceiras de arroz" : "touceira de arroz";
}

std::string Jogo::estadoPlanta(const Planta& p) const {
    bool molhada = minutos < p.molhadaAte;
    if (p.horas >= HORAS_PARA_COLHER) return "pronto para colher";
    double dias = (HORAS_PARA_COLHER - p.horas) / 24.0;
    std::string falta = dias > 1.75 ? "faltam uns " + std::to_string((int)std::lround(dias)) + " dias"
                      : dias > 0.75 ? "falta mais ou menos um dia" : "falta menos de um dia";
    std::string fase = p.horas < 12 ? "a semente ainda está debaixo da terra" : p.horas < 36 ? "brotou, ainda pequeno" : "crescendo bem";
    return fase + ", " + falta + (molhada ? "" : ". A terra está seca: precisa de água, senão não cresce");
}

// Espaço com machado, semente ou regador na mão. true: era coisa de roça e já respondeu.
bool Jogo::usarNaRoca(Coisa& c) {
    const std::string& t = c.tipo;
    bool semente = t.rfind("semente-", 0) == 0;
    if (t != "machado" && t != "regador" && !semente) return false;
    int fx = x + DX[rumo], fy = y + DY[rumo];
    if (!mapa.valido(fx, fy)) { fala::dizer("Não tem nada na sua frente."); return true; }
    Celula& k = mapa.em(fx, fy);
    int cel = fy * LADO + fx;
    if (t == "machado") {
        if (k.planta) {
            if (k.planta == 3 && plantas[cel].horas >= HORAS_PARA_COLHER) colher(cel, true);
            else fala::dizer("Cuidado: o machado estragaria o " + nomePlanta(k.planta) + ".");
            return true;
        }
        if (k.chao == Chao::Mato && k.obs == Obstaculo::Nada) {
            k.chao = Chao::TerraArada;
            limpos.push_back(cel);
            energia -= 1.5; sede -= 0.5; higiene -= 0.5;
            audio::tocar3D("machado", fx + 0.5f, fy + 0.5f, 0.5f, 0.9f, 2, (float)acaso(0.92, 1.08));
            int jaLimpos = (int)limpos.size();
            fala::dizer(jaLimpos == 1 ? "Você golpeia o matagal com o machado, puxa as raízes, e o chão aparece. Um quadrado de terra limpa, pronta para plantar."
                                      : "Mais um quadrado limpo. São " + std::to_string(jaLimpos) + ".");
            evento("limpou");
            return true;
        }
        if (k.obs == Obstaculo::Arvore) { fala::dizer("Derrubar árvore fica para outro dia."); return true; }
        fala::dizer("Não tem mato na sua frente para cortar.");
        return true;
    }
    if (semente) {
        int tipo = t == "semente-milho" ? 1 : t == "semente-feijao" ? 2 : 3;
        if (k.chao == Chao::Mato) { fala::dizer("Aí é matagal. Limpe primeiro com o machado."); return true; }
        if (k.planta) { fala::dizer("Já tem um " + nomePlanta(k.planta) + " aí."); return true; }
        if (k.chao != Chao::TerraArada || k.obs != Obstaculo::Nada || k.movel >= 0 || (fx == x && fy == y)) {
            fala::dizer("Semente se planta na terra limpa da roça, no quadrado da sua frente."); return true;
        }
        k.planta = (uint8_t)tipo;
        plantas[cel] = Planta{tipo, 0, 0};
        energia -= 0.5;
        audio::tocar3D("cavar", fx + 0.5f, fy + 0.5f, 0.2f, 0.9f, 2);
        std::string msg = "Você abre uma covinha com os dedos, põe a semente e cobre de terra. " + maiuscula(nomePlanta(tipo)) + " plantado. Agora precisa de água.";
        if (tirar({t, 0})) msg += " Você pega outra semente no saco.";
        else { c = {}; msg += " Acabaram as sementes desse tipo."; }
        fala::dizer(msg);
        evento("plantou");
        return true;
    }
    // regador
    int ax, ay;
    if (!k.planta && mapa.aguaMaisPerto(x, y, 3, ax, ay)) {
        c.carga = 10;
        audio::tocar3D("passos-agua", ax + 0.5f, ay + 0.5f, 0, 1.0f, 3);
        fala::dizer("Você mergulha o regador no córrego até borbulhar. Cheio: dá para 10 regas.");
        evento("encheu");
        return true;
    }
    if (k.planta) {
        if (c.carga <= 0) { fala::dizer("O regador está vazio. Encha na beira do córrego."); return true; }
        c.carga--;
        plantas[cel].molhadaAte = minutos + 24 * 60;
        energia -= 0.3;
        audio::tocar3D("regador", fx + 0.5f, fy + 0.5f, 0.3f, 0.9f, 2);
        fala::dizer("Você rega o " + nomePlanta(k.planta) + ". A terra escurece, molhada." + (c.carga == 0 ? " O regador ficou vazio." : ""));
        evento("regou");
        return true;
    }
    fala::dizer("O regador se usa de frente para uma planta, ou na beira do córrego, para encher.");
    return true;
}

void Jogo::colher(int cel, bool comMachado) {
    Planta p = plantas[cel];
    int fx = cel % LADO, fy = cel / LADO;
    if (p.horas < HORAS_PARA_COLHER) { fala::dizer(maiuscula(nomePlanta(p.tipo)) + ": " + estadoPlanta(p) + "."); return; }
    if (p.tipo == 3 && !comMachado) { fala::dizer("O arroz está pronto. Arroz se colhe com o machado: segure o machado e aperte Espaço."); return; }
    int qtd = p.tipo == 1 ? 3 : p.tipo == 2 ? 4 : 3;
    guardar({PRODUTO[p.tipo], 0}, qtd);
    guardar({SEMENTE[p.tipo], 0}, 2);
    plantas.erase(cel);
    mapa.em(fx, fy).planta = 0;
    energia -= 0.5;
    audio::tocar3D(p.tipo == 3 ? "machado" : "colher", fx + 0.5f, fy + 0.5f, 0.5f, 0.9f, 2);
    const char* txt = p.tipo == 1 ? "Você quebra as espigas do pé de milho, uma, duas, três, com aquele estalo da palha."
                    : p.tipo == 2 ? "Você puxa as vagens compridas do feijão de corda e debulha numa mão cheia."
                                  : "Você corta a touceira de arroz rente ao chão com o machado e amarra os feixes.";
    fala::dizer(std::string(txt) + " Foi tudo para a mochila, e ainda sobraram 2 sementes para plantar de novo. A terra fica limpa para outra planta.");
    evento("colheu");
}

void Jogo::atualizarRoca(double dm) {
    for (auto& [cel, p] : plantas) {
        if (minutos >= p.molhadaAte || p.horas >= HORAS_PARA_COLHER) continue;
        p.horas = std::min(HORAS_PARA_COLHER, p.horas + dm / 60.0);
    }
}

// ---------- seu Tião e dona Nezinha ----------
void Jogo::cenaChegada() {
    if (cenaTiao) return;
    cenaTiao = true;
    int t = idx("tiao"), n = idx("nezinha");
    andandoFrente = andandoTras = false; andandoLado = 0; caminho.clear(); rastreando = false;
    fala::dizer("Seu Tião vê você chegando, abre um sorriso e grita para dentro de casa: Nezinha, ô Nezinha! Corre ver, " + gen("o menino", "a menina") + " do Alceu chegou!");
    fala::dizer("E vem de braços abertos: Aôôôba! O Alceu me falou que tu vinha, mas " + gen("rapaz, tu tá um homem", "guria, tu tá uma mulher") + "! Ó, vou te falar, as vaca aqui são meio arredia no começo, viu? "
                "Tu vai ter que se acostumar com os coice. Faz parte da vida na lida. As galinha também são cheia de novela, tem que ter milho pra alimentar esse povo todo.", false);
    fala::dizer("Ele entra e volta com um saco de pano e umas ferramentas: Toma, isso aqui é pra ti. Semente de milho, de feijão de corda e de arroz. "
                "A roça fica pra lá da ponte do córrego, do lado direito da estrada, mas tá tomada de matagal: tem que limpar no machado. "
                "E esse regador aqui é véio, meio amassado, mas ainda rega. A água tu pega no córrego, que passa do ladinho da roça.", false);
    marcar(1.0, [this, t]() { if (t >= 0) audio::tocar3D("passos-calcada", moveis[t].x + 0.5f, moveis[t].y + 2.5f, 0.2f, 0.6f, 4); });
    marcar(14.0, [this]() {
        guardar({"semente-milho", 0}, 20); guardar({"semente-feijao", 0}, 20); guardar({"semente-arroz", 0}, 20);
        guardar({"machado", 0}); guardar({"regador", 0});
        audio::tocar("sementes", 0.8f);
    });
    // dona Nezinha sai de dentro de casa e vem até a porta
    for (int k = 0; k < 6; k++)
        marcar(26.0 + k * 0.5, [this, n, k]() {
            if (n < 0) return;
            audio::tocar3D("passos-calcada", 372.5f, 200.5f - k * 2.0f, 0.1f, 0.7f, 3);
            if (k == 5 && mapa.em(372, 187).movel < 0) {
                mapa.em(moveis[n].x, moveis[n].y).movel = -1;
                moveis[n].x = 372; moveis[n].y = 187;
                mapa.em(372, 187).movel = (int16_t)n;
            }
        });
    fala::dizer("Dona Nezinha aparece na porta, enxugando as mãos no avental: Ô, " + gen("menino", "menina") + ", se aprochegue! Vamos tomar um café, passei ainda agora!", false);
    fala::dizer("Ela serve uma xícara de café, quente e doce. Você toma devagar, e o cansaço da viagem vai embora.", false);
    marcar(34.0, [this]() { audio::tocar("cafe", 0.8f); });
    marcar(37.0, [this]() { audio::tocar("gole", 0.8f); energia = std::min(100.0, energia + 15); sede = std::min(100.0, sede + 5); });
    fala::dizer("E leva essa garrafa térmica pra roça, que capinar cansa. Quando acabar, volta aqui que eu encho de novo.", false);
    marcar(40.0, [this]() { guardar({"garrafa-cafe", 8}); audio::tocar("mochila", 0.6f); diaCafe = diaDoJogo(); });
    fala::dizer("Ganhou, na mochila: 20 sementes de milho, 20 de feijão de corda, 20 de arroz, um machado, um regador e uma garrafa térmica de café com 8 goles. "
                "Como fazer: com o machado na mão, Espaço de frente para o matagal limpa a terra. Com a semente na mão, Espaço de frente para a terra limpa planta. "
                "Com o regador na mão, Espaço de frente para a planta rega; na beira do córrego, enche. Planta regada cresce; com 3 dias do jogo fica pronta. "
                "Para colher, mão vazia e Espaço; o arroz se colhe com o machado. "
                "Seu Tião ainda conta: o que colher, tu põe na caixa de remessa, lá perto da porteira, que o prefeito passa toda noite e paga. "
                "Para achar a roça: Ç até Lugares, e L até Roça.", false);
    evento("tiao");
}

void Jogo::conversarCom(int i) {
    const std::string& t = moveis[i].tipo;
    if (t == "tiao" && !cenaTiao) { cenaChegada(); return; }
    static const char* LINHAS_TIAO[] = {
        "Tá vendo esse céu? Amanhã é dia bom de plantar.",
        "As vaca tão arisca hoje, nem chega perto do curral, viu?",
        "Milho bem regado dá espiga que é uma beleza. Água todo dia, tchê.",
        "Colheu? Põe na caixa de remessa, lá na porteira. O Manel do Bacanal passa de noite e paga direitinho.",
    };
    static const char* LINHAS_NEZINHA[] = {
        "Tu tá comendo direito, MENINO? Essas barra aí não sustenta ninguém.",
        "Cuidado com o sol na roça, viu? Leva água.",
        "Aqui em casa o café tá sempre passado. Se aprochegue quando quiser.",
    };
    Menu m;
    if (t == "tiao") {
        m.titulo = "Seu Tião";
        m.opcoes = [this]() {
            std::vector<Opcao> v;
            v.push_back({"Conversar", [this]() { fala::dizer(std::string("Seu Tião: ") + LINHAS_TIAO[(int)acaso(0, 3.99)]); }, nullptr, nullptr});
            return v;
        };
    } else {
        m.titulo = "Dona Nezinha";
        m.opcoes = [this]() {
            std::vector<Opcao> v;
            v.push_back({"Conversar", [this]() {
                std::string l = LINHAS_NEZINHA[(int)acaso(0, 2.99)];
                auto p = l.find("MENINO"); if (p != std::string::npos) l.replace(p, 6, gen("menino", "menina"));
                fala::dizer("Dona Nezinha: " + l);
            }, nullptr, nullptr});
            v.push_back({"Pedir para encher a garrafa de café", [this]() {
                if (diaCafe == diaDoJogo()) { fala::dizer("Dona Nezinha: Hoje já enchi, " + gen("menino", "menina") + "! Amanhã passo mais."); return; }
                bool cheia = false;  // enche a que estiver na mão; senão, a da mochila; senão, dá outra
                for (Coisa* g : {&maoDireita, &maoEsquerda}) if (!cheia && g->tipo == "garrafa-cafe") { g->carga = 8; cheia = true; }
                for (size_t k = 0; k < mochila.size() && !cheia; k++)
                    if (mochila[k].coisa.tipo == "garrafa-cafe") { Coisa velha = mochila[k].coisa; tirar(velha); guardar({"garrafa-cafe", 8}); cheia = true; }
                if (!cheia) { fala::dizer("Dona Nezinha: Cadê a garrafa, " + gen("menino", "menina") + "? Perdeu? Toma outra, então.", false); guardar({"garrafa-cafe", 8}); }
                diaCafe = diaDoJogo();
                audio::tocar("cafe", 0.8f);
                fala::dizer("Dona Nezinha enche a garrafa térmica até a boca: café fresquinho, 8 goles.");
            }, nullptr, nullptr});
            return v;
        };
    }
    abrirMenu(m);
}
