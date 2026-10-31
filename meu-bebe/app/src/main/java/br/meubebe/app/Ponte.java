package br.meubebe.app;

import android.content.Context;
import android.os.Build;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.webkit.JavascriptInterface;

import org.json.JSONArray;

/** Métodos que o jogo chama pelo objeto window.Android. */
public class Ponte {
    private final Context ctx;

    Ponte(Context ctx) { this.ctx = ctx.getApplicationContext(); }

    /** Recebe a lista de avisos [{quando, chamado, titulo, texto}] e agenda todos. */
    @JavascriptInterface
    public void agendar(String json) { Avisos.agendar(ctx, json); }

    @JavascriptInterface
    public void cancelar() { Avisos.cancelarTudo(ctx); }

    @JavascriptInterface
    public void testar(String titulo, String texto) { Avisos.mostrar(ctx, titulo, texto, false); }

    /** Padrão em milissegundos: vibra, pausa, vibra... */
    @JavascriptInterface
    @SuppressWarnings("deprecation")
    public void vibrar(String json) {
        try {
            JSONArray a = new JSONArray(json);
            long[] padrao = new long[a.length() + 1];
            padrao[0] = 0;
            for (int i = 0; i < a.length(); i++) padrao[i + 1] = a.getLong(i);
            Vibrator v = (Vibrator) ctx.getSystemService(Context.VIBRATOR_SERVICE);
            if (v == null || !v.hasVibrator()) return;
            if (Build.VERSION.SDK_INT >= 26) v.vibrate(VibrationEffect.createWaveform(padrao, -1));
            else v.vibrate(padrao, -1);
        } catch (Exception e) { /* padrão inválido */ }
    }
}
