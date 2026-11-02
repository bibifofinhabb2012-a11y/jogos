package br.meubebe.app;

import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;

import java.util.Locale;

/**
 * Grava o microfone e manda ao jogo, a cada 64 ms, quatro números sobre o som:
 * volume em dB, altura da voz em Hz (0 quando não é voz), clareza (0 a 1: quanto o som
 * se repete, voz limpa perto de 1, voz áspera ou sussurro mais baixo) e taxa de cruzamentos de zero.
 * Não guarda nem envia o áudio: só esses números, e só enquanto o jogo pede.
 */
final class Voz {
    private static final int TAXA = 16000, QUADRO = 1024, MAX_MS = 15000;
    private static volatile boolean ouvindo;

    private Voz() {}

    static void comecar(MainActivity tela) {
        if (ouvindo) return;
        ouvindo = true;
        new Thread(() -> {
            AudioRecord ar = null;
            try {
                int min = AudioRecord.getMinBufferSize(TAXA, AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT);
                // VOICE_RECOGNITION: sem o controle automático de volume, para o grito soar mais alto que a fala
                ar = new AudioRecord(MediaRecorder.AudioSource.VOICE_RECOGNITION, TAXA,
                        AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT, Math.max(min, QUADRO * 4));
                if (ar.getState() != AudioRecord.STATE_INITIALIZED) return;
                ar.startRecording();
                short[] buf = new short[QUADRO];
                long fim = System.currentTimeMillis() + MAX_MS;
                while (ouvindo && System.currentTimeMillis() < fim) {
                    int lidos = 0;
                    while (lidos < QUADRO && ouvindo) {
                        int n = ar.read(buf, lidos, QUADRO - lidos);
                        if (n <= 0) break;
                        lidos += n;
                    }
                    if (lidos < QUADRO) continue;
                    double[] f = analisar(buf);
                    tela.js(String.format(Locale.US, "window.vozQuadro && window.vozQuadro(%.1f,%.1f,%.3f,%.4f)", f[0], f[1], f[2], f[3]));
                }
            } catch (Exception e) {
                /* sem permissão ou microfone ocupado: o jogo recebe vozFim e avisa */
            } finally {
                if (ar != null) {
                    try { ar.stop(); } catch (Exception e) { /* já parado */ }
                    ar.release();
                }
                ouvindo = false;
                tela.js("window.vozFim && window.vozFim()");
            }
        }, "voz").start();
    }

    static void parar() { ouvindo = false; }

    /** Volume, altura (autocorrelação entre 70 e 700 Hz), clareza e cruzamentos de zero de um quadro. */
    static double[] analisar(short[] b) {
        int n = b.length;
        double media = 0;
        for (short v : b) media += v;
        media /= n;
        double[] x = new double[n];
        double energia = 0;
        int cruzamentos = 0;
        for (int i = 0; i < n; i++) {
            x[i] = (b[i] - media) / 32768.0;
            energia += x[i] * x[i];
            if (i > 0 && (x[i] >= 0) != (x[i - 1] >= 0)) cruzamentos++;
        }
        double db = 10 * Math.log10(energia / n + 1e-12);
        int lagMin = TAXA / 700, lagMax = TAXA / 70;
        double[] r = new double[lagMax + 1];
        double maior = 0;
        for (int lag = lagMin; lag <= lagMax; lag++) {
            double s = 0, e1 = 0, e2 = 0;
            for (int i = 0; i + lag < n; i++) { s += x[i] * x[i + lag]; e1 += x[i] * x[i]; e2 += x[i + lag] * x[i + lag]; }
            r[lag] = s / Math.sqrt(e1 * e2 + 1e-12);
            if (r[lag] > maior) maior = r[lag];
        }
        // o primeiro pico perto do maior evita confundir a voz com a oitava de baixo
        int escolhido = 0;
        for (int lag = lagMin + 1; lag < lagMax; lag++) {
            if (r[lag] >= 0.9 * maior && r[lag] >= r[lag - 1] && r[lag] >= r[lag + 1]) { escolhido = lag; break; }
        }
        double hz = (maior > 0.5 && escolhido > 0) ? (double) TAXA / escolhido : 0;
        return new double[]{db, hz, Math.max(0, maior), cruzamentos / (double) n};
    }
}
