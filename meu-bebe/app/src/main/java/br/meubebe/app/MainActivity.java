package br.meubebe.app;

import android.Manifest;
import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.webkit.WebResourceRequest;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;

/** Tela única: o jogo roda numa WebView; a ponte "Android" agenda avisos e vibra. */
public class MainActivity extends Activity implements SensorEventListener {
    private WebView web;
    private SensorManager sensores;
    private Sensor movimento;
    private boolean semGravidade;
    private final float[] gravidade = new float[3];
    private long ultimoPico;

    @Override
    protected void onCreate(Bundle estado) {
        super.onCreate(estado);
        Avisos.criarCanais(this);

        web = new WebView(this);
        WebSettings ws = web.getSettings();
        ws.setJavaScriptEnabled(true);
        ws.setDomStorageEnabled(true);
        ws.setMediaPlaybackRequiresUserGesture(false);
        ws.setAllowFileAccess(true);
        web.setWebViewClient(new WebViewClient() {
            @Override
            public boolean shouldOverrideUrlLoading(WebView v, WebResourceRequest req) {
                Uri u = req.getUrl();
                if ("file".equals(u.getScheme())) return false;
                // links externos abrem no navegador
                try { startActivity(new Intent(Intent.ACTION_VIEW, u)); } catch (Exception e) { /* sem navegador */ }
                return true;
            }
        });
        web.addJavascriptInterface(new Ponte(this), "Android");
        setContentView(web);
        web.loadUrl("file:///android_asset/index.html");

        // balançar o celular é dançar com o bebê no colo
        sensores = (SensorManager) getSystemService(SENSOR_SERVICE);
        if (sensores != null) {
            movimento = sensores.getDefaultSensor(Sensor.TYPE_LINEAR_ACCELERATION);
            semGravidade = movimento != null;
            if (movimento == null) movimento = sensores.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
        }

        if (Build.VERSION.SDK_INT >= 33
                && checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS) != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(new String[]{Manifest.permission.POST_NOTIFICATIONS}, 1);
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        web.onResume();
        if (movimento != null) sensores.registerListener(this, movimento, SensorManager.SENSOR_DELAY_GAME);
        Avisos.cancelarTudo(this);
        web.evaluateJavascript("window.aoVoltar && window.aoVoltar()", null);
    }

    @Override
    protected void onPause() {
        // o jogo calcula quando o bebê vai precisar de atenção e chama Android.agendar
        web.evaluateJavascript("window.aoSair && window.aoSair()", null);
        if (sensores != null) sensores.unregisterListener(this);
        web.onPause();
        super.onPause();
    }

    /** Manda ao jogo só os picos de movimento (no máximo um a cada 250 ms), com a força em m/s². */
    @Override
    public void onSensorChanged(SensorEvent e) {
        float x = e.values[0], y = e.values[1], z = e.values[2];
        if (!semGravidade) {
            // sem sensor de aceleração linear: tira a gravidade com um filtro simples
            for (int i = 0; i < 3; i++) gravidade[i] = 0.9f * gravidade[i] + 0.1f * e.values[i];
            x -= gravidade[0]; y -= gravidade[1]; z -= gravidade[2];
        }
        double forca = Math.sqrt(x * x + y * y + z * z);
        long agora = System.currentTimeMillis();
        if (forca > 3.0 && agora - ultimoPico > 250) {
            ultimoPico = agora;
            web.evaluateJavascript("window.balancou && window.balancou(" + String.format(java.util.Locale.US, "%.1f", forca) + ")", null);
        }
    }

    @Override
    public void onAccuracyChanged(Sensor s, int precisao) { }

    @Override
    @SuppressWarnings("deprecation")
    public void onBackPressed() {
        web.evaluateJavascript("window.voltarAndroid ? String(window.voltarAndroid()) : 'false'", v -> {
            if (!"\"true\"".equals(v)) moveTaskToBack(true);
        });
    }
}
