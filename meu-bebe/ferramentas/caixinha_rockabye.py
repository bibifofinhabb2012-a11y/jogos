import numpy as np, wave
import os
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)),'caixinha_brahms.py')).read().split('Q=0.6')[0])  # freq() e constantes
Q=0.62
# Rock-a-bye Baby (melodia tradicional, domínio público), Dó maior, 3/4. Escrita de memória.
q,c,m,h,p=1,.5,1.5,2,3   # semínima, colcheia, semínima pontuada, mínima, mínima pontuada
mel=[('E5',m),('G5',c),('E5',q), ('D5',h),('C5',q), ('E5',m),('G5',c),('E5',q), ('D5',p),
     ('D5',m),('F5',c),('D5',q), ('C5',h),('B4',q), ('D5',m),('F5',c),('D5',q), ('C5',p),
     ('E5',m),('G5',c),('C6',q), ('B5',h),('A5',q), ('G5',m),('A5',c),('G5',q), ('F5',h),('F5',q),
     ('F5',m),('F5',c),('E5',q), ('D5',h),('C5',q), ('D5',m),('E5',c),('D5',q), ('C5',p)]
acordes=['C','C','C','G', 'G','C','G','C', 'C','G','C','F', 'F','G','G','C']
raiz={'C':['C4','G4','E4'],'F':['F4','C5','A4'],'G':['G4','D5','B4']}
total=16*3*Q+2.5
x=np.zeros(int(total*sr))
def tine(t0,f,vol,dec=1.6):
    n=int(min(dec*2.2,total-t0)*sr); t=np.arange(n)/sr
    s=(np.sin(2*np.pi*f*t)*np.exp(-t/dec)+0.25*np.sin(2*np.pi*f*2.0*t)*np.exp(-t/(dec*0.4))
       +0.12*np.sin(2*np.pi*f*5.43*t)*np.exp(-t/0.08)+0.06*np.sin(2*np.pi*f*9.1*t)*np.exp(-t/0.03))
    s*=np.minimum(1,t/0.002)*(1+0.004*np.sin(2*np.pi*5*t))
    i=int(t0*sr); x[i:i+n]+=vol*s
t=0
for nota,d in mel: tine(t,freq(nota),0.5); t+=d*Q
assert abs(t-16*3*Q)<1e-6, t/Q
for b,cd in enumerate(acordes):
    for k,nn in enumerate(raiz[cd]): tine((b*3+k)*Q, freq(nn), 0.22 if k==0 else 0.13, 1.2)
x/=np.abs(x).max()*1.12
with wave.open('rockabye.wav','wb') as w:
    w.setnchannels(1); w.setsampwidth(2); w.setframerate(sr); w.writeframes((x*32767).astype(np.int16).tobytes())
print('duração', round(total,1))
