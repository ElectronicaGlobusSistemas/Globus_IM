#include <WiFi.h>

const char *ssid = "GLOBUS-DESARROLLO";
const char *password = "Globus2020*";
const int wifi_timeout = 10000;

unsigned long previousMillis = 0;
unsigned long interval = 30000;

const IPAddress serverIP(192, 168, 5, 208); // Direccion del servidor
uint16_t serverPort = 1001;                 // Puerto del servidor

WiFiClient client; // Declara un objeto cliente para conectarse al servidor

String buffer;

extern bool flag_dato_valido_recibido;
extern bool flag_dato_no_valido_recibido;

/***************************************************************************************************************************/
/***************************************************************************************************************************/

void CONNECT_WIFI(void)
{
  WiFi.mode(WIFI_STA); // MODO STA.

  IPAddress Local_IP(192, 168, 5, 155);
  IPAddress primaryDNS(8, 8, 8, 8);   // optional
  IPAddress secondaryDNS(8, 8, 4, 4); // optional

  if (!WiFi.config(Local_IP, primaryDNS, secondaryDNS))
  {
    Serial.println("STA Failed to configure"); // mensaje Monitor Serial.
  }

  WiFi.setSleep(false); // Desactiva la suspensión de wifi en modo STA para mejorar la velocidad de respuesta

  Serial.print("Conectando a... ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < wifi_timeout)
  {
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("\nWiFi connected!");
    Serial.print("Conectado a: ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("ESP Mac Address: ");
    Serial.println(WiFi.macAddress());
  }
  else
  {
    Serial.print("\nNo se puede conectar a... ");
    Serial.println(ssid);
  }
}

/***************************************************************************************************************************/
/***************************************************************************************************************************/

void Task_Verifica_Conexion_Wifi(void *parameter)
{
  unsigned long tiempo_inicial, tiempo_final = 0;

  for (;;)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("Wifi conectado");
      vTaskDelay(60000 / portTICK_PERIOD_MS);
      continue;
    }
    else
    {
      Serial.print("Conectando a... ");
      Serial.println(ssid);

      WiFi.begin(ssid, password);
      tiempo_inicial = millis();
      while (WiFi.status() != WL_CONNECTED && (tiempo_inicial - tiempo_final) < wifi_timeout)
      {
      }
      tiempo_final = tiempo_inicial;
      if (WiFi.status() != WL_CONNECTED)
      {
        Serial.print("\nNo se puede conectar a... ");
        Serial.println(ssid);
        vTaskDelay(30000 / portTICK_PERIOD_MS);
        continue;
      }
      else
      {
        Serial.println("\nWiFi connected!");
        Serial.print("Conectado a: ");
        Serial.println(ssid);
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        Serial.print("ESP Mac Address: ");
        Serial.println(WiFi.macAddress());
      }
    }
  }
}

/***************************************************************************************************************************/
/***************************************************************************************************************************/

void CONNECT_SERVER_TCP(void)
{
  if (WiFi.isConnected())
  {
    Serial.println("\nConectando al servidor TCP...");
    if (client.connect(serverIP, serverPort)) // Intenta acceder a la dirección de destino
    {
      Serial.println("Conexion exitosa");
      Serial.print("Conectado a Server IP: ");
      Serial.println(serverIP);
      Serial.print("Conectado a Server PORT: ");
      Serial.println(serverPort);
    }
    else
    {
      Serial.println("Acceso denegado");
      client.stop(); // Cerrar el cliente
    }
  }
}

/***************************************************************************************************************************/
/***************************************************************************************************************************/

void Task_Verifica_Conexion_Servidor_TCP(void *parameter)
{
  for (;;)
  {
    if (WiFi.isConnected())
    {
      if (client.connected())
      {
        Serial.println("Servidor conectado");
        vTaskDelay(10000 / portTICK_PERIOD_MS);
        continue;
      }

      Serial.println("Conectando al servidor TCP...");
      client.connect(serverIP, serverPort);

      if (!client.connected())
      {
        Serial.println("Acceso denegado");
        client.stop(); // Cerrar el cliente
        vTaskDelay(20000 / portTICK_PERIOD_MS);
        continue;
      }
      else
      {
        Serial.println("Conexion exitosa");
        Serial.print("Conectado a Server IP: ");
        Serial.println(serverIP);
        Serial.print("Conectado a Server PORT: ");
        Serial.println(serverPort);
      }
    }
    else
    {
      vTaskDelay(20000 / portTICK_PERIOD_MS);
      continue;
    }
  }
}

/***************************************************************************************************************************/
/***************************************************************************************************************************/

void DISCONNECT_SERVER_TCP(void)
{
  Serial.println("Cerrar la conexión actual");
  client.stop(); // Cerrar el cliente
}

/***************************************************************************************************************************/
/***************************************************************************************************************************/

void Task_Verifica_Mensajes_Servidor_TCP(void *parameter)
{
  for (;;)
  {
    if (client.available())
    {
      buffer = client.readStringUntil('\n'); // Leer datos a nueva línea
      Serial.print("Dato entrante...");

      if (Buffer.Set_buffer_recepcion(buffer))
      {
        Serial.println("CRC de datos entrante OK");
        flag_dato_valido_recibido = true;
      }
      else
      {
        Serial.println("CRC de datos entrante ERROR");
        flag_dato_no_valido_recibido = true;
      }
      vTaskDelay(50 / portTICK_PERIOD_MS);
      continue;
    }
    else
    {
      vTaskDelay(50 / portTICK_PERIOD_MS);
      continue;
    }
  }
}