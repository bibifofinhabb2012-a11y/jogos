#include "voz.h"
#include <AL/al.h>
#include <AL/alc.h>
#include <opus.h>
#include <cmath>
#include <deque>
#include <map>
#include "audio.h"

namespace voz {
constexpr int TAXA = 16000, QUADRO = 320;   // 20 ms
static ALCdevice* microfone = nullptr;
static OpusEncoder* codificador = nullptr;
static std::vector<short> sobra;
static int falando = 0;                      // quadros de "rabo" depois da última voz, para não cortar o fim da palavra

struct Ouvinte {                             // a voz de um jogador
    OpusDecoder* dec = nullptr;
    ALuint fonte = 0;
    std::deque<ALuint> livres;
    float x = 0, y = 0;
    bool audivel = true;
};
static std::map<int, Ouvinte> vozes;

bool iniciarCaptura() {
    if (microfone) return true;
    microfone = alcCaptureOpenDevice(nullptr, TAXA, AL_FORMAT_MONO16, TAXA / 2);
    if (!microfone) return false;
    int erro = 0;
    if (!codificador) {
        codificador = opus_encoder_create(TAXA, 1, OPUS_APPLICATION_VOIP, &erro);
        if (erro != OPUS_OK) { codificador = nullptr; alcCaptureCloseDevice(microfone); microfone = nullptr; return false; }
        opus_encoder_ctl(codificador, OPUS_SET_BITRATE(16000));
    }
    alcCaptureStart(microfone);
    return true;
}

void pararCaptura() {
    if (microfone) { alcCaptureStop(microfone); alcCaptureCloseDevice(microfone); microfone = nullptr; }
    sobra.clear();
}

void capturar(const std::function<void(const unsigned char*, int)>& enviar) {
    if (!microfone || !codificador) return;
    ALCint n = 0;
    alcGetIntegerv(microfone, ALC_CAPTURE_SAMPLES, 1, &n);
    if (n <= 0) return;
    size_t antes = sobra.size();
    sobra.resize(antes + (size_t)n);
    alcCaptureSamples(microfone, sobra.data() + antes, n);
    size_t pos = 0;
    while (sobra.size() - pos >= QUADRO) {
        const short* q = sobra.data() + pos;
        double soma = 0;
        for (int i = 0; i < QUADRO; i++) soma += (double)q[i] * q[i];
        double rms = std::sqrt(soma / QUADRO);
        if (rms > 500) falando = 15; else if (falando > 0) falando--;  // só manda enquanto tem voz, e um pouquinho depois
        if (falando > 0) {
            unsigned char pacote[400];
            int b = opus_encode(codificador, q, QUADRO, pacote, sizeof pacote);
            if (b > 0) enviar(pacote, b);
        }
        pos += QUADRO;
    }
    sobra.erase(sobra.begin(), sobra.begin() + (long)pos);
}

static void colocar(Ouvinte& o) {
    if (!o.fonte) return;
    float p[3] = {o.x, 1.6f, -o.y};  // mesma conversão do audio.cpp: x leste, altura, norte para a frente
    alSourcefv(o.fonte, AL_POSITION, p);
    alSourcef(o.fonte, AL_GAIN, o.audivel ? audio::volume("voz") * 1.4f : 0.0f);
}

void receber(int id, const unsigned char* dados, int n, float x, float y) {
    if (!audio::ativo()) return;
    Ouvinte& o = vozes[id];
    if (!o.dec) {
        int erro = 0;
        o.dec = opus_decoder_create(TAXA, 1, &erro);
        if (erro != OPUS_OK) { o.dec = nullptr; return; }
        alGenSources(1, &o.fonte);
        alSourcef(o.fonte, AL_REFERENCE_DISTANCE, 3.0f);
        alSourcef(o.fonte, AL_MAX_DISTANCE, 40.0f);
        alSourcef(o.fonte, AL_ROLLOFF_FACTOR, 1.5f);
        ALuint b[12]; alGenBuffers(12, b);
        for (ALuint k : b) o.livres.push_back(k);
    }
    o.x = x; o.y = y;
    colocar(o);
    // devolve os buffers que já tocaram
    ALint feitos = 0; alGetSourcei(o.fonte, AL_BUFFERS_PROCESSED, &feitos);
    while (feitos-- > 0) { ALuint b; alSourceUnqueueBuffers(o.fonte, 1, &b); o.livres.push_back(b); }
    if (o.livres.empty()) return;  // atrasou demais: descarta este pedaço
    short pcm[QUADRO * 3];
    int amostras = opus_decode(o.dec, dados, n, pcm, QUADRO * 3, 0);
    if (amostras <= 0) return;
    ALuint b = o.livres.front(); o.livres.pop_front();
    alBufferData(b, AL_FORMAT_MONO16, pcm, amostras * (int)sizeof(short), TAXA);
    alSourceQueueBuffers(o.fonte, 1, &b);
    ALint estado = 0, fila = 0;
    alGetSourcei(o.fonte, AL_SOURCE_STATE, &estado);
    alGetSourcei(o.fonte, AL_BUFFERS_QUEUED, &fila);
    if (estado != AL_PLAYING && fila >= 3) alSourcePlay(o.fonte);  // espera 60 ms de folga antes de tocar
}

void posicionar(int id, float x, float y, bool audivel) {
    auto it = vozes.find(id);
    if (it == vozes.end()) return;
    it->second.x = x; it->second.y = y; it->second.audivel = audivel;
    colocar(it->second);
}

void encerrar() {
    pararCaptura();
    for (auto& [id, o] : vozes) {
        if (o.fonte) { alSourceStop(o.fonte); alDeleteSources(1, &o.fonte); }
        if (o.dec) opus_decoder_destroy(o.dec);
    }
    vozes.clear();
    if (codificador) { opus_encoder_destroy(codificador); codificador = nullptr; }
}

int codificarTeste(const short* pcm, unsigned char* saida, int max) {
    int erro = 0;
    if (!codificador) { codificador = opus_encoder_create(TAXA, 1, OPUS_APPLICATION_VOIP, &erro); opus_encoder_ctl(codificador, OPUS_SET_BITRATE(16000)); }
    return opus_encode(codificador, pcm, QUADRO, saida, max);
}
int decodificarTeste(const unsigned char* dados, int n, short* pcm) {
    static OpusDecoder* d = nullptr; int erro = 0;
    if (!d) d = opus_decoder_create(TAXA, 1, &erro);
    return opus_decode(d, dados, n, pcm, QUADRO * 3, 0);
}
}
