 #include <Arduino.h>
 #include "RFID.h"
 #include "Maquina_Estados.h"
 #include "Clase_Variables_Globales.h"
// /*-----------------------> Definimos Estados <----------------------------*/

extern Variables_Globales Variables_globales; // Objeto contiene Variables Globales
// /*-------------------------------------------------------------------------*/

int Estado=0;
int Sig_Estado=0;
int Debug_=0;
int Cash=0;



void Maquina_Estados(int Estado)
{
    Estado=Variables_globales.Get_Variable_Global_Int(Sig_Estado_RFID);

    if (Estado == Default)
    {
        /*Lee Tarjetas*/
        
       Lee_Tarjeta();
    }

    else if (Estado==Player_Tracking)
    {
        if(Debug_==0)
        {
            Serial.println("Inicia Player Tracking...");
            Debug_=1;
        }
       Player_Tracking_();
    }

    else if(Estado==Cashless_AFT)
    {
        Serial.println("Inicia Cashless AFT");
        Serial.println("Consulta info cliente");
        Serial.println("Saldo insuficiente......");
        /*Fondo insuficiente  Inicia player tracking*/

       Variables_globales.Set_Variable_Global_Int(Sig_Estado_RFID,Player_Tracking);
    }
    else if(Estado==Cashless_EFT)
    {
        /*Fondo insuficiente  Inicia player tracking*/
        Serial.println("Inicia Cashless EFT");
        Serial.println("Consulta info cliente");
        Serial.println("Saldo insuficiente......");
        Variables_globales.Set_Variable_Global_Int(Sig_Estado_RFID,Player_Tracking);
    }
    else if(Estado==Cashless_AFT_Single)
    {
        /*Fondo insuficiente  Inicia player tracking*/
        Serial.println("Inicia Cashless AFT SINGLE");
        Serial.println("Consulta info cliente");
        Serial.println("Saldo insuficiente......");
        Variables_globales.Set_Variable_Global_Int(Sig_Estado_RFID,Player_Tracking);
    }

    else if(Estado==Operador_Maquina)
    {
        if(Variables_globales.Get_Variable_Global(Flag_Cashout_Button))
        {
            Variables_globales.Set_Variable_Global_Int(Flag_Cashout_Button,false);
            /*<-Maquina Bloqueada->*/
            Serial.println("Desbloquea Maquina Operador");
        }   
    }
}

