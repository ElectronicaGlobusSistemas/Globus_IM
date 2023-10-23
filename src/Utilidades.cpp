#include <Arduino.h>
#include "Buffer_Cashless.h"
#include <WiFi.h>
#include "Utilidades_Maquina.h"
#include "Utilidades.h"
extern Buffer_RX_AFT  Buffer_Cashless;


extern unsigned char Serial_Cashless_UniMil;
extern unsigned char Serial_Cashless_Centenas;
extern unsigned char Serial_Cashless_Decenas;
extern unsigned char Serial_Cashless_Unidades;
extern unsigned char Ant_Serial_Cashless_UniMil;
extern unsigned char Ant_Serial_Cashless_Centenas;
extern unsigned char Ant_Serial_Cashless_Decenas;
extern unsigned char Ant_Serial_Cashless_Unidades;
extern unsigned char New_Serial_Cashless_UniMil;
extern unsigned char New_Serial_Cashless_Centenas;
extern unsigned char New_Serial_Cashless_Decenas;
extern unsigned char New_Serial_Cashless_Unidades;


#define Registro    1
#define Status_Registro 2
#define Init_Registro   3
#define Cancela_Registro 4




bool Compara_Serial_Cashless_Extern (void)
{
    if (Ant_Serial_Cashless_Unidades == New_Serial_Cashless_Unidades)
    {
        if (Ant_Serial_Cashless_Decenas == New_Serial_Cashless_Decenas)
        {
            if (Ant_Serial_Cashless_Centenas == New_Serial_Cashless_Centenas)
            {
                if (Ant_Serial_Cashless_UniMil == New_Serial_Cashless_UniMil)
                {
                    return true;
                }
            }
        }
    }
    else
    {
        return false;
    }
}

bool Almacena_Serial_Cashless_Extern(unsigned int Dato)
{
     switch (Dato)
    {
    case 1: // Almacena los valores de la nueva serie de transaccion cashless
        //--------------------------------------------------------------------------
        Ant_Serial_Cashless_Unidades = Serial_Cashless_Unidades;
        //--------------------------------------------------------------------------
        Ant_Serial_Cashless_Decenas = Serial_Cashless_Decenas;
        //--------------------------------------------------------------------------
        Ant_Serial_Cashless_Centenas = Serial_Cashless_Centenas;
        //--------------------------------------------------------------------------
        Ant_Serial_Cashless_UniMil = Serial_Cashless_UniMil;
        //--------------------------------------------------------------------------
        break;
    case 2:
        break;

    }

}



void Genera_Registro_Maquina(void)
{
    Buffer_Cashless.Set_RX_AFT_2(6);
}


/*
Return (1)   =  Maquina registrada
Return (2)   = La maquina fue registrada anteriormente
Return (3)   = Error de registro
Return (0)   = No acepto el registro
return (10)  = No realizo Ningun proceso 
*/
unsigned char Registra_Machine(void)
{
    Buffer_Cashless.Clear_Buffer(Buffer_RX_AFT_);
    unsigned char Status;
   
    Status=Interroga_Est_Reg_AFT_Funcion();

    if (Status == 1)
    {
        Genera_Registro_Maquina();
        /**/
      
        if (Transmite_Poll_Registro_AFT_Mq(Init_Registro) == 1)
        {
         
          
            if (Transmite_Poll_Registro_AFT_Mq(Registro) == 2)
            {
                return 1; //Maquina registrada....
            }else{
                return 0;
            }
        }
    }
    else if (Status == 2)
    {
        return 2; // La maquina  fue registrada anteriormente..
    }
    else if (Status == 6)
    {
        return 3; // Error de registro...
    }else
    {
        return 10;
    }
}
/*
Return  (1) = Registro Cancelado
Return (2)= Registro no Cancelado
*/
unsigned char Delete_Registro_Machine(void){
    unsigned char Status;
    Status = Transmite_Poll_Registro_AFT_Mq(Cancela_Registro);

    if (Status == 4)
    {
        return 1; // No registrada
    }
        return 2; // Registrada
}

unsigned char  Transmite_Poll_Registro_AFT_Mq(int Type_Transmite_Poll)
{
    unsigned char Status;
    if(Type_Transmite_Poll==Status_Registro)
    {
        Status = Interroga_Estado_registro_AFT();
    }
    if(Type_Transmite_Poll==Init_Registro)
    {
        Status = Inicializa_Registro_AFT();
    }

    if(Type_Transmite_Poll==Cancela_Registro)
    {
        Status = Cancela_Registro_AFT();

    }
    if(Type_Transmite_Poll==Registro)
    {
        Status=Registra_MAQ_AFT();
        
    }

    delay(350);
    Serial.println(Buffer_Cashless.Get_RX_AFT(Buffer_RX_AFT_)[3],DEC);
    if(Buffer_Cashless.Get_RX_AFT(Buffer_RX_AFT_)[3]==0x00)
    {
       return 1;
    }
    else if(Buffer_Cashless.Get_RX_AFT(Buffer_RX_AFT_)[3]==0x01)
    {
       return 2;
    }
    else if(Buffer_Cashless.Get_RX_AFT(Buffer_RX_AFT_)[3]==0x40)
    {
        return 3;
    }else if(Buffer_Cashless.Get_RX_AFT(Buffer_RX_AFT_)[3]==0x80)
    {
        Buffer_Cashless.Set_RX_AFT(Asset_Pos_Id_,Buffer_Cashless.Get_RX_AFT(Buffer_RX_AFT_));
        return 4;
    }else if(Buffer_Cashless.Get_RX_AFT(Buffer_RX_AFT_)[3]==0xFF)
    {
        return 5;
    } else{
        return 6;
    }
}

unsigned char Interroga_Est_Reg_AFT_Funcion(void) {

    unsigned char Status;
    Status= Transmite_Poll_Registro_AFT_Mq(Status_Registro);
   
    delay(350);
    if(Status==4)
    {
        return 1; // No registrada
    }else if(Status ==2)
    {
        return 2; // Registrada 
    }else if(Status==6)
    {
        return 6; // Hubo error de registro
    }
}