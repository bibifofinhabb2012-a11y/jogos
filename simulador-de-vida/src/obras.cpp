// Casa do pecuarista (cidade): vende galinha, porco e coelho, que chegam de caminhonete na fazenda.
// Obras com madeira (lenha das árvores derrubadas), pedidas ao seu Tião: chiqueiro e galinheiro de luxo.
#include <algorithm>
#include <cctype>
#include "audio.h"
#include "fala.h"
#include "jogo.h"
#include "util.h"

void Jogo::menuPecuarista() {
    Menu m;
    m.titulo = "Seu Juca, o pecuarista";
    m.opcoes = [this]() {
        std::vector<Opcao> v;
        v.push_back({"Conversar", [this]() {
            static const char* L[] = {
                "Bicho bom é bicho bem tratado. Água limpa e comida na hora certa, que o resto vem.",
                "Galinha nova bota todo dia. Depois de velha, só serve pra panela.",
                "Porquinho trufeiro é ouro: de dia sai fuçando o pasto, de noite volta pro chiqueiro de luxo com a trufa.",
                "Porco come de tudo, mas precisa de chiqueiro firme, senão foge pro vizinho.",
                "Coelho é bicho delicado: galinheiro de luxo, coberto, que ele não aguenta chuva.",
            };
            fala::dizer(std::string("Seu Juca: ") + L[(int)acaso(0, 4.99)]);
        }, nullptr, nullptr});
        struct B { const char* tipo; const char* nome; int preco; const char* obra; const char* precisa; int maximo; };
        static const B BICHOS[] = {
            {"galinha", "Galinha nova, que bota um ovo por dia", 15, "", "", 20},
            {"porco", "Porco", 60, "chiqueiro", "um chiqueiro: seu Tião ajuda a fazer, com madeira", 10},
            {"coelho", "Coelho", 25, "coelheira", "um galinheiro de luxo: seu Tião ajuda a fazer, com madeira", 12},
            {"porco-trufa", "Porquinho trufeiro, que caça trufa e não é para abate", 80, "chiqueiro-luxo", "um chiqueiro de luxo: seu Tião ajuda a fazer, com madeira", 6},
        };
        for (const B& b : BICHOS)
            v.push_back({std::string(b.nome) + ": " + std::to_string(b.preco) + " moedas", [this, b]() {
                if (*b.obra && !obras.count(b.obra)) { fala::dizer(std::string("Seu Juca: Vendo, mas onde tu vai pôr? Precisa de ") + b.precisa + "."); return; }
                int tem = contarVacas(b.tipo) + (int)std::count(entregas.begin(), entregas.end(), b.tipo);
                if (mundoAtual != "fazenda" && fazendaGuardada) {
                    tem = (int)std::count(entregas.begin(), entregas.end(), b.tipo) +
                          (int)std::count_if(fazendaGuardada->moveis.begin(), fazendaGuardada->moveis.end(), [&](const Movel& mv) { return mv.tipo == b.tipo; });
                }
                if (tem >= b.maximo) { fala::dizer("Seu Juca: Já tem bicho demais lá, não cabe mais."); return; }
                int* mo = carteira();
                if (!mo) { fala::dizer("Você está sem a carteira."); return; }
                if (*mo < b.preco) { fala::dizer("Seu Juca: Tá faltando dinheiro. Você tem " + std::to_string(*mo) + " moedas."); return; }
                *mo -= b.preco;
                entregas.push_back(b.tipo);
                audio::tocar("moedas", 0.7f);
                fala::dizer("Comprado. Restam " + std::to_string(*mo) + " moedas. Seu Juca: Levo de caminhonete; quando tu chegar na fazenda, já tá lá.");
            }, nullptr, nullptr});
        return v;
    };
    abrirMenu(m);
}

static std::string minuscula(std::string n) { if (!n.empty()) n[0] = (char)std::tolower((unsigned char)n[0]); return n; }

void Jogo::construir(const std::string& obra) {
    obras.insert(obra);
    mapa.construir(obra);
}

// Pedido ao seu Tião: obras feitas com toras de lenha.
void Jogo::menuObras() {
    Menu m;
    m.titulo = "Construir com madeira. Você tem " + std::to_string(contarTipo("lenha")) + " toras de lenha";
    m.opcoes = [this]() {
        std::vector<Opcao> v;
        struct O { const char* id; const char* nome; int toras; const char* onde; };
        static const O OBRAS[] = {
            {"chiqueiro", "Chiqueiro, para porcos", 20, "a leste da casa, no terreiro, com porteira do lado oeste"},
            {"coelheira", "Galinheiro de luxo, coberto, para coelhos", 15, "do lado leste do galinheiro, com porta para o sul"},
            {"chiqueiro-luxo", "Chiqueiro de luxo, para os porquinhos trufeiros", 25, "colado no chiqueiro comum, do lado leste, com uma parede dividindo os dois e a porta do lado oposto, a leste"},
        };
        for (const O& o : OBRAS)
            v.push_back({std::string(o.nome) + ": " + std::to_string(o.toras) + " toras" + (obras.count(o.id) ? ". Já está feito" : ""), [this, o]() {
                if (obras.count(o.id)) { fala::dizer(std::string("Já está feito: ") + o.onde + "."); return; }
                if (mundoAtual != "fazenda") { fala::dizer("Obra se faz na sua fazenda."); return; }
                int tem = contarTipo("lenha");
                if (tem < o.toras) {
                    fala::dizer("Seu Tião: Pra isso precisa de " + std::to_string(o.toras) + " toras, tchê. Tu tem " + std::to_string(tem) +
                                ". Derruba umas árvores no machado, que cada uma dá 4.");
                    return;
                }
                if (energia < 15) { fala::dizer("Seu Tião: Tu tá morto de cansado. Descansa primeiro, que obra cansa."); return; }
                gastarTipo("lenha", o.toras);
                construir(o.id);
                energia = std::max(0.0, energia - 12);
                higiene = std::max(0.0, higiene - 15);
                while (!menus.empty()) fecharMenu();
                audio::tocar("serra", 0.8f);
                marcar(2.5, [this]() { audio::tocar("martelo", 0.9f); });
                marcar(5.0, [this]() { audio::tocar("martelo", 0.8f, 1.1f); });
                fala::dizer(std::string("Você e seu Tião passam a tarde serrando as toras em tábuas e mourões, cavando buraco, fincando e pregando. ") +
                            "No fim, suado e com as mãos cheias de farpa, está pronto: " + minuscula(o.nome) + ", " + o.onde + ". No Ç, em Lugares. " +
                            "Agora seu Juca, o pecuarista da cidade, pode trazer os bichos.");
            }, nullptr, nullptr});
        return v;
    };
    abrirMenu(m);
}
