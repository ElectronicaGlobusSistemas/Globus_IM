#include "Stream.h"
#include <iostream>

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

using namespace std;

#define  Info_MQ_AFT 1
#define   Interroga_Registro 2
#define  Buffer_registro_Mq_ 3
#define Asset_Pos_Id_        4
#define Buffer_RX_AFT_       5
#define Buffer_RX_Cashless   6


#define CASHABLES            1
#define RESTRICTED           2
#define NON_RESTRICTED       3



#define Location_Tick 0
#define Adress_1_Tick 1
#define Adress_2_Tick 2
#define Restricted_Ticket_Title_Tick 3
#define Debit_Ticket_Title_Tick 4

class Buffer_RX_AFT
{

private: // Variables Privadas Buffer
  char Info_MQ_AFT_[38];
  char Interrog_registro_ [258];
  char Buffer_registro_Mq [128];
  char Buffer_registro_Mq_AFT[20];

  char Asset_Pos_Id[ 8 ];
  char Buffer_RX_AFT_MQ[128];
  char Buffer_RX_Transfer_AFT[128];
  char Buffer_RX_Credit_Cashless[128];


  char Buffer_RX_Transfer_EFT[128];
  char Buffer_RX_Credit_Cashless_EFT[128];

  char Buffer_Rx_TITO[128];
  char Buffer_Rx_TITO_58[128];
  char Buffer_Rx_TITO_70[128];

  char Buffer_Rx_TITO_4D[128];
  char Buffer_Rx_TITO_3D;

  char Buffer_Rx_TITO_7C[128];

  char Cashout_Type=0xAA;
  char Amount_Ticket[6]={0xAA,0xAA,0xAA,0xAA,0xAA,0xAA};

  

public:
  
  bool Set_RX_AFT(int Filtro_buffer, char[]);  // Metodo Set buff
  char *Get_RX_AFT(int Filtro_buffer);
  bool Clear_Buffer(int Filtro_Buffer);
  bool Set_RX_AFT_2(int Filtro_buffer);


  bool  Set_Key_Register_AFT(char Buffer[],bool Reset=false);
  bool  Delete_Key_Register_AFT(bool Reset=true);
  char* Get_Key_Register_AFT();
  bool  Set_Buffer_Transfer_AFT(char Buffer[]);

  
  char* Get_Bufffer_Transfer_AFT();
  
  bool Init_Buffer_Transfer_AFT(bool Boolean);
  String Get_Key_Register_AFT_String();
  String Get_AFT_Info_Config();

  char* Get_Buffer_Transfer_EFT();
  bool  Set_Buffer_Transfer_EFT(char Buffer[]);
  bool Init_Buffer_Transfer_EFT(bool Boolean);



  bool Set_Buffer_TITO(char Buffer[]);
  char* Get_Buffer_TITO(void);
  bool Init_Buffer_TITO(void);

  bool Set_Buffer_TITO_58(char Buffer[]);
  char *Get_Buffer_TITO_58(void);
  bool Init_Buffer_TITO_58(void);

  bool Set_Buffer_TITO_70(char Buffer[]);
  char *Get_Buffer_TITO_70(void);
  bool Init_Buffer_TITO_70(void);

  bool Set_Buffer_TITO_4D(char Buffer[]);
  char *Get_Buffer_TITO_4D(void);
  bool Init_Buffer_TITO_4D(void);

  bool Set_Buffer_TITO_3D_3E(char Buffer);
  char Get_Buffer_TITO_3D_3E(void);
  bool Init_Buffer_TITO_3D_3E(void);

  bool Set_Buffer_TITO_7C(char Buffer[]);
  char *Get_Buffer_TITO_7C(void);
  bool Init_Buffer_TITO_7C(void);


  bool Set_Cashout_Type_Ticket(char Cashout_Type);
  //bool Amount_Ticket(char Buffer[]);

  char Get_Cashout_Type_Informacion(void);
  char* Get_Amount_Ticket(void);

  bool Attend_Tito_Request(int Evento,bool Status_Machine,bool Transaction_Status=false,bool Enable_Tito=false);

  
  


};


class Transsaccion_Cashless
{


  private:
  bool Estado_Tranmision_Auto_Registro=false;
  uint32_t Num_Trans_ID;
  char Counter_Trans_ID[7];
  char Credit_To_Load[20];
  char ACK_Bonus[128]; 


  String Location="";
  String Adress_1=""; 
  String Adress_2=""; 
  String Restricted_Ticket_Title=""; 
  String Debit_Ticket_Title="";
  

  public:

    bool Set_Parameter_Print_Ticket(String Location_S, String Adr1_S,String Adr2_S,String Restricted_Ticket_S, String Debit_Title_S);
    
    String Get_Parameter_Print_Ticket(int Filtro);
    bool Cashless_Loading_Transaction_AFT(char res[]);
    bool Cashless_Download_Transaction_AFT(char res[]);
    bool Enable_Cashless_Loads(bool Enable);
    bool AFT_Registration(bool);


    bool Transaccion_Bonus_Cashless(bool);
    /* AFT*/
    bool Save_Transaction_ID_Number(bool);
    bool Delete_Transaction_ID_Number();
    bool Increase_Transaction_Number_ID();
    char* Get_Trans_ID(void);
    uint32_t Get_Trans_ID_Int(void);
    /*EFT*/
    unsigned char Get_Trans_ID_EFT(void);
    bool Increase_Transaction_Number_ID_EFT(void);


   

    bool Set_Inicial_Trans_ID(uint32_t Trans_ID_Memory);
    bool Sincroniza_Transaction_Number_ID(uint32_t Trans_ID_Maq);
    bool Delete_Trans_ID(bool Status=true);

    bool Set_Amount_To_Load(uint32_t Credit_Cashables=0, uint32_t Credit_Restringidos=0, uint32_t Credit_No_Restringidos=0);

    bool Set_Credit_To_Load(char res[]);
    char *Get_Credit_To_Load(void);

    bool Read_Machine_Registration(void);

    bool Cashless_Loading_Transaction_EFT(char res[]);
    bool Cashless_Download_Transaction_EFT(char res[]);

    bool Set_ACK_Bonus( char res []);
    char* Get_ACK_Bonus(void);
    bool Delete_ACK_Bonus(void);

    bool Init_API_Server(void);
    uint32_t Get_Credit_Number(int Type);
    uint32_t BCDtoUint32(char Credit_To_Load[], int Select);
    uint32_t BCDtoUint32_Pos(char Credit_To_Load[], int Select, int Inicial_Index = 0);
    void Registra_Maquina_Auto(void);
    bool Transmite_Info_Registro(String Json);
    void Set_Reintento_Registro(bool Set);
    bool Get_Reintento_Registro();
    
   
};  
String Token_Generator_Update(String Url);