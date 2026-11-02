#include "audio.h"
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <map>
#include <random>

#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"

namespace fs = std::filesystem;

namespace audio {
static ALCdevice* dispositivo = nullptr;
static ALCcontext* contexto = nullptr;
static std::map<std::string, std::vector<ALuint>> banco;  // grupo -> variações
static std::vector<ALuint> fontes;                          // fontes de toque único, reaproveitadas
static std::map<int, ALuint> lacos;                         // fontes contínuas e móveis
static std::map<int, std::pair<std::string, float>> lacoInfo;  // categoria e volume pedido de cada uma
static std::map<std::string, float> volumes = {{"ambiente", 1}, {"passos", 1}, {"efeitos", 1}, {"interface", 1}, {"musica", 1}, {"voz", 1}};

static std::string categoriaDe(const std::string& nome) {
    if (nome.rfind("passos", 0) == 0) return "passos";
    static const char* amb[] = {"vento", "corrego", "grilos", "cigarra", "passaros", "sapos", "galo", "geladeira", "carro"};
    for (const char* a : amb) if (nome == a) return "ambiente";
    static const char* intf[] = {"gota", "bip", "chegou", "menu-abrir", "menu-fechar", "tutorial-ok", "chat"};
    for (const char* a : intf) if (nome == a) return "interface";
    if (nome.rfind("musica", 0) == 0) return "musica";
    return "efeitos";
}
float volume(const std::string& c) { auto it = volumes.find(c); return it == volumes.end() ? 1.0f : it->second; }
void definirVolume(const std::string& c, float v) {
    volumes[c] = v;
    for (auto& [id, info] : lacoInfo) if (info.first == c && lacos.count(id)) alSourcef(lacos[id], AL_GAIN, info.second * v);
}
static int proximoId = 1;
static std::mt19937 sorte{std::random_device{}()};

bool ativo() { return contexto != nullptr; }

// Converte do jogo (x leste, y norte, z cima) para o OpenAL (x direita, y cima, -z frente).
static void paraAL(float x, float y, float z, float out[3]) { out[0] = x; out[1] = z; out[2] = -y; }

static ALuint carregarMp3(const fs::path& arquivo) {
    drmp3_config cfg;
    drmp3_uint64 quadros = 0;
    drmp3_int16* pcm = drmp3_open_file_and_read_pcm_frames_s16(arquivo.string().c_str(), &cfg, &quadros, nullptr);
    if (!pcm) return 0;
    // som 3D precisa ser mono: mistura os canais
    std::vector<drmp3_int16> mono(quadros);
    for (drmp3_uint64 i = 0; i < quadros; i++) {
        int soma = 0;
        for (drmp3_uint32 c = 0; c < cfg.channels; c++) soma += pcm[i * cfg.channels + c];
        mono[i] = (drmp3_int16)(soma / (int)cfg.channels);
    }
    drmp3_free(pcm, nullptr);
    ALuint buf = 0;
    alGenBuffers(1, &buf);
    alBufferData(buf, AL_FORMAT_MONO16, mono.data(), (ALsizei)(mono.size() * sizeof(drmp3_int16)), (ALsizei)cfg.sampleRate);
    return buf;
}

// "passos-grama-3.mp3" pertence ao grupo "passos-grama"; "corrego.mp3", ao grupo "corrego".
static std::string grupoDe(const std::string& nome) {
    auto p = nome.find_last_of('-');
    if (p != std::string::npos && p + 1 < nome.size() && std::isdigit((unsigned char)nome[p + 1])) return nome.substr(0, p);
    return nome;
}

bool iniciar(const std::string& pastaSons) {
    dispositivo = alcOpenDevice(nullptr);
    if (!dispositivo) return false;
    ALCint atributos[] = {ALC_HRTF_SOFT, ALC_TRUE, 0};
    contexto = alcCreateContext(dispositivo, atributos);
    if (!contexto || !alcMakeContextCurrent(contexto)) { encerrar(); return false; }
    alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
    std::error_code erro;
    for (auto& e : fs::directory_iterator(pastaSons, erro)) {
        if (e.path().extension() != ".mp3") continue;
        ALuint b = carregarMp3(e.path());
        if (b) banco[grupoDe(e.path().stem().string())].push_back(b);
    }
    fontes.resize(48);
    alGenSources((ALsizei)fontes.size(), fontes.data());
    return true;
}

void encerrar() {
    if (contexto) {
        for (auto& [id, f] : lacos) { alSourceStop(f); alDeleteSources(1, &f); }
        lacos.clear();
        if (!fontes.empty()) alDeleteSources((ALsizei)fontes.size(), fontes.data());
        fontes.clear();
        for (auto& [g, v] : banco) alDeleteBuffers((ALsizei)v.size(), v.data());
        banco.clear();
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(contexto);
        contexto = nullptr;
    }
    if (dispositivo) { alcCloseDevice(dispositivo); dispositivo = nullptr; }
}

bool existe(const std::string& nome) { return banco.count(nome) > 0; }

static ALuint buffer(const std::string& nome) {
    auto it = banco.find(nome);
    if (it == banco.end() || it->second.empty()) return 0;
    std::uniform_int_distribution<size_t> d(0, it->second.size() - 1);
    return it->second[d(sorte)];
}

void ouvinte(float x, float y, float z, float rumo) {
    if (!contexto) return;
    float p[3]; paraAL(x, y, z, p);
    alListenerfv(AL_POSITION, p);
    float r = rumo * 3.14159265f / 180.0f;
    float frente[3]; paraAL(std::sin(r), std::cos(r), 0, frente);
    float ori[6] = {frente[0], frente[1], frente[2], 0, 1, 0};
    alListenerfv(AL_ORIENTATION, ori);
}

static ALuint fonteLivre() {
    for (ALuint f : fontes) {
        ALint estado; alGetSourcei(f, AL_SOURCE_STATE, &estado);
        if (estado != AL_PLAYING) return f;
    }
    return fontes.empty() ? 0 : fontes[0];
}

static void configurar(ALuint f, bool em3D, float x, float y, float z, float volume, float alcance, float tom) {
    alSourcef(f, AL_GAIN, volume);
    alSourcef(f, AL_PITCH, tom);
    alSourcei(f, AL_SOURCE_RELATIVE, em3D ? AL_FALSE : AL_TRUE);
    float p[3] = {0, 0, 0};
    if (em3D) paraAL(x, y, z, p);
    alSourcefv(f, AL_POSITION, p);
    alSourcef(f, AL_REFERENCE_DISTANCE, alcance);
    alSourcef(f, AL_MAX_DISTANCE, alcance * 40);
    alSourcef(f, AL_ROLLOFF_FACTOR, em3D ? 1.0f : 0.0f);
}

void tocar(const std::string& nome, float volume, float tom) {
    if (!contexto) return;
    ALuint b = buffer(nome); if (!b) return;
    ALuint f = fonteLivre(); alSourceStop(f);
    alSourcei(f, AL_BUFFER, (ALint)b); alSourcei(f, AL_LOOPING, AL_FALSE);
    configurar(f, false, 0, 0, 0, volume * audio::volume(categoriaDe(nome)), 1, tom);
    alSourcePlay(f);
}

void tocar3D(const std::string& nome, float x, float y, float z, float volume, float alcance, float tom) {
    if (!contexto) return;
    ALuint b = buffer(nome); if (!b) return;
    ALuint f = fonteLivre(); alSourceStop(f);
    alSourcei(f, AL_BUFFER, (ALint)b); alSourcei(f, AL_LOOPING, AL_FALSE);
    configurar(f, true, x, y, z, volume * audio::volume(categoriaDe(nome)), alcance, tom);
    alSourcePlay(f);
}

static int novaFonte(const std::string& nome, bool em3D, float x, float y, float z, float volume, float alcance, bool repetir) {
    if (!contexto) return 0;
    ALuint b = buffer(nome); if (!b) return 0;
    ALuint f = 0; alGenSources(1, &f);
    alSourcei(f, AL_BUFFER, (ALint)b);
    alSourcei(f, AL_LOOPING, repetir ? AL_TRUE : AL_FALSE);
    std::string cat = categoriaDe(nome);
    configurar(f, em3D, x, y, z, volume * audio::volume(cat), alcance, 1.0f);
    alSourcePlay(f);
    int id = proximoId++;
    lacos[id] = f;
    lacoInfo[id] = {cat, volume};
    return id;
}

int laco(const std::string& nome, bool em3D, float volume, float alcance) { return novaFonte(nome, em3D, 0, 0, 0, volume, alcance, true); }
int fonteMovel(const std::string& nome, float x, float y, float z, float volume, float alcance, bool repetir) { return novaFonte(nome, true, x, y, z, volume, alcance, repetir); }

void lacoPosicao(int id, float x, float y, float z) {
    auto it = lacos.find(id); if (it == lacos.end()) return;
    float p[3]; paraAL(x, y, z, p); alSourcefv(it->second, AL_POSITION, p);
}
void moverFonte(int id, float x, float y, float z) { lacoPosicao(id, x, y, z); }
void lacoVolume(int id, float v) {
    auto it = lacos.find(id); if (it == lacos.end()) return;
    lacoInfo[id].second = v;
    alSourcef(it->second, AL_GAIN, v * audio::volume(lacoInfo[id].first));
}
void lacoTom(int id, float tom) { auto it = lacos.find(id); if (it != lacos.end()) alSourcef(it->second, AL_PITCH, tom); }
void pararFonte(int id) {
    auto it = lacos.find(id); if (it == lacos.end()) return;
    alSourceStop(it->second); alDeleteSources(1, &it->second); lacos.erase(it); lacoInfo.erase(id);
}
}
