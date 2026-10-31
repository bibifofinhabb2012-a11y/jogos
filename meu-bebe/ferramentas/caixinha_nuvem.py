import numpy as np, wave
sr=44100
SEMI={'C':0,'D':2,'E':4,'F':5,'G':7,'A':9,'B':11}
def freq(n):  # 'Eb5', 'Bb4', 'G5'
    s=SEMI[n[0]]; i=1
    if n[1]=='b': s-=1; i=2
    return 440*2**((s+12*(int(n[i:])+1)-69)/12)
Q=0.6
# "Nuvem de Algodão": valsa de ninar original, mi bemol maior, 3/4. Forma A A' B A''.
q,h,p=1,2,3
mel=[('Bb4',q),('G5',h), ('F5',q),('Eb5',q),('D5',q), ('Eb5',q),('C5',q),('Bb4',q), ('G4',p),
     ('Bb4',q),('G5',h), ('Ab5',q),('G5',q),('F5',q), ('Eb5',q),('F5',q),('G5',q), ('F5',p),
     ('Eb5',q),('C6',h), ('Bb5',q),('Ab5',q),('G5',q), ('F5',q),('G5',q),('Ab5',q), ('G5',p),
     ('Bb4',q),('G5',h), ('F5',q),('Eb5',q),('D5',q), ('C5',q),('D5',q),('F5',q), ('Eb5',p),
     # segunda parte: uma frase mais escura (dó menor), a subida ao ponto mais alto e um fecho novo
     ('G4',q),('Eb5',h), ('D5',q),('C5',q),('Bb4',q), ('C5',q),('Eb5',q),('G5',q), ('F5',p),
     ('G4',q),('Eb5',h), ('D5',q),('C5',q),('D5',q), ('Eb5',q),('F5',q),('Ab5',q), ('G5',p),
     ('Bb5',q),('Eb6',h), ('D6',q),('C6',q),('Bb5',q), ('Ab5',q),('C6',q),('Bb5',q), ('G5',p),
     ('Ab5',q),('F5',h), ('Eb5',q),('D5',q),('C5',q), ('Bb4',q),('D5',q),('F5',q), ('Eb5',p)]
acordes=['Eb','Bb','Ab','Eb', 'Eb','Ab','Eb','Bb', 'Ab','Eb','Bb','Eb', 'Eb','Bb','Bb','Eb',
         'Cm','Bb','Ab','Bb', 'Cm','Bb','Ab','Eb', 'Eb','Bb','Ab','Eb', 'Fm','Ab','Bb','Eb']
arp={'Eb':['Eb4','Bb4','G4'],'Ab':['Ab3','Eb4','C4'],'Bb':['Bb3','F4','D4'],'Cm':['C4','G4','Eb4'],'Fm':['F3','C4','Ab3']}
total=32*3*Q+2.5
x=np.zeros(int(total*sr))
def tine(t0,f,vol,dec=1.6):
    n=int(min(dec*2.2,total-t0)*sr); t=np.arange(n)/sr
    s=(np.sin(2*np.pi*f*t)*np.exp(-t/dec)+0.25*np.sin(2*np.pi*f*2.0*t)*np.exp(-t/(dec*0.4))
       +0.12*np.sin(2*np.pi*f*5.43*t)*np.exp(-t/0.08)+0.06*np.sin(2*np.pi*f*9.1*t)*np.exp(-t/0.03))
    s*=np.minimum(1,t/0.002)*(1+0.004*np.sin(2*np.pi*5*t))
    i=int(t0*sr); x[i:i+n]+=vol*s
t=0
for nota,d in mel: tine(t,freq(nota),0.5); t+=d*Q
assert abs(t-32*3*Q)<1e-6, t/Q
for b,c in enumerate(acordes):
    notas=arp[c] if b<16 else [arp[c][0],arp[c][2],arp[c][1]]
    for k,nn in enumerate(notas): tine((b*3+k)*Q, freq(nn), 0.22 if k==0 else 0.13, 1.2)
x/=np.abs(x).max()*1.12
with wave.open('nuvem2.wav','wb') as w:
    w.setnchannels(1); w.setsampwidth(2); w.setframerate(sr); w.writeframes((x*32767).astype(np.int16).tobytes())
print('duração', round(total,1), '| primeira nota', round(freq('Bb4'),1), 'Hz, tônica Eb5', round(freq('Eb5'),1), 'Hz')
