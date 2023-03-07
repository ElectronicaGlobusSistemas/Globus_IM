/**
 * @file RFID.cpp
 * @author Globus Sistemas
 * @brief 
 * @version 1.0
 * @date 
 * 
 * @copyright Copyright (c) 2023
 * 
 */


/*
#include <SPI.h>
#include <MFRC522.h>
#include "Buffers.h"
#include "Maquina_Estados.h"
#include "Configuracion.h"
#include "PCF8574.h"
#include <Adafruit_NeoPixel.h>
*/

// #define ADRESS          0x27
// #define RST_RFID        0
// #define Buzzer          1 
// #define Control         2
// #define LED1            3
// #define LED2            4
// #define LED3            5
// #define Sensor          6
// #define LED4            7
// #define SS_PIN          12



/* ------------------------------> Variables externas <-------------------------------------------*/
// extern Variables_Globales Variables_globales; // Objeto contiene Variables Globales
// extern Buffers Buffer;            // Objeto de buffer de mensajes servidor

// extern Configuracion_ESP32 Configuracion;

// extern int Estado;
// extern int Sig_Estado;
// extern int Debug_;
// int Contador_Colores=0;
/*------------------------------------------------------------------------------------------------*/
/* ------------------------------> Instancias <-------------------------------------------------- */
// MFRC522 mfrc522(SS_PIN, RST_RFID);   // Create MFRC522 instance.
// PCF8574 pcf8574(ADRESS); //  Dirección expansor I2C.
/*------------------------------------------------------------------------------------------------*/

/* -----------------------------> Controlador de tareas <-----------------------------------------*/
// TaskHandle_t RFID;    //  Manejador de tareas
/*------------------------------------------------------------------------------------------------*/

/* -----------------------------> Inicializa Barra de estatus <-----------------------------------*/
// Adafruit_NeoPixel Barra_Status_Sesion_Client=Adafruit_NeoPixel(4,Control,NEO_RGB+NEO_KHZ800);
/*------------------------------------------------------------------------------------------------*/


/* --------------------------------------> Variables <--------------------------------------------*/
// unsigned long Time_Previo_=0;
// unsigned long Timeout_Espera=80000; // Tiempo de espera  inactividad. 80s

// unsigned Verifica_RFD_=0;
// unsigned Verifica_RFD_2=0;
// unsigned Stop_=10000; // Tiempo para verificar conexión de modulo RFID 

// int sessionStartedFrequency = 1000; // Frecuencia para la sesión iniciada
// int sessionStartedDuration = 200;  //  Duración del tono para la sesión iniciada
// int sessionEndedFrequency = 500;  //   Frecuencia para la sesión terminada
// int sessionEndedDuration = 500;  //    Duración del tono para la sesión terminada
// int moduleStartFrequency = 2000; //    Frecuencia para el inicio del módulo
// int moduleStartDuration = 1000; //     Duración del tono para el inicio del módulo

// unsigned long Start_Cambio_Color=0;    //  Variable para Timer
// unsigned long Previous_Cambio_Color=0; //  Variable para Timer
// unsigned long OK_Color=5000;           //  Tiempo de cambio de  color  y figura en sesion abierta
/*------------------------------------------------------------------------------------------------*/
// void Init_RFID(void)
// {
    
//     /* ----------------------------- > Inicializa Hardware RFID <-----------------------------*/
//     pcf8574.pinMode(RST_RFID,OUTPUT);
//     pcf8574.pinMode(Buzzer,OUTPUT);
//     pcf8574.pinMode(Control,OUTPUT);
//     pcf8574.pinMode(LED1,OUTPUT);
//     pcf8574.pinMode(LED2,OUTPUT);
//     pcf8574.pinMode(LED3,OUTPUT);
//     pcf8574.pinMode(LED4,OUTPUT);
//     pcf8574.pinMode(Sensor,INPUT);
//     /*-----------------------------------------------------------------------------------------*/
    
//     mfrc522.PCD_Init(); // Init MFRC522 card
//     if(mfrc522.PCD_DumpVersionToSerial()/*&&pcf8574.begin()*/)
//     {
        
//         #ifdef Init_RFID_
//         Serial.println("Modulo RFID Inicializado....");
//         #endif
//         Variables_globales.Set_Variable_Global(Modulo_RFID_OK,true);

//         /*----------------------------> Barra LED <--------------------------------------- */
//        // Barra_Status_Sesion_Client.begin(); /* Inicializa Barra led status*/
//        // Barra_Status_Sesion_Client.show(); /* Inicializa Barra led status*/
//        // Barra_Status_Sesion_Client.setBrightness(100); /* Configura Brillo*/
//         /*---------------------------------------------------------------------------------*/


//         /*------------------------> Reproduce tono al iniciar modulo RFID<------------------*/
//        // pcf8574.digitalWrite(Buzzer, HIGH);
//        // tone(Buzzer, moduleStartFrequency, moduleStartDuration); // Reproducir tono para sesión iniciada
//        // delay(moduleStartDuration);
//        // pcf8574.digitalWrite(Buzzer, LOW);
//         /*----------------------------------------------------------------------------------*/

//         /*------------------------> Inicia Ocupado <----------------------------------------*/
//         //int R=255;
//         //int G=0;
//         //int B=0;
//         //for(int i=0; i<5;i++)
//         //{
//         //    Barra_Status_Sesion_Client.setPixelColor(i,R,G,B);
//        // }
//        // Barra_Status_Sesion_Client.show();
//     }else{
//         #ifdef Init_RFID_ 
//         Serial.println("Modulo RFID no Conectado....");
//         #endif
//         Variables_globales.Set_Variable_Global(Modulo_RFID_OK,false);
//     }

//     xTaskCreatePinnedToCore(
//         Read_RFID,            //  Función a implementar la tarea.
//         "Read_RFID",              //  Nombre de la tarea.
//         8000,                     //  Tamaño de stack en palabras (memoria)
//         NULL,                      //  Entrada de parametros.
//         10, //  Prioridad de la tarea.
//         &RFID,               //  Manejador de la tarea.
//         1);                        //  Core donde se ejecutara.
// }


/*Tarea para  procesar tarjetas RFID */
// static void Read_RFID(void *parameter) /* Lee y verifica  RFID*/
// {
//     int contador = 0;
//     for (;;)
//     {

//         Verifica_RFD_2=millis();
//         Start_Cambio_Color=millis();
//         if((Start_Cambio_Color-Previous_Cambio_Color)>=OK_Color)
//         {
//             int num=0;
//             if(random(1,5)<3)
//             {
//                 num=1;
//             }else{
//                 num=random(2,7);
//             }
//             /*Sesion_Abierta_Color(num)*/
//            // Serial.println("Este es el cambia Color: ------>"+String (num));
//             Previous_Cambio_Color=millis();
//         }
//         Maquina_Estados(Estado);
//         if(Verifica_RFD_2-Verifica_RFD_>=Stop_)
//         {
//             // Init MFRC522 card
//             if(mfrc522.PCD_DumpVersionToSerial())
//             {
//                 mfrc522.PCD_Init();
//                 #ifdef Init_RFID_  
//                 Serial.println("Modulo RFID OK");
//                 #endif
//                 Variables_globales.Set_Variable_Global(Modulo_RFID_OK,true);
//             }else{
//                 #ifdef Init_RFID_
//                 Serial.println("Error Modulo RFID");
//                 #endif
//                 Variables_globales.Set_Variable_Global(Modulo_RFID_OK,false);
//                 mfrc522.PCD_Init();    
//             }
//             Verifica_RFD_=millis();
//         }
//         vTaskDelay(500);
//     }
//     vTaskDelay(10);
// }

/*Funcion para  guardar el ID de cliente detectado*/

// void Lee_Tarjeta()
// {
//     Time_Previo_=millis(); /* Reset Timeout Espera de Sesion */

//     /*--------------------------------> Clave para acceso a memoria de tarjeta RFID <--------------------------------------------*/
//     MFRC522::MIFARE_Key key;
//     for (byte i = 0; i < 6; i++)
//         key.keyByte[i] = 0xFF;
//     byte block;
//     byte len;
//     MFRC522::StatusCode status;
//     /*---------------------------------------------------------------------------------------------------------------------------*/

//     /*-------------------------------> Control de lectura RFID <-----------------------------------------------------------------*/

//     if (!Variables_globales.Get_Variable_Global(Status_Sesion_Game) && Variables_globales.Get_Variable_Global(Comunicacion_Maq)==true && Variables_globales.Get_Variable_Global(Modulo_RFID_OK)==true)
//     {
//         Time_Previo_=millis(); /* Reset Timeout Espera de Sesion */

//         /*Indicadores led  modulo RFID disponible*/

//         if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial())
//         {
//             Time_Previo_=millis(); /* Reset Timeout Espera de Sesion */
           

//             byte buffer1[18];

//             block = 4;
//             len = 18;

//             //------------------------------------------- GET FIRST NAME
//             status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 4, &key, &(mfrc522.uid)); // line 834 of MFRC522.cpp file
//             if (status != MFRC522::STATUS_OK)
//             {
//                 Serial.print(F("Authentication failed: "));
//                 Serial.println(mfrc522.GetStatusCodeName(status));
//                 Buffer.Close_Client();
//                 return;
//             }

//             status = mfrc522.MIFARE_Read(block, buffer1, &len);
//             if (status != MFRC522::STATUS_OK)
//             {
//                 Serial.print(F("Reading failed: "));
//                 Serial.println(mfrc522.GetStatusCodeName(status));
//                 Buffer.Close_Client();
//                 return;
//             }

//             byte buffer2[4];
//             block = 1;

//             status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 1, &key, &(mfrc522.uid)); // line 834
//             if (status != MFRC522::STATUS_OK)
//             {
//                 Serial.print(F("Authentication failed: "));
//                 Serial.println(mfrc522.GetStatusCodeName(status));
//                 Buffer.Close_Client();
//                 return;
//             }

//             status = mfrc522.MIFARE_Read(block, buffer2, &len);
//             if (status != MFRC522::STATUS_OK)
//             {
//                 Serial.print(F("Reading failed: "));
//                 Serial.println(mfrc522.GetStatusCodeName(status));

//                 Buffer.Close_Client();
//                 return;
//             }
//             /*----------------------------------->Error de lectura<----------------------------------*/
//             if (buffer2[0] == '0' || buffer2[1] == '0' || buffer2[2] == '0' || buffer2[3] == '0')
//             {
//                 Serial.println("Error de lectura RFID");
//                 Serial.println("La sesion no se inicio");
//                 Buffer.Close_Client();
//                 return;
//             }
//             else if(buffer2[0] == NULL || buffer2[1] == NULL || buffer2[2] == NULL || buffer2[3] == NULL)
//             {
//                 Serial.println("Error de lectura RFID");
//                 Serial.println("La sesion no se inicio");
//                 Buffer.Close_Client();
//                 return;
                
//             }
//             /*---------------------------------------------------------------------------------------*/

//             /*----------------------------------->Lectura Correcta <---------------------------------*/
//             else
//             {
//                 Variables_globales.Set_Variable_Global(Status_Sesion_Game, true); //Inicia Sesion de juego.
//                 if (Variables_globales.Get_Variable_Global(Status_Sesion_Game)) /*Si la Sesion se inicio*/
//                 {
//                     Buffer.Set_ID_Cliente(buffer2); // Guarda ID de cliente en Buffer
//                     Serial.println(F("**Card Detected:**"));
//                     Serial.println(F("**BIENVENIDO**"));

//                     /*------------------------> Reproduce tono al iniciar sesion<------------------*/
//                     // pcf8574.digitalWrite(Buzzer, HIGH);
//                     // tone(Buzzer, sessionStartedFrequency, sessionStartedDuration); // Reproducir tono para sesión iniciada
//                     // delay(sessionStartedDuration);
//                     // pcf8574.digitalWrite(Buzzer, LOW);
//                     /*----------------------------------------------------------------------------------*/
//                     Variables_globales.Set_Variable_Global(Flag_Maquina_En_Juego,true);
//                     /*-------------------------->Lector Ocupado <-------------------------------------*/
//                     // int R=255;
//                     // int G=0;
//                     // int B=0;
//                     // for(int i=0; i<5;i++)
//                     //{
//                     //     Barra_Status_Sesion_Client.setPixelColor(i,R,G,B);
//                     // }
//                     // Barra_Status_Sesion_Client.show();
//                     /*----------------------------------------------------------------------------------*/
//                     Variables_globales.Set_Variable_Global(Flag_Contadores_Sesion_ON, true); /*Envio trama de contadores con ID Cliente*/
//                     /*---------------->Selecciona el  estado<---------------------------------*/
//                     if(Configuracion.Get_Configuracion(Tipo_Maquina, 0)==1)
//                     {
//                         Sig_Estado=Cashless_AFT; /* Inicia AFT*/
//                     }
//                     else if(Configuracion.Get_Configuracion(Tipo_Maquina, 0)==2)
//                     {
//                          Sig_Estado=Cashless_EFT; /* Inicia EFT*/
//                     }
//                     else if(Configuracion.Get_Configuracion(Tipo_Maquina, 0)==3)
//                     {
//                         Sig_Estado=Cashless_AFT_Single; /* Inicia AFT_SIGLE*/
//                     }
//                     else{
//                         Sig_Estado=Player_Tracking; /* Inicia Player Tracking*/
//                     }
//                     /*--------------------------------------------------------------------------*/
//                     Time_Previo_=millis(); /* Reset Timeout Espera de Sesion */ 
//                 }
//             }
//             /*---------------------------------------------------------------------------------------*/
//         }
//         mfrc522.PICC_HaltA(); /*Reset*/
//         mfrc522.PCD_StopCrypto1();
//     }
//     /*---------------------------------------------------------------------------------------------------------------------*/
// }

// void Player_Tracking_(void)
// {
    
//     if (Variables_globales.Get_Variable_Global(Status_Sesion_Game) == true && Variables_globales.Get_Variable_Global(Flag_Maquina_En_Juego) == false)
//     {
//         /* Si la sesion esta activa y la maquina no esta en juego. Conteo timeout*/
//       unsigned long  Time_Init_ = millis();

//         if (Time_Init_-Time_Previo_>=Timeout_Espera)
//         {
           
//             Variables_globales.Set_Variable_Global(Status_Sesion_Game, false); /* Cierra la sesion */
//             if (!Variables_globales.Get_Variable_Global(Status_Sesion_Game))   /*Verifica que si se cerro*/
//             {
//                 Time_Previo_=Time_Init_;
//                 Buffer.Close_Client();                      /*Borra ID de trama de contadores*/
//                 Serial.println(F("Timeout agotado"));       /* Mensaje Debug*/
//                 Serial.println(F("**SESION FINALIZADA**")); /* Mensaje Debug*/
//                 /*Termina Sesion por inactividad*/
//                 Sig_Estado = Default;
//                 Debug_=0;
//                 Variables_globales.Set_Variable_Global(Flag_Contadores_Sesion_ON, true); /*Envio trama de contadores sesion finalizada*/
//             }
//         }

//         if (Variables_globales.Get_Variable_Global(Modulo_RFID_OK) == false && Variables_globales.Get_Variable_Global(Status_Sesion_Game) == true)
//         {
            
//             Variables_globales.Set_Variable_Global(Status_Sesion_Game, false); /* Cierra la sesion */
//             if(!Variables_globales.Get_Variable_Global(Status_Sesion_Game))
//             {
//                 Time_Previo_ = Time_Init_;
//                 Serial.println(F("Error Modulo  RFID no Conectado..")); /* Mensaje Debug*/
//                 Serial.println(F("**SESION FINALIZADA**"));             /* Mensaje Debug*/
//                 /*Termina Sesion Error en modulo RFID*/
//                 Sig_Estado = Default;
//                 Debug_ = 0;
//                 Buffer.Close_Client();
//                 Variables_globales.Set_Variable_Global(Flag_Contadores_Sesion_ON, true); /*Envio trama de contadores*/
//             }
//         }
//     }
// }

// void Consulta_Info_Cliente(void)
// {
//     /*Consulta info cliente */
//     /*Flag de consulta AFT */
//     /*Serializacion Cashless*/
// }


// void Sesion_Abierta_Color(int Figura)
// {
//     int R,G,B;
//     int intensidad;
//     int Style; 
//     switch (Figura)
//     {
//     case 1:/*Sesion de juego disponible*/
//         Barra_Status_Sesion_Client.setBrightness(100); /* Configura Brillo*/
//         Barra_Status_Sesion_Client.setPixelColor(0, 0, 255, 0); 
//         Barra_Status_Sesion_Client.setPixelColor(1, 0, 255, 0); 
//         Barra_Status_Sesion_Client.setPixelColor(2, 0, 255, 0); 
//         Barra_Status_Sesion_Client.setPixelColor(3, 0, 255, 0); 
//         Barra_Status_Sesion_Client.show(); // Actualizamos la tira de LED
//     break;

//     case 2: /*Random de colores*/
//         Barra_Status_Sesion_Client.setBrightness(100); /* Configura Brillo*/
//         for (int i = 0; i < 5; i++)
//         {
//                 R = random(256);                                  
//                 G = random(256);                                  
//                 B = random(256);
//                 if(R!=255 && G!=0 && B!=0)
//                 {
//                     Barra_Status_Sesion_Client.setPixelColor(i, R, G, B);
//                 }                                  
//         }
//         Barra_Status_Sesion_Client.show(); // Actualizamos la tira de LED
//     break;
   
//     case 3: /*Desplazamiento Derecha-Izquierda*/
//         Barra_Status_Sesion_Client.setBrightness(100); /* Configura Brillo*/
//         R = random(256);                                  
//         G = random(256);                                  
//         B = random(256);

//         for(int i=0; i<5;i++)
//         {
//             Barra_Status_Sesion_Client.setPixelColor(i,R,G,B);
//             Barra_Status_Sesion_Client.show();
//             delay(150);
//         }

//         for(int i=3; i>=0;i--)
//         {
//             Barra_Status_Sesion_Client.setPixelColor(i,R,G,B);
//             Barra_Status_Sesion_Client.show();
//             delay(150); 
//         }

//     break;
    
//     case 4:/*Solo un Bit*/
//         Barra_Status_Sesion_Client.setBrightness(100); /* Configura Brillo*/
//         R = random(256);
//         G = random(256);
//         B = random(256);

//         for (int i = 0; i < 5; i++)
//         {
//                 Barra_Status_Sesion_Client.setPixelColor(i, R, G, B);
//                 Barra_Status_Sesion_Client.show();
//                 delay(150);
//                 Barra_Status_Sesion_Client.setPixelColor(i,0,0,0);
//                 Barra_Status_Sesion_Client.show();
//         }

//         for (int i = 3; i >= 0; i--)
//         {
//                 Barra_Status_Sesion_Client.setPixelColor(i, R, G, B);
//                 Barra_Status_Sesion_Client.show();
//                 delay(150);
//                 Barra_Status_Sesion_Client.setPixelColor(i,0,0,0);
//                 Barra_Status_Sesion_Client.show();
//         }
//     break;


//     case 5:/* Aleatorio posición*/
//         Barra_Status_Sesion_Client.setBrightness(100); /* Configura Brillo*/
//         R = random(256);
//         G = random(256);
//         B = random(256);
//         for(int i=0; i<5; i++)
//         {
//             int pos = random(0, 4);
//             Barra_Status_Sesion_Client.setPixelColor(pos, R, G, B);
//             Barra_Status_Sesion_Client.show(); // Actualizamos la tira de LED
//             delay(150);
//             Barra_Status_Sesion_Client.setPixelColor(pos, 0, 0, 0);
//             Barra_Status_Sesion_Client.show(); // Actualizamos la tira de LED
//         }
        
//     break;

//     case 6: /*Cambia intensidad*/
//          intensidad =random(25,100);
//          Style =random(2,5);
//          Barra_Status_Sesion_Client.setBrightness(intensidad); /* Configura Brillo*/
//          Barra_Status_Sesion_Client.show(); // Actualizamos la tira de LED

//          switch (Style)
//          {
//          case 2:
//             for (int i = 0; i < 5; i++)
//             {
//                 R = random(256);
//                 G = random(256);
//                 B = random(256);
//                 if (R != 255 && G != 0 && B != 0)
//                 {
//                     Barra_Status_Sesion_Client.setPixelColor(i, R, G, B);
//                 }
//             }
//             Barra_Status_Sesion_Client.show(); // Actualizamos la tira de LED

//             break;
//          case 3:

//             R = random(256);
//             G = random(256);
//             B = random(256);

//             for (int i = 0; i < 5; i++)
//             {
//                 Barra_Status_Sesion_Client.setPixelColor(i, R, G, B);
//                 Barra_Status_Sesion_Client.show();
//                 delay(150);
//             }

//             for (int i = 3; i >= 0; i--)
//             {
//                 Barra_Status_Sesion_Client.setPixelColor(i, R, G, B);
//                 Barra_Status_Sesion_Client.show();
//                 delay(150);
//             }

//             break;
//         case 4:

//             R = random(256);
//             G = random(256);
//             B = random(256);

//             for (int i = 0; i < 5; i++)
//             {
//                 Barra_Status_Sesion_Client.setPixelColor(i, R, G, B);
//                 Barra_Status_Sesion_Client.show();
//                 delay(150);
//                 Barra_Status_Sesion_Client.setPixelColor(i, 0, 0, 0);
//                 Barra_Status_Sesion_Client.show();
//             }

//             for (int i = 3; i >= 0; i--)
//             {
//                 Barra_Status_Sesion_Client.setPixelColor(i, R, G, B);
//                 Barra_Status_Sesion_Client.show();
//                 delay(150);
//                 Barra_Status_Sesion_Client.setPixelColor(i, 0, 0, 0);
//                 Barra_Status_Sesion_Client.show();
//             }
//         break;

//         case 5:

//             R = random(256);
//             G = random(256);
//             B = random(256);
//             for (int i = 0; i < 5; i++)
//             {
//                 int pos = random(0, 4);
//                 Barra_Status_Sesion_Client.setPixelColor(pos, R, G, B);
//                 Barra_Status_Sesion_Client.show(); // Actualizamos la tira de LED
//                 delay(150);
//                 Barra_Status_Sesion_Client.setPixelColor(pos, 0, 0, 0);
//                 Barra_Status_Sesion_Client.show(); // Actualizamos la tira de LED
//             }
//         break;
        
//         }
//     break;
    
//     default:
//         Barra_Status_Sesion_Client.setBrightness(100); /* Configura Brillo*/
//         Barra_Status_Sesion_Client.setPixelColor(0, 0, 255, 0); 
//         Barra_Status_Sesion_Client.setPixelColor(1, 0, 255, 0); 
//         Barra_Status_Sesion_Client.setPixelColor(2, 0, 255, 0); 
//         Barra_Status_Sesion_Client.setPixelColor(3, 0, 255, 0); 
//         Barra_Status_Sesion_Client.show(); // Actualizamos la tira de LED
//         break;
//     }
    
// }
