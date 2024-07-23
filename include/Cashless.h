
#include <array>
#include <iostream>
#include "Arduino.h"
//-------> ESTADOS REGISTRO AFT <-----------------
#define  No_Registrada  1
#define  Registrada     2
#define  Error_Registro 6
//-------------------------------------------------
#define NUM_DIGITS 7 // Número total de dígitos en el número de transacción

#define OFFLINE                    1
#define ERROR_TOKEN                2
#define CLIENT_DOES_NOT_HAVE_BONUS 3
#define CLIENT_HAVE_BONUS          4 

class Cashless
{
private:
    /* data */
    char transID[7]={0,0,0,0,0,0,0};
        

public:
   bool Cashless_Loading_Transaction_AFT(char res[]);
   bool Cashless_Download_Transaction_AFT(char res[]);
   bool Enable_Cashless_Loads(bool Enable);
   bool AFT_Registration(bool);
   bool Transaccion_Bonus_Cashless(bool);
   bool Save_Transaction_ID_Number(bool);
   bool Delete_Transaction_ID_Number();
   bool Increase_Transaction_Number_ID();
   bool Read_Machine_Registration(void);
   bool Cashless_Loading_Transaction_EFT(char res[]);
   bool Cashless_Download_Transaction_EFT(char res[]);
};


class Fidelizacion
{
private:
    /* data */
public:
    int Consult_Customer_Information(uint8_t Id_Cliente[],char Coin_In[],char Conin_Out[],char Total_Drop[],char Total_Cancel_Credit[],char Jackpot[],char Total_Game[],String Access_Token);

    bool Init_Player_Tracking( byte Id_Client[8] );
    bool Close_Sesion_Player_Tracking(byte Id_Client[8]);
    bool Await_For_Conexion(void);
};





