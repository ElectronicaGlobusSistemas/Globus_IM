#include "Web_Config.h"
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include "Arduino.h"
#include "ArduinoJson.h"
#include "Configuracion.h"
#include <iostream>
#include <sstream>
#include <string> 
#include "nvs_flash.h"
#include "Preferences.h"
#include "Clase_Variables_Globales.h"


extern Variables_Globales Variables_globales; // Objeto contiene Variables Globales
extern Configuracion_ESP32 Configuracion;
extern Preferences NVS;

extern int Inactividad_Usuario_Player_Tracking;
extern int Tiempo_Transmision_En_Juego;
extern int Tiempo_Transmision_No_Juego;
extern int Tiempo_Inactividad_Maquina;


AsyncWebServer server(PORT);


/* Metodo para iniciar  modo  punto de acceso */
void Web_Config::Init_Web_Server(void)
{
    /* El modo se inicia cuando el  boton reset  es presionado durante 10 segundos de ejecución del programa.
       El modo AP finaliza cuando el boton reset es presionado nuevamente durante 10 segundos de ejecución del programa 
       Siempre y cuando  este se encuentre activado ó  se guarde una configuración exitosa! */

    /* --------------------------> Genera Direccion IP para punto de acceso <------------------------------------*/
    char SegMentoAp[4];
    memcpy(SegMentoAp, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(SegMentoAp) / sizeof(SegMentoAp[0]));
    IPAddress localIP(192, 168, 5, 2);
    IPAddress gateway(192, 168, 5, 1);
    IPAddress subnet(255, 255, 255, 0);
    /*-----------------------------------------------------------------------------------------------------------*/

    /*------------------------------------> Configura e inicia modo AP <-----------------------------------------*/
    WiFi.softAPConfig(localIP, gateway, subnet);
    WiFi.disconnect(true,true);
    WiFi.mode(WIFI_MODE_AP);
    String Ssid="GLOBUS-Config-"+ WiFi.macAddress();
    WiFi.softAP(Ssid.c_str(),password);
    /*-------------------------------------------------------------------------------------------------------------*/
    Upload_Form(); /* Formulario */
    Save_Config(); /* Respuestas */
    Off_AP(); /* Reinicia ESP*/
    server.begin(); 
}

String Convert_bool_to_String(bool Booleano)
{

    if(Booleano)
        return "true";
    else
        return "false";
}

/* Convierte Tipo IP en String */
std::string IP_toString(char IP_Char[])
{
    std::stringstream ss;

    // Agregar cada octeto al stringstream
    for (int i = 0; i < 4; ++i) {
        ss << static_cast<int>(IP_Char[i]); // Convertir char a int para imprimir el valor numérico
        if (i < 3) {
            ss << '.'; // Agregar puntos entre los octetos
        }
    }

    // Obtener el string resultante
    std::string ipString = ss.str();

    return ipString;
}

/* Funcion  para convertir IP string a char* */
char* stringToIP(const String& ipString) {
    static char IP_Char[4];  // Declaramos el array estáticamente

    String tempString = ipString;
    int pos = 0;

    for (int i = 0; i < 4; ++i) {
        int dotPos = tempString.indexOf('.');
        if (dotPos != -1) {
            IP_Char[i] = static_cast<char>(tempString.substring(0, dotPos).toInt());
            tempString = tempString.substring(dotPos + 1);
        } else {
            // Si no hay más puntos, es el último octeto
            IP_Char[i] = static_cast<char>(tempString.toInt());
        }
    }

    return IP_Char; // Devolver el puntero al array resultante
}

/* Metodo para cargar formulario Web con parametros de  configuración actual */
void Web_Config::Upload_Form(void)
{
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){

    /* ----------------------------------------> Load Current Parameters <--------------------------------*/
    /* >Declara variables< */
    char Current_IP[4];
    char Current_IP_GW[4];
    char Current_Mask[4];
    char Current_IP_SERVER[4];
    String Current_Name_Machine;
    String Current_SSID;
    String Current_Password="";
    int Current_Type_Machine;
    int Current_Server_Port;
    char Currnet_Primary_DNS[4];
    char Current_Secundary_DNS[4];
    bool Current_Type_transmission;
    String Current_Timeout_Player_Tracking;
    String Current_Tiempo_Transmission_Game;
    String Current_Tiempo_Transmission_Not_Game;
    String Current_Timeout_Inactivity_Machine;
    bool Current_Tipo_Socket;
    String Id_Maquina_SAS;

    String Controlador_Principal_;
    String Metodo_RTC_;
    String Metodo_Contadores_;
    String Metodo_Eventos_;
    String Metodo_Token_;

    memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));
    memcpy(Current_IP_GW, Configuracion.Get_Configuracion(Direccion_IP_GW, 'x'), sizeof(Current_IP_GW) / sizeof(Current_IP_GW[0]));
    memcpy(Current_Mask, Configuracion.Get_Configuracion(Direccion_SN_MASK, 'x'), sizeof(Current_Mask) / sizeof(Current_Mask[0]));
    memcpy(Current_IP_SERVER, Configuracion.Get_Configuracion(Direccion_IP_Server, 'x'), sizeof(Current_IP_SERVER) / sizeof(Current_IP_SERVER[0]));
    memcpy(Currnet_Primary_DNS, Configuracion.Get_Configuracion(Dns_One_IP, 'x'), sizeof(Currnet_Primary_DNS) / sizeof(Currnet_Primary_DNS[0]));
    memcpy(Current_Secundary_DNS, Configuracion.Get_Configuracion(Dns_Two_IP, 'x'), sizeof(Current_Secundary_DNS) / sizeof(Current_Secundary_DNS[0]));

    Current_Name_Machine= Configuracion.Get_Configuracion(Nombre_Maquina, "Nombre_Maq");
    Current_SSID=Configuracion.Get_Configuracion(SSID, "Nombre_Red");
    Current_Type_Machine=Configuracion.Get_Configuracion(Tipo_Maquina, 0);
    Current_Server_Port=Configuracion.Get_Configuracion(Puerto_Server, 0);
    Current_Password=Configuracion.Get_Configuracion(Password, "Password_red");

    Current_Type_transmission = Variables_globales.Get_Variable_Global(Gmaster_API_Mode);


    switch (Inactividad_Usuario_Player_Tracking)
    {
    case 90000:
        Current_Timeout_Player_Tracking = "0";
        break;

    case 120000:
        Current_Timeout_Player_Tracking = "1";
        break;

    case 150000:
        Current_Timeout_Player_Tracking = "2";
        break;
    case 180000:
        Current_Timeout_Player_Tracking = "3";
        break;

    case 210000:
        Current_Timeout_Player_Tracking = "4";
        break;

    case 240000:
        Current_Timeout_Player_Tracking = "5";
        break;

    case 270000:
        Current_Timeout_Player_Tracking = "6";
        break;

    case 300000:
        Current_Timeout_Player_Tracking = "7";
        break;

    case 330000:
        Current_Timeout_Player_Tracking = "8";
        break;

    case 360000:
        Current_Timeout_Player_Tracking = "9";
        break;

    default:
        Current_Timeout_Player_Tracking = "0";
        break;
    }

    switch (Tiempo_Transmision_En_Juego)
    {
    case 30:
        Current_Tiempo_Transmission_Game = "0";
        break;

    case 40:
        Current_Tiempo_Transmission_Game = "1";
        break;

    case 50:
        Current_Tiempo_Transmission_Game = "2";
        break;
    case 60:
        Current_Tiempo_Transmission_Game = "3";
        break;

    case 70:
        Current_Tiempo_Transmission_Game = "4";
        ;
        break;

    case 80:
        Current_Tiempo_Transmission_Game = "5";
        break;

    case 90:
        Current_Tiempo_Transmission_Game = "6";
        break;

    case 100:
        Current_Tiempo_Transmission_Game = "7";
        break;

    case 110:
        Current_Tiempo_Transmission_Game = "8";
        break;

    case 120:
        Current_Tiempo_Transmission_Game = "9";
        break;

    default:
        Current_Tiempo_Transmission_Game = "0";
        break;
    }

    switch (Tiempo_Transmision_No_Juego)
    {
    case 150000:
        Current_Tiempo_Transmission_Not_Game = "0";
        break;

    case 180000:
        Current_Tiempo_Transmission_Not_Game = "1";
        break;

    case 210000:
        Current_Tiempo_Transmission_Not_Game = "2";
        break;
    case 240000:
        Current_Tiempo_Transmission_Not_Game = "3";
        break;

    case 270000:
        Current_Tiempo_Transmission_Not_Game = "4";
        break;

    case 300000:
        Current_Tiempo_Transmission_Not_Game = "5";
        break;

    case 330000:
        Current_Tiempo_Transmission_Not_Game = "6";
        break;

    case 360000:
        Current_Tiempo_Transmission_Not_Game = "7";
        break;

    case 480000:
        Current_Tiempo_Transmission_Not_Game = "8";
        break;

    case 600000:
        Current_Tiempo_Transmission_Not_Game = "9";
        break;

     case 1800000:
        Current_Tiempo_Transmission_Not_Game = "10";
        break;

    case 3600000:
        Current_Tiempo_Transmission_Not_Game = "11";
        break;

    case 5400000:
        Current_Tiempo_Transmission_Not_Game = "12";
        break;

    default:
        Current_Tiempo_Transmission_Not_Game = "0";
        break;
    }

    switch (Tiempo_Inactividad_Maquina)
    {
    case 50:
        Current_Timeout_Inactivity_Machine = "0";
        break;

    case 80:
        Current_Timeout_Inactivity_Machine = "1";
        break;

    case 115:
        Current_Timeout_Inactivity_Machine = "2";
        break;
    case 150:
        Current_Timeout_Inactivity_Machine = "3";
        break;

    case 180:
        Current_Timeout_Inactivity_Machine = "4";
        break;

    case 230:
        Current_Timeout_Inactivity_Machine = "5";
        break;

    default:

        Current_Timeout_Inactivity_Machine = "0";
        break;
    }
        
    Current_Tipo_Socket = Configuracion.Get_Configuracion(Tipo_Conexion);

    Id_Maquina_SAS=Configuracion.Get_Configuracion(Id_Maquina, "Id_Maquina");
        
    Controlador_Principal_=Configuracion.Get_Configuracion_ES(Controlador_P,"Controlador_Principal");
    Metodo_RTC_=Configuracion.Get_Configuracion_ES(Metodo_Sincro_RTC,"Metodo_SincroRTC");
    Metodo_Contadores_=Configuracion.Get_Configuracion_ES(Metodo_Conta,"Metodo_Contadores");
    Metodo_Eventos_=Configuracion.Get_Configuracion_ES(Metodo_Event,"Metodo_Eventos");
    Metodo_Token_=Configuracion.Get_Configuracion_ES(Metodo_Access_T,"Metodo_Token");

   
    /*----------------------------------------------------------------------------------------------------*/

    /* -------------------------------------------> Genera Json <-----------------------------------------*/    
    StaticJsonDocument<1024> jsonDocument;

    jsonDocument["ssid"] = Current_SSID;
    jsonDocument["password"] = Current_Password;
    jsonDocument["local_ip"] = IP_toString(Current_IP);
    jsonDocument["puerto"] = Current_Server_Port;
    jsonDocument["nombre_maquina"] =Current_Name_Machine;
    jsonDocument["subnet_mask"] = IP_toString(Current_Mask);
    jsonDocument["gateway"] = IP_toString(Current_IP_GW);
    jsonDocument["primary_dns"] = IP_toString(Currnet_Primary_DNS);
    jsonDocument["secondary_dns"] =IP_toString(Current_Secundary_DNS);
    jsonDocument["tipo_maquina"] =  Current_Type_Machine;

    jsonDocument["tipo_transmision"] =Convert_bool_to_String(Current_Type_transmission);
    jsonDocument["Timeout_Player_Tracking"] = Current_Timeout_Player_Tracking;
    jsonDocument["Timer_Send_Game"] = Current_Tiempo_Transmission_Game;
    jsonDocument["Timer_Not_Game"] = Current_Tiempo_Transmission_Not_Game;
    jsonDocument["Timeout_Inactivity_machine"] = Current_Timeout_Inactivity_Machine;
    jsonDocument["Type_Socket"] = Convert_bool_to_String(Current_Tipo_Socket);
    jsonDocument["Id_Maquina"]=Id_Maquina_SAS;

    jsonDocument["Controlador"] =Controlador_Principal_;
    jsonDocument["Api_Contadores"] = Metodo_Contadores_;
    jsonDocument["Api_Eventos"] = Metodo_Eventos_;
    jsonDocument["Api_RTC"] = Metodo_RTC_;
    jsonDocument["Api_Token"] = Metodo_Token_;


    String Json;
    serializeJson(jsonDocument,Json); /* Serializa Data */
    /* --------------------------------> Envia Formulario en formato Json <------------------------------*/
    request->send(200, "application/json", Json);
    /*---------------------------------------------------------------------------------------------------*/
    // html += "<h1>CONFIG-GLOBUS-IM-ESP32</h1>";
    // html += "<form id='configForm' method='post'>";
    // html += "SSID: <input type='text' name='ssid' value='" + miSSID + "'><br>";
    // html += "Contraseña: <input type='password' name='password' value='" + miPassword + "'><br>";
    // html += "Dirección IP Local: <input type='text' name='local_ip' value='" + Local_IP.toString() + "'><br>";
    // html += "Puerto: <input type='text' name='puerto' value='" + puerto + "'><br>";
    // html += "Nombre de la Máquina: <input type='text' name='nombre_maquina' value='"+miMaquina+"'><br>";
    // html += "Máscara de Subred: <input type='text' name='subnet_mask' value='" + SubnetMask.toString() + "'><br>";
    // html += "Puerta de Enlace: <input type='text' name='gateway' value='" + Gateway.toString() + "'><br>";
    // html += "Primary DNS: <input type='text' name='primary_dns' value='" + PrimaryDNS.toString() + "'><br>";
    // html += "Secondary DNS: <input type='text' name='secondary_dns' value='" + SecondaryDNS.toString() + "'><br>";
    // html += "Tipo de Máquina: <input type='text' name='tipo_maquina' value='" + tipoMaquina + "'><br>";
    // html += "<input type='button' value='Guardar' onclick='submitForm()'>";
    // html += "</form>";

    // html += "</body></html>";
});
}

/* Convierte String "true" a Booleano true */
bool String_to_Bool(String Valor, int Type)
{
    if(Valor=="true")
        return true;
    else if(Valor=="false")
        return false;

    else if(Valor=="True")
        return true;
    else if(Valor=="TRUE")
        return true;

    else if(Valor=="False")
        return false;
    else if(Valor=="FALSE")
        return false;
    else if("1")
        return true;
    else if("0")
        return false;

    else{
        if(Type== Tipo_Conexion) /* Si el dato es diferente  de true o false  deja los valores actuales*/
            return Configuracion.Get_Configuracion(Tipo_Conexion);
        else if(Type==Gmaster_API_Mode)
            return Variables_globales.Get_Variable_Global(Gmaster_API_Mode);

        else
            return false;
    }
    
}

/* Metodo para reiniciar dispositivo despues de aplicar configuración */
void Web_Config::Off_AP(void)
{
    server.on("/Reset", HTTP_POST, [](AsyncWebServerRequest *request){

    /* ----------------------->Genera Json<--------------------------------- */
    StaticJsonDocument<500> jsonDocument;
    jsonDocument["Descripcion"] = "Dispositivo reiniciado con exito!";
    jsonDocument["Codigo"] = 200;
    String Json;
    serializeJson(jsonDocument, Json);
    /*-----------------------------------------------------------------------*/

    /*                      >Envia Respuesta<                                */
    request->send(200, "application/json", Json);
    delay(150);
    /* --------------------->Reinicia dispositivo<-------------------------- */
    ESP.restart(); /* No reestablece  configuración WIFI,BT,UART0,SPI1*/
    /*-----------------------------------------------------------------------*/
    });
}

/* Retorna  json con informacion de condiguracion 
Type = (0)  retorna información de red 
Type = (1)  retorna informacion de API
Type = (2)  retorna informacion generica de tarjeta 
*/
String Web_Config::Get_Info_Config(int Type_Info)
{

    char Current_IP[4];
    char Current_IP_GW[4];
    char Current_Mask[4];
    char Current_IP_SERVER[4];
    String Current_Name_Machine;
    String Current_SSID;
    String Current_Password = "";
    int Current_Type_Machine;
    int Current_Server_Port;
    char Currnet_Primary_DNS[4];
    char Current_Secundary_DNS[4];
    bool Current_Type_transmission;
    String Current_Timeout_Player_Tracking;
    String Current_Tiempo_Transmission_Game;
    String Current_Tiempo_Transmission_Not_Game;
    String Current_Timeout_Inactivity_Machine;
    bool Current_Tipo_Socket;
    String Current_Id_Maquina;
    String Current_Controlador_Principal_;
    String Current_Metodo_RTC_;
    String Current_Metodo_Contadores_;
    String Current_Metodo_Eventos_;
    String Current_Metodo_Token_;

    bool Current_Status_Reset_Premio_;
    bool Current_Status_Event_Mecanic_;
    uint16_t Current_Puerto_RS232_SAS_;

    memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));
    memcpy(Current_IP_GW, Configuracion.Get_Configuracion(Direccion_IP_GW, 'x'), sizeof(Current_IP_GW) / sizeof(Current_IP_GW[0]));
    memcpy(Current_Mask, Configuracion.Get_Configuracion(Direccion_SN_MASK, 'x'), sizeof(Current_Mask) / sizeof(Current_Mask[0]));
    memcpy(Current_IP_SERVER, Configuracion.Get_Configuracion(Direccion_IP_Server, 'x'), sizeof(Current_IP_SERVER) / sizeof(Current_IP_SERVER[0]));

    memcpy(Currnet_Primary_DNS, Configuracion.Get_Configuracion(Dns_One_IP, 'x'), sizeof(Currnet_Primary_DNS) / sizeof(Currnet_Primary_DNS[0]));
    memcpy(Current_Secundary_DNS, Configuracion.Get_Configuracion(Dns_Two_IP, 'x'), sizeof(Current_Secundary_DNS) / sizeof(Current_Secundary_DNS[0]));

    Current_Name_Machine = Configuracion.Get_Configuracion(Nombre_Maquina, "Nombre_Maq");
    Current_SSID = Configuracion.Get_Configuracion(SSID, "Nombre_Red");
    Current_Type_Machine = Configuracion.Get_Configuracion(Tipo_Maquina, 0);
    Current_Server_Port = Configuracion.Get_Configuracion(Puerto_Server, 0);
    Current_Password = Configuracion.Get_Configuracion(Password, "Password_red");
    Current_Type_transmission = Variables_globales.Get_Variable_Global(Gmaster_API_Mode);

    switch (Inactividad_Usuario_Player_Tracking)
    {
    case 90000:
        Current_Timeout_Player_Tracking = "0";
        break;

    case 120000:
        Current_Timeout_Player_Tracking = "1";
        break;

    case 150000:
        Current_Timeout_Player_Tracking = "2";
        break;
    case 180000:
        Current_Timeout_Player_Tracking = "3";
        break;

    case 210000:
        Current_Timeout_Player_Tracking = "4";
        break;

    case 240000:
        Current_Timeout_Player_Tracking = "5";
        break;

    case 270000:
        Current_Timeout_Player_Tracking = "6";
        break;

    case 300000:
        Current_Timeout_Player_Tracking = "7";
        break;

    case 330000:
        Current_Timeout_Player_Tracking = "8";
        break;

    case 360000:
        Current_Timeout_Player_Tracking = "9";
        break;

    default:
        Current_Timeout_Player_Tracking = "0";
        break;
    }

    switch (Tiempo_Transmision_En_Juego)
    {
    case 30:
        Current_Tiempo_Transmission_Game = "0";
        break;

    case 40:
        Current_Tiempo_Transmission_Game = "1";
        break;

    case 50:
        Current_Tiempo_Transmission_Game = "2";
        break;
    case 60:
        Current_Tiempo_Transmission_Game = "3";
        break;

    case 70:
        Current_Tiempo_Transmission_Game = "4";
        ;
        break;

    case 80:
        Current_Tiempo_Transmission_Game = "5";
        break;

    case 90:
        Current_Tiempo_Transmission_Game = "6";
        break;

    case 100:
        Current_Tiempo_Transmission_Game = "7";
        break;

    case 110:
        Current_Tiempo_Transmission_Game = "8";
        break;

    case 120:
        Current_Tiempo_Transmission_Game = "9";
        break;

    default:
        Current_Tiempo_Transmission_Game = "0";
        break;
    }

    switch (Tiempo_Transmision_No_Juego)
    {
    case 150000:
        Current_Tiempo_Transmission_Not_Game = "0";
        break;

    case 180000:
        Current_Tiempo_Transmission_Not_Game = "1";
        break;

    case 210000:
        Current_Tiempo_Transmission_Not_Game = "2";
        break;
    case 240000:
        Current_Tiempo_Transmission_Not_Game = "3";
        break;

    case 270000:
        Current_Tiempo_Transmission_Not_Game = "4";
        break;

    case 300000:
        Current_Tiempo_Transmission_Not_Game = "5";
        break;

    case 330000:
        Current_Tiempo_Transmission_Not_Game = "6";
        break;

    case 360000:
        Current_Tiempo_Transmission_Not_Game = "7";
        break;

    case 480000:
        Current_Tiempo_Transmission_Not_Game = "8";
        break;

    case 600000:
        Current_Tiempo_Transmission_Not_Game = "9";
        break;

    case 1800000:
        Current_Tiempo_Transmission_Not_Game = "10";
        break;

    case 3600000:
        Current_Tiempo_Transmission_Not_Game = "11";
        break;

    case 5400000:
        Current_Tiempo_Transmission_Not_Game = "12";
        break;

    default:
        Current_Tiempo_Transmission_Not_Game = "0";
        break;
    }

    switch (Tiempo_Inactividad_Maquina)
    {
    case 50:
        Current_Timeout_Inactivity_Machine = "0";
        break;

    case 80:
        Current_Timeout_Inactivity_Machine = "1";
        break;

    case 115:
        Current_Timeout_Inactivity_Machine = "2";
        break;
    case 150:
        Current_Timeout_Inactivity_Machine = "3";
        break;

    case 180:
        Current_Timeout_Inactivity_Machine = "4";
        break;

    case 230:
        Current_Timeout_Inactivity_Machine = "5";
        break;

    default:

        Current_Timeout_Inactivity_Machine = "0";
        break;
    }

    Current_Tipo_Socket = Configuracion.Get_Configuracion(Tipo_Conexion);
    /*-----------------------------------------------------------------------------------------------*/
    Current_Id_Maquina = Configuracion.Get_Configuracion(Id_Maquina, "Id_Maquina");

    Current_Controlador_Principal_ = Configuracion.Get_Configuracion_ES(Controlador_P, "Controlador_Principal");
    Current_Metodo_RTC_ = Configuracion.Get_Configuracion_ES(Metodo_Sincro_RTC, "Metodo_SincroRTC");
    Current_Metodo_Contadores_ = Configuracion.Get_Configuracion_ES(Metodo_Conta, "Metodo_Contadores");
    Current_Metodo_Eventos_ = Configuracion.Get_Configuracion_ES(Metodo_Event, "Metodo_Eventos");
    Current_Metodo_Token_ = Configuracion.Get_Configuracion_ES(Metodo_Access_T, "Metodo_Token");


    /* Parametros  Adicionales */
    Current_Status_Reset_Premio_  = Variables_globales.Get_Variable_Global(Type_Hanpay_Reset);
    Current_Status_Event_Mecanic_ = Variables_globales.Get_Variable_Global(Enable_Mechanical_Events);
    Current_Puerto_RS232_SAS_=Variables_globales.Get_Variable_Global_Uint16(Uart_Port_Select);
    
    /*------------------------------------> Genera Json <--------------------------------------------*/
    StaticJsonDocument<1024> jsonDocument;
    jsonDocument.clear(); /* Limpia documento */

    if(Type_Info==INFO_RED) /* Informacion de red */
    {
        jsonDocument["ssid"] = Current_SSID;
        jsonDocument["local_ip"] = IP_toString(Current_IP);
        jsonDocument["puerto"] = Current_Server_Port;
        jsonDocument["subnet_mask"] = IP_toString(Current_Mask);
        jsonDocument["gateway"] = IP_toString(Current_IP_GW);
        jsonDocument["p_dns"] = IP_toString(Currnet_Primary_DNS);
        jsonDocument["s_dns"] = IP_toString(Current_Secundary_DNS);
        jsonDocument["tipo_transmision"] = Convert_bool_to_String(Current_Type_transmission);
        jsonDocument["Type_Socket"] = Convert_bool_to_String(Current_Tipo_Socket);

    }if(Type_Info==INFO_API) /* Información API */
    {
        jsonDocument["Controlador"] = Current_Controlador_Principal_;
        jsonDocument["Api_Contadores"] = Current_Metodo_Contadores_;
        jsonDocument["Api_Eventos"] = Current_Metodo_Eventos_;
        jsonDocument["Api_RTC"] = Current_Metodo_RTC_;
        jsonDocument["Api_Token"] = Current_Metodo_Token_;

    }if(Type_Info==INFO_GENERIC) /* Información Generica */
    {
        jsonDocument["tipo_maq"] = Current_Type_Machine;
        jsonDocument["Player_T"] = Current_Timeout_Player_Tracking;
        jsonDocument["Send_Game"] = Current_Tiempo_Transmission_Game;
        jsonDocument["Not_Game"] = Current_Tiempo_Transmission_Not_Game;
        jsonDocument["Inactivity"] = Current_Timeout_Inactivity_Machine;
        jsonDocument["Id_Maquina"] = Current_Id_Maquina;
        jsonDocument["Tipo_Reset"] = Current_Status_Reset_Premio_;
        jsonDocument["Eventos_M"]  = Current_Status_Event_Mecanic_;
        jsonDocument["COM"]= Current_Puerto_RS232_SAS_;
    }
    /*------------------------------------------------------------------------------------------------*/
    String Json;
    serializeJson(jsonDocument, Json); /* >Serializa Data< */
    return Json; /* Serializado*/
}

/* Metodo para guardar  configuracion de dispositivo */
void Web_Config::Save_Config(void)
{
    server.on("/Guardar", HTTP_POST, [](AsyncWebServerRequest *request){
   

    /* ----------------------->   Obtiene  parametros de configuración <-------------------------------*/     
    String Json_SSID  = request->arg("ssid");
    String Json_PASS = request->arg("password");
    String Json_Local_IP=request->arg("local_ip");
    String Json_Puerto=request->arg("puerto");
    String Json_nombre_maquina=request->arg("nombre_maquina");
    String Json_subnet_mask=request->arg("subnet_mask");
    String Json_gateway=request->arg("gateway");
    String Json_primary_dns=request->arg("primary_dns");
    String Json_secondary_dns=request->arg("secondary_dns");
    String Json_tipo_maquina=request->arg("tipo_maquina");

    String Json_Socket_API=request->arg("tipo_transmision");
    String Json_TimeOut_Player_Tracking=request->arg("Timeout_Player_Tracking");
    String Json_Tiempo_Transmission_Game=request->arg("Timer_Send_Game");
    String Json_Tiempo_Transmission_Not_Game=request->arg("Timer_Not_Game");
    String Json_Timeout_Inactivity_Machine=request->arg("Timeout_Inactivity_machine");
    String Json_Tipo_Socket=request->arg("Type_Socket");


    String Json_Id_Maquina=request->arg("Id_Maquina");
    String Json_Controlador=request->arg("Controlador");
    String Json_Metodo_Conta=request->arg("Metodo_Contadores");
    String Json_Metodo_Event=request->arg("Metodo_Eventos");
    String Json_Metodo_RTC=request->arg("Metodo_RTC");
    String Json_Metodo_Token=request->arg("Metodo_Token");
    


    /*---------------------------------------------------------------------------------------------------*/


    /* ----------------------------------------> Verifica si existen datos Nulos<--------------------------*/
    if (Json_SSID == "" || Json_PASS == "" || Json_Local_IP == "" || Json_Puerto == "" || Json_nombre_maquina == "" || Json_subnet_mask == "" || Json_gateway == "" || Json_primary_dns == "" || Json_secondary_dns == "" || Json_tipo_maquina == "" || Json_Socket_API == "" ||
        Json_TimeOut_Player_Tracking == "" || Json_Tiempo_Transmission_Game == "" || Json_Tiempo_Transmission_Not_Game == "" || Json_Timeout_Inactivity_Machine == "" || Json_Tipo_Socket == "" || Json_Id_Maquina=="" ||Json_Controlador==""||Json_Metodo_Conta==""||Json_Metodo_Event==""||Json_Metodo_RTC==""||Json_Metodo_Token=="")
    {

        /* Falta uno  o mas parametros de configuración, la interfaz envia la configuración actual */

        /*---------------------------> Obtiene parametros de configuracion actuales <---------------------*/
        char Current_IP[4];
        char Current_IP_GW[4];
        char Current_Mask[4];
        char Current_IP_SERVER[4];
        String Current_Name_Machine;
        String Current_SSID;
        String Current_Password = "";
        int Current_Type_Machine;
        int Current_Server_Port;
        char Currnet_Primary_DNS[4];
        char Current_Secundary_DNS[4];
        bool Current_Type_transmission;
        String Current_Timeout_Player_Tracking;
        String Current_Tiempo_Transmission_Game;
        String Current_Tiempo_Transmission_Not_Game;
        String Current_Timeout_Inactivity_Machine;
        bool Current_Tipo_Socket;
        String Current_Id_Maquina;
        String Current_Controlador_Principal_;
        String Current_Metodo_RTC_;
        String Current_Metodo_Contadores_;
        String Current_Metodo_Eventos_;
        String Current_Metodo_Token_;

        memcpy(Current_IP, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(Current_IP) / sizeof(Current_IP[0]));
        memcpy(Current_IP_GW, Configuracion.Get_Configuracion(Direccion_IP_GW, 'x'), sizeof(Current_IP_GW) / sizeof(Current_IP_GW[0]));
        memcpy(Current_Mask, Configuracion.Get_Configuracion(Direccion_SN_MASK, 'x'), sizeof(Current_Mask) / sizeof(Current_Mask[0]));
        memcpy(Current_IP_SERVER, Configuracion.Get_Configuracion(Direccion_IP_Server, 'x'), sizeof(Current_IP_SERVER) / sizeof(Current_IP_SERVER[0]));

        memcpy(Currnet_Primary_DNS, Configuracion.Get_Configuracion(Dns_One_IP, 'x'), sizeof(Currnet_Primary_DNS) / sizeof(Currnet_Primary_DNS[0]));
        memcpy(Current_Secundary_DNS, Configuracion.Get_Configuracion(Dns_Two_IP, 'x'), sizeof(Current_Secundary_DNS) / sizeof(Current_Secundary_DNS[0]));


        Current_Name_Machine = Configuracion.Get_Configuracion(Nombre_Maquina, "Nombre_Maq");
        Current_SSID = Configuracion.Get_Configuracion(SSID, "Nombre_Red");
        Current_Type_Machine = Configuracion.Get_Configuracion(Tipo_Maquina, 0);
        Current_Server_Port = Configuracion.Get_Configuracion(Puerto_Server, 0);
        Current_Password = Configuracion.Get_Configuracion(Password, "Password_red");
        Current_Type_transmission = Variables_globales.Get_Variable_Global(Gmaster_API_Mode);

        switch (Inactividad_Usuario_Player_Tracking)
        {
        case 90000:
            Current_Timeout_Player_Tracking = "0";
            break;

        case 120000:
            Current_Timeout_Player_Tracking = "1";
            break;

        case 150000:
            Current_Timeout_Player_Tracking = "2";
            break;
        case 180000:
            Current_Timeout_Player_Tracking = "3";
            break;

        case 210000:
            Current_Timeout_Player_Tracking = "4";
            break;

        case 240000:
            Current_Timeout_Player_Tracking = "5";
            break;

        case 270000:
            Current_Timeout_Player_Tracking = "6";
            break;

        case 300000:
            Current_Timeout_Player_Tracking = "7";
            break;

        case 330000:
            Current_Timeout_Player_Tracking = "8";
            break;

        case 360000:
            Current_Timeout_Player_Tracking = "9";
            break;

        default:
            Current_Timeout_Player_Tracking = "0";
            break;
        }

        switch (Tiempo_Transmision_En_Juego)
        {
        case 30:
            Current_Tiempo_Transmission_Game = "0";
            break;

        case 40:
            Current_Tiempo_Transmission_Game = "1";
            break;

        case 50:
            Current_Tiempo_Transmission_Game = "2";
            break;
        case 60:
            Current_Tiempo_Transmission_Game = "3";
            break;

        case 70:
            Current_Tiempo_Transmission_Game = "4";
            ;
            break;

        case 80:
            Current_Tiempo_Transmission_Game = "5";
            break;

        case 90:
            Current_Tiempo_Transmission_Game = "6";
            break;

        case 100:
            Current_Tiempo_Transmission_Game = "7";
            break;

        case 110:
            Current_Tiempo_Transmission_Game = "8";
            break;

        case 120:
            Current_Tiempo_Transmission_Game = "9";
            break;

        default:
            Current_Tiempo_Transmission_Game = "0";
            break;
        }

        switch (Tiempo_Transmision_No_Juego)
        {
        case 150000:
            Current_Tiempo_Transmission_Not_Game = "0";
            break;

        case 180000:
            Current_Tiempo_Transmission_Not_Game = "1";
            break;

        case 210000:
            Current_Tiempo_Transmission_Not_Game = "2";
            break;
        case 240000:
            Current_Tiempo_Transmission_Not_Game = "3";
            break;

        case 270000:
            Current_Tiempo_Transmission_Not_Game = "4";
            break;

        case 300000:
            Current_Tiempo_Transmission_Not_Game = "5";
            break;

        case 330000:
            Current_Tiempo_Transmission_Not_Game = "6";
            break;

        case 360000:
            Current_Tiempo_Transmission_Not_Game = "7";
            break;

        case 480000:
            Current_Tiempo_Transmission_Not_Game = "8";
            break;

        case 600000:
            Current_Tiempo_Transmission_Not_Game = "9";
            break;

        case 1800000:
            Current_Tiempo_Transmission_Not_Game ="10";
            break;
        
        case 3600000:
            Current_Tiempo_Transmission_Not_Game ="11";
            break;
        
        case 5400000:
            Current_Tiempo_Transmission_Not_Game ="12";
            break;

        default:
            Current_Tiempo_Transmission_Not_Game = "0";
            break;
        }

        switch (Tiempo_Inactividad_Maquina)
        {
        case 50:
            Current_Timeout_Inactivity_Machine = "0";
            break;

        case 80:
            Current_Timeout_Inactivity_Machine = "1";
            break;

        case 115:
            Current_Timeout_Inactivity_Machine = "2";
            break;
        case 150:
            Current_Timeout_Inactivity_Machine = "3";
            break;

        case 180:
            Current_Timeout_Inactivity_Machine = "4";
            break;

        case 230:
            Current_Timeout_Inactivity_Machine = "5";
            break;

        default:

            Current_Timeout_Inactivity_Machine = "0";
            break;
        }
        
        Current_Tipo_Socket=Configuracion.Get_Configuracion(Tipo_Conexion);
        /*-----------------------------------------------------------------------------------------------*/
        Current_Id_Maquina=Configuracion.Get_Configuracion(Id_Maquina, "Id_Maquina");

        Current_Controlador_Principal_ = Configuracion.Get_Configuracion_ES(Controlador_P, "Controlador_Principal");
        Current_Metodo_RTC_ = Configuracion.Get_Configuracion_ES(Metodo_Sincro_RTC, "Metodo_SincroRTC");
        Current_Metodo_Contadores_ = Configuracion.Get_Configuracion_ES(Metodo_Conta, "Metodo_Contadores");
        Current_Metodo_Eventos_ = Configuracion.Get_Configuracion_ES(Metodo_Event, "Metodo_Eventos");
        Current_Metodo_Token_ = Configuracion.Get_Configuracion_ES(Metodo_Access_T, "Metodo_Token");

        /*------------------------------------> Genera Json <--------------------------------------------*/
        StaticJsonDocument<1024> jsonDocument;
        jsonDocument["Estado_Config"] = false;
        jsonDocument["ssid"] = Current_SSID;
        jsonDocument["password"] = Current_Password;
        jsonDocument["local_ip"] = IP_toString(Current_IP);
        jsonDocument["puerto"] = Current_Server_Port;
        jsonDocument["nombre_maquina"] = Current_Name_Machine;
        jsonDocument["subnet_mask"] = IP_toString(Current_Mask);
        jsonDocument["gateway"] = IP_toString(Current_IP_GW);
        jsonDocument["primary_dns"] = IP_toString(Currnet_Primary_DNS);
        jsonDocument["secondary_dns"] = IP_toString(Current_Secundary_DNS);
        jsonDocument["tipo_maquina"] = Current_Type_Machine;
        jsonDocument["tipo_transmision"] = Convert_bool_to_String(Current_Type_transmission);
        jsonDocument["Timeout_Player_Tracking"] = Current_Timeout_Player_Tracking; 
        jsonDocument["Timer_Send_Game"] = Current_Tiempo_Transmission_Game; 
        jsonDocument["Timer_Not_Game"] =  Current_Tiempo_Transmission_Not_Game;
        jsonDocument["Timeout_Inactivity_machine"] = Current_Timeout_Inactivity_Machine;
        jsonDocument["Type_Socket"] = Convert_bool_to_String(Current_Tipo_Socket);
        
        jsonDocument["Id_Maquina"]=Current_Id_Maquina;
        jsonDocument["Controlador"] = Current_Controlador_Principal_;
        jsonDocument["Api_Contadores"] = Current_Metodo_Contadores_;
        jsonDocument["Api_Eventos"] = Current_Metodo_Eventos_;
        jsonDocument["Api_RTC"] = Current_Metodo_RTC_;
        jsonDocument["Api_Token"] = Current_Metodo_Token_;

        jsonDocument["Descripcion"]="Falta uno o mas parametros";
        jsonDocument["Codigo"]=404;
        /*------------------------------------------------------------------------------------------------*/
        String Json;
        serializeJson(jsonDocument, Json); /* >Serializa Data< */

        /*----------------------------> Envia Configuración actual <--------------------------------------*/
        request->send(200, "application/json", Json);
        /*------------------------------------------------------------------------------------------------*/
    }
    else
    {

        /* --------------> Parametros de Maquina <--------------------------- */
        String Name_Machine_Update=Json_nombre_maquina;
        uint16_t    Type_Machine_Update=Json_tipo_maquina.toInt();
        /*--------------------------------------------------------------------*/
       /* ----------------> Parametros de red <------------------------------ */
        IPAddress local_ip;
        IPAddress gateway;
        IPAddress subnet_mask;
        IPAddress primary_dns;
        IPAddress secondary_dns;

        /*----------------------------------------------------------------------*/
        String SSID_Update = Json_SSID;
        String Password_Update = Json_PASS;
        String Local_IP_Update = Json_Local_IP;
        uint16_t Puerto_Update = Json_Puerto.toInt();
        String Subnet_Mask_Update = Json_subnet_mask;
        String GW_Update = Json_gateway;
        String DNS_P = Json_primary_dns;
        String DNS_S = Json_secondary_dns;
        String Id_Maquina_Update=Json_Id_Maquina;


        bool Socket_API_Update=String_to_Bool(Json_Socket_API,Gmaster_API_Mode);
        int TimeOut_Player_Tracking_Update;
        int Tiempo_Transmission_Game_Update;
        int Tiempo_Transmission_Not_Game_Update;
        int Timeout_Inactivity_Machine_Update;
        bool Tipo_Socket_Update=String_to_Bool(Json_Tipo_Socket,Tipo_Conexion);

        switch (Json_TimeOut_Player_Tracking.toInt())
        {
        case 0:
            TimeOut_Player_Tracking_Update=90000;
            break;

        case 1:
            TimeOut_Player_Tracking_Update=120000;
            break;

        case 2:
            TimeOut_Player_Tracking_Update=150000;
            break;

        case 3:
            TimeOut_Player_Tracking_Update=180000;
            break;

        case 4:
            TimeOut_Player_Tracking_Update=210000;
            break;

        case 5:
            TimeOut_Player_Tracking_Update=240000;
            break;

        case 6:
            TimeOut_Player_Tracking_Update=270000;
            break;

        case 7:
            TimeOut_Player_Tracking_Update=300000;
            break;

        case 8:
            TimeOut_Player_Tracking_Update=330000;
            break;

        case 9:
            TimeOut_Player_Tracking_Update=360000;
            break;

        default:
            TimeOut_Player_Tracking_Update=90000; 
            break;
        }

        switch (Json_Tiempo_Transmission_Game.toInt())
        {
        case 0:
            Tiempo_Transmission_Game_Update= 30;
            break;

        case 1:
            Tiempo_Transmission_Game_Update=40;
            break;

        case 2:
            Tiempo_Transmission_Game_Update=50;
            break;

        case 3:
            Tiempo_Transmission_Game_Update=60;
            break;

        case 4:
            Tiempo_Transmission_Game_Update=70;
            break;

        case 5:
            Tiempo_Transmission_Game_Update=80;
            break;

        case 6:
            Tiempo_Transmission_Game_Update=90;
            break;

        case 7:
            Tiempo_Transmission_Game_Update=100;
            break;

        case 8:
            Tiempo_Transmission_Game_Update=110;
            break;

        case 9:
            Tiempo_Transmission_Game_Update=120;
            break;

        default:
            Tiempo_Transmission_Game_Update=30; 
            break;
        }

        switch (Json_Tiempo_Transmission_Not_Game.toInt())
        {
        case 0:
            Tiempo_Transmission_Not_Game_Update=150000; /* 2.5 minutos */
            break;

        case 1:
            Tiempo_Transmission_Not_Game_Update=180000;
            break;

        case 2:
            Tiempo_Transmission_Not_Game_Update=210000;
            break;

        case 3:
            Tiempo_Transmission_Not_Game_Update= 240000;
            break;

        case 4:
            Tiempo_Transmission_Not_Game_Update=270000;
            break;

        case 5:
            Tiempo_Transmission_Not_Game_Update=300000;
            break;

        case 6:
            Tiempo_Transmission_Not_Game_Update=330000;
            break;

        case 7:
            Tiempo_Transmission_Not_Game_Update=360000;
            break;

        case 8:
            Tiempo_Transmission_Not_Game_Update=480000;
            break;

        case 9:
            Tiempo_Transmission_Not_Game_Update= 600000;
            break;

        case 10:
            Tiempo_Transmission_Not_Game_Update=1800000;
            break;

        case 11:
            Tiempo_Transmission_Not_Game_Update=3600000;
            break;

        case 12:
            Tiempo_Transmission_Not_Game_Update=5400000;
            break;

        default:
            Tiempo_Transmission_Not_Game_Update=150000; /* 2.5 minutos */
            break;
        }

        switch (Json_Timeout_Inactivity_Machine.toInt())
        {
        case 0:
            Timeout_Inactivity_Machine_Update=50;
            break;

        case 1:
            Timeout_Inactivity_Machine_Update= 80;
            break;

        case 2:
            Timeout_Inactivity_Machine_Update=115; /* 1 Minuto 30s */
            break;

        case 3:
            Timeout_Inactivity_Machine_Update=150; /* 2 Minutos */
            break;

        case 4:
            Timeout_Inactivity_Machine_Update=180; /* 2 Minutos 30s */
            break;

        case 5:
            Timeout_Inactivity_Machine_Update=230; /* 3 Minutos*/
            break;

        default:
            Timeout_Inactivity_Machine_Update=50; /* 30s */
            break;
        }

        String Controlador_Principal_Update=Json_Controlador;
        String Metodo_RTC_Update=Json_Metodo_RTC;
        String Metodo_Contadores_Update=Json_Metodo_Conta;
        String Metodo_Eventos_Update=Json_Metodo_Event;
        String Metodo_Token_Update=Json_Metodo_Token;

        if(SSID_Update!="" &&Password_Update!="" &&Puerto_Update>0&&Subnet_Mask_Update!="" && GW_Update!="" &&Type_Machine_Update>0 && Id_Maquina_Update!="")
        {

            String Update_TimeOut_Player_Tracking_Update;
            String Update_Tiempo_Transmission_Game_Update;
            String Update_Tiempo_Transmission_Not_Game_Update;
            String Update_Inactivity_Machine_Update;

            /*  Valores validos*/

            NVS.begin("Config_ESP32", false);

            String Controlador_Principal_Actual = Configuracion.Get_Configuracion_ES(Controlador_P, "Controlador_Principal");
            String Metodo_RTC__Actual = Configuracion.Get_Configuracion_ES(Metodo_Sincro_RTC, "Metodo_SincroRTC");
            String Metodo_Contadores__Actual = Configuracion.Get_Configuracion_ES(Metodo_Conta, "Metodo_Contadores");
            String Metodo_Eventos__Actual = Configuracion.Get_Configuracion_ES(Metodo_Event, "Metodo_Eventos");
            String Metodo_Token__Actual = Configuracion.Get_Configuracion_ES(Metodo_Access_T, "Metodo_Token");

            bool Socket_API_Actual= Variables_globales.Get_Variable_Global(Gmaster_API_Mode);
            String Id_Maquina_Actual=Configuracion.Get_Configuracion(Id_Maquina, "Id_Maquina");
            

            if(Controlador_Principal_Actual!=Controlador_Principal_Update || Metodo_RTC__Actual!=Metodo_RTC_Update|| Metodo_Contadores__Actual!=Metodo_Contadores_Update||Metodo_Eventos__Actual!=Metodo_Eventos_Update|| Metodo_Token__Actual!=Metodo_Token_Update)
            {

                String joinedString = "";
                joinedString = Controlador_Principal_Update + "|" + Metodo_RTC_Update + "|" + Metodo_Contadores_Update + "|" + Metodo_Eventos_Update + "|" + Metodo_Token_Update;
                
                NVS.putString("Param_API", joinedString);

                String Parameters_Ap[] = {Controlador_Principal_Update, Metodo_RTC_Update, Metodo_Contadores_Update, Metodo_Eventos_Update,Metodo_Token_Update};
              //  NVS.putBytes("Param_API",Parameters_Ap, sizeof(Parameters_Ap));
                Configuracion.Set_Configuracion_ESP32_ES(Controlador_P, Parameters_Ap);
                Configuracion.Set_Configuracion_ESP32_ES(Metodo_Conta, Parameters_Ap);
                Configuracion.Set_Configuracion_ESP32_ES(Metodo_Event, Parameters_Ap);
                Configuracion.Set_Configuracion_ESP32_ES(Metodo_Sincro_RTC, Parameters_Ap);
                Configuracion.Set_Configuracion_ESP32_ES(Metodo_Access_T, Parameters_Ap);
            }

            if(Id_Maquina_Actual!=Id_Maquina_Update)
            {
                String Id_Maquina_tt = Id_Maquina_Update;
                NVS.putString("ID_MQ", Id_Maquina_tt);
                Configuracion.Set_Configuracion_ESP32(Id_Maquina, Id_Maquina_tt);
            }
            if(Socket_API_Actual!=Socket_API_Update)
            {
                bool TRANSMISSION_DATA = Socket_API_Update;
                NVS.putBool("TYPE_TM", TRANSMISSION_DATA);
                bool Type_T = NVS.getBool("TYPE_TM");
                Variables_globales.Set_Variable_Global(Gmaster_API_Mode,Type_T);
            }

            if(Inactividad_Usuario_Player_Tracking!=TimeOut_Player_Tracking_Update)
            {
                int Timer_Player = TimeOut_Player_Tracking_Update;
                NVS.putUInt("TimePlayer", Timer_Player);
                Inactividad_Usuario_Player_Tracking=NVS.getUInt("TimePlayer",90000);
            }

            if(Tiempo_Transmision_En_Juego!=Tiempo_Transmission_Game_Update)
            {
                int Timer_Transmission_In_Game = Tiempo_Transmission_Game_Update; /* Cada 30s*/
                NVS.putUInt("T_En_Juego", Timer_Transmission_In_Game);
                Tiempo_Transmision_En_Juego=NVS.getUInt("T_En_Juego",30);
            }

            if(Tiempo_Transmision_No_Juego!=Tiempo_Transmission_Not_Game_Update)
            {
                int Timer_Transmission_Not_Game = Tiempo_Transmission_Not_Game_Update; /* Cada 2.5 minutos */
                NVS.putUInt("T_No_Juego", Timer_Transmission_Not_Game);
                Tiempo_Transmision_No_Juego=NVS.getUInt("T_No_Juego",150000);
            }

            if(Tiempo_Inactividad_Maquina!=Timeout_Inactivity_Machine_Update)
            {
                int Timer_User = Timeout_Inactivity_Machine_Update;
                NVS.putUInt("TimerUser", Timer_User);
                Tiempo_Inactividad_Maquina = NVS.getUInt("TimerUser", 50);
            }

            bool Tipo_Socket_Actual=Configuracion.Get_Configuracion(Tipo_Conexion);
            if(Tipo_Socket_Actual!=Tipo_Socket_Update)
            {
                bool Conexion_Server = Tipo_Socket_Update;
                NVS.putBool("TYPE_CONNECT", Conexion_Server);
                Configuracion.Set_Configuracion_ESP32(Tipo_Conexion, Conexion_Server);
            }

            uint16_t port_server_actual=Configuracion.Get_Configuracion(Puerto_Server, 0);
            if(port_server_actual!=Puerto_Update)
            {
              //  Serial.println("Puerto diferente...");
                NVS.putUInt("Socket", Puerto_Update);
                Configuracion.Set_Configuracion_ESP32(Puerto_Server, Puerto_Update);
            }

            /* Nombre de la maquina */
            String Name_Actual=Configuracion.Get_Configuracion(Nombre_Maquina, "Nombre_Maq");
            if(Name_Actual!=Name_Machine_Update)
            {
                NVS.putString("Name_Maq", Name_Machine_Update);
                Configuracion.Set_Configuracion_ESP32(Nombre_Maquina, Name_Machine_Update);
            }
            /*  SSID */
            String Red_Actual=Configuracion.Get_Configuracion(SSID, "Nombre_Red");
            if(Red_Actual!=SSID_Update)
            {
                NVS.putString("SSID_DESA", SSID_Update);
                Configuracion.Set_Configuracion_ESP32(SSID, SSID_Update); 
            }
            /*PASSWORD*/
            String Password_Actual=Configuracion.Get_Configuracion(Password, "Password_red");
            if(Password_Actual!=Password_Update)
            {
                NVS.putString("PASS_DESA", Password_Update);
                Configuracion.Set_Configuracion_ESP32(Password, Password_Update);
            }
            /* Tipo Maquina */
            uint16_t Config_Type_Machine_Actual=Configuracion.Get_Configuracion(Tipo_Maquina, 0);
            if(Config_Type_Machine_Actual!=Type_Machine_Update)
            {
                NVS.putUInt("TYPE_MAQ", Type_Machine_Update);
                Configuracion.Set_Configuracion_ESP32(Tipo_Maquina, Type_Machine_Update);
            }
           
           

            local_ip.fromString(Local_IP_Update);
            subnet_mask.fromString(Subnet_Mask_Update);
            gateway.fromString(GW_Update);
            primary_dns.fromString(DNS_P);
            secondary_dns.fromString(DNS_S);


            char DNS_Primary_Starage[4];
            memcpy(DNS_Primary_Starage, Configuracion.Get_Configuracion(Dns_One_IP, 'x'), sizeof(DNS_Primary_Starage) / sizeof(DNS_Primary_Starage[0]));
            
            if(primary_dns[0]!=DNS_Primary_Starage[0]||primary_dns[1]!=DNS_Primary_Starage[1]||primary_dns[2]!=DNS_Primary_Starage[2]||primary_dns[3]!=DNS_Primary_Starage[3])
            {
                uint8_t ip_dns_update[] = {primary_dns[0], primary_dns[1], primary_dns[2], primary_dns[3]};
                NVS.putBytes("Dns_Primary", ip_dns_update, sizeof(ip_dns_update));
                DNS_Primary_Starage[0]=ip_dns_update[0];
                DNS_Primary_Starage[1]=ip_dns_update[1];
                DNS_Primary_Starage[2]=ip_dns_update[2];
                DNS_Primary_Starage[3]=ip_dns_update[3];
                Configuracion.Set_Configuracion_ESP32(Dns_One_IP, DNS_Primary_Starage);
            }
            
            char DNS_Secondary_Starage[4];
            memcpy(DNS_Secondary_Starage, Configuracion.Get_Configuracion(Dns_Two_IP, 'x'), sizeof(DNS_Secondary_Starage) / sizeof(DNS_Secondary_Starage[0]));
            

            if(secondary_dns[0]!=DNS_Secondary_Starage[0]||secondary_dns[1]!=DNS_Secondary_Starage[1]||secondary_dns[2]!=DNS_Secondary_Starage[2]||secondary_dns[3]!=DNS_Secondary_Starage[3])
            {
                uint8_t ip_dns_sec_update[] = {secondary_dns[0], secondary_dns[1], secondary_dns[2], secondary_dns[3]};
                NVS.putBytes("Dns_Secondary", ip_dns_sec_update, sizeof(ip_dns_sec_update));
                DNS_Secondary_Starage[0]=ip_dns_sec_update[0];
                DNS_Secondary_Starage[1]=ip_dns_sec_update[1];
                DNS_Secondary_Starage[2]=ip_dns_sec_update[2];
                DNS_Secondary_Starage[3]=ip_dns_sec_update[3];
                Configuracion.Set_Configuracion_ESP32(Dns_Two_IP, DNS_Secondary_Starage);
            }



            char IP_GW_Storage[4];
            memcpy(IP_GW_Storage, Configuracion.Get_Configuracion(Direccion_IP_GW, 'x'), sizeof(IP_GW_Storage) / sizeof(IP_GW_Storage[0]));


            /* Verifica GW*/
            if(gateway[0]!=IP_GW_Storage[0]||gateway[1]!=IP_GW_Storage[1]||gateway[2]!=IP_GW_Storage[2] ||gateway[3]!=IP_GW_Storage[3])
            {
                uint8_t ip_gw_upd[] = {gateway[0], gateway[1], gateway[2], gateway[3]};
                NVS.putBytes("Dir_IP_GW", ip_gw_upd, sizeof(ip_gw_upd));
                IP_GW_Storage[0]=ip_gw_upd[0];
                IP_GW_Storage[1]=ip_gw_upd[1];
                IP_GW_Storage[2]=ip_gw_upd[2];
                IP_GW_Storage[3]=ip_gw_upd[3];
                Configuracion.Set_Configuracion_ESP32(Direccion_IP_GW, IP_GW_Storage);
            }

            /* Verifica Ip local */
            char IP_Local_[4];
            memcpy(IP_Local_, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(IP_Local_) / sizeof(IP_Local_[0]));

            if(local_ip[0]!=IP_Local_[0]||local_ip[1]!=IP_Local_[1]|| local_ip[2]!=IP_Local_[2] || local_ip[3]!=IP_Local_[3])
            {
                uint8_t ip_local_temp[] = {local_ip[0], local_ip[1], local_ip[2], local_ip[3]};
                NVS.putBytes("Dir_IP", ip_local_temp, sizeof(ip_local_temp));

                /* Actualiza */
                IP_Local_[0]=ip_local_temp[0];
                IP_Local_[1]=ip_local_temp[1];
                IP_Local_[2]=ip_local_temp[2];
                IP_Local_[3]=ip_local_temp[3];
                Configuracion.Set_Configuracion_ESP32(Direccion_IP, IP_Local_);
            }

            /* Verifica sub*/

            char Submask_[4];
            memcpy(Submask_, Configuracion.Get_Configuracion(Direccion_SN_MASK, 'x'), sizeof(Submask_) / sizeof(Submask_[0]));

            if(subnet_mask[0]!=Submask_[0]||subnet_mask[1]!=Submask_[1]|| subnet_mask[2]!=Submask_[2] || subnet_mask[3]!=Submask_[3])
            {
                uint8_t Submask__[] = {subnet_mask[0], subnet_mask[1], subnet_mask[2], subnet_mask[3]};
                NVS.putBytes("Dir_SN_MASK", Submask__, sizeof(Submask__));

                Submask_[0]=Submask__[0];
                Submask_[1]=Submask__[1];
                Submask_[2]=Submask__[2];
                Submask_[3]=Submask__[3];
                Configuracion.Set_Configuracion_ESP32(Direccion_SN_MASK, Submask_);
            }


            NVS.end();
            /*---------------------------------------------------------------------*/

            char IP_Local_P[4];
            char IP_GW_P[4];
            char SN_MASK_P[4];
            char DNS_ONE_IP[4];
            char DNS_TWO_IP[4];

            memcpy(IP_Local_P, Configuracion.Get_Configuracion(Direccion_IP, 'x'), sizeof(IP_Local_P) / sizeof(IP_Local_P[0]));
            memcpy(IP_GW_P, Configuracion.Get_Configuracion(Direccion_IP_GW, 'x'), sizeof(IP_GW_P) / sizeof(IP_GW_P[0]));
            memcpy(SN_MASK_P, Configuracion.Get_Configuracion(Direccion_SN_MASK, 'x'), sizeof(SN_MASK_P) / sizeof(SN_MASK_P[0]));


            memcpy(DNS_ONE_IP, Configuracion.Get_Configuracion(Dns_One_IP, 'x'), sizeof(DNS_ONE_IP) / sizeof(DNS_ONE_IP[0]));
            memcpy(DNS_TWO_IP, Configuracion.Get_Configuracion(Dns_Two_IP, 'x'), sizeof(DNS_TWO_IP) / sizeof(DNS_TWO_IP[0]));

            String SSID_Wifi_Apli = Configuracion.Get_Configuracion(SSID, "Nombre_Red");
            String Password_Wifi_Apli = Configuracion.Get_Configuracion(Password, "Password_red");

            IPAddress Local_IP(IP_Local_P[0], IP_Local_P[1], IP_Local_P[2], IP_Local_P[3]);
            IPAddress Gateway(IP_GW_P[0], IP_GW_P[1], IP_GW_P[2], IP_GW_P[3]);
            IPAddress SubnetMask(SN_MASK_P[0], SN_MASK_P[1], SN_MASK_P[2], SN_MASK_P[3]);
            IPAddress primaryDNS(DNS_ONE_IP[0], DNS_ONE_IP[1], DNS_ONE_IP[2], DNS_ONE_IP[3]);   // optional
            IPAddress secondaryDNS(DNS_TWO_IP[0],DNS_TWO_IP[1], DNS_TWO_IP[2], DNS_TWO_IP[3]); // optional

            uint16_t Port_Config = Configuracion.Get_Configuracion(Puerto_Server, 0);
            String Name_Config = Configuracion.Get_Configuracion(Nombre_Maquina, "Nombre_Maq");
            uint16_t Type_Machine_Config = Configuracion.Get_Configuracion(Tipo_Maquina, 0);
            String Id_Maquina_Config=Configuracion.Get_Configuracion(Id_Maquina, "Id_Maquina");


            String Controlador_Principal_Config = Configuracion.Get_Configuracion_ES(Controlador_P, "Controlador_Principal");
            String Metodo_RTC_Config = Configuracion.Get_Configuracion_ES(Metodo_Sincro_RTC, "Metodo_SincroRTC");
            String Metodo_Contadores_Config = Configuracion.Get_Configuracion_ES(Metodo_Conta, "Metodo_Contadores");
            String Metodo_Eventos_Config = Configuracion.Get_Configuracion_ES(Metodo_Event, "Metodo_Eventos");
            String Metodo_Token_Config = Configuracion.Get_Configuracion_ES(Metodo_Access_T, "Metodo_Token");

           
            Serial.print("Red configurada: ");
            Serial.println(SSID_Wifi_Apli);
            Serial.print("IP address: ");
            Serial.println(WiFi.localIP());
            Serial.print("ESP Mac Address: ");
            Serial.println(WiFi.macAddress());
            Serial.print("Nivel Señal WIFI: ");
            Serial.println(WiFi.RSSI());

            Serial.print("Dirección IP configurada: ");
            Serial.println(Local_IP);
            Serial.print("Dirección IP Gateway configurada: ");
            Serial.println(Gateway);
            Serial.print("Subnet Mask Configurada : ");
            Serial.println(SubnetMask);
            Serial.print("Puerto configurado: ");
            Serial.println(Port_Config);
            Serial.print("Nombre_Maquina Configurado: ");
            Serial.println(Name_Config);
            Serial.print("Tipo de maquina Condigurado: ");
            Serial.println(Type_Machine_Config);

            StaticJsonDocument<1024> jsonDocument;
            jsonDocument["Estado_Config"] = true;
            jsonDocument["ssid"] = SSID_Wifi_Apli;
            jsonDocument["password"] = Password_Wifi_Apli;
            jsonDocument["local_ip"] = Local_IP.toString();
            jsonDocument["puerto"] = Port_Config;
            jsonDocument["nombre_maquina"] = Name_Config;
            jsonDocument["subnet_mask"] = SubnetMask.toString();
            jsonDocument["gateway"] = Gateway.toString();
            jsonDocument["primary_dns"] = primaryDNS.toString();
            jsonDocument["secondary_dns"] = secondaryDNS.toString();
            jsonDocument["tipo_maquina"] = Type_Machine_Config;


            
            switch (TimeOut_Player_Tracking_Update)
            {
            case 90000:
                Update_TimeOut_Player_Tracking_Update = "0";
                break;

            case 120000:
                Update_TimeOut_Player_Tracking_Update = "1";
                break;

            case 150000:
                Update_TimeOut_Player_Tracking_Update = "2";
                break;
            case 180000:
                Update_TimeOut_Player_Tracking_Update = "3";
                break;

            case 210000:
                Update_TimeOut_Player_Tracking_Update = "4";
                break;

            case 240000:
                Update_TimeOut_Player_Tracking_Update = "5";
                break;

            case 270000:
                Update_TimeOut_Player_Tracking_Update = "6";
                break;

            case 300000:
                Update_TimeOut_Player_Tracking_Update = "7";
                break;

            case 330000:
                Update_TimeOut_Player_Tracking_Update = "8";
                break;

            case 360000:
                Update_TimeOut_Player_Tracking_Update = "9";
                break;

            default:

                Update_TimeOut_Player_Tracking_Update = "0";
                break;
            }

            switch (Tiempo_Transmission_Game_Update)
            {
            case 30:
                Update_Tiempo_Transmission_Game_Update = "0";
                break;

            case 40:
                Update_Tiempo_Transmission_Game_Update = "1";
                break;

            case 50:
                Update_Tiempo_Transmission_Game_Update = "2";
                break;
            case 60:
                Update_Tiempo_Transmission_Game_Update = "3";
                break;

            case 70:
                Update_Tiempo_Transmission_Game_Update = "4";
                ;
                break;

            case 80:
                Update_Tiempo_Transmission_Game_Update = "5";
                break;

            case 90:
                Update_Tiempo_Transmission_Game_Update = "6";
                break;

            case 100:
                Update_Tiempo_Transmission_Game_Update = "7";
                break;

            case 110:
                Update_Tiempo_Transmission_Game_Update = "8";
                break;

            case 120:
                Update_Tiempo_Transmission_Game_Update = "9";
                break;

            default:
                Update_Tiempo_Transmission_Game_Update = "0";
                break;
            }

            switch (Tiempo_Transmission_Not_Game_Update)
            {
            case 150000:
                Update_Tiempo_Transmission_Not_Game_Update = "0";
                break;

            case 180000:
                Update_Tiempo_Transmission_Not_Game_Update = "1";
                break;

            case 210000:
                Update_Tiempo_Transmission_Not_Game_Update = "2";
                break;
            case 240000:
                Update_Tiempo_Transmission_Not_Game_Update = "3";
                break;

            case 270000:
                Update_Tiempo_Transmission_Not_Game_Update = "4";
                break;

            case 300000:
                Update_Tiempo_Transmission_Not_Game_Update = "5";
                break;

            case 330000:
                Update_Tiempo_Transmission_Not_Game_Update = "6";
                break;

            case 360000:
                Update_Tiempo_Transmission_Not_Game_Update = "7";
                break;

            case 480000:
                Update_Tiempo_Transmission_Not_Game_Update = "8";
                break;

            case 600000:
                Update_Tiempo_Transmission_Not_Game_Update = "9";
                break;

            case 1800000:
                Update_Tiempo_Transmission_Not_Game_Update = "10";
                break;

            case 3600000:
                Update_Tiempo_Transmission_Not_Game_Update = "11";
                break;

            case 5400000:
                Update_Tiempo_Transmission_Not_Game_Update = "12";
                break;

            default:
                Update_Tiempo_Transmission_Not_Game_Update = "0";
                break;
            }

            switch (Timeout_Inactivity_Machine_Update)
            {
            case 50:
                Update_Inactivity_Machine_Update = "0";
                break;
            case 80:
                Update_Inactivity_Machine_Update = "1";
                break;
            case 115:
                Update_Inactivity_Machine_Update = "2";
                break;
            case 150:
                Update_Inactivity_Machine_Update = "3";
                break;
            case 180:
                Update_Inactivity_Machine_Update = "4";
                break;
            case 230:
                Update_Inactivity_Machine_Update = "5";
                break;

            default:
                Update_Inactivity_Machine_Update = "0";
                break;
            }


            Serial.print("Tipo de transmisión configurado: ");
            if(Socket_API_Update)
                Serial.println("API");
            else
                Serial.println("Socket");

            Serial.print("Type_Socket Configurado: ");
            if(Tipo_Socket_Update)
                Serial.println("TCP");
            else
                Serial.println("UDP");

            jsonDocument["tipo_transmision"] =Convert_bool_to_String(Socket_API_Update);
            jsonDocument["Timeout_Player_Tracking"] = Update_TimeOut_Player_Tracking_Update;
            jsonDocument["Timer_Send_Game"] = Update_Tiempo_Transmission_Game_Update;
            jsonDocument["Timer_Not_Game"] = Update_Tiempo_Transmission_Not_Game_Update;
            jsonDocument["Timeout_Inactivity_machine"] = Update_Inactivity_Machine_Update;
            jsonDocument["Type_Socket"] = Convert_bool_to_String(Tipo_Socket_Update);
            jsonDocument["Id_Maquina"]=Id_Maquina_Config;

            jsonDocument["Controlador"] = Controlador_Principal_Config;
            jsonDocument["Api_Contadores"] =Metodo_Contadores_Config;
            jsonDocument["Api_Eventos"] = Metodo_Eventos_Config;
            jsonDocument["Api_RTC"] = Metodo_RTC_Config;
            jsonDocument["Api_Token"] = Metodo_Token_Config;

            jsonDocument["Descripcion"] = "Configuracion aplicada con exito!";
            jsonDocument["Codigo"] = 200;
            
            String Json;
            serializeJson(jsonDocument, Json);
            request->send(200, "application/json", Json);
            // delay(500);
            // ESP.restart(); /* Configuracion con exito!*/ 
        }
        else
        {
            String Update_TimeOut_Player_Tracking_Update;
            String Update_Tiempo_Transmission_Game_Update;
            String Update_Tiempo_Transmission_Not_Game_Update;
            String Update_Inactivity_Machine_Update;

            StaticJsonDocument<500> jsonDocument;
            jsonDocument["Estado_Config"] = false;
            jsonDocument["ssid"] = SSID_Update;
            jsonDocument["password"] = Password_Update;
            jsonDocument["local_ip"] = Local_IP_Update;
            jsonDocument["puerto"] = Puerto_Update;
            jsonDocument["nombre_maquina"] = Name_Machine_Update;
            jsonDocument["subnet_mask"] = Subnet_Mask_Update;
            jsonDocument["gateway"] = GW_Update;
            jsonDocument["primary_dns"] = DNS_P;
            jsonDocument["secondary_dns"] = DNS_S;
            jsonDocument["tipo_maquina"] = Type_Machine_Update;

 
            switch (TimeOut_Player_Tracking_Update)
            {
            case 90000:
                Update_TimeOut_Player_Tracking_Update = "0";
                break;

            case 120000:
                Update_TimeOut_Player_Tracking_Update = "1";
                break;

            case 150000:
                Update_TimeOut_Player_Tracking_Update = "2";
                break;
            case 180000:
                Update_TimeOut_Player_Tracking_Update = "3";
                break;

            case 210000:
                Update_TimeOut_Player_Tracking_Update = "4";
                break;

            case 240000:
                Update_TimeOut_Player_Tracking_Update = "5";
                break;

            case 270000:
                Update_TimeOut_Player_Tracking_Update = "6";
                break;

            case 300000:
                Update_TimeOut_Player_Tracking_Update = "7";
                break;

            case 330000:
                Update_TimeOut_Player_Tracking_Update = "8";
                break;

            case 360000:
                Update_TimeOut_Player_Tracking_Update = "9";
                break;

            default:

                Update_TimeOut_Player_Tracking_Update = "0";
                break;
            }

            switch (Tiempo_Transmission_Game_Update)
            {
            case 30:
                Update_Tiempo_Transmission_Game_Update = "0";
                break;

            case 40:
                Update_Tiempo_Transmission_Game_Update = "1";
                break;

            case 50:
                Update_Tiempo_Transmission_Game_Update = "2";
                break;
            case 60:
                Update_Tiempo_Transmission_Game_Update = "3";
                break;

            case 70:
                Update_Tiempo_Transmission_Game_Update = "4";
                ;
                break;

            case 80:
                Update_Tiempo_Transmission_Game_Update = "5";
                break;

            case 90:
                Update_Tiempo_Transmission_Game_Update = "6";
                break;

            case 100:
                Update_Tiempo_Transmission_Game_Update = "7";
                break;

            case 110:
                Update_Tiempo_Transmission_Game_Update = "8";
                break;

            case 120:
                Update_Tiempo_Transmission_Game_Update = "9";
                break;

            default:
                Update_Tiempo_Transmission_Game_Update = "0";
                break;
            }

            switch (Tiempo_Transmission_Not_Game_Update)
            {
            case 150000:
                Update_Tiempo_Transmission_Not_Game_Update = "0";
                break;

            case 180000:
                Update_Tiempo_Transmission_Not_Game_Update = "1";
                break;

            case 210000:
                Update_Tiempo_Transmission_Not_Game_Update = "2";
                break;
            case 240000:
                Update_Tiempo_Transmission_Not_Game_Update = "3";
                break;

            case 270000:
                Update_Tiempo_Transmission_Not_Game_Update = "4";
                break;

            case 300000:
                Update_Tiempo_Transmission_Not_Game_Update = "5";
                break;

            case 330000:
                Update_Tiempo_Transmission_Not_Game_Update = "6";
                break;

            case 360000:
                Update_Tiempo_Transmission_Not_Game_Update = "7";
                break;

            case 480000:
                Update_Tiempo_Transmission_Not_Game_Update = "8";
                break;

            case 600000:
                Update_Tiempo_Transmission_Not_Game_Update = "9";
                break;

             case 1800000:
                Update_Tiempo_Transmission_Not_Game_Update = "10";
                break;

            case 3600000:
                Update_Tiempo_Transmission_Not_Game_Update = "11";
                break;

            case 5400000:
                Update_Tiempo_Transmission_Not_Game_Update = "12";
                break;

            default:
                Update_Tiempo_Transmission_Not_Game_Update = "0";
                break;
            }

            switch (Timeout_Inactivity_Machine_Update)
            {
            case 50:
                Update_Inactivity_Machine_Update = "0";
                break;
            case 80:
                Update_Inactivity_Machine_Update = "1";
                break;
            case 115:
                Update_Inactivity_Machine_Update = "2";
                break;
            case 150:
                Update_Inactivity_Machine_Update = "3";
                break;
            case 180:
                Update_Inactivity_Machine_Update = "4";
                break;
            case 230:
                Update_Inactivity_Machine_Update = "5";
                break;

            default:
                Update_Inactivity_Machine_Update = "0";
                break;
            }

            jsonDocument["tipo_transmision"] = Convert_bool_to_String(Socket_API_Update);
            jsonDocument["Timeout_Player_Tracking"] = Update_TimeOut_Player_Tracking_Update;
            jsonDocument["Timer_Send_Game"] = Update_Tiempo_Transmission_Game_Update;
            jsonDocument["Timer_Not_Game"] = Update_Tiempo_Transmission_Not_Game_Update;
            jsonDocument["Timeout_Inactivity_machine"] = Update_Inactivity_Machine_Update;
            jsonDocument["Type_Socket"] = Convert_bool_to_String(Tipo_Socket_Update);
            jsonDocument["Id_Maquina"] = Id_Maquina_Update;

            jsonDocument["Controlador"] = Controlador_Principal_Update;
            jsonDocument["Api_Contadores"] = Metodo_Contadores_Update;
            jsonDocument["Api_Eventos"] = Metodo_Eventos_Update;
            jsonDocument["Api_RTC"] = Metodo_RTC_Update;
            jsonDocument["Api_Token"] = Metodo_Token_Update;

            jsonDocument["Descripcion"] = "Error en parametro tipo maquina";
            jsonDocument["Codigo"] = 501;
            String Json_res;
            serializeJson(jsonDocument, Json_res);
            request->send(200, "application/json", Json_res);
        }
    }
  });
}



