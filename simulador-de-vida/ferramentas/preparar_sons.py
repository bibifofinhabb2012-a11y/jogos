"""Prepara os sons do Freesound para o Simulador de Vida.
Lê as gravações baixadas (branch sons-candidatos), corta, nivela o volume, converte para mono
(som 3D precisa ser mono) e grava em simulador-de-vida/sons/<nome>-<n>.mp3.
Passos: detecta cada pisada na gravação e corta uma por arquivo.
Uso: python3 preparar_sons.py /caminho/para/sons-candidatos"""
import json, os, subprocess, sys
import numpy as np

ORIGEM = sys.argv[1]
DESTINO = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "sons")
TAXA = 44100

def ler(caminho):
    raw = subprocess.run(["ffmpeg", "-v", "quiet", "-i", caminho, "-ac", "1", "-ar", str(TAXA), "-f", "s16le", "-"], capture_output=True).stdout
    return np.frombuffer(raw, dtype=np.int16).astype(np.float32) / 32768

def gravar(x, nome, alvo_db=-20.0):
    x = x - np.mean(x)
    rms = np.sqrt(np.mean(x ** 2)) + 1e-9
    x = x * (10 ** (alvo_db / 20) / rms)
    pico = np.max(np.abs(x))
    if pico > 0.95: x = x * 0.95 / pico
    f = int(0.006 * TAXA)
    x[:f] *= np.linspace(0, 1, f); x[-f * 4:] *= np.linspace(1, 0, f * 4)
    pcm = (x * 32767).astype(np.int16).tobytes()
    subprocess.run(["ffmpeg", "-v", "error", "-y", "-f", "s16le", "-ar", str(TAXA), "-ac", "1", "-i", "-", "-b:a", "64k",
                    os.path.join(DESTINO, nome + ".mp3")], input=pcm, check=True)

def arquivo(grupo, sid):
    return os.path.join(ORIGEM, grupo, f"{sid}.mp3")

def passos(grupo, sid, nome, quantos=6, dur=0.42, alvo=-22.0, inicio_n=1):
    """Acha os picos de energia (as pisadas) e corta cada um."""
    x = ler(arquivo(grupo, sid))
    jan = int(0.01 * TAXA)
    env = np.sqrt(np.convolve(x ** 2, np.ones(jan) / jan, mode="same"))
    limiar = np.percentile(env, 90) * 0.5
    pisadas, ultimo = [], -TAXA
    for i in range(1, len(env)):
        if env[i] > limiar and env[i - 1] <= limiar and i - ultimo > 0.28 * TAXA:
            pisadas.append(i); ultimo = i
    # as mais fortes, sem as que encostam no fim
    pisadas = [p for p in pisadas if p + dur * TAXA < len(x)]
    pisadas.sort(key=lambda p: -np.max(env[p:p + int(0.1 * TAXA)]))
    n = inicio_n
    for p in sorted(pisadas[:quantos]):
        ini = max(0, p - int(0.03 * TAXA))
        gravar(x[ini:ini + int(dur * TAXA)].copy(), f"{nome}-{n}", alvo); n += 1
    return n - inicio_n

def trecho(grupo, sid, nome, ini=0.0, dur=None, alvo=-20.0):
    x = ler(arquivo(grupo, sid))
    a = int(ini * TAXA); b = len(x) if dur is None else min(len(x), a + int(dur * TAXA))
    gravar(x[a:b].copy(), nome, alvo)

def laco(grupo, sid, nome, alvo=-22.0, dur=None):
    """Som contínuo: emenda o fim no começo com um cruzamento de 1 segundo, para repetir sem estalo."""
    x = ler(arquivo(grupo, sid))
    if dur: x = x[:int(dur * TAXA)]
    c = TAXA
    if len(x) > 3 * c:
        fim = x[-c:] * np.linspace(1, 0, c) + x[:c] * np.linspace(0, 1, c)
        x = np.concatenate([fim, x[c:-c]])
    x = x - np.mean(x)
    rms = np.sqrt(np.mean(x ** 2)) + 1e-9
    x = x * (10 ** (alvo / 20) / rms)
    pico = np.max(np.abs(x))
    if pico > 0.95: x = x * 0.95 / pico
    pcm = (x * 32767).astype(np.int16).tobytes()
    subprocess.run(["ffmpeg", "-v", "error", "-y", "-f", "s16le", "-ar", str(TAXA), "-ac", "1", "-i", "-", "-b:a", "64k",
                    os.path.join(DESTINO, nome + ".mp3")], input=pcm, check=True)

def fatias(grupo, sid, nome, dur, quantas, alvo=-22.0):
    """Corta uma gravação longa em pedaços para tocar soltos (pássaros, sapos)."""
    x = ler(arquivo(grupo, sid))
    passo = max(1, (len(x) - int(dur * TAXA)) // max(1, quantas))
    for k in range(quantas):
        a = k * passo
        gravar(x[a:a + int(dur * TAXA)].copy(), f"{nome}-{k + 1}", alvo)

os.makedirs(DESTINO, exist_ok=True)
for f in os.listdir(DESTINO):
    if f.endswith(".mp3"): os.remove(os.path.join(DESTINO, f))

usados = []
def u(*ids): usados.extend(ids)

# passos, um por chão
print("grama", passos("sv-passos-grama", 400123, "passos-grama", 6)); u(400123)
print("mato", passos("sv-passos-mato", 411148, "passos-mato", 6, 0.45)); u(411148)
print("terra", passos("sv-passos-terra", 571952, "passos-terra", 6)); u(571952)
print("folhas", passos("sv-passos-terra", 571957, "passos-folhas", 6)); u(571957)
for i, sid in enumerate([729196, 729197, 729198, 729199, 729200], 1): trecho("sv-passos-cascalho", sid, f"passos-cascalho-{i}", alvo=-22); u(sid)
print("calçada", passos("sv-passos-calcada", 813622, "passos-calcada", 6)); u(813622)
print("madeira", passos("sv-passos-madeira", 119914, "passos-madeira", 6)); u(119914)
print("cerâmica", passos("sv-passos-ceramica", 138059, "passos-ceramica", 6)); u(138059)
# ambiente
laco("sv-corrego", 829672, "corrego", -20); u(829672)
laco("sv-vento", 563080, "vento", -24); u(563080)
laco("sv-cigarra", 824926, "cigarra", -24); u(824926)
laco("sv-grilos", 424871, "grilos", -24); u(424871)
fatias("sv-passaros", 456122, "passaros", 4.0, 5); u(456122)
fatias("sv-sapos", 396993, "sapos", 3.0, 3); u(396993)
# objetos e ações
for i, sid in enumerate([404553, 404552, 404551, 404547], 1): trecho("sv-palmas", sid, f"palmas-{i}", alvo=-16); u(sid)
for i, sid in enumerate([590219, 590217, 590215], 1): trecho("sv-embalagem", sid, f"embalagem-{i}", alvo=-20); u(sid)
trecho("sv-tampa", 346060, "tampa", alvo=-20); u(346060)
trecho("sv-beber", 27876, "beber", alvo=-20); u(27876)
trecho("sv-mastigar", 362024, "mastigar", 0, 2.6, -20); u(362024)
trecho("sv-mochila", 778083, "mochila", 0, 1.6, -22); u(778083)
trecho("sv-moedas", 853705, "moedas", alvo=-20); u(853705)
trecho("sv-saco-dormir", 776275, "saco-dormir", 0, 3.0, -22); u(776275)
trecho("sv-cerca", 406494, "cerca", 0, 1.2, -18); u(406494)
trecho("sv-parede", 457741, "parede", alvo=-18); u(457741)
trecho("sv-porta", 125958, "porta", alvo=-20); u(125958)
trecho("sv-porteira", 637583, "porteira", 0, 3.0, -20); u(637583)
print("água", passos("sv-agua", 328168, "passos-agua", 5, 0.5)); u(328168)
for i, sid in enumerate([435508, 435507, 435506], 1): trecho("sv-galo", sid, f"galo-{i}", alvo=-20); u(sid)
laco("sv-carro", 401552, "carro", -18); u(401552)

# bússola: um clique curto sintetizado
t = np.arange(int(0.05 * TAXA)) / TAXA
gravar(np.sin(2 * np.pi * 2400 * t) * np.exp(-t * 90), "bussola", -24)

# créditos
cred = {x["id"]: x for x in json.load(open(os.path.join(ORIGEM, "creditos.json")))}
with open(os.path.join(DESTINO, "CREDITOS.txt"), "w", encoding="utf-8") as f:
    f.write("Sons do Simulador de Vida. Todos do Freesound (freesound.org), com licença CC0 (domínio público) ou CC BY (pede crédito).\n")
    f.write("O clique da bússola foi sintetizado para o jogo.\n\n")
    for sid in dict.fromkeys(usados):
        c = cred.get(sid)
        if not c: continue
        lic = "CC0" if "zero" in c["licenca"] else "CC BY " + c["licenca"].rstrip("/").split("/")[-1]
        f.write(f'"{c["titulo"]}", de {c["autor"]}. freesound.org/s/{sid}. {lic}.\n')
print("pronto:", len([f for f in os.listdir(DESTINO) if f.endswith('.mp3')]), "arquivos")
