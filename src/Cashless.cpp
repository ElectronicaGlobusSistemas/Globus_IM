



// 0-3 id comando
 //4- 27 maq

 /*
res[5] = mac[0];
    res[6] = mac[1];
    res[7] = mac[2];
    res[8] = mac[3];
    res[9] = mac[4];
    res[10] = mac[5];
    res[11] = mac[6];
    res[12] = mac[7];
    res[13] = mac[8];
    res[14] = mac[9];
    res[15] = mac[10];
    res[16] = mac[11];
    res[17] = mac[12];
    res[18] = mac[13];
    res[19] = mac[14];
    res[20] = mac[15];
    res[21] = mac[16];
    res[22] = '|';


28-29 tipo maq


3-34 version 
fecha boot 35-43

44-46   estado std
47-50   espacio std
51-52   FTP.


int valida_contraseña_Bootloader(void)
{
    int i,j;
    char cadena_Texto[16]={"St4rt$B0ot&Glob5"};

    j=0;
    for(i=4; i<20; i++)
    {
        if(buffer[i]!=cadena_texto[j])
        return 0;

        j++;
    }
    return 1;
}

*/