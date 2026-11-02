// Mundos: a sua fazenda, a cidade (bar do seu Zé, prefeitura) e as fazendas dos outros jogadores.
// Viaja-se de trator. Enquanto você está fora, a sua fazenda fica guardada em fazendaGuardada e continua
// viva: as plantas crescem e o prefeito recolhe a remessa.
#include <algorithm>
#include <sstream>
#include "audio.h"
#include "fala.h"
#include "jogo.h"
#include "rede.h"
#include "util.h"

// onde se desce do trator, de frente para ele (para subir de novo é só apertar Espaço)
static const int FAZENDA_CHEGADA[3] = {505, 97, 2}, CIDADE_CHEGADA[3] = {501, 30, 2};

void Jogo::trocarComGuardada() {
    std::swap(mapa, fazendaGuardada->mapa);
    std::swap(moveis, fazendaGuardada->moveis);
    std::swap(plantas, fazendaGuardada->plantas);
    std::swap(limpos, fazendaGuardada->limpos);
    std::swap(x, fazendaGuardada->x); std::swap(y, fazendaGuardada->y); std::swap(rumo, fazendaGuardada->rumo);
}

void Jogo::pararSonsDosMoveis() {
    for (Movel& m : moveis) if (m.som) { audio::pararFonte(m.som); m.som = 0; m.ligado = m.tipo == "geladeira" ? m.ligado : false; }
    if (somBar) { audio::pararFonte(somBar); somBar = 0; }
}

void Jogo::criarMoveisCidade() {
    moveis.clear();
    auto m = [&](const char* tipo, int mx, int my) { Movel v; v.tipo = tipo; v.x = mx; v.y = my; moveis.push_back(v); };
    m("trator", 502, 30);
    m("balcao", 455, 150); m("ze", 455, 152);
    m("mesa-bar", 445, 140); m("mesa-bar", 465, 140); m("mesa-bar", 455, 136);
    m("prefeito", 550, 155); m("mesa", 550, 153);
    m("mesa-festa", 455, 195);
    ocupar();
}

int* Jogo::carteira() {
    if (maoDireita.tipo == "carteira") return &maoDireita.carga;
    if (maoEsquerda.tipo == "carteira") return &maoEsquerda.carga;
    for (Pilha& p : mochila) if (p.coisa.tipo == "carteira") return &p.coisa.carga;
    return nullptr;
}

// Sobe no trator e vai. A viagem leva uns segundos, com o motor roncando.
void Jogo::viajar(const std::string& destino) {
    if (destino == mundoAtual) { fala::dizer("Você já está aí."); return; }
    while (!menus.empty()) fecharMenu();
    viajando = true;
    andandoFrente = andandoTras = false; andandoLado = 0; caminho.clear(); rastreando = false; segurando = -1;
    postura = Postura::EmPe; sentadoEm = -1;
    audio::tocar("porta-fechar", 0.6f);
    if (!somViagem) somViagem = audio::laco("trator", false, 0.7f);
    std::string para = destino == "cidade" ? "a cidade" : destino == "fazenda" ? "a sua fazenda" : "a fazenda de " + destino.substr(7);
    fala::dizer("Você sobe no trator, dá a partida, e o motor engasga, tosse e pega. Lá vamos nós para " + para + ", pela estrada de terra.");
    marcar(7.0, [this, destino]() {
        if (somViagem) { audio::pararFonte(somViagem); somViagem = 0; }
        chegarEm(destino);
        viajando = false;
    });
}

void Jogo::chegarEm(const std::string& destino) {
    pararSonsDosMoveis();
    // sai do lugar atual: se era a fazenda, guarda
    if (mundoAtual == "fazenda") { fazendaGuardada = std::make_unique<Mundo>(); trocarComGuardada(); }
    mundoAtual = destino;
    categoria = -1; alvos.clear();
    if (destino == "fazenda") {
        trocarComGuardada();
        fazendaGuardada.reset();
        x = FAZENDA_CHEGADA[0]; y = FAZENDA_CHEGADA[1]; rumo = FAZENDA_CHEGADA[2];
        lugarAtual = mapa.nomeLugar(x, y);
        audio::tocar("porta-fechar", 0.6f);
        if (!chegadaCalada) fala::dizer("O trator para na frente da casa sede, você desce e o motor morre. De volta à sua fazenda.");
    } else if (destino == "cidade") {
        mapa = Mapa(true);
        plantas.clear(); limpos.clear();
        criarMoveisCidade();
        x = CIDADE_CHEGADA[0]; y = CIDADE_CHEGADA[1]; rumo = CIDADE_CHEGADA[2];
        lugarAtual = mapa.nomeLugar(x, y);
        bool festa = evento_.tipo == "festa" && agora < evento_.fim;
        somBar = audio::fonteMovel(festa ? "festa" : "bar", 455.5f, festa ? 192.5f : 145.5f, 1.0f, 0.6f, 4, true);
        if (!chegadaCalada) fala::dizer("O trator para no estacionamento da cidade. Você desce. Daqui se ouve o movimento do bar do seu Zé, a oeste da praça; a prefeitura fica a leste. "
                    "Ç até Lugares acha os dois. Para voltar, Espaço no trator, que fica bem na sua frente.");
    } else {  // fazenda de outro jogador: o mesmo desenho, com o estado que o dono mandar
        std::string dono = destino.substr(7);
        mapa = Mapa();
        plantas.clear(); limpos.clear();
        criarMoveis();
        x = FAZENDA_CHEGADA[0]; y = FAZENDA_CHEGADA[1]; rumo = FAZENDA_CHEGADA[2];
        lugarAtual = mapa.nomeLugar(x, y);
        rede::pedirFazenda(dono);
        if (chegadaCalada) {} else if (moradorDe(dono)) fala::dizer("O trator para na frente da casa sede. Em casa: a fazenda de " + dono + " e " + rede::meuNome() + ". Aqui você pode limpar, plantar, regar, colher e abrir portas.");
        else fala::dizer("O trator para na frente da casa sede da fazenda de " + dono + ". Você desce. Na casa dos outros, dá para andar, olhar e conversar, mas não mexer nas coisas.");
    }
    rede::mudouDeMundo(mundoCanonico());
    evento("viajou");
}

void Jogo::menuTrator() {
    Menu m;
    m.titulo = "Trator, para onde ir";
    m.opcoes = [this]() {
        std::vector<Opcao> v;
        if (souMorador()) {  // casou e se mudou: a casa é a fazenda do casal
            if (mundoAtual != "visita:" + moraCom) v.push_back({"Ir para casa, na fazenda de " + moraCom + " e " + rede::meuNome(), [this]() {
                auto o = rede::outrosJogadores();
                if (std::find(o.begin(), o.end(), moraCom) == o.end()) { fala::dizer("A fazenda de vocês só abre quando " + moraCom + " estiver online."); return; }
                viajar("visita:" + moraCom);
            }, nullptr, nullptr});
        } else if (mundoAtual != "fazenda") v.push_back({"Voltar para a minha fazenda", [this]() { viajar("fazenda"); }, nullptr, nullptr});
        if (mundoAtual != "cidade") v.push_back({"Ir para a cidade, no bar do seu Zé", [this]() { viajar("cidade"); }, nullptr, nullptr});
        for (const std::string& n : rede::outrosJogadores())
            if (mundoAtual != "visita:" + n && !(souMorador() && n == moraCom)) v.push_back({"Ir para a fazenda de " + n, [this, n]() { viajar("visita:" + n); }, nullptr, nullptr});
        return v;
    };
    abrirMenu(m);
}

// ---------- bar do seu Zé ----------
struct Produto { const char* nome; Coisa coisa; int preco; int qtd; };
static const Produto PRODUTOS[] = {
    {"Pacote de pó de café, meio quilo", {"po-cafe", 0}, 12, 1},
    {"Garrafa de água, cheia", {"garrafa", 4}, 2, 1},
    {"Garrafa vazia, para guardar leite", {"garrafa", 0}, 1, 1},
    {"Saco de açúcar, um quilo", {"acucar", 0}, 8, 1},
    {"Saco de farinha de trigo, um quilo", {"farinha", 0}, 7, 1},
    {"Botijão de gás, para o fogão", {"gas", 0}, 60, 1},
    {"Garrafa de cerveja, bem gelada", {"cerveja", 0}, 6, 1},
    {"Maço de cigarro, 20 cigarros", {"cigarro", 20}, 10, 1},
    {"Isqueiro", {"isqueiro", 30}, 5, 1},
    {"Latinha de gás para isqueiro, dá 5 cargas", {"gas-isqueiro", 5}, 8, 1},
    {"Sementes de milho, pacote com 10", {"semente-milho", 0}, 4, 10},
    {"Sementes de feijão de corda, pacote com 10", {"semente-feijao", 0}, 4, 10},
    {"Sementes de arroz, pacote com 10", {"semente-arroz", 0}, 4, 10},
    {"Sementes de laranja, pacote com 10", {"semente-laranja", 0}, 12, 10},
    {"Sementes de banana, pacote com 10", {"semente-banana", 0}, 12, 10},
    {"Sementes de abóbora, pacote com 10", {"semente-abobora", 0}, 8, 10},
    {"Sementes de cenoura, pacote com 10", {"semente-cenoura", 0}, 6, 10},
    {"Sementes de cacau, pacote com 10, para fazer chocolate", {"semente-cacau", 0}, 20, 10},
};

void Jogo::menuZe() {
    Menu m;
    m.titulo = "Seu Zé, do outro lado do balcão";
    m.opcoes = [this]() {
        std::vector<Opcao> v;
        v.push_back({"Conversar", [this]() {
            static const char* L[] = {
                "Chega mais, FREGUES! O que vai ser hoje?",
                "Aqui no bar todo mundo se encontra no fim da tarde. É só chegar e puxar uma cadeira.",
                "O Manel do Bacanal passa aqui toda manhã pra tomar um cafezinho e contar vantagem.",
                "Gás tá caro, viu? Mas pra quem tem roça, compensa.",
            };
            std::string l = L[(int)acaso(0, 3.99)];
            auto p = l.find("FREGUES"); if (p != std::string::npos) l.replace(p, 7, gen("freguês", "freguesa"));
            fala::dizer("Seu Zé: " + l);
        }, nullptr, nullptr});
        v.push_back({"Comprar", [this]() {
            Menu c; c.titulo = "O que o seu Zé vende";
            c.opcoes = [this]() {
                std::vector<Opcao> v2;
                for (const Produto& p : PRODUTOS)
                    v2.push_back({std::string(p.nome) + ": " + std::to_string(p.preco) + (p.preco == 1 ? " moeda" : " moedas"), [this, p]() {
                        int* m = carteira();
                        if (!m) { fala::dizer("Você está sem a carteira."); return; }
                        if (*m < p.preco) { fala::dizer("Seu Zé: Tá faltando dinheiro, " + gen("freguês", "freguesa") + ". Você tem " + std::to_string(*m) + " moedas."); return; }
                        *m -= p.preco;
                        guardar(p.coisa, p.qtd);
                        audio::tocar("moedas", 0.7f);
                        fala::dizer("Comprado. Restam " + std::to_string(*m) + " moedas.");
                        evento("comprou");
                    }, nullptr, nullptr});
                return v2;
            };
            abrirMenu(c);
        }, nullptr, nullptr});
        return v;
    };
    abrirMenu(m);
}

void Jogo::conversarPrefeito() {
    static const char* L[] = {
        "Seu Manel do Bacanal levanta da cadeira de couro: Seja bem-vind@ à prefeitura! Remessa toda noite, às dez, não esquece!",
        "Seu Manel do Bacanal: Essa cidade ainda vai crescer, você vai ver. Com gente trabalhadora como você, vai.",
        "Seu Manel do Bacanal: Milho, feijão, arroz, eu compro tudo. Põe na caixa de remessa que eu passo de noite.",
    };
    std::string l = L[(int)acaso(0, 2.99)];
    auto p = l.find('@'); if (p != std::string::npos) l.replace(p, 1, gen("o", "a"));
    fala::dizer(l);
}

// ---------- estado da fazenda, para quem visita ----------
std::string Jogo::estadoFazenda() {
    bool fora = (bool)fazendaGuardada;
    if (fora) trocarComGuardada();
    std::ostringstream f;
    for (int c : limpos) f << "limpo " << c << '\n';
    for (const auto& [c, p] : plantas) f << "planta " << c << ' ' << p.tipo << ' ' << p.horas << ' ' << p.molhadaAte << '\n';
    for (size_t i = 0; i < moveis.size(); i++) f << "movel " << i << ' ' << moveis[i].x << ' ' << moveis[i].y << '\n';
    for (auto [px, py] : mapa.passagens()) f << "porta " << px << ' ' << py << ' ' << (mapa.em(px, py).aberta ? 1 : 0) << '\n';
    if (fora) trocarComGuardada();
    return f.str();
}

void Jogo::aplicarEstado(const std::string& texto) {
    if (mundoAtual.rfind("visita:", 0) != 0) return;
    for (const auto& [c, p] : plantas) mapa.em(c % LADO, c / LADO).planta = 0;  // o dono manda tudo de novo
    plantas.clear();
    std::istringstream in(texto);
    std::string linha;
    while (std::getline(in, linha)) {
        std::istringstream s(linha);
        std::string k; s >> k;
        if (k == "limpo") { int c; s >> c; if (c >= 0 && c < LADO * LADO) mapa.em(c % LADO, c / LADO).chao = Chao::TerraArada; }
        else if (k == "planta") { int c; Planta p; s >> c >> p.tipo >> p.horas >> p.molhadaAte; if (c >= 0 && c < LADO * LADO) { plantas[c] = p; mapa.em(c % LADO, c / LADO).planta = (uint8_t)p.tipo; } }
        else if (k == "movel") { size_t i; int mx, my; s >> i >> mx >> my; if (i < moveis.size() && mapa.valido(mx, my)) { moveis[i].x = mx; moveis[i].y = my; } }
        else if (k == "porta") { int px, py, a; s >> px >> py >> a; if (mapa.ehPassagem(px, py)) mapa.em(px, py).aberta = a != 0; }
    }
    ocupar();
    if (!mapa.passa(x, y)) { x = FAZENDA_CHEGADA[0]; y = FAZENDA_CHEGADA[1]; }
}

// Cerveja, vinho, uísque: uma dose é uma garrafa de cerveja. Passou de duas, o mundo roda; da terceira em diante, vomita.
void Jogo::beberAlcool(double doses, const std::string& texto) {
    double antes = alcool;
    alcool += doses;
    std::string efeito;
    if (alcool > 2.5 && antes <= 2.5) {
        efeito = " O mundo começa a girar. Os pés não obedecem direito, e você perde a noção de para onde está virad" + gen("o", "a") + ".";
        marcar(15, [this]() {
            if (alcool <= 2.5) return;
            audio::tocar("vomito", 0.9f);
            fome = std::max(0.0, fome - 25); sede = std::max(0.0, sede - 15); higiene = std::max(0.0, higiene - 20);
            alcool = std::max(0.0, alcool - 0.3);
            avisar("O estômago revira. Você se dobra e vomita tudo ali mesmo. Que vergonha. Melhor parar por hoje e beber água.");
        });
    } else if (alcool > 1.5 && antes <= 1.5) efeito = " Bate aquela alegria leve, e as bochechas esquentam.";
    fala::dizer(texto + efeito);
}
