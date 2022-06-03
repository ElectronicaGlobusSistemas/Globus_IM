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



String Encabezado_Contadores="Hora,Total Cancel Credit,Coin In,Coin Out,Jackpot,Total Drop, Cancel Credit Hand Pay,Bill Amount, Casheable In, Casheable Restricted In, Casheable Non Restricted In, Casheable Out, Casheable Restricted Out,Casheable Nonrestricted Out, Games Played";


void Init_SD(void)
{
   SD.begin();
    xTaskCreatePinnedToCore(
        Task_Verifica_Conexion_SD,
        "SD_Check",
        10000,
        NULL,
        5,
        NULL,
        1); // Rum in Core 1  
}

static void Task_Verifica_Conexion_SD(void *parameter)
{
    int Intento_Connect_SD = 0; // Variable Contadora de Intentos de Conexión SD.
    for (;;)
    {
        if (!card.init(SPI_FULL_SPEED) ) // SD Desconectada...
        {
            
            Serial.println("Memoria SD Desconectada..");
            //SD.begin(SD_ChipSelect); // Intenta Conectar.
            card.init(SPI_FULL_SPEED);
            digitalWrite(SD_Status, LOW); // Apaga  Indicador LED SD Status.
            Serial.print("Fallo en Conexión SD");
            Serial.print(" Intento #: ");
            Serial.println(Intento_Connect_SD);
            Intento_Connect_SD++;
            
        }
        else if (card.init(SPI_FULL_SPEED)) // SD OK
        {
            Intento_Connect_SD = 0;
            Serial.println("SD OK");
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
    if (!SD.exists(ARCHIVO)) // Si el archivo no existe lo Crea con encabezado para Excel!!
    {
        myFile = SD.open(ARCHIVO, FILE_WRITE);
        myFile.println(Encabezado);
        myFile.close();
        Serial.println("Archivo: "+(String)ARCHIVO+ " Creado con Encabezado");
    }
}
//--------------------------------------------------------------------------------------------------------

//---------------------------> Función para Escribir En Archivos CSV <------------------------------------
void Write_Data_File(String Datos,char *ARCHIVO,bool select)
{
    if (SD.exists(ARCHIVO))
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
                Serial.println("Dato Guardado en SD");
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
                Serial.println("Dato Guardado en SD");
            }
        }
        myFile.close();
    }
}
//--------------------------------------------------------------------------------------------------------
//---------------------------> Función para Escribir En Archivos CSV <------------------------------------
void Write_Data_File2(String Datos,char *archivo,bool select, String Encabezado)
{
    if (SD.exists(archivo))
    {
        myFile = SD.open(archivo, FILE_WRITE);
        if (select == false)
        {
            if (!myFile)
            {
                Serial.println("Error No se pudo Abrir el  Archivo: " + (String)archivo);
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
                Serial.println("Error No se pudo Abrir el  Archivo: " + (String)archivo);
            }
            else
            {
                myFile.println(Datos);
                Serial.println("Dato: " + Datos + " Guardado en SD");
            }
        }
        myFile.close();
    }
    else
    {
        Create_ARCHIVE_Excel(archivo, Encabezado);
        delay(3);
        Write_Data_File(Datos, archivo, select);
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