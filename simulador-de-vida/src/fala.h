// Fala: tudo o que o jogo diz passa por aqui.
// No Windows, vai para o leitor de tela que estiver aberto (NVDA, JAWS e outros) pela biblioteca Tolk,
// ou para a voz do Windows (SAPI) se nenhum leitor estiver rodando.
// Em outros sistemas, ou no modo de teste, o texto é escrito no terminal.
#pragma once
#include <string>

namespace fala {
void iniciar(bool modoTeste);
void encerrar();
// interromper: corta o que estiver sendo dito. false: entra na fila, depois do que já está sendo falado.
void dizer(const std::string& texto, bool interromper = true);
void calar();
const std::string& ultima();
}
