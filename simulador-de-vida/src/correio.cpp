// Caixa de correio (cartas dos NPCs), caixa de remessa (o que se vende) e a escolha de homem ou mulher.
// Toda noite, às 22 horas, seu Manel do Bacanal, o prefeito, recolhe a remessa e paga na carteira,
// esteja a pessoa acordada ou dormindo.
#include <algorithm>
#include <cmath>
#include "audio.h"
#include "fala.h"
#include "jogo.h"
#include "util.h"

// o que o prefeito compra, e por quanto (moedas por unidade)
static int preco(const std::string& t) {
    if (t == "milho") return 4;
    if (t == "feijao") return 3;
    if (t == "arroz") return 5;
    if (t == "pedra") return 1;
    if (t == "adubo") return 2;
    if (t == "leite") return 6;
    return 0;
}

void Jogo::escolherGenero(bool depoisAbertura) {
    Menu m;
    m.titulo = "Simulador de Vida. Antes de começar: seu personagem é homem ou mulher? Setas para escolher, Enter para confirmar";
    m.pausa = true;
    m.obrigatorio = true;
    m.opcoes = [this, depoisAbertura]() {
        auto escolher = [this, depoisAbertura](char g) {
            genero = g;
            menus.clear();
            audio::tocar("menu-fechar", 0.5f);
            if (depoisAbertura) abertura(true);
            else fala::dizer(std::string("Pronto: ") + (g == 'm' ? "mulher." : "homem.") + " O jogo continua.");
        };
        return std::vector<Opcao>{
            {"Homem", [escolher]() { escolher('h'); }, nullptr, nullptr},
            {"Mulher", [escolher]() { escolher('m'); }, nullptr, nullptr},
        };
    };
    abrirMenu(m);
}

void Jogo::avisar(const std::string& texto) {
    if (dormindo) aoAcordar.push_back(texto);
    else fala::dizer(texto, false);
}

// ---------- cartas ----------
std::string Jogo::remetente(const Carta& c) const {
    if (c.id == "manel1") return "Manel do Bacanal, prefeito";
    return "alguém";
}

std::string Jogo::textoCarta(const Carta& c) const {
    if (c.id == "manel1")
        return "Prezad" + gen("o", "a") + " fazendeir" + gen("o", "a") + ", seja bem-vind" + gen("o", "a") + " à nossa cidade! "
               "Aqui quem escreve é o Manel do Bacanal, prefeito, às suas ordens. Fiquei sabendo que " + gen("o filho", "a filha") + " do Alceu assumiu a fazenda, "
               "e isso é uma alegria para todo mundo. A prefeitura fica a leste da sede: é só seguir a estrada para a cidade, pela porteira leste. "
               "E tem mais: toda noite, lá pelas dez horas, eu passo na caixa de remessa, aquela perto da porteira principal, e compro tudo o que você deixar lá. "
               "Milho, feijão, arroz, e mais para frente pedra, adubo, leite, o que a terra der. O dinheiro vai direto para a sua carteira, na mesma noite. "
               "Um abraço do seu amigo, Manel do Bacanal.";
    return "";
}

void Jogo::chegouCarta(const std::string& id) {
    if (std::any_of(cartas.begin(), cartas.end(), [&](const Carta& c) { return c.id == id; })) return;
    cartas.push_back({id, false});
    int i = idx("correio");
    if (i >= 0 && mundoAtual == "fazenda") audio::tocar3D("porta-fechar", moveis[i].x + 0.5f, moveis[i].y + 0.5f, 1.0f, 0.4f, 3, 1.6f);
    avisar("Chegou carta nova na caixa de correio, ao lado direito da porta da casa sede.");
}

void Jogo::abrirCorreio() {
    int i = idx("correio");
    if (i >= 0) audio::tocar3D("porta-abrir", moveis[i].x + 0.5f, moveis[i].y + 0.5f, 1.0f, 0.5f, 2, 1.6f);
    Menu m;
    m.titulo = "Caixa de correio";
    m.opcoes = [this]() {
        std::vector<Opcao> v;
        for (size_t k = 0; k < cartas.size(); k++)
            v.push_back({"Carta de " + remetente(cartas[k]) + (cartas[k].lida ? ", lida" : ", nova"), [this, k]() {
                cartas[k].lida = true;
                audio::tocar("embalagem", 0.4f);
                fala::dizer(textoCarta(cartas[k]));
                evento("leu");
            }, nullptr, nullptr});
        if (v.empty()) v.push_back({"Nenhuma carta", [] {}, nullptr, nullptr});
        return v;
    };
    abrirMenu(m);
}

// ---------- remessa ----------
void Jogo::abrirRemessa(int i) {
    audio::tocar3D("porta-abrir", moveis[i].x + 0.5f, moveis[i].y + 0.5f, 0.8f, 0.5f, 2, 1.3f);
    Menu m;
    m.titulo = "Caixa de remessa. Seu Manel do Bacanal recolhe toda noite, às 10, e paga na hora";
    m.opcoes = [this, i]() {
        std::vector<Opcao> v;
        int total = 0;
        for (const Pilha& p : moveis[i].partes[0]) total += p.qtd * preco(p.coisa.tipo);
        for (size_t k = 0; k < moveis[i].partes[0].size(); k++) {
            const Pilha& p = moveis[i].partes[0][k];
            Coisa c = p.coisa;
            v.push_back({maiuscula(plural(c, p.qtd, nomeCoisa(c))) + ": " + std::to_string(p.qtd) + ", vale " + std::to_string(p.qtd * preco(c.tipo)) + " moedas",
                [this, i, c]() {  // Enter: tira uma de volta
                    auto& l = moveis[i].partes[0];
                    for (size_t j = 0; j < l.size(); j++) if (l[j].coisa == c) { if (--l[j].qtd <= 0) l.erase(l.begin() + (long)j); break; }
                    guardar(c); audio::tocar("mochila", 0.4f);
                    fala::dizer("Você tira " + nomeCoisa(c, true) + " da caixa e põe na mochila.");
                }, nullptr, nullptr});
        }
        v.push_back({"Colocar na caixa" + std::string(total ? ". Já tem " + std::to_string(total) + " moedas em mercadoria" : ""), [this, i]() {
            Menu g;
            g.titulo = "Colocar na caixa de remessa";
            g.opcoes = [this, i]() {
                std::vector<Opcao> v2;
                for (const Pilha& p : mochila) {
                    if (!preco(p.coisa.tipo)) continue;
                    Coisa c = p.coisa;
                    auto por = [this, i](const Coisa& c, int n) {
                        for (int k = 0; k < n; k++) tirar(c);
                        auto& l = moveis[i].partes[0];
                        bool junto = false;
                        for (Pilha& q : l) if (q.coisa == c) { q.qtd += n; junto = true; }
                        if (!junto) l.push_back({c, n});
                        audio::tocar("mochila", 0.4f);
                        fala::dizer(std::to_string(n) + (n == 1 ? " posto" : " postos") + " na caixa. Seu Manel paga " + std::to_string(n * preco(c.tipo)) + " moedas à noite.");
                        evento("remessa");
                    };
                    v2.push_back({maiuscula(plural(c, p.qtd, nomeCoisa(c))) + ": " + std::to_string(p.qtd) + ", a " + std::to_string(preco(c.tipo)) + " moedas cada",
                        [por, c]() { por(c, 1); }, [this, por, c]() { int n = 0; for (const Pilha& q : mochila) if (q.coisa == c) n = q.qtd; por(c, n); }, nullptr});
                }
                if (v2.empty()) v2.push_back({"Nada na mochila que o prefeito compre. Ele compra milho, feijão e arroz", [] {}, nullptr, nullptr});
                return v2;
            };
            abrirMenu(g);
        }, nullptr, nullptr});
        return v;
    };
    abrirMenu(m);
}

// Às 22 horas de todo dia: o prefeito recolhe tudo e paga na carteira.
void Jogo::coletarRemessa() {
    auto& mv = meusMoveis();  // a remessa é da sua fazenda, mesmo que você esteja na cidade
    int i = -1;
    for (size_t k = 0; k < mv.size(); k++) if (mv[k].tipo == "remessa") i = (int)k;
    if (i < 0) return;
    auto& l = mv[i].partes[0];
    int total = 0;
    for (const Pilha& p : l) total += p.qtd * preco(p.coisa.tipo);
    if (!total) return;
    l.clear();
    Coisa* naMao = maoDireita.tipo == "carteira" ? &maoDireita : maoEsquerda.tipo == "carteira" ? &maoEsquerda : nullptr;
    Pilha* naMochila = nullptr;
    for (Pilha& p : mochila) if (p.coisa.tipo == "carteira") naMochila = &p;
    if (naMao) naMao->carga += total; else if (naMochila) naMochila->coisa.carga += total; else guardar({"carteira", total});
    if (mundoAtual == "fazenda") audio::tocar3D("carro", mv[i].x + 2.5f, mv[i].y - 4.0f, 0.6f, 0.5f, 8);
    avisar("Lá pela porteira, um carro para e a caixa de remessa range: é seu Manel do Bacanal, recolhendo. Ele levou tudo e pagou " +
           std::to_string(total) + " moedas, que já estão na sua carteira.");
    evento("coleta");
}
