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

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothProfile;
import java.util.UUID;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanResult;

public class MainActivity extends AppCompatActivity {

    // 1. AS VARIÁVEIS AGORA ESTÃO DENTRO DA CLASSE (Correto)
    private FusedLocationProviderClient fusedLocationClient;
    private static final String URL_SERVIDOR = "https://eowuf16t6ge1o8.m.pipedream.net";

    private static final UUID SERVICE_UUID = UUID.fromString("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
    private static final UUID CHARACTERISTIC_UUID = UUID.fromString("beb5483e-36e1-4688-b7f5-ea07361b26a8");

    private BluetoothGatt bluetoothGatt;
    private BluetoothGattCharacteristic espCharacteristic;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Button btnSimular = new Button(this);
        btnSimular.setText("SIMULAR QUEDA (TESTE MANUAL)");
        btnSimular.setTextSize(18f);
        setContentView(btnSimular);

        fusedLocationClient = LocationServices.getFusedLocationProviderClient(this);

        // Pede TODAS as permissões de uma vez (GPS e Bluetooth)
        if (ActivityCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED ||
                ActivityCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_SCAN) != PackageManager.PERMISSION_GRANTED) {

            ActivityCompat.requestPermissions(this, new String[]{
                    Manifest.permission.ACCESS_FINE_LOCATION,
                    Manifest.permission.BLUETOOTH_SCAN,
                    Manifest.permission.BLUETOOTH_CONNECT
            }, 100);
        } else {
            // Se já tem tudo liberado, começa a procurar!
            procurarDispositivoESP32();
        }

        btnSimular.setOnClickListener(v -> {
            obterLocalizacaoEEnviar();
        });
    }

    private void procurarDispositivoESP32() {
        BluetoothAdapter bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        if (bluetoothAdapter == null) return;

        BluetoothLeScanner scanner = bluetoothAdapter.getBluetoothLeScanner();
        if (scanner == null) {
            Log.d("TESTE", "Bluetooth está desligado ou indisponível.");
            return;
        }

        Log.d("TESTE", "Radar ligado! Procurando pelo Detector_De_Queda...");
        Toast.makeText(this, "Procurando cinto ESP32...", Toast.LENGTH_LONG).show();

        ScanCallback scanCallback = new ScanCallback() {
            @SuppressLint("MissingPermission")
            @Override
            public void onScanResult(int callbackType, ScanResult result) {
                BluetoothDevice device = result.getDevice();

                if (device.getName() != null && device.getName().equals("Detector_De_Queda")) {
                    Log.d("TESTE", "ESP32 Encontrado! Parando escaneamento e conectando...");
                    scanner.stopScan(this);
                    bluetoothGatt = device.connectGatt(MainActivity.this, false, gattCallback);
                }
            }
        };

        scanner.startScan(scanCallback);
    }

    private final BluetoothGattCallback gattCallback = new BluetoothGattCallback() {
        @SuppressLint("MissingPermission")
        @Override
        public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                Log.d("TESTE", "Conectado ao ESP32! Buscando serviços...");
                gatt.discoverServices();
            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                System.out.println("ESP32 Desconectado.");
            }
        }

        @SuppressLint("MissingPermission")
        @Override
        public void onServicesDiscovered(BluetoothGatt gatt, int status) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                BluetoothGattService service = gatt.getService(SERVICE_UUID);
                if (service != null) {
                    espCharacteristic = service.getCharacteristic(CHARACTERISTIC_UUID);
                    if (espCharacteristic != null) {
                        gatt.setCharacteristicNotification(espCharacteristic, true);
                        Log.d("TESTE", "Serviço e Característica encontrados com sucesso! Escutando...");
                    }
                }
            }
        }

        @Override
        public void onCharacteristicChanged(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic) {
            String valorRecebido = new String(characteristic.getValue());

            if (valorRecebido.equals("1")) {
                Log.d("TESTE", "Sinal de EMERGÊNCIA recebido do ESP32 via Bluetooth!");

                // 2. A MÁGICA CONECTADA: Roda na Thread principal para o Toast funcionar e aciona o GPS
                runOnUiThread(() -> obterLocalizacaoEEnviar());
            }
        }
    };

    @SuppressLint("MissingPermission")
    private void obterLocalizacaoEEnviar() {
        Toast.makeText(this, "Buscando GPS e Endereço...", Toast.LENGTH_SHORT).show();

        fusedLocationClient.getLastLocation().addOnSuccessListener(this, location -> {
            if (location != null) {
                enviarParaServidor(location);
            } else {
                Toast.makeText(this, "Erro: Ligue o GPS do celular.", Toast.LENGTH_LONG).show();
            }
        });
    }

    private void enviarParaServidor(android.location.Location location) {
        new Thread(() -> {
            try {
                Geocoder geocoder = new Geocoder(MainActivity.this, Locale.getDefault());
                List<Address> enderecos = geocoder.getFromLocation(location.getLatitude(), location.getLongitude(), 1);

                String enderecoLegivel = "Endereço não identificado";
                if (enderecos != null && !enderecos.isEmpty()) {
                    enderecoLegivel = enderecos.get(0).getAddressLine(0);
                }

                String dados = "QUEDA DETECTADA!\n" +
                        "Coordenadas: " + location.getLatitude() + ", " + location.getLongitude() + "\n" +
                        "Local: " + enderecoLegivel;

                URL url = new URL(URL_SERVIDOR);
                HttpURLConnection conn = (HttpURLConnection) url.openConnection();
                conn.setRequestMethod("POST");

                // Formato padrão de texto (ideal para o Pipedream)
                conn.setRequestProperty("Content-Type", "text/plain; charset=UTF-8");
                conn.setDoOutput(true);

                try (OutputStream os = conn.getOutputStream()) {
                    byte[] input = dados.getBytes("utf-8");
                    os.write(input, 0, input.length);
                }

                int code = conn.getResponseCode();
                Log.d("TESTE", "Enviado para o Pipedream! Status: " + code);
                conn.disconnect();

                resetarHardwareESP32();

            } catch (Exception e) {
                Log.e("TESTE", "Erro no envio: " + e.getMessage());
            }
        }).start();
    }

    @SuppressLint("MissingPermission")
    private void resetarHardwareESP32() {
        if (bluetoothGatt != null && espCharacteristic != null) {
            String comandoDeReset = "RESET_MEDALHAO";

            espCharacteristic.setValue(comandoDeReset.getBytes());
            espCharacteristic.setWriteType(BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT);

            boolean sucesso = bluetoothGatt.writeCharacteristic(espCharacteristic);

            if (sucesso) {
                System.out.println("Comando RESET_MEDALHAO enviado. ESP32 voltando ao NORMAL.");
            } else {
                System.out.println("Falha ao enviar o reset.");
            }
        }
    }
}