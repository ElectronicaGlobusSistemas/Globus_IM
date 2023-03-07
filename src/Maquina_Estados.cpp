// #include <Arduino.h>
// #include "RFID.h"
// #include "Maquina_Estados.h"
// /*-----------------------> Definimos Estados <----------------------------*/


// /*-------------------------------------------------------------------------*/

// int Estado=0;
// int Sig_Estado=0;
// int Debug_=0;
// int Cash=0;

// /*Maquina de estados Selecciona entre 
// Estado (0) Lectura de tarjeta Default
// Estado (1) Player Tracking
// Estado (2) Cashless AFT
// Estado (3) Cashless EFT
// Estado (4) Cashless AFT_SINGLE  
// */
// void Maquina_Estados(int Estado)
// {
//     Estado=Sig_Estado;

//     if (Estado == Default)
//     {
//         /*Lee Tarjetas*/
//        Lee_Tarjeta();
//     }

//     else if (Estado==Player_Tracking)
//     {
//        /*Si la sesion estaba activa en player tracking y se cierra la sesion Pasa a estado default*/ 
//        if(Debug_==0)
//        {
//         Serial.println("Inicia Player Tracking...");
//         Debug_=1;
//        }
//        Player_Tracking_();
//     }

//     else if(Estado==Cashless_AFT)
//     {
//         Serial.println("Inicia Cashless AFT");
//         Serial.println("Consulta info cliente");
//         Serial.println("Saldo insuficiente......");
//         /*Fondo insuficiente  Inicia player tracking*/
//         Sig_Estado=Player_Tracking;
        
//     }
//     else if(Estado==Cashless_EFT)
//     {
//         /*Fondo insuficiente  Inicia player tracking*/
//         Serial.println("Inicia Cashless EFT");
//         Serial.println("Consulta info cliente");
//         Serial.println("Saldo insuficiente......");
//         Sig_Estado=Player_Tracking;
//     }
//     else if(Estado==Cashless_AFT_Single)
//     {
//         /*Fondo insuficiente  Inicia player tracking*/
//         Serial.println("Inicia Cashless AFT SINGLE");
//         Serial.println("Consulta info cliente");
//         Serial.println("Saldo insuficiente......");
//         Sig_Estado=Player_Tracking;
//     }

// }