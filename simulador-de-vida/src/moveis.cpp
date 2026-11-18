// Móveis e aparelhos da casa: onde ficam, o que fazem, arrastar, sentar, deitar, banho.
#include <algorithm>
#include <cmath>
#include "audio.h"
#include "fala.h"
#include "jogo.h"
#include "util.h"

struct TipoMovel { const char* tipo; const char* nome; bool fem, arrasta, senta, deita; };
static const TipoMovel TIPOS[] = {
    {"sofa", "sofá", false, true, true, false},
    {"poltrona", "poltrona", true, true, true, false},
    {"mesa", "mesa de centro", true, true, false, false},
    {"cama", "cama", true, true, true, true},
    {"guarda-roupa", "guarda-roupa", false, true, false, false},
    {"geladeira", "geladeira", true, true, false, false},
    {"fogao", "fogão a gás", false, true, false, false},
    {"pia", "pia", true, false, false, false},
    {"vaso", "vaso sanitário", false, false, true, false},
    {"chuveiro", "chuveiro", false, false, false, false},
    {"tiao", "seu Tião", false, false, false, false},
    {"nezinha", "dona Nezinha", true, false, false, false},
    {"correio", "caixa de correio", true, false, false, false},
    {"remessa", "caixa de remessa", true, false, false, false},
    {"trator", "trator", false, false, false, false},
    {"balcao", "balcão do bar", false, false, false, false},
    {"mesa-bar", "mesa do bar", true, true, false, false},
    {"ze", "seu Zé", false, false, false, false},
    {"prefeito", "seu Manel do Bacanal, o prefeito", false, false, false, false},
    {"mesa-festa", "mesa de comes e bebes", true, false, false, false},
    {"vaca", "vaca de corte", true, false, false, false},
    {"moedor", "moedor de café", false, false, false, false},
    {"cafeteira", "cafeteira expresso", true, false, false, false},
    {"prensa", "prensa de queijo", true, false, false, false},
    {"moenda", "moenda de cana", true, false, false, false},
    {"moinho", "moinho de trigo", false, false, false, false},
    {"vaca-leite", "vaca de leite", true, false, false, false},
    {"galinha", "galinha", true, false, false, false},
    {"porco", "porco", false, false, false, false},
    {"coelho", "coelho", false, false, false, false},
    {"porco-trufa", "porquinho trufeiro", false, false, false, false},
    {"forno", "forno a lenha", false, false, false, false},
    {"pecuarista", "seu Juca, o pecuarista", false, false, false, false},
};
static const TipoMovel& tipoDe(const std::string& t) {
    for (const auto& k : TIPOS) if (t == k.tipo) return k;
    return TIPOS[0];
}
static const char* PORTAS_ARMARIO[3] = {"Porta da esquerda", "Porta do meio", "Porta da direita"};

void Jogo::criarMoveis() {
    auto m = [&](const char* tipo, int mx, int my) { Movel v; v.tipo = tipo; v.x = mx; v.y = my; moveis.push_back(v); return (int)moveis.size() - 1; };
    moveis.clear();
    // sala
    m("sofa", 470, 150); m("poltrona", 462, 146); m("mesa", 470, 146);
    // cozinha, encostado na parede da sala
    m("pia", 420, 161); m("fogao", 430, 161);
    int g = m("geladeira", 440, 161);
    moveis[g].partes = {{{{"garrafa", 4}, 10}}};
    // quarto
    m("cama", 460, 305);
    int gr = m("guarda-roupa", 440, 306);
    moveis[gr].partes = {{}, {{{"camisa", 0}, 1}, {{"calca", 0}, 1}, {{"chapeu", 0}, 1}}, {}};
    // banheiro social e banheiro da suíte
    m("pia", 520, 161); m("vaso", 530, 161); m("chuveiro", 595, 199);
    m("pia", 412, 231); m("vaso", 418, 231); m("chuveiro", 403, 250);
    // gente: seu Tião espera na frente da casinha; dona Nezinha está lá dentro
    m("tiao", 370, 187); m("nezinha", 372, 200);
    // caixas: correio à direita da porta da frente (olhando de fora), remessa no gramado da porteira
    m("correio", 502, 108);
    int r = m("remessa", 505, 8);
    moveis[r].partes = {{}};
    m("trator", 506, 97);   // estacionado no terreiro, na frente da casa, do lado da estrada
    // cozinha: moedor de café, cafeteira expresso e prensa de queijo, na bancada depois da geladeira; paiol: moenda de cana e moinho de trigo
    m("moedor", 450, 161); m("cafeteira", 455, 161); m("prensa", 462, 161);
    m("moenda", 334, 345); m("moinho", 346, 345);
    m("forno", 504, 311);   // forno a lenha, logo que se sai pela porta dos fundos (500, 308), do lado direito
    ocupar();
    for (int k = 0; k < 5; k++) poeVaca("vaca");  // vacas no fim da lista
    for (int k = 0; k < 4; k++) poeVaca("vaca-leite");
    for (int k = 0; k < 5; k++) if (poeVaca("galinha")) moveis.back().carga = diaDoJogo() - k * 3;  // idades diferentes: a mais velha fica velha logo
}

void Jogo::ocupar() {
    for (int yy = 0; yy < LADO; yy++) for (int xx = 0; xx < LADO; xx++) mapa.em(xx, yy).movel = -1;
    for (size_t i = 0; i < moveis.size(); i++) mapa.em(moveis[i].x, moveis[i].y).movel = (int16_t)i;
}

int Jogo::movelEm(int mx, int my) const { return mapa.valido(mx, my) ? mapa.em(mx, my).movel : -1; }
int Jogo::movelFrente() const { return movelEm(x + DX[rumo], y + DY[rumo]); }

std::string Jogo::nomeMovel(int i, bool artigo) const {
    const TipoMovel& t = tipoDe(moveis[i].tipo);
    return (artigo ? std::string(t.fem ? "a " : "o ") : std::string()) + t.nome;
}

void Jogo::dizerHigiene() {
    const char* t = higiene >= 80 ? "Limpeza em dia, cheiro de sabonete." : higiene >= 55 ? "Um pouco de suor, mas ainda dá para encarar gente."
                  : higiene >= 30 ? "Sujeira acumulada: precisando de banho." : "Muita sujeira: dá para sentir o cheiro de longe. Banho, já.";
    fala::dizer(std::string(t) + " Limpeza: " + std::to_string((int)std::lround(higiene)) + " de 100.");
}

// Espaço com a mão selecionada vazia, de frente para um móvel.
void Jogo::interagir(int i) {
    Movel& m = moveis[i];
    const std::string& t = m.tipo;
    float mx = m.x + 0.5f, my = m.y + 0.5f;
    if (t == "geladeira" || t == "guarda-roupa") { abrirRecipiente(i); return; }
    if (t == "tiao" || t == "nezinha") { conversarCom(i); return; }
    if (t == "trator") { menuTrator(); return; }
    if (t == "balcao" || t == "ze") { menuZe(); return; }
    if (t == "prefeito") { menuPrefeito(); return; }
    if (ehBicho(t)) { carinhoVaca(i); return; }
    if (t == "pecuarista") { menuPecuarista(); return; }
    if (t == "forno") { fornoMaoVazia(i); return; }
    if (t == "cafeteira") { menuCafeteira(i); return; }
    if (t == "prensa") { prensaPronta(i); return; }
    if (t == "moedor") { fala::dizer("Moedor de café, de manivela, com funil em cima e gavetinha embaixo. Café torrado na mão, Espaço: mói 3 punhados num pacote de pó."); return; }
    if (t == "moenda") { fala::dizer("Moenda de cana, de ferro, com dois rolos e a roda de girar. Cana-de-açúcar na mão, Espaço: sai garapa."); return; }
    if (t == "moinho") { fala::dizer("Moinho de trigo, de pedra, com funil e manivela. Feixe de trigo na mão, Espaço: 2 feixes dão um saco de farinha."); return; }
    if (t == "mesa-festa") { menuMesaFesta(); return; }
    if (t == "mesa-bar") { fala::dizer("Uma mesa de bar, de lata, com marcas de copo e um cinzeiro."); return; }
    if (mundoAtual.rfind("visita:", 0) == 0 && !moradorDe(mundoAtual.substr(7))) { fala::dizer("Na casa dos outros, melhor não mexer."); return; }
    if (mundoAtual.rfind("visita:", 0) == 0 && (t == "geladeira" || t == "guarda-roupa" || t == "correio" || t == "remessa")) { fala::dizer("Isso fica com " + mundoAtual.substr(7) + ", por enquanto."); return; }
    if (t == "correio") { abrirCorreio(); return; }
    if (t == "remessa") { abrirRemessa(i); return; }
    if (t == "fogao") {
        if (!m.cozinhando.empty()) { tirarDoFogo(i); return; }
        if (!m.ligado && m.gas <= 0) { fala::dizer("Você gira o botão, mas não sai gás: o botijão acabou. Compre outro no bar do seu Zé, segure e aperte Espaço de frente para o fogão."); return; }
        m.ligado = !m.ligado;
        audio::tocar3D(m.ligado ? "fogao" : "fogao-desligar", mx, my, 0.9f, 0.9f, 2);
        if (m.ligado) { if (!m.som) m.som = audio::fonteMovel("chama", mx, my, 0.9f, 0.5f, 1.5f, true); }
        else if (m.som) { audio::pararFonte(m.som); m.som = 0; }
        fala::dizer(m.ligado ? "Você gira o botão e aperta: tec, tec, tec, e a chama azul acende com um sopro. Boca do fogão acesa." : "Você gira o botão e a chama apaga.");
        evento("fogao");
        return;
    }
    if (t == "pia") {
        audio::tocar3D("torneira", mx, my, 0.9f, 0.9f, 2);
        higiene = std::min(100.0, higiene + 3);
        fala::dizer("Você abre a torneira, lava as mãos e o rosto, e fecha. Para encher uma garrafa, segure a garrafa aberta e aperte Espaço de frente para a pia.");
        return;
    }
    if (t == "vaso") { audio::tocar3D("descarga", mx, my, 0.4f, 0.9f, 3); fala::dizer("Você aperta a descarga."); return; }
    if (t == "chuveiro") { chuveiro(); return; }
    if (t == "cama") { fala::dizer("Cama de casal, colchão de espuma. Para deitar e dormir, aperte Control R de frente para ela."); return; }
    if (t == "sofa" || t == "poltrona") { fala::dizer(maiuscula(nomeMovel(i, true)) + ". Para sentar, aperte R de frente para " + (t == "sofa" ? "ele." : "ela.")); return; }
    if (t == "mesa") { fala::dizer("Uma mesa de centro de madeira, baixinha, com marcas de copo."); return; }
}

// Control Espaço: segura o móvel da frente para arrastar; de novo, solta.
void Jogo::arrastar() {
    if (segurando >= 0) { fala::dizer("Você solta " + nomeMovel(segurando, true) + "."); segurando = -1; return; }
    int i = movelFrente();
    if (i < 0) { fala::dizer("Não tem móvel na sua frente."); return; }
    if (moveis[i].tipo == "tiao" || moveis[i].tipo == "nezinha" || moveis[i].tipo == "ze" || moveis[i].tipo == "prefeito") { fala::dizer("Arrastar gente não pode, né?"); return; }
    if (mundoAtual.rfind("visita:", 0) == 0) { fala::dizer("Na casa dos outros, melhor não mexer."); return; }
    if (!tipoDe(moveis[i].tipo).arrasta) { fala::dizer(maiuscula(nomeMovel(i, true)) + " é fixa no lugar. Não dá para arrastar."); return; }
    if (postura != Postura::EmPe) { fala::dizer("Para arrastar, levante primeiro: aperte R."); return; }
    segurando = i;
    fala::dizer("Você segura " + nomeMovel(i, true) + ". Ande para a frente para empurrar, para trás para puxar, ou para os lados. Control Espaço solta.");
    evento("segurar");
}

bool Jogo::moverMovel(int i, int nx, int ny) {
    if (!mapa.valido(nx, ny) || (nx == x && ny == y)) return false;
    const Celula& k = mapa.em(nx, ny);
    if (k.obs != Obstaculo::Nada || k.movel >= 0) return false;
    Movel& m = moveis[i];
    mapa.em(m.x, m.y).movel = -1;
    m.x = nx; m.y = ny;
    mapa.em(nx, ny).movel = (int16_t)i;
    if (m.som) audio::moverFonte(m.som, nx + 0.5f, ny + 0.5f, 0.9f);
    audio::tocar3D("arrastar", nx + 0.5f, ny + 0.5f, 0.3f, 0.8f, 2, (float)acaso(0.9, 1.1));
    return true;
}

// R ou Control R de frente para sofá, poltrona, vaso ou cama.
void Jogo::sentarEm(int i, Postura p) {
    const TipoMovel& t = tipoDe(moveis[i].tipo);
    std::string n = nomeMovel(i, true);
    andandoFrente = andandoTras = false; andandoLado = 0;
    if (p == Postura::Deitado) {
        if (!t.deita) { fala::dizer("Não dá para deitar n" + n + "."); return; }
        postura = Postura::Deitado; sentadoEm = i;
        audio::tocar3D("cama", moveis[i].x + 0.5f, moveis[i].y + 0.5f, 0.5f, 0.9f, 2);
        if (energia <= 85) { fala::dizer("Você se deita na cama. O colchão afunda macio."); dormir(); }
        else fala::dizer("Você se deita na cama, mas está sem sono. Para levantar, aperte R.");
        return;
    }
    postura = Postura::Sentado; sentadoEm = i;
    audio::tocar3D(moveis[i].tipo == "vaso" ? "parede" : "sofa", moveis[i].x + 0.5f, moveis[i].y + 0.5f, 0.5f, 0.6f, 2);
    fala::dizer("Você senta n" + n + ".");
    evento("sentar");
}

// Espaço com a mão vazia de frente para o chuveiro (de dentro do box): liga e desliga. Alt Espaço também.
void Jogo::chuveiro() {
    int i = movelFrente();
    if (i < 0 || moveis[i].tipo != "chuveiro") { fala::dizer("Não tem chuveiro na sua frente."); return; }
    Movel& m = moveis[i];
    m.ligado = !m.ligado;
    if (m.ligado) {
        if (!m.som) m.som = audio::fonteMovel("chuveiro", m.x + 0.5f, m.y + 0.5f, 2.0f, 0.9f, 2, true);
        fala::dizer("Você abre o registro. A água cai quente, com barulho de chuva forte. Fique embaixo dela para tomar banho.");
    } else {
        if (m.som) { audio::pararFonte(m.som); m.som = 0; }
        fala::dizer("Você fecha o registro. A água para.");
    }
    tomandoBanho = 0;
}

// Geladeira e guarda-roupa abrem um menu com o que tem dentro.
static bool vaiNaGeladeira(const std::string& t) {
    static const char* OK[] = {"barra", "barra-aberta", "garrafa", "garrafa-aberta", "leite", "comida", "cerveja", "bolo", "brigadeiro", "beijinho", "camafeu",
                               "empada", "torta", "risole", "coxinha", "refrigerante", "vinho", "uisque", "salada", "maionese", "laranja", "banana", "cenoura", "abobora", "maca", "cereja", "manga", "carne", "carne-assada", "queijo", "garapa", "expresso", "cappuccino", "ovo", "ovo-frito", "ovo-quebrado", "meio-queijo", "omelete", "bolo-chocolate", "cacau-po", "trufa",
                               "cebola", "alho", "cebolinha", "salsinha", "coentro", "alface", "couve", "tomate", "pimentao", "pimenta", "batata",
                               "beterraba", "mandioca", "quiabo", "morango", "trigo", "cana", "cafe", "leite"};
    for (const char* o : OK) if (t == o) return true;
    return false;
}

void Jogo::abrirRecipiente(int i) {
    Movel& m = moveis[i];
    m.aberto = true;
    bool gel = m.tipo == "geladeira";
    audio::tocar3D(gel ? "geladeira-porta" : "porta-abrir", m.x + 0.5f, m.y + 0.5f, 1.0f, 0.8f, 2);
    auto listar = [this, i](int parte, const std::string& titulo, bool filtroGeladeira) {
        return [this, i, parte, titulo, filtroGeladeira]() {
            std::vector<Opcao> v;
            auto& lista = moveis[i].partes[parte];
            for (size_t k = 0; k < lista.size(); k++) {
                const Pilha& p = lista[k];
                Coisa c = p.coisa;
                v.push_back({maiuscula(plural(c, p.qtd, nomeCoisa(c))) + (p.qtd > 1 ? ": " + std::to_string(p.qtd) : ""),
                    [this, i, parte, c]() {  // Enter: pega uma
                        auto& l = moveis[i].partes[parte];
                        for (size_t j = 0; j < l.size(); j++) if (l[j].coisa == c) { if (--l[j].qtd <= 0) l.erase(l.begin() + (long)j); break; }
                        guardar(c); audio::tocar("mochila", 0.5f);
                        fala::dizer("Você pega " + nomeCoisa(c, true) + ". Está na mochila.");
                        evento("pegou");
                    },
                    [this, i, parte, c]() {  // Shift Enter: pega todas
                        auto& l = moveis[i].partes[parte];
                        for (size_t j = 0; j < l.size(); j++) if (l[j].coisa == c) { int n = l[j].qtd; guardar(c, n); l.erase(l.begin() + (long)j);
                            audio::tocar("mochila", 0.5f); fala::dizer("Você pega " + std::to_string(n) + ". Estão na mochila."); break; }
                    }, nullptr});
            }
            v.push_back({filtroGeladeira ? "Guardar comida" : "Guardar aqui", [this, i, parte, titulo, filtroGeladeira]() {
                Menu g;
                g.titulo = filtroGeladeira ? "Guardar na geladeira" : "Guardar: " + titulo;
                g.opcoes = [this, i, parte, filtroGeladeira]() {
                    std::vector<Opcao> v2;
                    for (const Pilha& p : mochila) {
                        if (filtroGeladeira && !vaiNaGeladeira(p.coisa.tipo)) continue;
                        Coisa c = p.coisa;
                        auto por = [this, i, parte](const Coisa& c, int n) {
                            for (int k = 0; k < n; k++) tirar(c);
                            auto& l = moveis[i].partes[parte];
                            bool junto = false;
                            for (Pilha& q : l) if (q.coisa == c) { q.qtd += n; junto = true; }
                            if (!junto) l.push_back({c, n});
                        };
                        v2.push_back({maiuscula(plural(c, p.qtd, nomeCoisa(c))) + (p.qtd > 1 ? ": " + std::to_string(p.qtd) : ""),
                            [this, c, por]() { por(c, 1); audio::tocar("mochila", 0.4f); fala::dizer("Você guarda " + nomeCoisa(c, true) + "."); evento("guardou"); },
                            [this, c, por]() { int n = 0; for (const Pilha& q : mochila) if (q.coisa == c) n = q.qtd; por(c, n); audio::tocar("mochila", 0.4f);
                                fala::dizer("Você guarda " + std::to_string(n) + "."); evento("guardou"); }, nullptr});
                    }
                    if (v2.empty()) v2.push_back({filtroGeladeira ? "Não tem nada na mochila que vá para a geladeira" : "A mochila está vazia", [] {}, nullptr, nullptr});
                    return v2;
                };
                abrirMenu(g);
            }, nullptr, nullptr});
            return v;
        };
    };
    Menu menu;
    if (gel) {
        menu.titulo = "Geladeira aberta";
        menu.opcoes = listar(0, "Geladeira", true);
    } else {
        menu.titulo = "Guarda-roupa aberto";
        menu.opcoes = [this, i, listar]() {
            std::vector<Opcao> v;
            for (int p = 0; p < 3; p++) {
                int n = 0; for (const Pilha& q : moveis[i].partes[p]) n += q.qtd;
                v.push_back({std::string(PORTAS_ARMARIO[p]) + (n ? ": " + std::to_string(n) + (n == 1 ? " coisa" : " coisas") : ": vazia"), [this, i, p, listar]() {
                    audio::tocar3D("porta-abrir", moveis[i].x + 0.5f, moveis[i].y + 0.5f, 1.0f, 0.5f, 2, 1.3f);
                    Menu sub; sub.titulo = PORTAS_ARMARIO[p]; sub.opcoes = listar(p, PORTAS_ARMARIO[p], false);
                    abrirMenu(sub);
                }, nullptr, nullptr});
            }
            return v;
        };
    }
    menu.aoFechar = [this, i, gel]() {
        moveis[i].aberto = false;
        audio::tocar3D(gel ? "geladeira-porta" : "porta-fechar", moveis[i].x + 0.5f, moveis[i].y + 0.5f, 1.0f, 0.7f, 2, 1.1f);
        fala::dizer(gel ? "Você fecha a geladeira." : "Você fecha o guarda-roupa.", false);
    };
    abrirMenu(menu);
    evento(gel ? "geladeira" : "guarda-roupa");
}

// Motor da geladeira ligando e desligando; banho embaixo do chuveiro.
void Jogo::atualizarMoveis(double dt) {
    for (Movel& m : moveis) {
        if (m.tipo == "geladeira") {
            if (!m.som) { m.som = audio::fonteMovel("geladeira", m.x + 0.5f, m.y + 0.5f, 0.8f, 0.0f, 1.2f, true); m.ciclo = 0; }
            m.ciclo += dt;
            if (m.ciclo > 110) m.ciclo = 0;
            audio::lacoVolume(m.som, m.ciclo < 40 ? 0.45f : 0.0f);  // 40 segundos ligado, 70 desligado
        }
        if (m.tipo == "fogao") atualizarFogo(m, dt);
        if (m.tipo == "forno") atualizarForno(m, dt);
        if (m.tipo == "fogao" && m.ligado) {  // a chama gasta o botijão
            m.gas -= dt;
            if (m.gas <= 0) {
                m.gas = 0; m.ligado = false;
                if (m.som) { audio::pararFonte(m.som); m.som = 0; }
                avisar("A chama do fogão vai diminuindo, azulzinha, e apaga: acabou o gás do botijão.");
            }
        }
        if (m.tipo == "chuveiro" && m.ligado && x == m.x && y == m.y - 1) {
            higiene = std::min(100.0, higiene + 8 * dt);
            if (tomandoBanho >= 0 && higiene >= 100) {  // avisa uma vez por banho
                tomandoBanho = -1;
                fala::dizer("Pronto: banho tomado, da cabeça aos pés. Cheiro de sabonete. Pode desligar o chuveiro com Espaço, de mão vazia.");
                evento("banho");
            }
        }
    }
}
