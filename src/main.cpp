#include <Arduino.h>
#include "Memory_SD.h"
#include "time.h"
//#include "Bootloader.h"
//#include "Internet.h"
#include "RS232.h"
#include <stdio.h>

//#include <string.h>
#include "Config_Perifericos.h"
#include <iostream>
#include <bitset>

using namespace std;
using std::bitset;
unsigned long tiempo_inicial, tiempo_final = 0;
int contadore=0;

void setup()
{
  
  Init_Config();
  pinMode(2, OUTPUT);



}


void loop()
{


//Create_ARCHIVE_Excel("06062022.csv","Titulo1,Titulo2,Titulo3");
Write_Data_File2("Jose1","06062022.csv",false,"Titulo1,Titulo2,Titulo3");
Write_Data_File2("Jose2","06062022.csv",false,"Titulo1,Titulo2,Titulo3");
Write_Data_File2("Jose3","06062022.csv",true,"Titulo1,Titulo2,Titulo3");

contadore++;
delay(1000); 
    
}
