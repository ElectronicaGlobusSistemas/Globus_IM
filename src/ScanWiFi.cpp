#include "WiFi.h"

void ScanWiFi(void)
{
    int n = WiFi.scanNetworks(); // Realiza el escaneo

    if (n == 0)
    {
        Serial.println("No se encontraron redes :(");
    }
    else
    {
        Serial.printf("Se encontraron %d redes:\n", n);
        for (int i = 0; i < n; ++i)
        {
            Serial.printf("%d: %s (%d dBm) %s\n", i + 1,
                          WiFi.SSID(i).c_str(), // Nombre SSID
                          WiFi.RSSI(i),         // Potencia de señal
                          (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "abierta" : "protegida");
            delay(10);
        }
    }
}
