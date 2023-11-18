


// int Config_Update(void);
// void firmwareUpdate();
// int FirmwareVersionCheck();
// // void Auto_Update(bool Flag_Maquina_En_Juego_=false,bool Hopper_Enable,bool =false,bool Flag_Premio_Pagado=false,bool Billete_Insertado=false);


// #define URL_fw_Version "https://raw.githubusercontent.com/ElectronicaGlobusSistemas/04-fide/main/bin_version.txt"
// #define URL_fw_Bin "https://raw.githubusercontent.com/ElectronicaGlobusSistemas/04-fide/main/fw.bin"
// #define URL_config "https://raw.githubusercontent.com/ElectronicaGlobusSistemas/04-fide/main/config.txt"

#include <iostream>
#include <Arduino.h>

class AutoUpdate
{
private:
  /* data */
  
    String URL_Version_Firmware;
    String URL_Bin;
    String Token;
    String FirmwareVer;
    unsigned long Millis_R;
    unsigned long Millis_P;
    int Invert_Update;
    

public:
 
  int FirmwareVersionCheck();
  void firmwareUpdate(void);
  void Init_AutoUpdate(uint8_t Version_Firmware_[]);
  void Auto_Update(bool Flag_Maquina_en_Juego__=false, bool Hopper_Poker__=false, bool Billete_Insert__=false, bool Flag_Premio_pagado_=false,bool Flag_Sesion_Player_Tracking=false);
};







