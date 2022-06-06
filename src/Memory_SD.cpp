#include <SPI.h>
#include <mySD.h>
#include "Memory_SD.h"

#define MOSI 23
#define MISO 19
#define CLK  18

Sd2Card card;
File myFile;
SdVolume volume;
SdFile root;

String Encabezado_Contadores2="Hora,Total Cancel Credit,Coin In,Coin Out,Jackpot,Total Drop, Cancel Credit Hand Pay,Bill Amount, Casheable In, Casheable Restricted In, Casheable Non Restricted In, Casheable Out, Casheable Restricted Out,Casheable Nonrestricted Out, Games Played";
TaskHandle_t SD_CHECK;// Manejador de tareas
void Init_SD(void)
{
    SD.begin();
    xTaskCreatePinnedToCore(
        Task_Verifica_Conexion_SD, //Función a implementar la tarea.
        "SD_Check", //  Nombre de la tarea
        10000,      //  Tamaño de stack en palabras (memoria)
        NULL,       //  Entrada de parametros
        5,          //  Prioridad de la tarea.
        &SD_CHECK,   //  Manejador de la tarea.
        1);         //  Core donde se ejecutara.  
}


static void Task_Verifica_Conexion_SD(void *parameter)
{
    vTaskSuspend(SD_CHECK);// Inicio  Tarea Suspendida.
    //vTaskResume(SD_CHECK);
    int Intento_Connect_SD = 0; // Variable Contadora de Intentos de Conexión SD.
    for (;;)
    {
        if (!card.init(SPI_FULL_SPEED) ) // SD Desconectada...
        {
            
            Serial.println("Memoria SD Desconectada..");
            //SD.begin(SD_ChipSelect); // Intenta Conectar.
            card.init(SPI_FULL_SPEED); // Intenta Conectar Despues de Fallo.
            
            Serial.print("Fallo en Conexión SD"); // Mensaje de Fallo.
            Serial.print(" Intento #: ");
            Serial.println(Intento_Connect_SD); // Imprime conteo de Fallos.
            Intento_Connect_SD++; // Aaumento de Contador.
            digitalWrite(SD_Status, LOW); // Apaga  Indicador LED SD Status.
            
        }
        else if (card.init(SPI_FULL_SPEED)) // SD OK
        {
            Intento_Connect_SD = 0; // Reset Contador de Fallos.
            Serial.println("SD OK"); // Mensaje de Conexión SD.
            digitalWrite(SD_Status, HIGH); // Enciende Indicador LED SD Status.
            
        }
        
        vTaskDelay(10000); //Pausa Tarea 10000ms
    }
    vTaskDelete(NULL);
   
}

//---------------------> Función Para Crear Archivos Txt sin Formato <---------------------------------
void Create_ARCHIVE_Txt( char* ARCHIVO)
{
    if (!SD.exists(ARCHIVO)) // Pregunta si el archivo Existe.
    {
        myFile = SD.open(ARCHIVO, FILE_WRITE); // Abre Archivo.
        myFile.close();
    }
}
//------------------------------------------------------------------------------------------------------

//-----------------------------> Función Para escribir en Archivo Txt <---------------------------------
void Write_Data_File_Txt(String Datos, char* ARCHIVO)
{
    Create_ARCHIVE_Txt(ARCHIVO); // Crea Archivo si no Existe.
    delay(3);
    myFile = SD.open(ARCHIVO, FILE_WRITE);
    if (!myFile)
    {
        Serial.println("Error al Escribir en Archivo: " + (String)ARCHIVO);
    }
    else
    {
        myFile.println(Datos);
        Serial.println("Dato: " + Datos + " Guardado en SD");
    }
    myFile.close();
}
//-------------------------------------------------------------------------------------------------------
//---------------------------> Función Para Crear Archivos CSV <-----------------------------------------
void Create_ARCHIVE_Excel(char* ARCHIVO, String  Encabezado)
{
    if(card.init(SPI_FULL_SPEED))
    {
        if (!SD.exists(ARCHIVO)) // Si el archivo no existe lo Crea con encabezado para Excel!!
        {
            myFile = SD.open(ARCHIVO, FILE_WRITE);
            myFile.println(Encabezado);
            myFile.close();
            Serial.println("Archivo: " + (String)ARCHIVO + " Creado con Encabezado");
        }
        else
        {
            delay(1);
        }
    }else{
        Serial.println("No Fue Posible Crear el Archivo: "+ String(ARCHIVO));
    }
}
//--------------------------------------------------------------------------------------------------------

//---------------------------> Función para Escribir En Archivos CSV <------------------------------------
void Write_Data_File(String Datos,char *ARCHIVO,bool select)
{
    myFile = SD.open(ARCHIVO, FILE_WRITE);
    if (select == false)
    {
        if (!myFile)
        {
            Serial.println("Error al escribir en SD");
        }
        else
        {
            myFile.print(Datos);
            myFile.print(",");
            Serial.println("Dato: " + Datos + " Guardado en SD");
        }
    }
    else
    {
        if (!myFile)
        {
            Serial.println("Error al escribir en SD");
        }
        else
        {
            myFile.println(Datos);
            Serial.println("Dato: " + Datos + " Guardado en SD");
        }
    }
    myFile.close();
}
//--------------------------------------------------------------------------------------------------------
//---------------------------> Función para Escribir En Archivos CSV <------------------------------------
void Write_Data_File2(char* Datos,char *archivo,bool select, String Encabezado)
{
    Create_ARCHIVE_Excel(archivo, Encabezado);
    delay(10);
    if (card.init(SPI_FULL_SPEED))
    {
        myFile = SD.open(archivo, FILE_WRITE);
        if (select == false)
        {
            if (!myFile)
            {
                Serial.println("Error No se pudo Abrir el  Archivo: " + (String)archivo);
                myFile.close();
            }
            else
            {
                myFile.print(Datos);
                myFile.print(",");
                Serial.println("Dato: " +(String) Datos + " Guardado en SD");
            }
        }
        else
        {
            if (!myFile)
            {
                Serial.println("Error No se pudo Abrir el  Archivo: " + (String)archivo);
                myFile.close();
            }
            else
            {
                myFile.println(Datos);
                Serial.println("Dato: " +(String)Datos + " Guardado en SD");
            }
        }
        myFile.close();
    }
    else
    {
        Serial.println("No se Puede Escribir en SD Desconectada.");
    }
}
void Write_Data_File3(String Datos,char *archivo,bool select, String Encabezado)
{
    Create_ARCHIVE_Excel(archivo, Encabezado);
    delay(10);
    if (card.init(SPI_FULL_SPEED))
    {
        myFile = SD.open(archivo, FILE_WRITE);
        if (!myFile)
        {
            Serial.println("Error No se pudo Abrir el  Archivo: " + (String)archivo);
            myFile.close();
        }
        else
        {
            myFile.println();
            myFile.close();
        }
    }
    else
    {
        Serial.println("No se Puede Escribir en SD Desconectada.");
    }
}

void Read_File(const char* ARCHIVO)
{
    myFile = SD.open(ARCHIVO);
    if (myFile)
    {
        while (myFile.available())
        {
            Serial.write(myFile.read());
        }
        myFile.close();
    }
    else
    {
        Serial.print("Error en lectura de archivo ");
        Serial.println(ARCHIVO);
    }
}

void Remove_Archive(char* ARCHIVO)
{
    if (SD.exists(ARCHIVO))
    {
        SD.remove(ARCHIVO);
        Serial.print("\nSe ha Eliminado ");
        Serial.print(ARCHIVO);
        Serial.println(" Exitosamente.");
    }
    else
    {
        Serial.println("\nNo existe archivo: "+(String)ARCHIVO);
    }
}

void Nueva_Carpeta(char* Carpeta)
{
    SD.mkdir(Carpeta);
}