import numpy as np, wave
from scipy.signal import butter, sosfilt
sr=44100; rng=np.random.default_rng(11)
def sacudida(dur, forca, densidade):
    n=int(dur*sr); t=np.arange(n)/sr
    env=np.minimum(1,t/0.008)*np.exp(-t/0.07)*forca
    imp=(rng.random(n)<densidade/sr)*(rng.random(n)*0.7+0.3)
    grao=np.exp(-np.arange(int(0.002*sr))/(0.0004*sr))
    tex=np.convolve(imp,grao)[:n]*(rng.random(n)*2-1)
    return env*(0.75*tex+0.25*(rng.random(n)*2-1))
def gerar(lo,hi,dens,nome):
    partes=[]
    for dur,forca,pausa in [(0.16,1,0.07),(0.16,.85,0.07),(0.2,1,0.16),(0.14,.8,0.06),(0.14,.75,0.06),(0.22,1,0.25)]:
        partes+= [sacudida(dur,forca,dens), np.zeros(int(pausa*sr))]
    x=np.concatenate(partes)
    x=sosfilt(butter(4,[lo,hi],'bandpass',fs=sr,output='sos'),x)
    x=x/np.abs(x).max()*0.85
    with wave.open(nome,'wb') as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(sr); w.writeframes((x*32767).astype(np.int16).tobytes())
for k,(lo,hi,d) in {'e':(2500,9000,3000),'f':(3000,12000,6000),'g':(1800,7000,2000),'h':(4000,14000,9000)}.items(): gerar(lo,hi,d,f'choc-{k}.wav')
