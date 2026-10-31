# Meu Bebê — app Android

Jogo de cuidar de um bebê, acessível com TalkBack. O jogo em si é a página `app/src/main/assets/index.html`; o código Java só abre essa página e cuida das notificações.

## Como gerar o APK

Opção 1, no computador com o Android Studio ou o Android SDK instalado:

    ./gradlew assembleRelease

O APK sai em `app/build/outputs/apk/release/app-release.apk`.

Opção 2, sem instalar nada: o fluxo em `.github/workflows/meu-bebe-apk.yml`, na raiz do repositório, gera o APK sozinho a cada envio. Ele fica para baixar na aba Actions, em "meu-bebe-apk".

O APK é assinado com a chave de depuração: serve para instalar direto no celular (é preciso permitir "instalar apps de fontes desconhecidas"), não para publicar na Play Store.

## Como funcionam os avisos

- Ao sair do app, o jogo simula o futuro e descobre quando o bebê vai precisar de atenção e quando a situação fica crítica.
- A partir do primeiro momento de atenção, chega uma notificação a cada 10 minutos (até 6 horas).
- Depois do momento crítico, as notificações viram chamados: canal de alta prioridade, três bipes três vezes e vibração.
- Ao abrir o app, os avisos pendentes são cancelados e o jogo conta o que aconteceu enquanto você esteve fora.
- Pausar o tempo, no menu Opções, para o bebê e os avisos.

O Android pode atrasar alarmes em modo de economia de bateria; o intervalo mínimo garantido é de cerca de 9 minutos. Para chegar na hora certa, desative a otimização de bateria do Meu Bebê nas configurações do celular.

## Créditos dos sons

Veja "Como jogar", dentro do app.
