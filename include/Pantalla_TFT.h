
void Init_TFT_Display(void);
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingDataPtr, int len);
bool Init_Player_TFT(String User_Name ="", int Total_Playertracking_Points=0, int Total_Points_Tickets=0, int User_Level=0,uint32_t Saldo_Canjeable=0, uint32_t Saldo_Sin_Restriccion=0,uint32_t Saldo_No_Canjeable=0,int Current_Playertracking_Points=0,int Current_Points_Tickets=0);



void Prueba_TFT(void);

bool Close_Player_TFT();
bool Get_Status_Sesion_Player(void);