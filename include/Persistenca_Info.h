

#include "ESP32Time.h"
#include <string>
#include <vector>


#ifndef Persistencia_H
#define Persistencia_H

class Persistenca_Info
{
private:
    // Vector para almacenar las transacciones pendientes
    std::vector<String> Maq_Datos;

    unsigned long TimeoutInicial=0;
    unsigned long TimeoutFinal=0;
    bool Procesando=false;

public:
    void enviarInformacionMaquina(const String &json);
    bool Enviar_Info(const String &json,int Timeout=5000);
    bool SD_(const String& json,String Ruta);
    bool Init_Archive_Backup();

    String Test(void);
    void Task_Info(int Timeout=10000);

    uint32_t Generate_CheckSum(char Ip_ip[],char Ip_Server[],int Type_Machine);
    /*0xDEADBEEF*/
    String Eventos_Accounting(int ComandoGpollSAS, int Evento_SAS);
    String Contadores_Accounting(int ComandoGpollSAS);
};

#endif 
