// Árvores: radar de galhos (Alt K e Alt L de frente para uma árvore do pomar), Alt O mira a fruta,
// Espaço de mão vazia colhe. Cada árvore do pomar dá de 1 a 4 frutas por dia.
// Machado derruba árvore (as do pomar não); picareta tira pedra da roça.
#include <algorithm>
#include <cmath>
#include "audio.h"
#include "fala.h"
#include "jogo.h"
#include "util.h"

static const char* POSICAO[6] = {"baixo, à esquerda", "baixo, à direita", "do meio, à esquerda", "do meio, à direita", "alto, à esquerda", "alto, à direita"};

static std::string frutaDe(const std::string& arvore) {
    if (arvore == "macieira") return "maca";
    if (arvore == "laranjeira") return "laranja";
    if (arvore == "cerejeira") return "cereja";
    if (arvore == "bananeira") return "banana";
    return "manga";
}
static std::string parteDe(const std::string& arvore) { return arvore == "bananeira" ? "cacho" : "galho"; }

int Jogo::arvoreDaFrente() const {
    int fx = x + DX[rumo], fy = y + DY[rumo];
    for (size_t i = 0; i < mapa.objetos.size(); i++)
        if (mapa.objetos[i].x == fx && mapa.objetos[i].y == fy) return (int)i;
    return -1;
}

// Frutas do dia: na virada do dia a árvore enche de novo, de 1 a 4, em galhos sorteados.
uint8_t& Jogo::frutasDa(int i) {
    if ((int)pomar.size() < (int)mapa.objetos.size()) pomar.resize(mapa.objetos.size());
    EstadoArvore& e = pomar[i];
    if (e.dia != diaDoJogo()) {
        e.dia = diaDoJogo();
        e.frutas = 0;
        int n = 1 + (int)acaso(0, 3.99);
        while (__builtin_popcount(e.frutas) < n) e.frutas |= (uint8_t)(1 << (int)acaso(0, 5.99));
    }
    return e.frutas;
}

bool Jogo::radarValido(int i) const {
    return i >= 0 && radarArvore == i && radarX == x && radarY == y && radarRumo == rumo;
}

void Jogo::radarArvoreTecla(int lado) {
    int i = arvoreDaFrente();
    if (i < 0) { fala::dizer("O radar de árvore funciona de frente para uma árvore do pomar. Ache uma no Ç, em Objetos, e chegue com Alt O."); return; }
    if (mundoAtual != "fazenda" && !souMorador()) { fala::dizer("Essa árvore é de outra pessoa."); return; }
    if (!radarValido(i)) { radarArvore = i; radarX = x; radarY = y; radarRumo = rumo; radarItem = lado > 0 ? -1 : 0; frutaMirada = -1; }
    radarItem = (radarItem + lado + 7) % 7;  // 0 tronco, 1 a 6 galhos
    const Objeto& o = mapa.objetos[i];
    uint8_t f = frutasDa(i);
    int dir = (rumo + 2) % 8;  // para a direita de quem olha
    if (radarItem == 0) {
        audio::tocar3D("bip", o.x + 0.5f, o.y + 0.5f, 0.8f, 0.5f, 6, 0.7f);
        fala::dizer("Tronco da " + o.tipo + ". " + std::to_string(__builtin_popcount(f)) + (__builtin_popcount(f) == 1 ? " fruta" : " frutas") + " hoje.");
        return;
    }
    int g = radarItem - 1;
    bool tem = f & (1 << g);
    float lado3d = g % 2 == 0 ? -0.6f : 0.6f, altura = 1.2f + 0.9f * (g / 2);
    audio::tocar3D(tem ? "gota" : "bip", o.x + 0.5f + DX[dir] * lado3d, o.y + 0.5f + DY[dir] * lado3d, altura, 0.6f, 6, tem ? 1.2f : 0.9f);
    std::string fr = frutaDe(o.tipo);
    fala::dizer(maiuscula(parteDe(o.tipo)) + " " + POSICAO[g] + (tem ? ": com uma " + nomeCoisa({fr, 0}) + ". Alt O mira a fruta." : ": sem fruta."));
}

bool Jogo::mirarFruta() {
    int i = arvoreDaFrente();
    if (!radarValido(i) || radarItem < 0) return false;
    const Objeto& o = mapa.objetos[i];
    if (radarItem == 0) { fala::dizer("Esse é o tronco. Com Alt K e Alt L, escolha um " + parteDe(o.tipo) + " com fruta."); return true; }
    int g = radarItem - 1;
    if (!(frutasDa(i) & (1 << g))) { fala::dizer("Esse " + parteDe(o.tipo) + " não tem fruta."); return true; }
    frutaMirada = g;
    audio::tocar("chegou", 0.6f);
    fala::dizer("Você vira de frente para o " + parteDe(o.tipo) + " " + POSICAO[g] + ", e estica o braço: " + nomeCoisa({frutaDe(o.tipo), 0}, true) +
                " está ao alcance. Espaço, de mão vazia, pega.");
    return true;
}

// Espaço de mão vazia de frente para uma árvore do pomar: colhe a fruta mirada, ou descreve.
void Jogo::maoNaArvore(int i) {
    const Objeto& o = mapa.objetos[i];
    if (radarValido(i) && frutaMirada >= 0 && (frutasDa(i) & (1 << frutaMirada))) {
        if (mundoAtual != "fazenda" && !souMorador()) { fala::dizer("Essa árvore é de outra pessoa."); return; }
        frutasDa(i) &= (uint8_t)~(1 << frutaMirada);
        std::string fr = frutaDe(o.tipo);
        guardar({fr, 0});
        audio::tocar("colher", 0.8f);
        int resta = __builtin_popcount(frutasDa(i));
        fala::dizer("Você puxa " + nomeCoisa({fr, 0}, true) + " do " + parteDe(o.tipo) + ", que balança e volta. Na mochila. " +
                    (resta ? "Ainda " + std::string(resta == 1 ? "tem 1 fruta" : "tem " + std::to_string(resta) + " frutas") + " na árvore." : "A árvore ficou sem fruta: amanhã dá de novo."));
        frutaMirada = -1;
        return;
    }
    olharArvore(o);
    int n = (mundoAtual == "fazenda" || souMorador()) ? __builtin_popcount(frutasDa(i)) : 0;
    if (n) fala::dizer("Tem " + std::to_string(n) + (n == 1 ? " fruta" : " frutas") + " hoje. Alt K e Alt L procuram nos galhos; Alt O mira a fruta; Espaço pega.", false);
}

// Machado numa árvore: 5 golpes derrubam. As do pomar ficam.
void Jogo::cortarArvore(int fx, int fy) {
    if (mapa.objetoEm(fx, fy)) { fala::dizer("Essa é árvore de fruta do pomar. Melhor não cortar: ela dá fruta todo dia."); return; }
    int cel = fy * LADO + fx;
    int& g = golpes[cel];
    g++;
    energia -= 1.0; sede -= 0.3; higiene -= 0.3;
    audio::tocar3D("machado", fx + 0.5f, fy + 0.5f, 1.0f, 1.0f, 3, (float)acaso(0.85, 0.95));
    if (g < 5) { fala::dizer(g == 1 ? "Você crava o machado no tronco: lasca voa. Mais uns golpes e ela cai." : (5 - g == 1 ? std::string("Mais um golpe. Falta 1.") : "Mais um golpe. Faltam " + std::to_string(5 - g) + ".")); return; }
    golpes.erase(cel);
    mapa.em(fx, fy).obs = Obstaculo::Nada;
    cortadas.push_back(cel);
    acaoNaFazenda("cortada " + std::to_string(cel));
    guardar({"lenha", 0}, 4);
    audio::tocar3D("arvore", fx + 0.5f, fy + 0.5f, 2.0f, 1.0f, 30);
    fala::dizer("O tronco estala, range, e a árvore cai com um estrondo, sacudindo o chão. Você corta em pedaços: 4 toras de lenha, na mochila.");
    evento("cortou");
}

// Picareta na pedra da roça: arranca, e a terra fica limpa.
void Jogo::usarPicareta() {
    int fx = x + DX[rumo], fy = y + DY[rumo];
    if (!mapa.valido(fx, fy)) { fala::dizer("Não tem nada na sua frente."); return; }
    Celula& k = mapa.em(fx, fy);
    if (k.chao == Chao::Mato) { fala::dizer("Isso é matagal: o machado resolve. A picareta é para as pedras."); return; }
    if (k.chao != Chao::Pedras) { fala::dizer("Não tem pedra na sua frente para arrancar."); return; }
    int cel = fy * LADO + fx;
    k.chao = Chao::TerraArada;
    limpos.push_back(cel);
    acaoNaFazenda("limpo " + std::to_string(cel));
    energia -= 2.0; sede -= 0.6; higiene -= 0.6;
    guardar({"pedra", 0});
    audio::tocar3D("picareta", fx + 0.5f, fy + 0.5f, 0.3f, 1.0f, 3, (float)acaso(0.92, 1.08));
    fala::dizer("Você finca a picareta, alavanca e a pedra sai do chão. Terra limpa, pronta para plantar. A pedra vai para a mochila: o prefeito compra.");
    evento("limpou");
    if (categoria == 6) { montarAlvos(); itemAlvo = alvos.empty() ? 0 : (int)alvos.size() - 1; }
}
