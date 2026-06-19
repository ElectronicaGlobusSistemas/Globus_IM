struct GameFeatures {
    float loss_rate;           
    float win_variance;        
    float time_since_last_win; 
};

void Test_IA();
float calculate_variance();
void IA_ActualizarDatos();
void IA_TerminarSesion();
void IA_IniciarSesion(float current_credit);
int predict_rage_quit(GameFeatures f);

void conectarMQTT();
void Init();
void Lopper();