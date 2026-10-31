package br.meubebe.app;

import android.Manifest;
import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.webkit.WebResourceRequest;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;

/** Tela única: o jogo roda numa WebView; a ponte "Android" agenda avisos e vibra. */
public class MainActivity extends Activity {
    private WebView web;

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

        if (Build.VERSION.SDK_INT >= 33
                && checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS) != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(new String[]{Manifest.permission.POST_NOTIFICATIONS}, 1);
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        web.onResume();
        Avisos.cancelarTudo(this);
        web.evaluateJavascript("window.aoVoltar && window.aoVoltar()", null);
    }

    @Override
    protected void onPause() {
        // o jogo calcula quando o bebê vai precisar de atenção e chama Android.agendar
        web.evaluateJavascript("window.aoSair && window.aoSair()", null);
        web.onPause();
        super.onPause();
    }

    @Override
    @SuppressWarnings("deprecation")
    public void onBackPressed() {
        web.evaluateJavascript("window.voltarAndroid ? String(window.voltarAndroid()) : 'false'", v -> {
            if (!"\"true\"".equals(v)) moveTaskToBack(true);
        });
    }
}
