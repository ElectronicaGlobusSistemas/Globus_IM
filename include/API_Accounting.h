#include "ESP32Time.h"
#include <string>

#ifndef Accounting_H  // Protección contra inclusión múltiple
#define Accounting_H

class API_Accounting
{
private:
    /* data */
public:

    unsigned long Convert_Counter(char Buffer[]);
    unsigned long Convert_Counter_4digit(char buffer[]);
    void  Save_Handpay_Informations(char Buffer_MET[128], char Contador[]);
    bool Sed_Handpay_Informatios(ESP32Time RTC,String Data);
    void Report_Informations_SAS(void);

    void Load_Premios_SAS(void);

};
std::string IP_toString_Acc(char IP_Char[]);
#endif // Accounting_H
