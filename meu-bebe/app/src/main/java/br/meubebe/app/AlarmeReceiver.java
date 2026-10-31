package br.meubebe.app;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

/** Dispara na hora marcada e mostra o aviso. */
public class AlarmeReceiver extends BroadcastReceiver {
    @Override
    public void onReceive(Context c, Intent it) {
        Avisos.mostrar(c, it.getStringExtra("titulo"), it.getStringExtra("texto"),
                it.getBooleanExtra("chamado", false));
    }
}
