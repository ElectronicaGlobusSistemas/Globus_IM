#include <Arduino.h>
#include <WiFi.h>

#define WIFI_MODE WIFI_STA

const char *ssid = "GLOBUS-DESARROLLO";
const char *password = "Globus2020*";

unsigned long previousMillis = 0;
unsigned long interval = 30000;

void CONNECT_WIFI(void)
{

  WiFi.mode(WIFI_MODE); // MODO STA.
  IPAddress Local_IP(192, 168, 5, 137);
  IPAddress primaryDNS(8, 8, 8, 8);   // optional
  IPAddress secondaryDNS(8, 8, 4, 4); // optional

  if (!WiFi.config(Local_IP, primaryDNS, secondaryDNS))
  {
    Serial.println("STA Failed to configure"); // mensaje Monitor Serial.
  }
  Serial.print("Connecting to.... ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  Serial.println("");
  Serial.println("WiFi connected!");
  Serial.print("Conectado a: ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("ESP Mac Address: ");
  Serial.println(WiFi.macAddress());
}
void RECONNECT_WIFI(void)
{

  unsigned long currentMillis = millis();
  // if WiFi is down, try reconnecting every CHECK_WIFI_TIME seconds
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >= interval))
  {
    Serial.print(millis());
    Serial.println("Reconnecting to WiFi...");
    WiFi.reconnect();

    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("");
      Serial.println("WiFi connected!");
      Serial.print("Conectado a: ");
      Serial.println(ssid);
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      Serial.print("ESP Mac Address: ");
      Serial.println(WiFi.macAddress());
    }

    previousMillis = currentMillis;
  }
}
