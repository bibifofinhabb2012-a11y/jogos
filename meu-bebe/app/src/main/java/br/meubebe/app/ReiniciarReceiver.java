package br.meubebe.app;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

/** Reagenda os avisos quando o celular é ligado de novo. */
public class ReiniciarReceiver extends BroadcastReceiver {
    @Override
    public void onReceive(Context c, Intent it) {
        if (Intent.ACTION_BOOT_COMPLETED.equals(it.getAction())) Avisos.reagendar(c);
    }
}
