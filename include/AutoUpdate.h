


// int Config_Update(void);
// void firmwareUpdate();
// int FirmwareVersionCheck();
// // void Auto_Update(bool Flag_Maquina_En_Juego_=false,bool Hopper_Enable,bool =false,bool Flag_Premio_Pagado=false,bool Billete_Insertado=false);


// #define URL_fw_Version "https://raw.githubusercontent.com/ElectronicaGlobusSistemas/04-fide/main/bin_version.txt"
// #define URL_fw_Bin "https://raw.githubusercontent.com/ElectronicaGlobusSistemas/04-fide/main/fw.bin"
// #define URL_config "https://raw.githubusercontent.com/ElectronicaGlobusSistemas/04-fide/main/config.txt"

#include <iostream>
#include <Arduino.h>


#define     RES_URL "RES_URL"
#define     VER_OK  "VER_OK"
#define     VER_NO  "VER_NO"
#define     DES_OK  "DES_OK"
#define     DES_NO  "DES_NO"
#define     INS_OK  "INS_OK" 
#define     INS_NO  "INS_NO" 

#define MACHINE_IN_GAME         "00" /* Actualización cancelada por maquina en juego */
#define NEW_VERSION             "01" /* Nueva Versión de firmware detectada */
#define INSTALL_VERSION_CURRENT "02" /* Version de firmware actual igual a versión instalada */
#define ERROR_DES               "03" /* Error en descarga de actualización */
#define URL_OK                  "04" /* Parametros de actualización recibidos */
#define ERROR_INSTALL           "05" /* Error en instalación de actualización */
#define INSTALL_OK              "06" /* Nueva versión de firmware instalada con exito! */
#define ERROR_ARCHIVO           "07" /* Error en verificación de  archivo de actualización .bin */





class AutoUpdate
{
private:
  /* data */
  
    String VERSION_FIR;
    String URL_GENERIC;
    String API_BIN;
    String API_RES;
    String TOKEN_VALI;
    String VERSION_FIR_LOCAL;
    String API_VER;

    unsigned long Millis_R;
    unsigned long Millis_P;
    int Invert_Update;
    

public:
 
  int  FirmwareVersionCheck();
  void firmwareUpdate(void);

  void Init_AutoUpdate(String Version_Firmware_, String URL_Generic_,String Api_ver, String URL_Bin_,String ,String Token_ ,uint8_t Buffer[]);
  void Auto_Update(bool Flag_Maquina_en_Juego__=false, bool Hopper_Poker__=false, bool Billete_Insert__=false, bool Flag_Premio_pagado_=false,bool Flag_Sesion_Player_Tracking=false,int Creditos_Actuales=0);
  bool Confirmacion_ACK_HTTPS(String Ack, String Code);
  String Token_Generator(void);
  bool DateTime_Update(bool Enable);
};







