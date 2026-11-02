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
        {SDLK_l, Tecla::L}, {SDLK_b, Tecla::B}, {SDLK_v, Tecla::V}, {SDLK_3, Tecla::Tres}, {SDLK_KP_3, Tecla::Tres}, {SDLK_F2, Tecla::F2}, {SDLK_h, Tecla::H}, {SDLK_q, Tecla::Q}, {SDLK_r, Tecla::R},
        {SDLK_1, Tecla::Um}, {SDLK_KP_1, Tecla::Um}, {SDLK_2, Tecla::Dois}, {SDLK_KP_2, Tecla::Dois},
        {SDLK_RETURN, Tecla::Enter}, {SDLK_KP_ENTER, Tecla::Enter}, {SDLK_SPACE, Tecla::Espaco},
        {SDLK_ESCAPE, Tecla::Esc}, {SDLK_F1, Tecla::F1},
    };
    auto it = mapa.find(k);
    if (it == mapa.end()) return false;
    t = it->second;
    return true;
}

// Roteiro de teste: uma ação por linha.
//   tecla cima | tecla shift enter | segurar cima 3 | esperar 60 | horas 5 | novo
static int rodarRoteiro(const char* arquivo) {
    static const std::map<std::string, Tecla> nomes = {
        {"cima", Tecla::Cima}, {"baixo", Tecla::Baixo}, {"esquerda", Tecla::Esquerda}, {"direita", Tecla::Direita},
        {"i", Tecla::I}, {"u", Tecla::U}, {"s", Tecla::S}, {"e", Tecla::E}, {"t", Tecla::T}, {"l", Tecla::L}, {"b", Tecla::B}, {"v", Tecla::V}, {"3", Tecla::Tres}, {"f2", Tecla::F2},
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
            auto it = nomes.find(n);
            if (it == nomes.end()) { std::printf("tecla desconhecida: %s\n", n.c_str()); continue; }
            jogo.tecla(it->second, shift, ctrl);
            jogo.atualizar(0.05);
            if (cmd == "segurar") {
                double seg = 1; s >> seg;
                for (double t = 0; t < seg; t += 0.05) jogo.atualizar(0.05);
                jogo.soltar(it->second);
            } else jogo.soltar(it->second);
        } else if (cmd == "esperar" || cmd == "horas") {
            double seg = 0; s >> seg;
            if (cmd == "horas") seg *= 300;  // uma hora do jogo = 5 minutos de verdade
            for (double t = 0; t < seg; t += 0.5) jogo.atualizar(0.5);
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
            if (ev.type == SDL_KEYDOWN && !ev.key.repeat && traduzir(ev.key.keysym.sym, t))
                jogo.tecla(t, (ev.key.keysym.mod & KMOD_SHIFT) != 0, (ev.key.keysym.mod & KMOD_CTRL) != 0);
            if (ev.type == SDL_KEYUP && traduzir(ev.key.keysym.sym, t)) jogo.soltar(t);
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
