#include <iostream>
#include <Arduino.h>



class API_Gmaster
{
private:
    String URL_API_Gmaster="";
    String Access_Token_Api_Gmaster="";
    String Controlador_Principal="";
    /* Day, Month, Year */
    int DateTime_Expires[3]={0,0,0};

    /* ----------------------> Datos Token <--------------- */
    unsigned long Timmer_Create_Token=0;
    unsigned long Timmer_Create_Token_Previous=0;
    int TimeOut_Token=300000;/*5 Minutos*/
    bool Token_Inicial=false;
    /*------------------------------------------------------*/


    unsigned long TimeOut_Verify_Token=0;
    unsigned long TimeOut_Verify_Token_Previous=0;
    int TimeOut_Verify=15000;
    //1800000 30Minutos


    unsigned long Timer_Sincro_RTC=0;
    unsigned long Timer_Sincro_Previo_RTC=0;
    int Timer_Sincro_Ok=300000; //10s
    bool Inicia_Solicitud=false;

  //  ESP32Time DateTime_Expires;
public:
  bool Init_API_Gmaster(String URL, String Access_Token);
  /* Metodo Transmite eventos  */
  void Transmite_Eventos_Gmaster_Api(char Buffer[], String Api, bool Token_Valido);
  /*Metodo Transmite Contadores */
  void Trasmite_Contadores_Gmaster_Api(char Buffer[],String api,bool Token_Valido);
  /* Metodo Transmite  ACK */
  void Transmite_Confirmacion_API(char res[],String api, bool Token_Valido);
  /* Sincroniza RTC */
  void Sincroniza_Reloj_RTC_API(String Api);
  bool Serie_trama_API(String Api);
  /* Inicializa Controlador Principal*/
  bool Init_Controlador_Principal(String Api_Controlador);
  /* Genera y retorna Token de acceso */
  String Token_Generator_Gmaster(String Api);
  /* Inicializa Token de acceso para solicitudes.*/
  bool Init_Access_Token(String Access_Token);
  /* Verifica fecha de expiración de Token  */
  int Verify_Expires_Token(bool SincroRTC=false, int Contador=21, bool Token_Valido=false, String Api_Token="");

  /* Guarda Fecha de expiracion de token formato int  Day Month Year */
  bool Set_DateTime_Expires_Token(int Day,int Month,int Year);

 /* return Day[0], Month[1], Year[2] retorna fecha de expiración de token */
  int* Get_DateTime_Expires_Token(void);

  /* Dia, Mes, año, Hora, Minutos,segundos, limite */
  bool Set_Event_Time(int Day_Evento=0, int Month_Evento=0, int Year_Evento=0, int Hour_Evento=0, int Minutes_Evento=0,int Seconds_Evento=0,int Limite=0);

  String Get_Access_Token_String();
  String Get_Controlador_Api(void);
  void Marca_Eventos_Api(void);

};

