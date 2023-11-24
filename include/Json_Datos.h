#include "Stream.h"
#include "ArduinoJson.h"
String Buffer_Json_info_IM(String Mac,
    int Type_MQ,                    // Type Machine 0 -> 9
    String Version_Firware,         // Version Firmware
    String Fecha_Bl,                // last put in bootloader mode
    int Status_SD,                  // Status memory SD  1-> 0
    String EL,                      // Size of available SD in MB
    String EU,                      // Size of use SD in MB
    String EM,                      // Size Maximun SD in MB
    int FTP,                        // Status FTP Mode
    String TP,String Hopper);                     // Temperature of the Tarjet

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
    String Restricted_Pool_ID);      //$74 = Restricted Pool ID          = 0000


StaticJsonDocument<512> Deserialize(const String& Recepcion_json);