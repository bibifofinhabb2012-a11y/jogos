// Áudio 3D com OpenAL Soft e HRTF: com fone de ouvido, o som vem de frente, de trás, de cima,
// da esquerda ou da direita, como na vida.
// Coordenadas do jogo: x cresce para o leste, y para o norte, z para cima, em metros.
#pragma once
#include <string>
#include <vector>

namespace audio {
bool iniciar(const std::string& pastaSons);  // false: sem som, o jogo continua
void encerrar();
bool ativo();

// Posição e direção de quem ouve. rumo em graus: 0 norte, 90 leste, 180 sul, 270 oeste.
void ouvinte(float x, float y, float z, float rumo);

// Toca uma vez. Com variações (passos-grama-1, -2...), sorteia uma.
void tocar(const std::string& nome, float volume = 1.0f, float tom = 1.0f);  // grudado na cabeça de quem ouve
void tocar3D(const std::string& nome, float x, float y, float z, float volume = 1.0f, float alcance = 8.0f, float tom = 1.0f);

// Som contínuo (córrego, vento, grilos). Devolve um número para mexer nele depois.
int laco(const std::string& nome, bool em3D, float volume = 1.0f, float alcance = 10.0f);
void lacoPosicao(int id, float x, float y, float z);
void lacoVolume(int id, float volume);
void lacoTom(int id, float tom);

// Som 3D de uma vez que se move (o carro do pai indo embora).
int fonteMovel(const std::string& nome, float x, float y, float z, float volume, float alcance, bool repetir);
void moverFonte(int id, float x, float y, float z);
void pararFonte(int id);
bool existe(const std::string& nome);
}
