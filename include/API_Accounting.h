#include "ESP32Time.h"
#include <string>

#ifndef Accounting_H  // Protección contra inclusión múltiple
#define Accounting_H

class API_Accounting
{
private:
    /* data */

    unsigned long Timer_Start_Premios=0;
    unsigned long Timer_End_Premios_SAS=0;
    int TimeOut_Premios_SAS=120000;
    bool Firts_SAS=false;
    bool Send_Handler_Cancel_Credit=false;

public:

    unsigned long Convert_Counter(char Buffer[]);
    unsigned long Convert_Counter_4digit(char buffer[]);
    void  Save_Handpay_Informations(char Buffer_MET[128], char Contador[]);
    bool Send_Handpay_Informatios(ESP32Time RTC,String Data);
    void Report_Handpay_Informations_SAS(bool Token_Cashless);

    void Delete_PremioSAS_On_List(const char *FileName);
    void Load_Premios_SAS(void);

    bool Get_Flag_Handler_Cancel_Credit(void);
    void Change_Flag_Handler_Cancel_Credit(bool Status_Flag);
    bool Send_Counter_App(void);

};
std::string IP_toString_Acc(char IP_Char[]);
#endif // Accounting_H
