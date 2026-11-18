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

# Volume igual para todos: o "alvo" de cada chamada é ignorado; todos os toques vão para ALVO_TOQUE
# e os sons contínuos para ALVO_LACO, medindo só a parte que soa (sem os silêncios do começo e do fim).
ALVO_TOQUE = -20.0
ALVO_LACO = -26.0

def rms_ativo(x):
    jan = max(1, int(0.05 * TAXA))
    n = len(x) // jan
    if n < 2: return np.sqrt(np.mean(x ** 2)) + 1e-9
    r = np.sqrt(np.mean(x[:n * jan].reshape(n, jan) ** 2, axis=1))
    r = np.sort(r)[::-1]
    return np.mean(r[:max(1, int(np.ceil(n * 0.4)))]) + 1e-9  # os 40% de janelas mais fortes

def limitar(x):
    """Sons secos (palmas, porta) têm pico alto e corpo baixo: deixa o pico passar até 6 dB do teto
    e arredonda com um limitador suave, para o corpo chegar perto do volume dos outros."""
    pico = np.max(np.abs(x))
    if pico > 1.9: x = x * 1.9 / pico
    return 0.95 * np.tanh(x / 0.95)

def gravar(x, nome, alvo_db=-20.0):
    x = x - np.mean(x)
    x = x * (10 ** (ALVO_TOQUE / 20) / rms_ativo(x))
    x = limitar(x)
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
    x = x * (10 ** (ALVO_LACO / 20) / rms_ativo(x))
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
trecho("sv-crocante", 554708, "mastigar-1", alvo=-18); u(554708)
trecho("sv-crocante", 507453, "mastigar-2", alvo=-18); u(507453)
trecho("sv-mochila", 778083, "mochila", 0, 1.6, -22); u(778083)
trecho("sv-moedas", 853705, "moedas", alvo=-20); u(853705)
trecho("sv-saco-dormir", 776275, "saco-dormir", 0, 3.0, -22); u(776275)
trecho("sv-cerca", 406494, "cerca", 0, 1.2, -18); u(406494)
trecho("sv-parede", 457741, "parede", alvo=-18); u(457741)
trecho("sv-porta", 125958, "porta", alvo=-20); u(125958)
trecho("sv-porta2", 67176, "porta-abrir", 0, 3.2, -20); u(67176)
trecho("sv-porta2", 550409, "porta-fechar", alvo=-18); u(550409)
# o lado engraçado do corpo
def forte(grupo, sid, nome, dur, alvo):
    x = ler(arquivo(grupo, sid)); jan = int(0.05 * TAXA)
    env = np.convolve(np.abs(x), np.ones(jan) / jan, mode="same")
    ini = max(0, int(np.argmax(env)) - int(0.3 * TAXA))
    gravar(x[ini:ini + int(dur * TAXA)].copy(), nome, alvo)
forte("sv-peido", 419108, "peido-1", 2.5, -16); u(419108)
trecho("sv-peido", 685269, "peido-2", alvo=-16); u(685269)
forte("sv-peido", 419105, "peido-3", 2.2, -16); u(419105)
laco("sv-xixi", 151912, "xixi", -20, dur=10); u(151912)
trecho("sv-coco", 436396, "coco-1", alvo=-18); u(436396)
print("cocô", passos("sv-coco", 776534, "coco", 3, 0.9, -18, inicio_n=2)); u(776534)
trecho("sv-porteira", 637583, "porteira", 0, 3.0, -20); u(637583)
print("água", passos("sv-agua", 328168, "passos-agua", 5, 0.5)); u(328168)
for i, sid in enumerate([435508, 435507, 435506], 1): trecho("sv-galo", sid, f"galo-{i}", alvo=-20); u(sid)
laco("sv-carro", 401552, "carro", -18); u(401552)

# casa: cozinha, banheiro, móveis
laco("sv-geladeira", 413566, "geladeira", -24); u(413566)
trecho("sv-geladeira-porta", 530824, "geladeira-porta", 0, 2.5, -20); u(530824)
trecho("sv-fogao", 751151, "fogao", 0, 3.5, -20); u(751151)
trecho("sv-fogao", 751151, "fogao-desligar", 0, 0.5, -24)
laco("sv-chama", 816922, "chama", -24); u(816922)
trecho("sv-torneira", 235618, "torneira", alvo=-20); u(235618)
trecho("sv-descarga", 625609, "descarga", alvo=-18); u(625609)
laco("sv-chuveiro", 802544, "chuveiro", -20); u(802544)
trecho("sv-box", 828148, "box", alvo=-20); u(828148)
for i, sid in enumerate([406703, 406700, 406699], 1): trecho("sv-arrastar", sid, f"arrastar-{i}", alvo=-20); u(sid)
trecho("sv-cama", 443796, "cama", alvo=-20); u(443796)
# roça e café
for i, sid in enumerate([275123, 275118, 275116, 275115], 1): trecho("sv-machado", sid, f"machado-{i}", alvo=-18); u(sid)
trecho("sv-cavar", 353907, "cavar", 0, 1.8, -20); u(353907)
trecho("sv-regador", 532890, "regador", 0, 2.5, -20); u(532890)
trecho("sv-colher", 699492, "colher", alvo=-18); u(699492)
trecho("sv-cafe", 432775, "cafe", 0, 3.0, -20); u(432775)
trecho("sv-gole", 272037, "gole", alvo=-18); u(272037)
trecho("sv-sementes", 235278, "sementes", alvo=-20); u(235278)
# cidade, viagem e vícios
laco("sv-trator", 256807, "trator", -18, dur=12); u(256807)
laco("sv-bar", 415974, "bar", -22); u(415974)
trecho("sv-isqueiro", 827018, "isqueiro", alvo=-18); u(827018)
trecho("sv-tosse", 743354, "tosse", alvo=-18); u(743354)
# bar e casamento
trecho("sv-vomito", 246308, "vomito", alvo=-16); u(246308)
trecho("sv-cerveja", 390337, "cerveja", alvo=-18); u(390337)
forte("sv-tragada", 853043, "tragada", 3.0, -22); u(853043)
laco("sv-festa", 246117, "festa", -22); u(246117)
trecho("sv-aplausos", 462362, "aplausos", alvo=-18); u(462362)
trecho("sv-sinos", 807688, "sinos", 0, 6.0, -20); u(807688)
# curral e cozinha
trecho("sv-mugido", 513557, "mugido-1", alvo=-18); u(513557)
trecho("sv-mugido", 700378, "mugido-2", alvo=-18); u(700378)
trecho("sv-mugido", 698969, "mugido-3", 0, 4.0, -18); u(698969)
trecho("sv-coice", 264062, "coice", alvo=-14); u(264062)
trecho("sv-baque", 504626, "baque", alvo=-16); u(504626)
trecho("sv-faca", 336588, "faca", alvo=-20); u(336588)
trecho("sv-fritura", 234469, "fritura", 0, 4.0, -20); u(234469)
trecho("sv-leite", 565964, "leite", 0, 5.0, -20); u(565964)
trecho("sv-aterrissar", 646658, "aterrissar-1", alvo=-16); u(646658)
trecho("sv-aterrissar", 646662, "aterrissar-2", alvo=-16); u(646662)
trecho("sv-picareta", 728758, "picareta-1", alvo=-16); u(728758)
trecho("sv-picareta", 728759, "picareta-2", alvo=-16); u(728759)
trecho("sv-picareta", 728757, "picareta-3", alvo=-16); u(728757)
trecho("sv-arvore", 494071, "arvore", 0, 7.0, -16); u(494071)
# cozinha e paiol
trecho("sv-moedor", 723041, "moedor", 0, 5.0); u(723041)
trecho("sv-expresso2", 578411, "expresso", 0, 7.0); u(578411)
trecho("sv-vapor2", 868483, "vaporizador", 0, 6.0); u(868483)
trecho("sv-moenda3", 395041, "moenda", 0, 6.0); u(395041)
trecho("sv-prensa", 590156, "prensa"); u(590156)
# bichos pequenos e obras
trecho("sv-galinha", 668804, "galinha-1"); u(668804)
trecho("sv-galinha", 668805, "galinha-2"); u(668805)
trecho("sv-galinha", 572955, "galinha-3"); u(572955)
trecho("sv-porco", 620082, "porco-1"); u(620082)
trecho("sv-porco", 620083, "porco-2"); u(620083)
trecho("sv-martelo", 707525, "martelo"); u(707525)
trecho("sv-serra", 258055, "serra", 0, 4.0); u(258055)
laco("sv-fogueira", 869081, "fogueira"); u(869081)
trecho("sv-fungar", 618486, "fungar"); u(618486)
trecho("sv-ronco", 447911, "ronco-1"); u(447911)
trecho("sv-ronco", 52293, "ronco-2", 0, 3.0); u(52293)
trecho("sv-ofegar", 682882, "ofegar", 0, 3.5); u(682882)
# bip descendente da energia baixa: feito aqui, sem gravação (um "piu" de 1000 para 500 Hz em 0,3 s)
def bip_descendo(nome):
    n = int(0.3 * TAXA); t = np.arange(n) / TAXA
    f = 1000 * (0.5 ** (t / 0.3))
    fase = 2 * np.pi * np.cumsum(f) / TAXA
    env = np.minimum(1, t / 0.006) * np.exp(-t * 6)
    gravar((np.sin(fase) * env).astype(np.float32), nome)
bip_descendo("energia")
trecho("sv-sofa", 556591, "sofa", alvo=-20); u(556591)

# sons de navegação, gerados para o jogo: gota suave, bip do rastreio, chegada, menu, tutorial
def tons(partes, nome, alvo=-22):
    pedacos = []
    for f0, f1, dur, dec in partes:
        t = np.arange(int(dur * TAXA)) / TAXA
        f = np.linspace(f0, f1, len(t))
        pedacos.append(np.sin(2 * np.pi * np.cumsum(f) / TAXA) * np.exp(-t * dec))
    gravar(np.concatenate(pedacos).astype(np.float32), nome, alvo)
tons([(900, 1500, 0.07, 45)], "gota", -26)
tons([(1000, 1000, 0.09, 30)], "bip", -20)
tons([(660, 660, 0.12, 12), (990, 990, 0.25, 9)], "chegou", -20)
tons([(500, 800, 0.08, 25)], "menu-abrir", -26)
tons([(800, 500, 0.08, 25)], "menu-fechar", -26)
tons([(523, 523, 0.1, 10), (659, 659, 0.1, 10), (784, 784, 0.3, 7)], "tutorial-ok", -22)
tons([(1175, 1175, 0.06, 30), (1568, 1568, 0.12, 22)], "chat", -24)

# bússola precária: a agulha chacoalhando na caixinha de lata
x = ler(arquivo("sv-bussola", 421154))
jan = int(0.05 * TAXA); env = np.convolve(np.abs(x), np.ones(jan) / jan, mode="same")
meio = int(np.argmax(env)); ini = max(0, meio - int(0.5 * TAXA))
gravar(x[ini:ini + int(1.1 * TAXA)].copy(), "bussola", -20); u(421154)
# ruído branco do sono, gerado: suavizado (passa-baixa leve) e com volume baixo
rng = np.random.default_rng(7); r = rng.standard_normal(12 * TAXA).astype(np.float32)
r = np.convolve(r, np.ones(4) / 4, mode="same")
c = TAXA; r = np.concatenate([r[-c:] * np.linspace(1, 0, c) + r[:c] * np.linspace(0, 1, c), r[c:-c]])
gravar(r, "ruido", -26)

# créditos
cred = {x["id"]: x for x in json.load(open(os.path.join(ORIGEM, "creditos.json")))}
with open(os.path.join(DESTINO, "CREDITOS.txt"), "w", encoding="utf-8") as f:
    f.write("Sons do Simulador de Vida. Todos do Freesound (freesound.org), com licença CC0 (domínio público) ou CC BY (pede crédito).\n")
    f.write("O ruído branco do sono e os sons de navegação (gota, bip, chegada, menu, tutorial, chat) foram gerados para o jogo.\n\n")
    for sid in dict.fromkeys(usados):
        c = cred.get(sid)
        if not c: continue
        lic = "CC0" if "zero" in c["licenca"] else "CC BY " + c["licenca"].rstrip("/").split("/")[-1]
        f.write(f'"{c["titulo"]}", de {c["autor"]}. freesound.org/s/{sid}. {lic}.\n')
print("pronto:", len([f for f in os.listdir(DESTINO) if f.endswith('.mp3')]), "arquivos")
