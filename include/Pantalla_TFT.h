
void Init_TFT_Display(void);
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingDataPtr, int len);
bool Init_Player_TFT(String User_Name, int Playertracking_Points, int Points_Tickets, int User_Level,uint32_t Saldo_Canjeable=0, uint32_t Saldo_Sin_Restriccion=0,uint32_t Saldo_No_Canjeable=0);
void Prueba_TFT(void);

bool Close_Player_TFT();
bool Get_Status_Sesion_Player(void);