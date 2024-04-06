



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


class TITO
{

private:

public:

   bool Printed_Ticket_Amount(); /* Imprime Ticket */
   bool Validations_Ticket_Expires(); /* Valida Expiración de ticket */
   bool Ticket_Transfer_To_Machine (); /* Transferencia de ticket to machine */
   bool Init_TITO(void); /* Inicializa TITO */
   bool Set_Custom_Format_Ticket(String Format); /* Formato de ticket */
   bool Varify_Support_Custom_Ticket(void); /* Verificacion de ticket */
};







