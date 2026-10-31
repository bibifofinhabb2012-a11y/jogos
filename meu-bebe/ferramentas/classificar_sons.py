"""Confere sons com o YAMNet, classificador de sons do Google (521 categorias do AudioSet).
Uso: python3 classificar_sons.py 20,0 arquivo1.mp3 arquivo2.wav
O primeiro argumento lista categorias de interesse (índice = linha do yamnet_labels.txt menos 1):
20 choro de bebê, 14 riso de bebê, 4 balbucio, 53 arroto, 55 pum, 0 fala de adulto.
Precisa de: pip install numpy ai-edge-litert, e ffmpeg."""
import sys, subprocess, numpy as np, os
import ai_edge_litert.interpreter as I
AQUI=os.path.dirname(os.path.abspath(__file__)); MODELO=os.path.join(AQUI,'yamnet.tflite')
if not os.path.exists(MODELO):
    import urllib.request, zipfile
    urllib.request.urlretrieve('https://storage.googleapis.com/mediapipe-models/audio_classifier/yamnet/float32/1/yamnet.tflite', MODELO)
    open(os.path.join(AQUI,'yamnet_labels.txt'),'wb').write(zipfile.ZipFile(MODELO).read('yamnet_label_list.txt'))
it=I.Interpreter(MODELO); it.allocate_tensors()
ii=it.get_input_details()[0]['index']; oo=it.get_output_details()[0]['index']
L=[l.strip() for l in open(os.path.join(AQUI,'yamnet_labels.txt'))]
def load(p):
    raw=subprocess.run(['ffmpeg','-v','quiet','-i',p,'-ac','1','-ar','16000','-f','s16le','-'],capture_output=True).stdout
    return np.frombuffer(raw,dtype=np.int16).astype(np.float32)/32768
def scores(p):
    x=load(p)
    if len(x)<15600: x=np.pad(x,(0,15600-len(x)))
    out=[]
    for s in range(0,len(x)-15600+1,8000):
        it.set_tensor(ii,x[s:s+15600]); it.invoke(); out.append(it.get_tensor(oo)[0].copy())
    return np.array(out)
want=[int(a) for a in sys.argv[1].split(',')]
for p in sys.argv[2:]:
    S=scores(p); m=S.mean(0); mx=S.max(0)
    top=np.argsort(m)[::-1][:3]
    print(f"{m[want[0]]:.2f} max={mx[want[0]]:.2f} fala={m[0]:.2f} | {os.path.basename(p)} | top: "+", ".join(f"{L[t]}={m[t]:.2f}" for t in top), flush=True)
