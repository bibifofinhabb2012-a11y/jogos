// O jogo em si, sem janela nem teclado: recebe teclas e o tempo que passou, e fala e toca sons.
// Assim dá para testar tudo por roteiro, sem abrir janela.
#pragma once
#include <functional>
#include <map>
#include <set>
#include <memory>
#include <string>
#include <vector>
#include "mapa.h"
#include "rede.h"

enum class Tecla { Cima, Baixo, Esquerda, Direita, I, U, S, E, T, L, H, Q, R, B, V, K, O, W, Cedilha, Um, Dois, Tres, Enter, Espaco, Esc, Tab, F1, F2, Backspace, Z, Letra };
enum class Postura { EmPe, Sentado, Agachado, Deitado };

// Uma coisa: o tipo e uma carga (goles numa garrafa, moedas na carteira).
struct Coisa {
    std::string tipo;
    int carga = 0;
    bool operator==(const Coisa& o) const { return tipo == o.tipo && carga == o.carga; }
};

struct Pilha { Coisa coisa; int qtd; };

// Um móvel ou aparelho da casa, num quadrado. Alguns guardam coisas (geladeira, guarda-roupa).
struct Movel {
    std::string tipo;
    int x = 0, y = 0;
    bool aberto = false, ligado = false;
    std::vector<std::vector<Pilha>> partes;  // compartimentos: portas do guarda-roupa, a geladeira inteira
    int som = 0;                             // som contínuo (motor da geladeira, chama, chuveiro)
    double ciclo = 0;                        // motor da geladeira: liga e desliga sozinho
    int ordenha = -1;                        // vaca de leite: dia do jogo da última ordenha
    double gas = 600;
    int carga = 0;                           // cafeteira: doses de pó no compartimento
    double pronto = 0;                       // prensa: minuto do jogo em que o queijo fica pronto (0: vazia)
    std::string cozinhando;                  // fogão: o que está no fogo (frigideira ou panela com comida)
    double cozinhaFalta = 0;                 // segundos de fogo que faltam (negativo: passando do ponto)
    int avisouCozinha = 0, somFrito = 0;                        // fogão: segundos de verdade de chama que o botijão ainda tem
};

// Opção de menu. ativar: Enter. tudo: Shift Enter. ajustar: setas para os lados.
struct Opcao {
    std::string texto;
    std::function<void()> ativar, tudo;
    std::function<void(int)> ajustar;
};
struct Menu {
    std::string titulo;
    std::function<std::vector<Opcao>()> opcoes;  // gerado na hora: o conteúdo muda
    int sel = 0;
    std::function<void()> aoFechar;
    bool pausa = false;        // o tempo do jogo para enquanto está aberto (Tab, escolha inicial)
    bool obrigatorio = false;  // Esc não fecha: precisa escolher (homem ou mulher)
};

// Carta na caixa de correio. O texto é montado na hora, pelo id (assim muda com homem ou mulher).
struct Carta { std::string id; bool lida = false; };

// Algo que dá para escolher com K e L e rastrear com O.
// Lugares grandes têm retângulo (x0..x1, y0..y1): chegar é entrar nele.
bool ehVaca(const std::string& t);   // vaca de corte ou de leite (vacas.cpp)
bool ehBicho(const std::string& t);  // qualquer bicho que anda no cercado: vacas, galinha, porco, coelho

struct Alvo { std::string nome; int x, y; bool chegarAoLado; int x0 = -1, y0 = 0, x1 = 0, y1 = 0; int jogador = -1; int movel = -1; };

// Campo de escrita (chat, nome, endereço). O jogo fala cada letra, porque o leitor de tela não vê a janela.
struct Campo { bool ativo = false; std::string titulo, texto; std::function<void(const std::string&)> ok; };

// Uma planta na roça. Cresce só enquanto a terra está molhada (um dia do jogo depois de regar).
struct Planta {
    int tipo = 1;            // 1 milho, 2 feijão de corda, 3 arroz
    double horas = 0;        // horas de crescimento com água
    double molhadaAte = 0;   // minuto do jogo até quando a rega vale
};

// Um lugar onde se pode estar: a sua fazenda, a cidade ou a fazenda de outro jogador.
// Coisa caída no chão (fruta que caiu da árvore, lenha que alguém largou, bosta de vaca): categoria Itens do Ç.
struct ItemChao { Coisa coisa; int x = 0, y = 0; };

struct Mundo {
    Mapa mapa;
    std::vector<Movel> moveis;
    std::map<int, Planta> plantas;
    std::vector<int> limpos;
    std::vector<int> cortadas;
    std::vector<ItemChao> chao;
    int x = 0, y = 0, rumo = 0;
};

// Árvore com nome (objeto do mapa): frutas do dia, um bit por galho; derrubada, brota de novo depois de 20 minutos de verdade.
struct EstadoArvore { int dia = -1; uint8_t frutas = 0; double renasce = 0; };


class Jogo {
public:
    Jogo();
    void novo();                                  // começa do zero, com a cena do pai
    bool carregar(const std::string& arquivo);    // true: havia um jogo salvo
    void salvar(const std::string& arquivo);
    void tecla(Tecla t, bool shift, bool ctrl, bool alt = false, char letra = 0);  // tecla apertada (não repetição)
    void soltar(Tecla t);
    void menuInicio();         // ao abrir com jogo salvo: continuar ou começar de novo
    std::string arquivoSalvo;  // onde o jogo salva (para o jogo novo apagar o velho na hora)                         // tecla solta
    void atualizar(double segundos);              // tempo real que passou
    bool querSair() const { return sair; }
    void abertura(bool novoJogo);                 // o que se ouve ao abrir o jogo
    bool pausado() const { for (const Menu& m : menus) if (m.pausa) return true; return false; }
    void carregarConfig(const std::string& arquivo);
    void salvarConfig(const std::string& arquivo) const;
    std::string arquivoConfig;
    // texto digitado (SDL_TEXTINPUT): vai para o campo aberto; dois-pontos abre o chat
    void texto(const std::string& utf8);
    bool digitando() const { return campo.ativo; }
    void servicoRede(double dt);   // chamar todo quadro, mesmo com menu aberto
    int pacotesVozLidos() const { return pacotesVoz; }
    void testeIr(int nx, int ny, int r) { x = nx; y = ny; rumo = r; lugarAtual = mapa.nomeLugar(x, y); }  // só para roteiros de teste

private:
    Mapa mapa;
    // onde está e para onde olha (0 norte, 1 nordeste, ... 7 noroeste)
    int x = 500, y = 9, rumo = 0;
    Postura postura = Postura::EmPe;
    double intestino = 30, bexiga = 30;  // de 0 (vazio) a 100 (não segura mais)
    int excretando = 0;                   // 1 xixi, 2 cocô, segurando a tecla
    double excretaT = 0, proximoPlop = 0;
    int somXixi = 0;
    // necessidades, de 0 a 100: 100 é satisfeito
    double fome = 85, sede = 85, energia = 90, saude = 100, higiene = 90;
    double minutos = 7 * 60;  // relógio do jogo, em minutos desde a meia-noite do dia 1
    std::vector<Pilha> mochila;
    Coisa maoDireita, maoEsquerda;  // tipo vazio = mão vazia
    bool mochilaAberta = false;
    int itemMochila = 0;
    bool dormindo = false;
    bool sair = false, confirmarSair = false;
    double relogioSalvar = 0, ultimoAviso[6] = {100, 100, 100, 100, 100, 100};
    // andar: tecla segurada e tempo até o próximo passo
    bool andandoFrente = false, andandoTras = false, correndo = false;
    int andandoLado = 0;   // -1 esquerda, 1 direita (passo de lado)
    int maoSel = 0;        // mão selecionada: 0 direita, 1 esquerda
    int somSono = 0;
    double esperaPasso = 0;
    std::string ultimoObstaculo;
    double toqueNum[2] = {-10, -10}, agora = 0;
    // sons do ambiente
    int somCorrego = 0, somVento = 0, somNoite = 0, somDia = 0, carroPai = 0;
    double carroT = -1, proximoPassaro = 5;
    std::string lugarAtual;
    // móveis
    std::vector<Movel> moveis;
    int segurando = -1;   // móvel sendo arrastado
    int sentadoEm = -1;   // móvel em que está sentado ou deitado (sofá, cama, vaso)
    double tomandoBanho = 0;
    // menus (geladeira, guarda-roupa, Tab)
    std::vector<Menu> menus;
    // objetos: categoria (Ç), item (K e L), rastreio (O) e caminho automático (Alt O)
    int categoria = -1, itemAlvo = 0;
    std::vector<Alvo> alvos;
    bool rastreando = false;
    Alvo rastreado{};
    double proximoBip = 0;
    std::vector<int> caminho;   // direções a seguir, uma por passo
    double esperaCaminho = 0;
    // tutorial
    int passoTutorial = -1;
    std::vector<std::string> eventos;   // o que a pessoa acabou de fazer, para o tutorial conferir
    double esperaTutorial = 0;
    std::string textoTutorial;  // a instrução do passo atual, para o Shift S repetir
    int totalTutorial = 0;
    double marcaEnergia = 100;
    bool avisouMachucado = false;
    bool temGalinhas = false;  // o salvo já é da versão com galinhas (salvo antigo: elas chegam ao carregar)  // a cada 50 pontos de energia perdidos, um bip (mais grave quanto menos energia)
    void navegarTutorial(int acao);
    // roça
    std::map<int, Planta> plantas;     // chave: y * LADO + x
    std::vector<int> limpos;           // quadrados de matagal já limpos (para salvar)
    // seu Tião e dona Nezinha
    bool cenaTiao = false;
    int diaCafe = 0;                   // último dia em que dona Nezinha encheu a garrafa
    std::vector<std::pair<double, std::function<void()>>> agenda;
    // homem ou mulher: muda como seu Tião, dona Nezinha e as cartas falam. '?' ainda não escolheu
    char genero = '?';
    std::vector<Carta> cartas;
    double proximaColeta = 22 * 60;          // minuto do jogo em que seu Manel passa na caixa de remessa
    std::vector<std::string> aoAcordar;      // o que aconteceu enquanto dormia
    double acordarEm = -1;                   // dormindo na cama: acorda às 6 da manhã seguinte
    // onde está: "fazenda" (a sua), "cidade" ou "visita:Nome". Fora de casa, a fazenda fica guardada aqui e continua viva.
    std::string mundoAtual = "fazenda";
    std::unique_ptr<Mundo> fazendaGuardada;
    bool viajando = false, chegadaCalada = false;  // calada: teleporte, sem a fala do trator
    int somViagem = 0, somBar = 0;
    // vícios: cigarro aceso (segundos que faltam) e álcool no sangue (garrafas de cerveja, vai baixando)
    double fumando = 0, proximaTragada = 0, alcool = 0;
    // casamento (online): com quem casou, e em qual fazenda o casal mora ("" ainda não decidiram)
    std::string conjuge, moraCom;
    char generoConjuge = '?';
    struct Evento { std::string tipo, a, b; char ga = 'h', gb = 'h'; double fim = 0; } evento_;  // "casamento" ou "festa"
    std::map<std::string, int> pegosNaFesta;
    double estadoRede = 0;
    // online, chat e voz
    Campo campo;
    int pacotesVoz = 0;            // quantos pedaços de voz chegaram (para teste)
    bool chatAtivo = true, vozAtiva = false;
    std::vector<std::string> historicoChat;
    std::string nomeOnline, ultimoIp;
    double relogioRede = 0, posicaoRede = 0;
    int enviadoX = -1, enviadoY = -1, enviadoRumo = -1;
    std::string enviadoMundo;  // coisas marcadas para daqui a pouco (segundos reais)
    int contadorPassos = 0;

    // mochila e mãos
    std::string nomeCoisa(const Coisa& c, bool comArtigo = false) const;
    void guardar(const Coisa& c, int qtd = 1);
    bool tirar(const Coisa& c);
    void falarItemMochila(const std::string& antes = "");
    void colocarNaMao(bool direita);
    void verMao(int mao);
    Coisa& mao(bool direita) { return direita ? maoDireita : maoEsquerda; }
    void combinar();
    void usar(bool direita);
    // corpo
    void dizerFome(); void dizerSede(); void dizerEnergia(); void dizerSaude();
    void avisosDoCorpo();
    void dormir();
    void acordar(const std::string& motivo);
    // mundo
    void passo(int sentido);
    void passoPara(int r);
    void virar(int lado);
    void bussola();
    void mudarPostura(Postura p);
    void abrirPorta();
    void anunciarPortas();
    void comecarExcretar(int tipo);
    void pararExcretar(bool cedo);
    void dizerIntestino();
    void dizerBexiga();
    void verMaos();
    std::string relativo(int dirAbs) const;
    void olhar();
    void hora();
    void ajuda();
    std::string passagemPerto(int ox, int oy, int r) const;
    void ambiente(double dt);
    std::string direcao(int r) const;
    // móveis (moveis.cpp)
    void criarMoveis();
    void ocupar();
    int movelEm(int mx, int my) const;
    int movelFrente() const;
    std::string nomeMovel(int i, bool artigo = false) const;
    void interagir(int i);
    void arrastar();
    bool moverMovel(int i, int nx, int ny);
    void chuveiro();
    void sentarEm(int i, Postura p);
    void dizerHigiene();
    void abrirRecipiente(int i);
    void atualizarMoveis(double dt);
    void olharArvore(const Objeto& o);
    // arvores.cpp
    std::vector<int> cortadas;              // árvores derrubadas (quadrados), vão no salvo
    std::map<int, int> golpes;              // golpes de machado em cada árvore, até cair
    std::vector<EstadoArvore> pomar;        // um por objeto do mapa
    int radarArvore = -1, radarItem = -1, radarX = -1, radarY = -1, radarRumo = -1, frutaMirada = -1;
    int arvoreDaFrente() const;
    uint8_t& frutasDa(int i);
    bool radarValido(int i) const;
    void radarArvoreTecla(int lado);
    bool mirarFruta();
    void maoNaArvore(int i);
    void cortarArvore(int fx, int fy);
    void usarPicareta();
    bool temCoisa(const std::string& tipo) const;
    void atualizarArvores(double seg);
    // itens no chão
    std::vector<ItemChao> chao;
    int itemChaoPerto() const;
    void pegarDoChao(int i);
    void largar();
    bool espalhar(const Coisa& c, int cx, int cy, int raio);
    // menu de início
    void confirmarJogoNovo(bool doInicio);
    void menuVolumeSons();
    // bichos pequenos, pecuarista e obras
    int idadeDe(const Movel& m) const;
    void abaterPequeno(int i, int dist);
    std::vector<std::string> entregas;     // bichos comprados que ainda não chegaram na fazenda
    std::set<std::string> obras;           // construído com madeira: "chiqueiro", "coelheira"
    void receberEntregas();
    void menuPecuarista();
    void menuObras();
    void construir(const std::string& obra);
    bool trufeiro(size_t i);
    bool avisouTrufas = false;
    // maquinas.cpp
    int contarTipo(const std::string& tipo, int carga = -1) const;
    void gastarTipo(const std::string& tipo, int n, int carga = -1, const Coisa& troca = {});
    bool usarMaquina(Coisa& c);
    void prensaPronta(int i);
    void menuCafeteira(int i);
    // cozinha.cpp
    bool combinarCozinha(Coisa& d, Coisa& e);
    bool separarCozinha(Coisa& c, Coisa& vazia);
    bool levarAoFogo(Coisa& c, int f);
    void tirarDoFogo(int f);
    void atualizarFogo(Movel& m, double dt);
    bool usarForno(Coisa& c, int f);
    void fornoMaoVazia(int i);
    void atualizarForno(Movel& m, double dt);
    std::string predioDe(int px, int py) const;
    std::string predioAlvos;  // onde a pessoa estava quando a lista de portas foi feita
    // pulo.cpp
    bool carregandoPulo = false;
    double cargaPulo = 0, noAr = 0, tempoNoAr = 0;
    int nivelPulo = 0, dirPulo = -1, alturaPulo = 0, pousoX = 0, pousoY = 0, passosPulo = 0;
    std::string bateuEm, plantaNoPouso;
    void comecarPulo();
    void direcaoPulo(Tecla t);
    void pular();
    void atualizarPulo(double seg);
    float alturaNoAr() const;
    // vacas.cpp
    int contarVacas(const std::string& tipo) const;
    bool poeVaca(const std::string& tipo, int vx = -1, int vy = -1);
    void ajustarVacas(int n);
    std::string linhasBichos() const;
    void lerBicho(const std::string& linha);
    void coice(Movel& v, int minimo, int maximo, const std::string& susto);
    bool ordenhar(Coisa& c);
    void menuCouro();
    void nascerBezerro();
    void atualizarVacas(double seg);
    void carinhoVaca(int i);
    void usarFaca();
    // menus (menu.cpp)
    void abrirMenu(Menu m);
    void fecharMenu();
    void falarOpcao(const std::string& antes = "");
    bool teclaMenu(Tecla t, bool shift, char letra);
    void menuPrincipal();
    void menuConfiguracoes();
    // objetos (guia.cpp)
    void trocarCategoria(int lado);
    void trocarItem(int lado);
    void montarAlvos();
    std::string descreverAlvo(const Alvo& a) const;
    void rastrear();
    void irAteAlvo();
    void atualizarGuia(double dt);
    bool chegou(const Alvo& a) const;
    // roça e gente (roca.cpp)
    int plantaFrente() const;
    std::string nomePlanta(int tipo, bool plural = false) const;
    std::string estadoPlanta(const Planta& p) const;
    bool usarNaRoca(Coisa& c);         // machado, semente, regador: true se fez algo
    void colher(int cel, bool comMachado);
    void atualizarRoca(double dm);
    void conversarCom(int i);
    void cenaChegada();
    void marcar(double segundos, std::function<void()> f);
    int idx(const std::string& tipo) const;
    std::string gen(const std::string& homem, const std::string& mulher) const { return genero == 'm' ? mulher : homem; }
    void escolherGenero(bool depoisAbertura);
    std::string textoCarta(const Carta& c) const;
    std::string remetente(const Carta& c) const;
    void chegouCarta(const std::string& id);
    void abrirCorreio();
    void abrirRemessa(int i);
    void coletarRemessa();
    void avisar(const std::string& texto);  // fala agora, ou guarda para quando acordar
    // mundos (mundo.cpp)
    void trocarComGuardada();                // troca o mundo atual com a fazenda guardada
    std::vector<Movel>& meusMoveis() { return fazendaGuardada ? fazendaGuardada->moveis : moveis; }
    void viajar(const std::string& destino);
    void chegarEm(const std::string& destino);
    void criarMoveisCidade();
    void menuTrator();
    void menuZe();
    void conversarPrefeito();
    int* carteira();                         // as moedas, onde a carteira estiver (mão ou mochila)
    std::string estadoFazenda();             // texto com o que muda na fazenda (para quem visita)
    void aplicarEstado(const std::string& texto);
    void pararSonsDosMoveis();
    void beberAlcool(double doses, const std::string& texto);
    // casamento e festa (casamento.cpp)
    void menuPrefeito();
    void usarAlianca();
    void receberApp(const std::string& tipo, const std::string& de, const std::string& dados);
    void comecarCasamento(const std::string& a, char ga, const std::string& b, char gb);
    void cerimonia();
    void comecarFesta();
    void teleportarPara(const std::string& mundoCan, int tx, int ty);
    void irAoEvento();
    void menuMesaFesta();
    void conversarSobreFazenda();
    bool moradorDe(const std::string& dono) const { return !moraCom.empty() && moraCom == dono && dono != rede::meuNome(); }
    bool souMorador() const { return !moraCom.empty() && moraCom != nomeOnline; }
    void acaoNaFazenda(const std::string& acao);   // morador mexeu: avisa o dono
    void aplicarAcao(const std::string& acao);     // dono aplica na fazenda dele
    // nome do lugar igual para todos os jogadores: "fazenda:Dono" ou "cidade"
    std::string mundoCanonico() const {
        if (mundoAtual == "fazenda") return "fazenda:" + (rede::meuNome().empty() ? nomeOnline : rede::meuNome());
        if (mundoAtual.rfind("visita:", 0) == 0) return "fazenda:" + mundoAtual.substr(7);
        return mundoAtual;
    }
    // texto, chat e online (texto.cpp)
    void abrirCampo(const std::string& titulo, const std::string& inicial, std::function<void(const std::string&)> ok);
    void teclaCampo(Tecla t);
    void abrirChat();
    rede::Ganchos ganchos();
    void hospedarOnline();
    void entrarOnline();
    void pedirNome(std::function<void()> depois);
    void ligarVoz(bool ligar);
    // tutorial (tutorial.cpp)
    void evento(const std::string& e);
    void comecarTutorial();
    void atualizarTutorial(double dt);
    int hora24() const { return ((int)minutos / 60) % 24; }
    int diaDoJogo() const { return 1 + (int)minutos / 1440; }
};
