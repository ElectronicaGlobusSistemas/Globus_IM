#include "ESP32Time.h"
#include <string>

#include "Contadores.h"
#include "Clase_Variables_Globales.h"
#ifndef Accounting_H  // Protección contra inclusión múltiple
#define Accounting_H



typedef struct
{
    int Com;
    bool IsSuccess;
    String Message;
} AccountingRequest_t;

extern QueueHandle_t QueueAccounting;

class API_Accounting
{
private:
    /* data */

    unsigned long Timer_Start_Premios=0;
    unsigned long Timer_End_Premios_SAS=0;
    int TimeOut_Premios_SAS=10000;
    bool Firts_SAS=false;
    bool Send_Handler_Cancel_Credit=false;

public:
    
    void Task_Accounting();
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

    bool Actualiza_Modulo_Accounting(int Com, Contadores_SAS contadores, ESP32Time RTC, Variables_Globales Variables_globales,bool IsSuccess=false,String Message="");
    bool Init_Cola_Accounting(void);
    bool Transmite_Contadores_Api_Cola(int Com, bool IsSuccess, String Message);
};  
std::string IP_toString_Acc(char IP_Char[]);
#endif // Accounting_H
