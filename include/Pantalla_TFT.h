#include "Arduino.h"
#define SINCRO    10
#define REESTART_TFT          11
#define DESINCRO  12
#define PING                  3
#define CONFIG                13
#define DOWNLOAD              14
#define REMOVE_IMG_TFT        15
#define UPDATE_TFT            16

#ifndef PANTALLA_TFT_H
#define PANTALLA_TFT_H

#define  PLAYER_TRACKING_SESION 0
#define  PLAYER_CASHLESS_SESION 1
#define  PLAYER_UNKNOW_SESION   2

#define TFT_M5STACK_DIAL        1
#define TFT_48S3_43             0
#define TFT_UNKNOW              100

bool Init_TFT_Display(bool EspNow=false, uint8_t MAC[6] = nullptr);
bool Stop_TFT_Display(void);
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingDataPtr, int len);
bool Init_Player_TFT(String User_Name ="", int Total_Playertracking_Points=0, int Total_Points_Tickets=0, int User_Level=0,uint32_t Saldo_Canjeable=0, uint32_t Saldo_Sin_Restriccion=0,uint32_t Saldo_No_Canjeable=0,int Current_Playertracking_Points=0,int Current_Points_Tickets=0,int Tipo_Sesion=PLAYER_TRACKING_SESION);

bool Await_ms(bool (*condicion)(), unsigned long timeout_ms);

void Prueba_TFT(void);
bool Send_TFT(uint8_t MAC[], uint8_t *Data, int len);
bool Close_Player_TFT(uint32_t Saldo_Canjeable=0, uint32_t Saldo_Sin_Restriccion=0, uint32_t Saldo_No_Canjeable=0, int Tipo_Sesion=PLAYER_TRACKING_SESION);
bool Get_Status_Sesion_Player(void);
bool Menssage_TFT(String Message,int timeout=1500,bool IsSuccess=true);
bool get_Flag_Sincro_TFT();
bool get_Flag_Config_TFT(void);
bool get_Flag_Descarga_TFT(void);
bool get_Flag_Borrar_TFT(void);
bool get_Flag_Update_TFT(void);
int get_Tipo_TFT(void);
void Task_Conexion_TFT(int Timeout);
void Init_Server(const char *ssid,const char *password);
bool Reset_TFT(void);
void RumUpdateTFT();
#endif // PANTALLA_TFT_H