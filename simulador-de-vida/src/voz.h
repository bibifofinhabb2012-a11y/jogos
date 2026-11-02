// Conversa por voz no jogo online: o microfone vira pacotinhos Opus, a rede leva, e a voz de cada
// jogador toca em 3D, de onde o personagem dele está. Perto ouve bem; longe some; em outro mapa, nada.
#pragma once
#include <functional>
#include <vector>

namespace voz {
bool iniciarCaptura();
void pararCaptura();
// lê o microfone e chama enviar para cada pacote de 20 ms com voz (silêncio não vai)
void capturar(const std::function<void(const unsigned char*, int)>& enviar);
void receber(int id, const unsigned char* dados, int n, float x, float y);
void posicionar(int id, float x, float y, bool audivel);
void encerrar();
// para testes: codifica 20 ms de som (320 amostras a 16 kHz) e decodifica de volta
int codificarTeste(const short* pcm, unsigned char* saida, int max);
int decodificarTeste(const unsigned char* dados, int n, short* pcm);
}
