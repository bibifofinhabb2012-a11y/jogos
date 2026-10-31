"""Embute os sons da pasta meu-bebe/sons no jogo.

O jogo guarda todos os sons dentro do próprio HTML, em `const SONS = {...}`, como data URIs.
Depois de trocar, acrescentar ou apagar um .mp3 em meu-bebe/sons, rode:
    python3 meu-bebe/ferramentas/embutir_sons.py
O nome do arquivo, sem .mp3, vira a chave usada no código (ex.: choro-fome.mp3 -> SONS["choro-fome"]).
Atualiza as duas cópias do jogo: a versão web e a do app Android."""
import base64, os, re
RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PASTA = os.path.join(RAIZ, "sons")
ALVOS = [os.path.join(RAIZ, "web", "meu-bebe.html"), os.path.join(RAIZ, "app", "src", "main", "assets", "index.html")]
sons = {f[:-4]: "data:audio/mpeg;base64," + base64.b64encode(open(os.path.join(PASTA, f), "rb").read()).decode()
        for f in sorted(os.listdir(PASTA)) if f.endswith(".mp3")}
bloco = "const SONS = {\n" + ",\n".join(f'  "{k}": "{v}"' for k, v in sons.items()) + "\n};\n"
for alvo in ALVOS:
    s = open(alvo, encoding="utf-8").read()
    s, n = re.subn(r"const SONS = \{\n.*?\n\};\n", lambda m: bloco, s, flags=re.S)
    assert n == 1, f"bloco SONS não encontrado em {alvo}"
    open(alvo, "w", encoding="utf-8").write(s)
    print(f"{alvo}: {len(sons)} sons, {len(s) // 1024} KB")
