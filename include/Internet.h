#include <WiFi.h>
#include <WiFiUdp.h>

#define WIFI_Status 15
const int wifi_timeout = 10000;

unsigned long previousMillis = 0;
unsigned long interval = 30000;

WiFiClient clientTCP; // Declara un objeto cliente para conectarse al servidor
WiFiUDP clientUDP;    // Declara un objeto para cliente UDP

String buffer;
char incomingPacket[258];

char IP_Local[4];   // IP del ESP32
char IP_GW[4];      // IP de enlace
char SN_MASK[4];    // Mascara Subred
char IP_Server[4];  // IP del servidor
IPAddress serverIP; // Objeto IP Servidor
// const IPAddress serverIP(192, 168, 5, 208); // Direccion del servidor
uint16_t serverPort;  // Puerto del servidor
String SSID_Wifi;     // Nombre de red
String Password_Wifi; // Contraseña de la red

TaskHandle_t Status_WIFI;
TaskHandle_t Status_SERVER;

void Task_Verifica_Conexion_Wifi(void *parameter);
void Task_Verifica_Conexion_Servidor(void *parameter);
void Task_Verifica_Mensajes_Servidor(void *parameter);

/***************************************************************************************************************************/
/***************************************************************************************************************************/
void Init_Wifi()
{
  xTaskCreatePinnedToCore(
      Task_Verifica_Conexion_Wifi, //  Funcion a implementar la tarea
      "Verifica conexion wifi",    //  Nombre de la tarea
      10000,                       //  Tamaño de stack en palabras (memoria)
      NULL,                        //  Entrada de parametros
      configMAX_PRIORITIES - 10,   //  Prioridad de la tarea
      &Status_WIFI,                //  Manejador de la tarea
      0);                          //  Core donde se ejecutara la tarea
  xTaskCreatePinnedToCore(
      Task_Verifica_Conexion_Servidor,
      "Verifica conexion server",
      5000,
      NULL,
      configMAX_PRIORITIES - 20,
      &Status_SERVER,
      0); // Core donde se ejecutara la tarea
  xTaskCreatePinnedToCore(
      Task_Verifica_Mensajes_Servidor,
      "Verifica mensajes server",
      5000,
      NULL,
      configMAX_PRIORITIES - 5,
      NULL,
      0); // Core donde se ejecutara la tarea
}

/***************************************************************************************************************************/
/***************************************************************************************************************************/

void CONNECT_WIFI(void)
{
  //-----------------------------------------------------------------------------------------------------------
  // Obtiene direccion IP guardada en Objeto Configuracion
  memcpy(IP_Local, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(IP_Local) / sizeof(IP_Local[0]));
  //  size_t ip_len = NVS.getBytesLength("Dir_IP");
  //  NVS.getBytes("Dir_IP", IP_Local, ip_len);
  //-----------------------------------------------------------------------------------------------------------
  // Obtiene direccion IP de enlace guardada en Objeto configuracion
  memcpy(IP_GW, Configuracion.Get_Configuracion(Direccion_IP_GW, 'x'), sizeof(IP_GW) / sizeof(IP_GW[0]));
  // size_t ip_len_gw = NVS.getBytesLength("Dir_IP_GW");
  // NVS.getBytes("Dir_IP_GW", IP_GW, ip_len_gw);
  //-----------------------------------------------------------------------------------------------------------
  // Obtiene direccion mascara subred
  memcpy(SN_MASK, Configuracion.Get_Configuracion(Direccion_SN_MASK, 'x'), sizeof(SN_MASK) / sizeof(SN_MASK[0]));
  // size_t dir_sn_mask = NVS.getBytesLength("Dir_SN_MASK");
  // NVS.getBytes("Dir_SN_MASK", SN_MASK, dir_sn_mask);
  //-----------------------------------------------------------------------------------------------------------
  WiFi.mode(WIFI_MODE_APSTA); // MODO STA y AP.
  pinMode(WIFI_Status, OUTPUT);
  IPAddress Local_IP(IP_Local[0], IP_Local[1], IP_Local[2], IP_Local[3]);
  IPAddress Gateway(IP_GW[0], IP_GW[1], IP_GW[2], IP_GW[3]);
  IPAddress SubnetMask(SN_MASK[0], SN_MASK[1], SN_MASK[2], SN_MASK[3]);
  IPAddress primaryDNS(8, 8, 8, 8);   // optional
  IPAddress secondaryDNS(8, 8, 4, 4); // optional

  //  bool WiFiSTAClass::config(IPAddress local_ip, IPAddress gateway, IPAddress subnet, IPAddress dns1, IPAddress dns2);
  if (!WiFi.config(Local_IP, Gateway, SubnetMask, primaryDNS, secondaryDNS))
  {
    Serial.println("STA Failed to configure"); // mensaje Monitor Serial.
  }

  WiFi.setSleep(false); // Desactiva la suspensión de wifi en modo STA para mejorar la velocidad de respuesta

  SSID_Wifi = Configuracion.Get_Configuracion(SSID, "Nombre_Red");
  Password_Wifi = Configuracion.Get_Configuracion(Password, "Password_red");

  WiFi.begin(SSID_Wifi.c_str(), Password_Wifi.c_str());

  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < wifi_timeout)
  {
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("Wifi conectado");
    Serial.print("Conectado a: ");
    Serial.println(SSID_Wifi);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("ESP Mac Address: ");
    Serial.println(WiFi.macAddress());
    digitalWrite(WIFI_Status, HIGH);
  }
  else
  {
    Serial.print("\nNo se puede conectar a... ");
    Serial.println(SSID_Wifi);
    digitalWrite(WIFI_Status, LOW);
  }
}

/***************************************************************************************************************************/
/***************************************************************************************************************************/

void Task_Verifica_Conexion_Wifi(void *parameter)
{
  Serial.println(" Verificador WIFI Activado");
  unsigned long tiempo_inicial, tiempo_final = 0;
  Serial.println("Inicia verifica conexion WIFI...");
  for (;;)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      digitalWrite(WIFI_Status, HIGH);
      Serial.println("Wifi conectado");
      Serial.println("Termina verifica conexion WIFI...");
      vTaskSuspend(Status_WIFI);
      vTaskDelay(10000 / portTICK_PERIOD_MS);
      continue;
    }
    else
    {
      Serial.print("Conectando a... ");
      Serial.println(SSID_Wifi);
      WiFi.disconnect();//desconecta red 
      WiFi.reconnect(); // intenta reconectar
      //WiFi.begin(SSID_Wifi.c_str(), Password_Wifi.c_str());
      tiempo_inicial = millis();
      while (WiFi.status() != WL_CONNECTED && (tiempo_inicial - tiempo_final) < wifi_timeout)
      {
      }
      tiempo_final = tiempo_inicial;
      if (WiFi.status() != WL_CONNECTED)
      {
        Serial.print("\nNo se puede conectar a... ");
        Serial.println(SSID_Wifi);
        digitalWrite(WIFI_Status, LOW);

        if (!Variables_globales.Get_Variable_Global(Fallo_Archivo_LOG))
        {
          if (Variables_globales.Get_Variable_Global(Sincronizacion_RTC) == true)
          {
            log_e("Error de Conexion a Red WIFI", 100);

            LOG_ESP(Archivo_LOG, Variables_globales.Get_Variable_Global(Enable_Storage));
          }
        }

        vTaskDelay(10000 / portTICK_PERIOD_MS);
        continue;
      }
      else
      {
        Serial.println("\nWiFi connected!");
        Serial.print("Conectado a: ");
        Serial.println(SSID_Wifi);
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        Serial.print("ESP Mac Address: ");
        Serial.println(WiFi.macAddress());
        Serial.println("Termina verifica conexion WIFI...");
        vTaskSuspend(Status_WIFI);
        vTaskDelay(10000 / portTICK_PERIOD_MS);
        continue;
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
    // Obtiene direccion IP Servidor guardada en Objeto configuracion
    memcpy(IP_Server, Configuracion.Get_Configuracion(Direccion_IP_Server, 'x'), sizeof(IP_Server) / sizeof(IP_Server[0]));
    // size_t ip_len = NVS.getBytesLength("Dir_IP_Serv");
    // NVS.getBytes("Dir_IP_Serv", IP_Server, ip_len);
    IPAddress serverIP(IP_Server[0], IP_Server[1], IP_Server[2], IP_Server[3]);

    // Obtiene Numero de puerto guardada en NVS
    serverPort = Configuracion.Get_Configuracion(Puerto_Server, 0);
    // serverPort = NVS.getUInt("Socket", 0);

    if (Configuracion.Get_Configuracion(Tipo_Conexion))
    {
      Serial.println("\nConectando al servidor TCP...");
      if (clientTCP.connect(serverIP, serverPort)) // Intenta acceder a la dirección de destino
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
        clientTCP.stop(); // Cerrar el cliente

      }
    }
    else
    {
      clientUDP.begin(serverPort);
      Serial.printf("Escuchando por la IP: %s, Puerto UDP: %d\n", WiFi.localIP().toString().c_str(), serverPort);
    }
  }
}

/***************************************************************************************************************************/
/***************************************************************************************************************************/

void Task_Verifica_Conexion_Servidor(void *parameter)
{
  for (;;)
  {
    if (Configuracion.Get_Configuracion(Tipo_Conexion))
    {
      if (WiFi.isConnected())
      {
        if (clientTCP.connected())
        {
          Serial.println("Servidor conectado");
          vTaskDelay(10000 / portTICK_PERIOD_MS);
          vTaskSuspend(Status_SERVER);
          continue;
        }

        Serial.println("Conectando al servidor TCP...");

        // Obtiene direccion IP Servidor guardada en Objeto configuracion
        memcpy(IP_Server, Configuracion.Get_Configuracion(Direccion_IP_Server, 'x'), sizeof(IP_Server) / sizeof(IP_Server[0]));
        IPAddress serverIP(IP_Server[0], IP_Server[1], IP_Server[2], IP_Server[3]);

        // Obtiene Numero de puerto guardada en Objeto configuracion
        serverPort = Configuracion.Get_Configuracion(Puerto_Server, 0);

        clientTCP.connect(serverIP, serverPort);

        if (!clientTCP.connected())
        {
          Serial.println("Acceso denegado");
          clientTCP.stop(); // Cerrar el cliente

          if (!Variables_globales.Get_Variable_Global(Fallo_Archivo_LOG))
          {
            if(Variables_globales.Get_Variable_Global(Sincronizacion_RTC) == true)
            {
              log_e("Error Conexión Servidor TCP", 102);
              LOG_ESP(Archivo_LOG, Variables_globales.Get_Variable_Global(Enable_Storage));
            }
          }
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
        vTaskSuspend(Status_SERVER);
        continue;
      }
    }
    else
    {
      vTaskSuspend(Status_SERVER);
    }
  }
}

/***************************************************************************************************************************/
/***************************************************************************************************************************/

void DISCONNECT_SERVER_TCP(void)
{
  Serial.println("Cerrar la conexión actual");
  clientTCP.stop(); // Cerrar el cliente
}

/***************************************************************************************************************************/
/***************************************************************************************************************************/

void Task_Verifica_Mensajes_Servidor(void *parameter)
{
  for (;;)
  {
    if (Configuracion.Get_Configuracion(Tipo_Conexion))
    {
      // Mensajes Servidor TCP
      if (clientTCP.available())
      {
        buffer = clientTCP.readStringUntil('\n'); // Leer datos a nueva línea
        Serial.println("Dato entrante...");

        if (Buffer.Set_buffer_recepcion_TCP(buffer))
        {
          Serial.println("CRC de datos entrante OK");
          Variables_globales.Set_Variable_Global(Dato_Entrante_Valido, true);
        }
        else
        {
          Serial.println("CRC de datos entrante ERROR");
          Variables_globales.Set_Variable_Global(Dato_Entrante_No_Valido, true);
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
    else
    {
      // Mensajes Servidor UDP
      int packetSize = clientUDP.parsePacket();
      if (packetSize)
      {
        // receive incoming UDP packets
        Serial.printf("Received %d bytes from %s, port %d\n", packetSize, clientUDP.remoteIP().toString().c_str(), clientUDP.remotePort());
        int len = clientUDP.read(incomingPacket, 258);
        if (len > 0)
        {
          incomingPacket[len] = 0;
        }
        //        Serial.printf("UDP packet contents: %s\n", incomingPacket);

        if (Buffer.Set_buffer_recepcion_UDP(incomingPacket))
        {
          Serial.println("CRC de datos entrante OK");
          Variables_globales.Set_Variable_Global(Dato_Entrante_Valido, true);
        }
        else
        {
          Serial.println("CRC de datos entrante ERROR");
          Variables_globales.Set_Variable_Global(Dato_Entrante_No_Valido, true);
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
}