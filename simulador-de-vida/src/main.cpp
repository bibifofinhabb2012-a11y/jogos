// Simulador de Vida: janela, teclado e laço principal.
// Uso normal: abre a janela e joga.
// Teste: simulador-de-vida --roteiro arquivo.txt  (sem janela nem som; escreve as falas no terminal)
#include <SDL.h>
#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include "audio.h"
#include "fala.h"
#include "jogo.h"

static bool traduzir(SDL_Keycode k, Tecla& t) {
    static const std::map<SDL_Keycode, Tecla> mapa = {
        {SDLK_UP, Tecla::Cima}, {SDLK_DOWN, Tecla::Baixo}, {SDLK_LEFT, Tecla::Esquerda}, {SDLK_RIGHT, Tecla::Direita},
        {SDLK_i, Tecla::I}, {SDLK_u, Tecla::U}, {SDLK_s, Tecla::S}, {SDLK_e, Tecla::E}, {SDLK_t, Tecla::T},
        {SDLK_l, Tecla::L}, {SDLK_b, Tecla::B}, {SDLK_v, Tecla::V}, {SDLK_k, Tecla::K}, {SDLK_o, Tecla::O}, {SDLK_w, Tecla::W}, {SDLK_TAB, Tecla::Tab}, {231, Tecla::Cedilha}, {SDLK_3, Tecla::Tres}, {SDLK_KP_3, Tecla::Tres}, {SDLK_F2, Tecla::F2}, {SDLK_h, Tecla::H}, {SDLK_q, Tecla::Q}, {SDLK_r, Tecla::R},
        {SDLK_1, Tecla::Um}, {SDLK_KP_1, Tecla::Um}, {SDLK_2, Tecla::Dois}, {SDLK_KP_2, Tecla::Dois},
        {SDLK_RETURN, Tecla::Enter}, {SDLK_KP_ENTER, Tecla::Enter}, {SDLK_SPACE, Tecla::Espaco},
        {SDLK_ESCAPE, Tecla::Esc}, {SDLK_F1, Tecla::F1},
    };
    auto it = mapa.find(k);
    if (it == mapa.end()) {
        if (k >= SDLK_a && k <= SDLK_z) { t = Tecla::Letra; return true; }  // outras letras: pular em listas
        return false;
    }
    t = it->second;
    return true;
}

// Roteiro de teste: uma ação por linha.
//   tecla cima | tecla shift enter | segurar cima 3 | esperar 60 | horas 5 | novo
static int rodarRoteiro(const char* arquivo) {
    static const std::map<std::string, Tecla> nomes = {
        {"cima", Tecla::Cima}, {"baixo", Tecla::Baixo}, {"esquerda", Tecla::Esquerda}, {"direita", Tecla::Direita},
        {"i", Tecla::I}, {"u", Tecla::U}, {"s", Tecla::S}, {"e", Tecla::E}, {"t", Tecla::T}, {"l", Tecla::L}, {"b", Tecla::B}, {"v", Tecla::V}, {"k", Tecla::K}, {"o", Tecla::O}, {"w", Tecla::W}, {"tab", Tecla::Tab}, {"ç", Tecla::Cedilha}, {"3", Tecla::Tres}, {"f2", Tecla::F2},
        {"h", Tecla::H}, {"q", Tecla::Q}, {"r", Tecla::R}, {"1", Tecla::Um}, {"2", Tecla::Dois},
        {"enter", Tecla::Enter}, {"espaco", Tecla::Espaco}, {"esc", Tecla::Esc}, {"f1", Tecla::F1},
    };
    fala::iniciar(true);
    Jogo jogo;
    jogo.abertura(true);
    std::ifstream f(arquivo);
    std::string linha;
    while (std::getline(f, linha)) {
        std::istringstream s(linha);
        std::string cmd; s >> cmd;
        if (cmd.empty() || cmd[0] == '#') continue;
        std::printf("> %s\n", linha.c_str());
        if (cmd == "tecla" || cmd == "segurar") {
            std::string n; bool shift = false, ctrl = false; s >> n;
            if (n == "shift") { shift = true; s >> n; }
            if (n == "ctrl") { ctrl = true; s >> n; }
            bool alt = false;
            if (n == "alt") { alt = true; s >> n; }
            auto it = nomes.find(n);
            char letra = n.size() == 1 && n[0] >= 'a' && n[0] <= 'z' ? n[0] : 0;
            if (it == nomes.end() && !letra) { std::printf("tecla desconhecida: %s\n", n.c_str()); continue; }
            Tecla tc = it == nomes.end() ? Tecla::Letra : it->second;
            jogo.tecla(tc, shift, ctrl, alt, letra);
            jogo.atualizar(0.05);
            if (cmd == "segurar") {
                double seg = 1; s >> seg;
                for (double t = 0; t < seg; t += 0.05) jogo.atualizar(0.05);
                jogo.soltar(tc);
            } else jogo.soltar(tc);
        } else if (cmd == "salvar" || cmd == "carregar") {
            std::string arq; s >> arq;
            if (cmd == "salvar") jogo.salvar(arq); else { jogo.carregar(arq); jogo.abertura(false); }
        } else if (cmd == "esperar" || cmd == "horas") {
            double seg = 0; s >> seg;
            if (cmd == "horas") seg *= 300;  // uma hora do jogo = 5 minutos de verdade
            for (double t = 0; t < seg; t += 0.1) jogo.atualizar(0.1);
        }
    }
    fala::encerrar();
    return 0;
}

int main(int argc, char** argv) {
    if (argc >= 3 && std::string(argv[1]) == "--roteiro") return rodarRoteiro(argv[2]);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) { std::fprintf(stderr, "SDL: %s\n", SDL_GetError()); return 1; }
    SDL_Window* janela = SDL_CreateWindow("Simulador de Vida", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_SHOWN);
    SDL_Renderer* tela = SDL_CreateRenderer(janela, -1, 0);
    fala::iniciar(false);

    char* base = SDL_GetBasePath();
    std::string pasta = base ? base : "./";
    SDL_free(base);
    char* pref = SDL_GetPrefPath("BibiJogos", "SimuladorDeVida");
    std::string salvo = std::string(pref ? pref : pasta.c_str()) + "salvo.txt";
    SDL_free(pref);

    bool comSom = audio::iniciar(pasta + "sons");
    Jogo jogo;
    jogo.carregarConfig(std::string(salvo.substr(0, salvo.size() - 9)) + "config.txt");
    bool tinha = jogo.carregar(salvo);
    jogo.abertura(!tinha);
    if (!comSom) fala::dizer("Aviso: não foi possível abrir o som. O jogo continua só com a fala.", false);

    Uint64 antes = SDL_GetPerformanceCounter();
    double salvarEm = 60;
    while (!jogo.querSair()) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) { jogo.salvar(salvo); goto fim; }
            Tecla t;
            SDL_Keycode kc = ev.type == SDL_KEYDOWN || ev.type == SDL_KEYUP ? ev.key.keysym.sym : 0;
            // no teclado ABNT2, o Ç fica onde o ponto e vírgula fica no americano
            if ((ev.type == SDL_KEYDOWN || ev.type == SDL_KEYUP) && ev.key.keysym.scancode == SDL_SCANCODE_SEMICOLON) kc = 231;
            if (ev.type == SDL_KEYDOWN && !ev.key.repeat && traduzir(kc, t)) {
                char letra = (kc >= SDLK_a && kc <= SDLK_z) ? (char)kc : 0;
                Uint16 mod = ev.key.keysym.mod;
                jogo.tecla(t, (mod & KMOD_SHIFT) != 0, (mod & KMOD_CTRL) != 0, (mod & KMOD_ALT) != 0, letra);
            }
            if (ev.type == SDL_KEYUP && traduzir(kc, t)) jogo.soltar(t);
        }
        Uint64 agora = SDL_GetPerformanceCounter();
        double dt = (double)(agora - antes) / (double)SDL_GetPerformanceFrequency();
        antes = agora;
        if (dt > 0.5) dt = 0.5;
        jogo.atualizar(dt);
        salvarEm -= dt;
        if (salvarEm <= 0) { jogo.salvar(salvo); salvarEm = 60; }
        SDL_SetRenderDrawColor(tela, 20, 40, 20, 255);
        SDL_RenderClear(tela);
        SDL_RenderPresent(tela);
        SDL_Delay(15);
    }
    jogo.salvar(salvo);
    SDL_Delay(1200);  // dá tempo de ouvir "Jogo salvo"
fim:
    audio::encerrar();
    fala::encerrar();
    SDL_DestroyRenderer(tela);
    SDL_DestroyWindow(janela);
    SDL_Quit();
    return 0;
}
