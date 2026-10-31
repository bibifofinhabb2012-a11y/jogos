import numpy as np, wave
sr=44100
N={'C':0,'D':2,'E':4,'F':5,'G':7,'A':9,'B':11}
def freq(n):  # 'E5', 'C6'
    return 440*2**((N[n[0]]+12*(int(n[1:])+1)-69)/12)
Q=0.6  # semínima em segundos
# Canção de Ninar de Brahms (Wiegenlied, op. 49 n.º 4, 1868), em Dó maior, compasso 3/4
mel=[('E5',.5),('E5',.5),
 ('G5',1.5),('E5',.5),('E5',.5), ('G5',2),('E5',.5),('G5',.5), ('C6',1),('B5',1.5),('A5',.5), ('A5',1),('G5',1),('D5',.5),('E5',.5),
 ('F5',1),('D5',1),('D5',.5),('E5',.5), ('F5',2),('D5',.5),('F5',.5), ('B5',.5),('A5',.5),('G5',1),('B5',1), ('C6',2),('C5',.5),('C5',.5),
 ('C6',2),('A5',.5),('F5',.5), ('G5',2),('E5',.5),('C5',.5), ('F5',1),('G5',1),('A5',1), ('G5',2),('C5',.5),('C5',.5),
 ('C6',2),('A5',.5),('F5',.5), ('G5',2),('E5',.5),('C5',.5), ('F5',1),('E5',1),('D5',1), ('C5',3)]
# baixo: uma nota de apoio por tempo, a partir do primeiro compasso
acordes=['C','C','F','G','G','G','G','C','F','C','F','C','F','C','G','C']
raiz={'C':['C4','G4','E4'],'F':['F4','C5','A4'],'G':['G4','D5','B4']}
total=(1+16*3)*Q+2.5
x=np.zeros(int(total*sr))
def tine(t0,f,vol,dec=1.6):
    n=int(min(dec*2.2,total-t0)*sr); t=np.arange(n)/sr
    # dente de metal: fundamental, harmônicos inarmônicos que morrem rápido, e o estalo do pino
    s=(np.sin(2*np.pi*f*t)*np.exp(-t/dec)
       +0.25*np.sin(2*np.pi*f*2.0*t)*np.exp(-t/(dec*0.4))
       +0.12*np.sin(2*np.pi*f*5.43*t)*np.exp(-t/0.08)
       +0.06*np.sin(2*np.pi*f*9.1*t)*np.exp(-t/0.03))
    s*=np.minimum(1,t/0.002)*(1+0.004*np.sin(2*np.pi*5*t))
    i=int(t0*sr); x[i:i+n]+=vol*s
t=0
for nota,d in mel:
    tine(t,freq(nota),0.5); t+=d*Q
for b,c in enumerate(acordes):
    for k,nn in enumerate(raiz[c]):
        tine((1+b*3+k)*Q, freq(nn), 0.22 if k==0 else 0.13, 1.2)
x/=np.abs(x).max()*1.12
with wave.open('caixinha.wav','wb') as w:
    w.setnchannels(1); w.setsampwidth(2); w.setframerate(sr); w.writeframes((x*32767).astype(np.int16).tobytes())
print('duração', round(total,1),'s')
