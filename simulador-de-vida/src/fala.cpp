#include "fala.h"
#include <cstdio>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "Tolk.h"
#endif

namespace fala {
static bool teste = false;
static std::string ultimaFala;

#ifdef _WIN32
static std::wstring largo(const std::string& s) {
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring w(n > 0 ? n - 1 : 0, L'\0');
    if (n > 1) MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, w.data(), n);
    return w;
}
#endif

void iniciar(bool modoTeste) {
    teste = modoTeste;
#ifdef _WIN32
    if (!teste) {
        Tolk_TrySAPI(true);
        Tolk_PreferSAPI(false);
        Tolk_Load();
    }
#endif
}

void encerrar() {
#ifdef _WIN32
    if (!teste) Tolk_Unload();
#endif
}

void dizer(const std::string& texto, bool interromper) {
    if (texto.empty()) return;
    ultimaFala = texto;
#ifdef _WIN32
    if (!teste) { Tolk_Output(largo(texto).c_str(), interromper); return; }
#endif
    std::printf("%s %s\n", interromper ? "[fala]" : "[fila]", texto.c_str());
    std::fflush(stdout);
}

void calar() {
#ifdef _WIN32
    if (!teste) Tolk_Silence();
#endif
}

const std::string& ultima() { return ultimaFala; }
}
