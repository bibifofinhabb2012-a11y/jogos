package br.meubebe.app;

import android.Manifest;
import android.app.AlarmManager;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.media.AudioAttributes;
import android.net.Uri;
import android.os.Build;

import org.json.JSONArray;
import org.json.JSONObject;

/** Canais, alarmes e notificações. */
final class Avisos {
    static final String CANAL_LEMBRETE = "lembretes";
    static final String CANAL_CHAMADO = "chamados";
    private static final String PREFS = "avisos";
    private static final int MAX = 48;
    private static final int ID_LEMBRETE = 1, ID_CHAMADO = 2;
    static final long[] VIBRACAO_CHAMADO = {0, 250, 120, 250, 120, 250, 400, 250, 120, 250, 120, 250};

    private Avisos() {}

    private static Uri som(Context c, int raw) {
        return Uri.parse("android.resource://" + c.getPackageName() + "/" + raw);
    }

    static void criarCanais(Context c) {
        if (Build.VERSION.SDK_INT < 26) return;
        NotificationManager nm = c.getSystemService(NotificationManager.class);
        AudioAttributes aa = new AudioAttributes.Builder()
                .setUsage(AudioAttributes.USAGE_NOTIFICATION)
                .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
                .build();

        NotificationChannel lembrete = new NotificationChannel(CANAL_LEMBRETE,
                c.getString(R.string.canal_lembrete), NotificationManager.IMPORTANCE_DEFAULT);
        lembrete.setDescription(c.getString(R.string.canal_lembrete_desc));
        lembrete.setSound(som(c, R.raw.lembrete), aa);

        NotificationChannel chamado = new NotificationChannel(CANAL_CHAMADO,
                c.getString(R.string.canal_chamado), NotificationManager.IMPORTANCE_HIGH);
        chamado.setDescription(c.getString(R.string.canal_chamado_desc));
        chamado.setSound(som(c, R.raw.chamado), aa);
        chamado.enableVibration(true);
        chamado.setVibrationPattern(VIBRACAO_CHAMADO);

        nm.createNotificationChannel(lembrete);
        nm.createNotificationChannel(chamado);
    }

    private static PendingIntent alarme(Context c, int i, Intent it, int flags) {
        return PendingIntent.getBroadcast(c, 1000 + i, it, flags | PendingIntent.FLAG_IMMUTABLE);
    }

    static void agendar(Context c, String json) {
        cancelarTudo(c);
        c.getSharedPreferences(PREFS, Context.MODE_PRIVATE).edit().putString("lista", json).apply();
        AlarmManager am = (AlarmManager) c.getSystemService(Context.ALARM_SERVICE);
        long agora = System.currentTimeMillis();
        try {
            JSONArray a = new JSONArray(json);
            for (int i = 0; i < a.length() && i < MAX; i++) {
                JSONObject o = a.getJSONObject(i);
                long quando = o.getLong("quando");
                if (quando <= agora) continue;
                Intent it = new Intent(c, AlarmeReceiver.class)
                        .putExtra("titulo", o.optString("titulo"))
                        .putExtra("texto", o.optString("texto"))
                        .putExtra("chamado", o.optBoolean("chamado"));
                PendingIntent pi = alarme(c, i, it, PendingIntent.FLAG_UPDATE_CURRENT);
                // inexato, mas atravessa o modo de economia; o Android garante no máximo um a cada ~9 min
                am.setAndAllowWhileIdle(AlarmManager.RTC_WAKEUP, quando, pi);
            }
        } catch (Exception e) { /* lista inválida: nada agendado */ }
    }

    static void cancelarTudo(Context c) {
        AlarmManager am = (AlarmManager) c.getSystemService(Context.ALARM_SERVICE);
        for (int i = 0; i < MAX; i++) {
            PendingIntent pi = alarme(c, i, new Intent(c, AlarmeReceiver.class), PendingIntent.FLAG_NO_CREATE);
            if (pi != null) { am.cancel(pi); pi.cancel(); }
        }
        c.getSharedPreferences(PREFS, Context.MODE_PRIVATE).edit().remove("lista").apply();
        NotificationManager nm = (NotificationManager) c.getSystemService(Context.NOTIFICATION_SERVICE);
        nm.cancel(ID_LEMBRETE);
        nm.cancel(ID_CHAMADO);
    }

    /** Depois de reiniciar o celular, os alarmes somem; reagenda os que ainda estão no futuro. */
    static void reagendar(Context c) {
        String json = c.getSharedPreferences(PREFS, Context.MODE_PRIVATE).getString("lista", null);
        if (json != null) agendar(c, json);
    }

    @SuppressWarnings("deprecation")
    static void mostrar(Context c, String titulo, String texto, boolean chamado) {
        if (Build.VERSION.SDK_INT >= 33
                && c.checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS) != PackageManager.PERMISSION_GRANTED) return;
        Intent abrir = new Intent(c, MainActivity.class)
                .setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_SINGLE_TOP);
        PendingIntent pa = PendingIntent.getActivity(c, 0, abrir,
                PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE);

        Notification.Builder nb = Build.VERSION.SDK_INT >= 26
                ? new Notification.Builder(c, chamado ? CANAL_CHAMADO : CANAL_LEMBRETE)
                : new Notification.Builder(c);
        nb.setSmallIcon(R.drawable.ic_notificacao)
                .setContentTitle(titulo)
                .setContentText(texto)
                .setStyle(new Notification.BigTextStyle().bigText(texto))
                .setAutoCancel(true)
                .setContentIntent(pa)
                .setCategory(chamado ? Notification.CATEGORY_ALARM : Notification.CATEGORY_REMINDER);
        if (Build.VERSION.SDK_INT < 26) {
            nb.setSound(som(c, chamado ? R.raw.chamado : R.raw.lembrete))
                    .setPriority(chamado ? Notification.PRIORITY_HIGH : Notification.PRIORITY_DEFAULT);
            if (chamado) nb.setVibrate(VIBRACAO_CHAMADO);
        }
        NotificationManager nm = (NotificationManager) c.getSystemService(Context.NOTIFICATION_SERVICE);
        // o aviso novo substitui o anterior do mesmo tipo, para não empilhar
        nm.notify(chamado ? ID_CHAMADO : ID_LEMBRETE, nb.build());
    }
}
