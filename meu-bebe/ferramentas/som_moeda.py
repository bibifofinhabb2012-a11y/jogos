import numpy as np, wave
from scipy.signal import butter, sosfilt
sr=44100; rng=np.random.default_rng(2)
def sino(f0, dur, vol, parciais=((1,1),(2.76,.5),(5.4,.3),(8.93,.15))):
    t=np.arange(int(dur*sr))/sr; s=np.zeros_like(t)
    for r,a in parciais: s+=a*np.sin(2*np.pi*f0*r*t)*np.exp(-t/(dur*0.35/r**0.4))
    return vol*s*np.minimum(1,t/0.001)
n=int(1.1*sr); x=np.zeros(n)
# estalo da gaveta: batida curta de metal e um baque grave
k=int(0.04*sr); clique=sosfilt(butter(2,[1500,6000],'bandpass',fs=sr,output='sos'),rng.random(k)*2-1)*np.exp(-np.arange(k)/(0.006*sr))
x[:k]+=0.6*clique
t=np.arange(int(0.12*sr))/sr; x[int(0.01*sr):int(0.01*sr)+len(t)]+=0.35*np.sin(2*np.pi*140*t)*np.exp(-t/0.03)
# o "tlin": sino agudo e um segundo sino logo depois
a=sino(2350,1.0,0.7); i=int(0.07*sr); x[i:i+len(a)]+=a[:n-i]
b=sino(3140,0.8,0.45); i=int(0.11*sr); x[i:i+len(b)]+=b[:n-i]
x=x/np.abs(x).max()*0.85
with wave.open('moeda.wav','wb') as w:
    w.setnchannels(1); w.setsampwidth(2); w.setframerate(sr); w.writeframes((x*32767).astype(np.int16).tobytes())
