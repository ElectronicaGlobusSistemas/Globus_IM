bool Almacena_Serial_Cashless_Extern(unsigned int Dato);
bool Compara_Serial_Cashless_Extern(void);

void Genera_Registro_Maquina(void);
unsigned char Interroga_Est_Reg_AFT_Funcion(void);
unsigned char Registra_Machine(void);
unsigned char  Transmite_Poll_Registro_AFT_Mq(int Type_Transmite_Poll);
unsigned char Delete_Registro_Machine(void);