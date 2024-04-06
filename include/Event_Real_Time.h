
#define OPEN_DOOR       0x11
#define CLOSE_DOOR      0x12
#define REMOVE_STACKER  0x1B
#define STACKER_INSTALL 0x1C



class Event_Real_Time
{
private:
    bool Last_State_Stacker=false;
    bool Report_Event=false;
    unsigned long Start_Time_Event=0;


    bool Last_State_Door=false;
    bool Report_Event_Door=false;
    unsigned long Start_Time_Event_Door=0;
public:
    void EVENT_REAL_TIME(int Pin_Stacker=35,int Pin_Door_Machine=34,int TimeOut_Signal=2000);
    void SetTimeOut_Signal(int TimeOut);
    void Enable_Event_Real_Time(bool Enable);
    bool Reset_Event_Class(void);
    
};


