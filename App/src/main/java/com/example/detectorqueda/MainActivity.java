package com.example.detectorqueda;

import android.Manifest;
import android.annotation.SuppressLint;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.util.Log;
import android.widget.Button;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;

import com.google.android.gms.location.FusedLocationProviderClient;
import com.google.android.gms.location.LocationServices;

import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;

import android.location.Address;
import android.location.Geocoder;
import java.util.List;
import java.util.Locale;

public class MainActivity extends AppCompatActivity {

    private FusedLocationProviderClient fusedLocationClient;
    private static final String URL_SERVIDOR = "https://webhook.site/15ee0d3c-00a5-47d0-bce7-17dfdf9637b7";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Criando a interface visual
        Button btnSimular = new Button(this);
        btnSimular.setText("SIMULAR QUEDA E ENVIAR LOCALIZAÇÃO");
        btnSimular.setTextSize(18f);
        setContentView(btnSimular);

        fusedLocationClient = LocationServices.getFusedLocationProviderClient(this);

        btnSimular.setOnClickListener(v -> {
            // Pede permissão se o usuário ainda não tiver dado
            if (ActivityCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.ACCESS_FINE_LOCATION}, 100);
            } else {
                obterLocalizacaoEEnviar();
            }
        });
    }

    @SuppressLint("MissingPermission")
    private void obterLocalizacaoEEnviar() {
        Toast.makeText(this, "Buscando GPS e Endereço...", Toast.LENGTH_SHORT).show();

        fusedLocationClient.getLastLocation().addOnSuccessListener(this, location -> {
            if (location != null) {
                // Passamos o objeto 'location' completo para o método de envio
                enviarParaServidor(location);
            } else {
                Toast.makeText(this, "Erro: Ligue o GPS do celular.", Toast.LENGTH_LONG).show();
            }
        });
    }

    private void enviarParaServidor(android.location.Location location) {
        new Thread(() -> {
            try {
                // 1. Usar o Geocoder para transformar Lat/Lon em texto
                Geocoder geocoder = new Geocoder(MainActivity.this, Locale.getDefault());
                List<Address> enderecos = geocoder.getFromLocation(location.getLatitude(), location.getLongitude(), 1);

                String enderecoLegivel = "Endereço não identificado";

                // Se encontrou um endereço, pega a primeira linha completa formatada
                if (enderecos != null && !enderecos.isEmpty()) {
                    enderecoLegivel = enderecos.get(0).getAddressLine(0);
                }

                // 2. Montar a mensagem final do log
                String dados = "QUEDA DETECTADA!\n" +
                        "Coordenadas: " + location.getLatitude() + ", " + location.getLongitude() + "\n" +
                        "Local: " + enderecoLegivel;

                // 3. Fazer o envio HTTP para o Webhook
                URL url = new URL(URL_SERVIDOR);
                HttpURLConnection conn = (HttpURLConnection) url.openConnection();
                conn.setRequestMethod("POST");
                conn.setRequestProperty("Content-Type", "text/plain; charset=UTF-8");
                conn.setDoOutput(true);

                try (OutputStream os = conn.getOutputStream()) {
                    byte[] input = dados.getBytes("utf-8");
                    os.write(input, 0, input.length);
                }

                int code = conn.getResponseCode();
                Log.d("TESTE", "Enviado! Status: " + code);
                conn.disconnect();

            } catch (Exception e) {
                Log.e("TESTE", "Erro: " + e.getMessage());
            }
        }).start();
    }
}