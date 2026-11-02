package br.meubebe.app;

import android.Manifest;
import android.app.Activity;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.webkit.JavascriptInterface;

import org.json.JSONArray;

import java.util.concurrent.FutureTask;
import java.util.concurrent.TimeUnit;

/** Métodos que o jogo chama pelo objeto window.Android. */
public class Ponte {
    private final Activity tela;
    private final Context ctx;

    Ponte(Activity tela) {
        this.tela = tela;
        this.ctx = tela.getApplicationContext();
    }

    /** Recebe a lista de avisos [{quando, chamado, titulo, texto}] e agenda todos. */
    @JavascriptInterface
    public void agendar(String json) { Avisos.agendar(ctx, json); }

    @JavascriptInterface
    public void cancelar() { Avisos.cancelarTudo(ctx); }

    @JavascriptInterface
    public void testar(String titulo, String texto) { Avisos.mostrar(ctx, titulo, texto, false); }

    // Microfone: o jogo pede, a gravação manda números (volume, altura da voz), nunca o áudio.

    @JavascriptInterface
    public boolean temMicrofone() {
        return Build.VERSION.SDK_INT < 23 || ctx.checkSelfPermission(Manifest.permission.RECORD_AUDIO) == PackageManager.PERMISSION_GRANTED;
    }

    @JavascriptInterface
    public void pedirMicrofone() {
        tela.runOnUiThread(() -> {
            if (Build.VERSION.SDK_INT >= 23) tela.requestPermissions(new String[]{Manifest.permission.RECORD_AUDIO}, MainActivity.PEDIDO_MICROFONE);
        });
    }

    @JavascriptInterface
    public void ouvir() { if (temMicrofone()) Voz.comecar((MainActivity) tela); }

    @JavascriptInterface
    public void pararDeOuvir() { Voz.parar(); }

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

    // A ponte roda numa linha de execução própria; a área de transferência e o compartilhamento
    // precisam da linha da tela.

    @JavascriptInterface
    public void copiar(String texto) {
        tela.runOnUiThread(() -> {
            ClipboardManager cm = (ClipboardManager) tela.getSystemService(Context.CLIPBOARD_SERVICE);
            if (cm != null) cm.setPrimaryClip(ClipData.newPlainText("Código do bebê", texto));
        });
    }

    @JavascriptInterface
    public String colar() {
        FutureTask<String> ler = new FutureTask<>(() -> {
            ClipboardManager cm = (ClipboardManager) tela.getSystemService(Context.CLIPBOARD_SERVICE);
            if (cm == null || !cm.hasPrimaryClip() || cm.getPrimaryClip().getItemCount() == 0) return "";
            CharSequence t = cm.getPrimaryClip().getItemAt(0).coerceToText(tela);
            return t == null ? "" : t.toString();
        });
        tela.runOnUiThread(ler);
        try { return ler.get(3, TimeUnit.SECONDS); } catch (Exception e) { return ""; }
    }

    /** Abre a lista de apps do Android (WhatsApp, e-mail, notas) para guardar o código. */
    @JavascriptInterface
    public void compartilhar(String texto) {
        tela.runOnUiThread(() -> {
            Intent envio = new Intent(Intent.ACTION_SEND)
                    .setType("text/plain")
                    .putExtra(Intent.EXTRA_SUBJECT, "Código do Meu Bebê")
                    .putExtra(Intent.EXTRA_TEXT, texto);
            try { tela.startActivity(Intent.createChooser(envio, "Guardar o código do bebê")); }
            catch (Exception e) { /* nenhum app para receber texto */ }
        });
    }
}
