# Repositório de jogos acessíveis

Jogos feitos para quem joga com leitor de tela. A pessoa que pede e testa os jogos é cega, usa TalkBack no Android e conversa em português do Brasil. Acessibilidade não é extra: todo elemento precisa funcionar só com som e texto.

Antes de mexer no Meu Bebê, leia `meu-bebe/ANOTACOES.md`: tem a estrutura do jogo, como publicar o APK, as restrições do ambiente, o que já existe e o que ficou combinado para depois.

## Regras rápidas

- Fonte da verdade do Meu Bebê: `meu-bebe/web/meu-bebe.html`. Copie para `meu-bebe/app/src/main/assets/index.html` em toda mudança.
- Cada versão nova: suba `versionCode` e `versionName` em `meu-bebe/app/build.gradle`, faça push na `main` e mande o link da execução do GitHub Actions (item `meu-bebe-apk`).
- Sons novos vão em `meu-bebe/sons/` e entram no jogo com `python3 meu-bebe/ferramentas/embutir_sons.py`. Registre autor e licença nos créditos de "Como jogar".
- Nada de chaves, senhas ou keystores no repositório: ele é público.
- Não suponha o gênero de quem joga nos textos.
- Freesound: a rede da sessão do Claude recebe 403 do Freesound (mesmo com acesso completo). A busca roda no GitHub Actions: fluxo "Buscar sons no Freesound" (`.github/workflows/freesound.yml`), que usa o segredo `FREESOUND_API_KEY` do repositório e salva os candidatos na branch `sons-candidatos`, com `creditos.json`. Depois, puxe a branch, passe no classificador e mande para ela só os melhores.
