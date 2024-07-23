#include <iostream>
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>

#define Debug_Acess_ponit 
#define PORT   80
#define INFO_RED 0
#define INFO_API 1
#define INFO_GENERIC 2

class Web_Config
{
private:

  
 const char* password = "password";
 String ssid_Wifi;
public:
 
    void Init_Web_Server(void);
    void Upload_Form(void);
    void Save_Config(void);
    void Off_AP(void);
    bool Evento_Conexion(void);
    String Get_Info_Config(int Type_Info);
};

void prueba();