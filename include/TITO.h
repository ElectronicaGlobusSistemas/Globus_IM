



#define Ticket_Insertado 67
#define Ticket_Transferencia_Completa 68
#define Cash_Out_Ticket_Printed       3D



/* Set custom AFT ticket data 
76

Address 01
Command 76
Length 01-nn  Leng of bytes, not crc
Function  nn --00 Set Data ELements--80 Clear All Data Parameters
Data Code nn   00 Custom AFT Location
Data length nn Length of data element following
---> adicional data code/Lentgh/Dataelement
CRC 

EJEMPLO Custom AFT Location (00) TEXT (0) 

    01 76 04 00 00 01 30 65 47


response 

Address 01
Command  76
Length 00-nn Number of bytes following  not CRC
Data Code 00 code for ech custom data element currently
CRC


/* Ticket Data element custom 

00  Custom AFT location ASCII text (40Max)
01  Custom AFT address 1 ASCII text (40Max)
02  Custom AFT address 2 ASCII text (40Max)
03  Custom AFT Graphis selector  ASCII text (3Max)
10 Custom AFT ticket Title  ASCII text (16Max)



NOTAS:

    72 si  el Flag bit 5  Esta en 1  usa la config Custom  y esta en 0 la por defecto.
    74  y en el bit 2 en 1 RESPONDE SI Soporta  Custom ticket 


*/



#include "Arduino.h"
#include <string>

#include "ESP32Time.h"
#include "time.h"
extern ESP32Time RTC; // Objeto contiene hora y fecha


#define Excepcion_57 57

#define  Cashable_Ticket                   (0x00)
#define  Restricted_Promotional_Ticket     (0x01)
#define  Not_Waiting_for_System_validation (0x80)
#define  Ticket_Type_Not_Identified        (0xAA)

#define  FAIL_COMUNICATIONS_WITH_MACHINE   (0x50)
#define  FAIL_CONEXION_WITH_SERVER         (0x51)
#define  AMOUNT_INVALID                    (0x52)
#define  PROBLEM_WITH_SERVER               (0x53)


#define VALID_CASHABLE_TICKET                  (0x00)
#define VALID_RESTRICTED_PROMOTIONAL_TICKET    (0x01)
#define VALID_NONRESTRICTED_PROMOTIONAL_TICKET (0x02)
#define UNABLE_TO_VALIDATE                     (0x80)
#define NOT_A_VALID_VALIDATION_NUMBER          (0x81)
#define VALIDATION_NUMBER_NOT_IN_SYSTEM        (0x82)
#define TICKET_MARKED_PENDING_IN_SYSTEM        (0x83)
#define TICKET_ALREADY_REDEEMED                (0x84)
#define TICKET_EXPIRED                         (0x85)
#define VALIDATION_INFORMATION_NOT_AVAILABLE   (0x86)
#define TICKET_AMOUNT_DOES_NOT_MATCH_SYSTEM    (0x87)
#define TICKET_AMOUNT_EXCEEDS_LIMIT            (0x88)
#define REQUEST_FOR_CURRENT_TICKETS_STATUS     (0xFF)


class TITO
{

private:

    int Validations_System_ID;
    char Validation_Number[8];

    char Validation_Number_Out[8];
    

    bool Solicitud_Token_Ticket=false;
    bool Solicitud_Redeemed_Ticket=false;
    bool Attend_Tito_OK=false;
    bool Attend_Tito_In_OK=false;

    bool Ticket_OK=false;
    bool Ticket_In_Process=false;

    char Amount_Ticket_Transfer[6];
    bool Current_4D_3E=false;


    bool Flag_New_Transfer_Ticket_In=false;
    bool Flag_New_Transfer_Ticket_Out=false;
    bool Flag_Parcial_New_Transfer_Ticket_In=false;
    bool Flag_Parcial_New_Transfer_Ticket_Out= false;
    bool Flag_New_Transfer_Ticket__Out_Pending_=false; 


    unsigned long Timeout_Tito_Transfer_Inicial=0;
    unsigned long Timeout_Tito_Transfer_Final=0;
    

public:
   
   int Convert_2BNR_Int(char HighByte,char LowByte);
   bool Printed_Ticket_Amount(); /* Imprime Ticket */
   bool Validations_Ticket_Expires(); /* Valida Expiración de ticket */
   bool Ticket_Transfer_To_Machine (); /* Transferencia de ticket to machine */
   bool Init_TITO(void); /* Inicializa TITO */
   bool Set_Custom_Format_Ticket(String Format); /* Formato de ticket */
   bool Varify_Support_Custom_Ticket(void); /* Verificacion de ticket */

   bool Pending_Cashout_Information_Response(char Info[]);
   void Ticket_Out(int Evento);

   int  Check_Ticket_In(void);
   bool Generate_Key_Ticket_Out(void);
   bool Reedeme_Ticket_In(void);

   bool Set_Parameter_Ticket(String  Validacion_Number_); 
   char* Get_Validacion_Number(void);
   uint8_t Get_Validacion_System_ID(void);

//    std::string IP_toString_Ticket(char IP_Char[]);

    bool Status_Ticket_Out(char Buffer_4D[], ESP32Time RTC);
    bool Status_Ticket_In_Data(char Buffer_4D[], ESP32Time RTC);
    bool Ticket_Information_Capture(char Buffer_70[],ESP32Time);
   uint32_t Convert_5BCD_Uint32(char Buffer[],int Inicial_Index);
   uint32_t Convert_8BCD_Uint32(char Buffer[], int Inicial_Index);
   uint32_t Convert_4BCD_Uint32(char Buffer[],int Inicial_Index);
   bool Remove_Validacion_Number(void);
   bool Remove_System_ID(void);

 
   void Request_Handle_Tito(void);
   bool Handle_Event_Tito(int Evento,bool Enable);

   void Silicitud_Reedemed_Ticket_Http(bool Status); 
   void Solicitud_Token_Ticket_Http(bool Status);
   bool Get_Status_Reedened_Ticket_Http(void);
   bool Get_Status_Token_Ticket_Http(void);
   

   bool Get_Status(void);
   void Set_Status(bool Set);

   String Validacion_Number(char Buffer[]);

   bool Get_Status_3D(void);
   void Set_Status_3D(bool Status);

   bool Update_Ticket(int Code, int Type_Ticket);

   
   void Status_Process_Ticket(bool Set);
   bool Get_Status_Process_Ticket(void);


   bool Increase_Transaction_Number_ID_Tito(void);
   bool Set_Inicial_Trans_ID_Tito(int New_Validation_System_ID);


   bool Get_Status_Ticket_In(void);
   
   void Set_Status_Ticket_In(bool Set);

   bool Requerimiento_TITO_Ticket_Out(int Evento, bool Habilita_Tito, bool Solo_Cashless, bool Solo_Tito);

   bool Consult_Ticket(String Ticket_Informations);
   bool Requerimiento_TITO_Ticket_In(int Evento, bool Habilita_Tito);
   char * Get_Amount_Ticket_Transfer(void);
   bool Set_Amount_Ticket_Transfer(uint32_t Amount_Transfer);
   bool Set_Validations_Number_Out(String);
   char* Get_Validatioins_Number_Out(void);

   void Set_Confirma_Ticket(bool Confirma);
   bool Get_Confirma_Ticket(void);
   void Load_Pending_Ticket_Transactions(void);

   void New_Transfer_Ticket(const String& json);
   bool Send_Transfer_Ticket(const String & json);
   bool Updated_Ticket_Counters(String Type_Transaccion);
   void Save_Ticket_Transaction(void);

   void Available_Ticket_Transfer(int Timeout=25000);
   void Process_New_Transfer(void);

   void Set_Flag_New_Ticket_In(bool);
   void Set_Flag_New_Ticket_Out(bool);
   bool Get_Flag_New_Ticket_In();
   bool Get_Flag_New_Ticket_Out();

   void Set_Flag_Ticket_Out_Pending(int);
   bool Get_Flag_Ticket_Out_Pending(void);

};







