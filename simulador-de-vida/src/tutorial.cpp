// Tutorial interativo: cada passo diz o que fazer e espera a pessoa fazer.
// Quando o jogo vê que foi feito, toca um sinal, diz "muito bem" e passa para o próximo.
// Para acrescentar passos: um texto e uma condição. Os eventos vêm de evento("nome") pelo jogo.
#include <algorithm>
#include "audio.h"
#include "fala.h"
#include "jogo.h"
#include "util.h"


void Jogo::evento(const std::string& e) {
    if (passoTutorial < 0) return;
    eventos.push_back(e);
    if (eventos.size() > 20) eventos.erase(eventos.begin());
}


void Jogo::comecarTutorial() {
    passoTutorial = 0;
    eventos.clear();
    esperaTutorial = 0.3;
    fala::dizer("Tutorial. Em cada passo eu digo o que fazer e espero você fazer. F2 repete a última fala. Para parar, Tab e Parar o tutorial.");
}

void Jogo::atualizarTutorial(double dt) {
    if (passoTutorial < 0) return;
    auto tem = [this](const char* e) { return std::find(eventos.begin(), eventos.end(), e) != eventos.end(); };
    struct P { const char* texto; std::function<bool()> feito; };
    static int passosNoInicio = 0;
    auto tenho = [this](const char* t) {
        if (maoDireita.tipo == t || maoEsquerda.tipo == t) return true;
        for (const Pilha& p : mochila) if (p.coisa.tipo == t) return true;
        return false;
    };
    auto naMaos = [this](const char* a, const char* b) {
        return (maoDireita.tipo == a && maoEsquerda.tipo == b) || (maoDireita.tipo == b && maoEsquerda.tipo == a);
    };
    std::vector<P> lista = {
        {"Primeiro passo: aperte B para saber onde você está.", [&] { return tem("b"); }},
        {"Agora Shift B: o lugar, a posição e o que tem à sua frente.", [&] { return tem("olhar"); }},
        {"Vire 90 graus para a direita: Shift mais seta para a direita.", [&] { return tem("virar"); }},
        {"Agora vire 90 graus para a esquerda, de volta: Shift mais seta para a esquerda.", [&] { return tem("virar"); }},
        {"Ande uns passos para a frente: segure a seta para cima.", [&] { return contadorPassos - passosNoInicio >= 5; }},
        {"Abra a mochila com I.", [&] { return mochilaAberta; }},
        {"Com a mochila aberta, aperte a letra B. Cada vez que você aperta, ela pula para a próxima coisa que começa com B: bússola, barra.", [&] { return tem("letra"); }},
        {"Aperte B até ouvir a barra de ração, e então seta para a direita, para pôr na mão direita.", [&] { return maoDireita.tipo == "barra"; }},
        {"Feche a mochila com Esc.", [&] { return !mochilaAberta; }},
        {"Aperte 3 para ouvir o que tem nas duas mãos.", [&] { return tem("maos"); }},
        {"Abra a barra: aperte T. A outra mão está vazia e rasga a embalagem.", [&] { return maoDireita.tipo == "barra-aberta"; }},
        {"Selecione a mão direita com 2, e coma com Espaço.", [&] { return tem("comeu"); }},
        {"Aperte U para saber como está a fome.", [&] { return tem("fome"); }},
        {"Sente no chão: aperte R.", [&] { return postura == Postura::Sentado; }},
        {"Levante: R de novo.", [&] { return postura == Postura::EmPe; }},
        {"Agora vamos achar a casa. Aperte Ç até ouvir Portas e porteiras.", [&] { return categoria == 2; }},
        {"Com L para a frente e K para trás, escolha a porta da frente da casa.", [&] { return !alvos.empty() && alvos[itemAlvo].x == 500 && alvos[itemAlvo].y == 109; }},
        {"Aperte O para rastrear. Um bip vai soar de onde a porta está, mais rápido quanto mais perto. Ande na direção do bip. Se quiser, Alt O leva você até lá.", [&] { return tem("chegou"); }},
        {"Você chegou na porta. Abra com Shift Enter.", [&] { return mapa.em(500, 109).aberta; }},
        {"Entre na casa: dois passos para a frente. Depois, aperte B para confirmar que está na sala.", [&] { return tem("b") && mapa.nomeLugar(x, y) == "sala"; }},
        {"Aperte W para saber como está a limpeza do corpo, e se precisa de banho.", [&] { return tem("higiene"); }},
        // segunda parte: cidade, bar do seu Zé, cigarro, cerveja, sementes, casamento
        {"INFO" "Segunda parte: a cidade e o bar do seu Zé. Vamos de trator até lá, comprar isqueiro, latinha de gás, cigarro e cerveja, "
         "e aprender a usar. Isso gasta 29 das suas 100 moedas.", [&] { return tem("enter"); }},
        {"Vamos achar o trator. Aperte Ç até ouvir Móveis, e L ou K até ouvir Trator.", [&] { return categoria == 0 && !alvos.empty() && alvos[itemAlvo].nome.rfind("Trator", 0) == 0; }},
        {"Alt O leva você até o trator, abrindo as portas no caminho. Chegando, com a mão selecionada vazia, aperte Espaço e escolha Ir para a cidade. "
         "Se a mão não estiver vazia, aperte duas vezes o número dela, 1 ou 2, para guardar o que tem nela.", [&] { return mundoAtual == "cidade"; }},
        {"Você está na cidade. Aperte Ç até Pessoas, L até seu Zé, no balcão do bar, e Alt O para ir até ele.", [&] { return tem("chegou") && categoria == 1; }},
        {"De frente para seu Zé, com a mão vazia, aperte Espaço. Escolha Comprar, e compre um isqueiro.", [&] { return tenho("isqueiro"); }},
        {"No mesmo menu, compre também a latinha de gás para isqueiro, o maço de cigarro e a garrafa de cerveja. "
         "Uma letra pula para a opção com aquela inicial. Depois, Esc até sair do menu.",
         [&] { return tenho("gas-isqueiro") && tenho("cigarro") && tenho("cerveja") && menus.empty(); }},
        {"Abra a mochila com I. Ponha o isqueiro numa mão e a latinha de gás na outra: seta para a direita põe na mão direita, seta para a esquerda, na esquerda. Depois feche com Esc.",
         [&] { return naMaos("isqueiro", "gas-isqueiro") && !mochilaAberta; }},
        {"Aperte T: a latinha enche o isqueiro. No jogo, faça isso quando o isqueiro disser que está sem gás.", [&] { return tem("encheu"); }},
        {"Agora o cigarro. Guarde a latinha: aperte duas vezes o número da mão em que ela está. Depois abra a mochila, ponha o maço de cigarro nessa mão, e feche.",
         [&] { return naMaos("isqueiro", "cigarro") && !mochilaAberta; }},
        {"Isqueiro numa mão, cigarro na outra: aperte T até acender. Às vezes falha, é só apertar de novo. Aceso, ele fuma sozinho.", [&] { return tem("fumou"); }},
        {"Agora a cerveja. Guarde o isqueiro ou o maço, ponha a garrafa de cerveja numa mão, selecione essa mão com 1 ou 2, e beba com Espaço. "
         "Uma só: mais de duas deixam o personagem tonto, cambaleando, e da terceira em diante ele vomita.", [&] { return tem("cerveja"); }},
        {"INFO" "Gás do fogão: a chama do fogão gasta o botijão. Quando acabar, compre um botijão com seu Zé, 60 moedas, "
         "segure na mão e aperte Espaço de frente para o fogão.", [&] { return tem("enter"); }},
        {"INFO" "Sementes: seu Zé vende pacotes com 10 de milho, feijão, arroz, laranja, banana, abóbora, cenoura e cacau. "
         "Planta-se como o milho: terra da roça limpa, semente na mão, Espaço. Cenoura fica pronta em 3 dias, abóbora em 4, banana em 5, laranja em 6 e cacau em 7. "
         "Laranja, banana e cacau são pés que ficam: depois de colher, dão de novo em 2 dias.", [&] { return tem("enter"); }},
        {"INFO" "Casamento, no jogo online: na prefeitura, do outro lado da praça, o prefeito vende a caixinha de alianças por 50 moedas. "
         "Com a aliança na mão, Espaço, e escolha com quem casar: você vai até a pessoa e ela recebe o pedido. "
         "Se ela aceitar, todo mundo recebe o aviso, e no Tab aparece Teleportar para o evento. Depois da cerimônia, a festa é no salão de festas, ao sul da praça: "
         "na mesa de comes e bebes, Espaço abre o menu, e cada um pega até 5 de cada coisa. "
         "Casados, no Tab aparece Conversar sobre a fazenda: os dois escolhem em qual fazenda vão morar, e a outra deixa de existir.", [&] { return tem("enter"); }},
        {"Para terminar, volte para casa: Ç até Móveis, L ou K até o trator, Alt O, Espaço com a mão vazia, e Voltar para a minha fazenda.", [&] { return mundoAtual == "fazenda"; }},
    };
    if (passoTutorial >= (int)lista.size()) {
        passoTutorial = -1;
        audio::tocar("tutorial-ok", 0.8f);
        fala::dizer("Fim do tutorial. Muito bem! Agora explore: com Ç, K e L você acha os móveis, as pessoas e os lugares. "
                    "Seu Tião, na casinha, ensina a roça. F1 fala todas as teclas. Bom jogo.", false);
        return;
    }
    if (esperaTutorial > 0) {
        esperaTutorial -= dt;
        if (esperaTutorial <= 0) {
            if (passoTutorial == 4) passosNoInicio = contadorPassos;
            eventos.clear();
            std::string t = lista[passoTutorial].texto;
            if (t.rfind("INFO", 0) == 0) t = t.substr(4) + " Enter para continuar.";
            fala::dizer(t, false);
        }
        return;
    }
    if (lista[passoTutorial].feito()) {
        passoTutorial++;
        audio::tocar("tutorial-ok", 0.8f);
        fala::dizer("Muito bem!", false);
        esperaTutorial = 0.8;
    }
}
