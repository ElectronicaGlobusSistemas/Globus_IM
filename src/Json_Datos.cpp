
/*Configuración de entidades para transmitir datos en formato JSON*/

#include "ArduinoJson.h"
#include "Json_Datos.h"


/****************************************************************************************
 **************************** > ENTIDAD INFO TARJETA < **********************************
 ****************************************************************************************/

String Buffer_Json_info_IM(
    String Mac,
    int Type_MQ,
    String Version_Firware,
    String Fecha_Bl,
    int Status_SD,
    String EL,
    String EU,
    String EM,
    int FTP,
    String TP, String Hopper)
{
    const size_t capacity = JSON_OBJECT_SIZE(20);
    StaticJsonDocument<capacity> doc;

    String Buffer_Out;
    JsonObject Comando;
    Comando = doc["INFO_TARJETA"].createNestedObject();

    //Info General
    Comando["MAC"]=Mac;
    Comando["Tipo_Maquina"]=Type_MQ;
    Comando["Version_Firmware"]=Version_Firware;
    Comando["Fecha_Bootlader"]=Fecha_Bl;
    Comando["Hopper_Enable"]=Hopper;
    JsonObject INFOTARJETA_0_Info_Memoria = Comando.createNestedObject("Info_Memoria");
    // Info SD
    INFOTARJETA_0_Info_Memoria["Estatus_SD"]=Status_SD;
    INFOTARJETA_0_Info_Memoria["Memoria"] = EM;
    INFOTARJETA_0_Info_Memoria["Libre"] = EL;
    INFOTARJETA_0_Info_Memoria["Usado"] = EU;
    INFOTARJETA_0_Info_Memoria["FTP"] = FTP;
    INFOTARJETA_0_Info_Memoria["Temperatura"] = TP;

    serializeJson(doc, Buffer_Out);
    doc.clear();
    return Buffer_Out;
}

/****************************************************************************************
 **************************** > ENTIDAD INFO CASHLESS < *********************************
 ****************************************************************************************/

String Buffer_Json_info_Cashless(

    String AFT_Game_Lock_and_Status, //$74 = AFT Game Lock and Status    = FF
    String Asset_Number,             //$74 = Asset Number                = 01000000
    String Available_Transfers,      //$74 = Available Transfers         = 13
    String Host_Cashout_Status,      //$74 = Host Cashout Status         = 02
    String AFT_Status,               //$74 = AFT Status                  = FA
    String Max_History_Index,        //$74 = Max History Index           = 7F
    String Current_Cashable,         //$74 = Current Cashable            = 0000010000
    String Current_Restricted,       //$74 = Current Restricted          = 0000000000
    String Current_Nonrestricted,    //$74 = Transfer Limit              = 0999990000
    String Transfer_Limit,           //$74 = Restricted Expiration       = 00000000
    String Restricted_Expiration,    //$74 = Restricted Expiration       = 00000000
    String Restricted_Pool_ID)       //$74 = Restricted Pool ID          = 0000
{
    const size_t capacity = JSON_OBJECT_SIZE(20);
    StaticJsonDocument<capacity> doc;

    String Buffer_Out;
    JsonObject Comando;
    Comando = doc["INFO_CASHLESS"].createNestedObject();

    //INFO GENERAL
    Comando["AFT_l"]=AFT_Game_Lock_and_Status;
    Comando["Asset_N"]=Asset_Number;
    Comando["Available_T"]=Available_Transfers;
    Comando["Cashout_S"]=Host_Cashout_Status;

    Comando["AFT_S"]=AFT_Status;
    Comando["Max_H"]=Max_History_Index;
    Comando["C_C"]=Current_Cashable;
    Comando["C_R"]=Current_Restricted;

    Comando["C_N"]=Current_Nonrestricted;
    Comando["T_L"]=Transfer_Limit;
    Comando["R_E"]=Restricted_Expiration;
    Comando["R_P"]=Restricted_Pool_ID;
    

    serializeJson(doc, Buffer_Out);
    doc.clear();
    return Buffer_Out;
}


String Deserialize()
{
    String Recepcion_json;
    StaticJsonDocument<0> filter;
    filter.set(true);
    const size_t capacity = JSON_OBJECT_SIZE(20);
    StaticJsonDocument<capacity> doc;

    DeserializationError error = deserializeJson(doc, Recepcion_json, DeserializationOption::Filter(filter));

    if (error)
    {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return "";
    }else
    {
        
    }

}