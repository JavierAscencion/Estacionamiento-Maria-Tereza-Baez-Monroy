//2018/06/19
// Basado en: Programa de ejemplo 10 para comunicarme con el módulo ENC28J60.
// Cliente TCP.
// Incluimos las definiciones necesarias de la placa utilizada.
#include "Plantilla_Inicio.c"
// Define el stack usado: ENC28J60.
#define STACK_USE_CCS_PICENS   1
// Utilizamos el stack para el protocolo ICPM.
#define STACK_USE_ICMP  1
// Utilizamos el stack para el protocolo ARP.
#define STACK_USE_ARP 1
// Utilizamos el stack para el protocolo TCP.
#define STACK_USE_TCP 1
#if STACK_USE_CCS_PICENS
 #define STACK_USE_MCPENC 1
#endif

// Definimos los pines utilizados.
#define  PIN_ENC_MAC_SO    PIN_C4
#define  PIN_ENC_MAC_SI    PIN_C5
#define  PIN_ENC_MAC_CLK   PIN_C3

#define  PIN_ENC_MAC_CS    PIN_C1
#define  PIN_ENC_MAC_RST   PIN_C0
#define  PIN_ENC_MAC_INT   PIN_D2
#define  PIN_ENC_MAC_WOL   PIN_D3

// Incluimos las definiciones necesarias para utilizar el stack TCP/IP.
#include "tcpip/stacktsk.c" 

// Dirección IP de la PC (destino).
IP_ADDR server;
// Puerto TCP.
#define EXAMPLE_TCP_PORT   (int16)7654
//int reintento_envio;
// Microchip VendorID, MAC: 00-04-A3-XX-XX-XX.

void MACAddrInit(void)
{
   MY_MAC_BYTE1=0x00;
   MY_MAC_BYTE2=0x04;
   MY_MAC_BYTE3=0xA3;
   MY_MAC_BYTE4=0x00;
   MY_MAC_BYTE5=0x00;
   MY_MAC_BYTE6=0x22;
}

void IPAddrInit(void) {
   // IP del dispositivo.
   MY_IP_BYTE1=192;
   MY_IP_BYTE2=168;
   MY_IP_BYTE3=1;
   MY_IP_BYTE4=152;

   // Puerta de enlace.
   MY_GATE_BYTE1=192;
   MY_GATE_BYTE2=168;
   MY_GATE_BYTE3=1;
   MY_GATE_BYTE4=254;

   // Máscara de Subred.
   MY_MASK_BYTE1=255;
   MY_MASK_BYTE2=255;
   MY_MASK_BYTE3=255;
   MY_MASK_BYTE4=0;
}

void ServerAddrInit(void) {
   // IP del servidor..
   server.v[0]=192;
   server.v[1]=168;
   server.v[2]=1;
   server.v[3]=150;
}
//this function is called by MyTCPTask() when the specified socket is connected
//to the PC running the TCPSERVER.EXE demo.
//returns TRUE if BUTTON2 was pressed, therefore we must disconnect the socket

int8 TCPConnectedTask(TCP_SOCKET socket) {
   char c;
   //static int8 counter;
   //char str[20];
   static int8 button1_held;
   
   if (TCPIsGetReady(socket)) {//ESCUCHA PUERTO TCP
      //fprintf(DEBUG,"\r\nLEE SOCKET");
//   if(!espera)   {
//      lcd_gotoxy(1,4);
//      lcd_putc("LEE SOCKET          ");         }
      edo_str=1;//bandera de dato recibido en socket actual
      i=0;
      while ( TCPGet(socket, &c) ) {
         rxtcp[i++]=c;
         if (c=='@') {
            edo_str=0;
            lcd_gotoxy(20,4);
            printf(lcd_putc,"%c",c);
         }
         if (i>port_size) {i=port_size;}
         rxtcp[i]=0;
      }
   }

//send message over TCP
   //if(envia_pc) fprintf(DEBUG,"envia_pc:%u !button1_held:%u TCPIsPutReady(socket):%u\r\n",envia_pc,button1_held, TCPIsPutReady(socket) );
   if (envia_pc && !button1_held && TCPIsPutReady(socket)) {//ENVIA MENSAJE POR TCP
      button1_held=TRUE;
      TCPPutArray(socket,txtcp,size_tx_tcp);//SOCKET,DATO,NO.BYTES
      TCPFlush(socket);
      reintento_envio=0;
      envia_pc=0;
      //fprintf(DEBUG,"Envio:%s\r\n",txtcp);
//      if(!espera)   {
//      lcd_gotoxy(1,4);
//      printf(lcd_putc,"Envio:%s       ",txtcp);}
   }//////////////////
   else if (envia_pc && !button1_held && !TCPIsPutReady(socket)) {//SI NO PUEDE VACIAR DATOS ROMPE CONEXION
      reintento_envio++;
      if(reintento_envio>=10) TCPDisconnect(socket);
   }///////////////////
   
   if (!envia_pc) {
      button1_held=FALSE;
   }
   
   //#if defined(ESTADO_entrada04)//CIERRA LAS CONEXIONES
   //if (ESTADO_entrada04) {
     // return(TRUE);
   //}
  //#endif

   return(TRUE);
}

void MyTCPTask() {
   static TICKTYPE lastTick;
   static TCP_SOCKET socket=INVALID_SOCKET;
   static enum {
      MYTCP_STATE_NEW=0, MYTCP_STATE_ARP_REQ=1, MYTCP_STATE_ARP_WAIT=2,
      MYTCP_STATE_CONNECT=3, MYTCP_STATE_CONNECT_WAIT=4,
      MYTCP_STATE_CONNECTED=5, MYTCP_STATE_DISCONNECT=6,
      MYTCP_STATE_FORCE_DISCONNECT=7
   } state=0;
   static NODE_INFO remote;
   TICKTYPE currTick;
   int8 dis;

   currTick=TickGet();

   switch (state) {
      case MYTCP_STATE_NEW:
         memcpy(&remote.IPAddr, &server, sizeof(IP_ADDR));
         //fprintf(DEBUG,"\n Solicitud ARP.");
         if(!espera)   {
         lcd_gotoxy(1,4);
         lcd_putc("Solicitud ARP.      ");}
         state=MYTCP_STATE_ARP_REQ;

      case MYTCP_STATE_ARP_REQ:
         if (ARPIsTxReady()) {
            ARPResolve(&remote.IPAddr);
            lastTick=currTick;
            state=MYTCP_STATE_ARP_WAIT;
         }
         break;

      case MYTCP_STATE_ARP_WAIT:
         if (ARPIsResolved(&remote.IPAddr, &remote.MACAddr)) {
            state=MYTCP_STATE_CONNECT;
            //fprintf(DEBUG,"\nCONECTANDO...      ");
            if(!espera)   {
            lcd_gotoxy(1,4);
            lcd_putc("CONECTANDO...       ");}
         }
         else if (TickGetDiff(currTick, lastTick) > (TICKS_PER_SECOND * 2)) {
            state=MYTCP_STATE_ARP_REQ;
         }
         break;

      case MYTCP_STATE_CONNECT:
         socket2=socket=TCPConnect(&remote, EXAMPLE_TCP_PORT);
         if (socket!=INVALID_SOCKET) {
            lastTick=TickGet();
            state=MYTCP_STATE_CONNECT_WAIT;
         }
         else {
            //fprintf(DEBUG,"\nError en el socket.");
            if(!espera)   {
               lcd_gotoxy(1,4); 
               lcd_putc("Error en el socket. ");}
         }
         break;

      case MYTCP_STATE_CONNECT_WAIT:
         if (TCPIsConnected(socket)) {
            state=MYTCP_STATE_CONNECTED;
            //fprintf(DEBUG,"\nCONECTADO! ");
            if(!espera)   {
               lcd_gotoxy(1,4);
               lcd_putc("CONECTADO           ");}
            output_high(salida06);
            inicializado=1;
         }
         else if (TickGetDiff(currTick, lastTick) > (TICKS_PER_SECOND * 10)) {
            state=MYTCP_STATE_FORCE_DISCONNECT;
         }
         break;

      case MYTCP_STATE_CONNECTED:
         if (TCPIsConnected(socket)) {
            dis=TCPConnectedTask(socket);
            //fprintf(DEBUG,"\ndis:%u ",dis);
            if (dis) {
               //state=MYTCP_STATE_DISCONNECT;
               lastTick=currTick;
            }
         }
         else {
            //fprintf(DEBUG,"\nDESCONECTADO.   ");
            //state=MYTCP_STATE_CONNECT;
            state=MYTCP_STATE_DISCONNECT;
            //output_low(salida06);
         }
         break;

      case MYTCP_STATE_DISCONNECT:
         //fprintf(DEBUG,"\nDESCONECTANDO.   ");
         if(!espera)   {
            lcd_gotoxy(1,4);
            lcd_putc("DESCONECTADO       ");}
         output_low(salida06);
         inicializado=0;
         if (TCPIsPutReady(socket)) {
            state=MYTCP_STATE_FORCE_DISCONNECT;
         }
         else if (TickGetDiff(currTick, lastTick) > (TICKS_PER_SECOND * 10)) {
            state=MYTCP_STATE_FORCE_DISCONNECT;
         }
         break;

      case MYTCP_STATE_FORCE_DISCONNECT:
         TCPDisconnect(socket);
         state=MYTCP_STATE_CONNECT;
         if(!espera)   {
            lcd_gotoxy(1,4);
            lcd_putc("DESCONECTADO       ");}
         output_low(salida06);
         inicializado=0;
         break;
   }
}

////wiegand///
#int_TIMER1 //se utiliza timer 1 porque el 0 esta asignado al wdt
void TIMER1_isr(void){
wiegand_cuenta++;
time_relay1++;
time_boleto++;
cuenta++;
cta_lcd++;
set_timer1(3036); //50 ms
}

#int_ext
void EXT_isr(void){//data1
   while (!input(PIN_B0) ) {}
   wiegand_cuenta=0;
   wieg=1;
   data[sub_indice]=1;
   sub_indice++;
   if(sub_indice==wieg_size)  {
      wieg_full=1;
      //wiegand_read_card();
   }
}

#int_ext1
void EXT1_isr(void){//data0
   while (!input(PIN_B1) ) {}
   wiegand_cuenta=0;
   wieg=1;
   data[sub_indice]=0;
   sub_indice++;
   if(sub_indice==wieg_size)  {
      wieg_full=1;
      //wiegand_read_card();
   }
}
///////////////////////
//funciones extra
void wiegand_read_card();
void enviar_tcp();
/////////boleto////////////////////
void reset();
void encri();
void barra1();
void barra2();
void CR();
void LF();
void ticket();
void sensores();
void llaves();
void rd_eeprom();
void zeller();
void horario();
void fecha();

void main(void) {
   //fprintf(DEBUG,"\r\n\nCLIENTE TCP/IP\r\n");
   lcd_putc("\fINICIANDO");
   setup_adc_ports(NO_ANALOGS);
   setup_adc(ADC_OFF);
   
   init_ext_eeprom();//iniciar memoria
   lcd_init();//iniciar LCD
   llaves();
   
   MACAddrInit();
   IPAddrInit();
   ServerAddrInit();
   StackInit();

   setup_timer_1(T1_INTERNAL|T1_DIV_BY_8);//Setup timer: Reloj interno, preescaler= 8
   enable_interrupts(INT_TIMER1);//Habilito interrupción particular del TIMER1
   set_timer1(3036);//Carga del TMR1
   ext_int_edge(0,L_TO_H);       //Asigno flancos de subida
   ext_int_edge(1,L_TO_H);       //Asigno flancos de subida
   enable_interrupts(INT_EXT1);
   enable_interrupts(INT_EXT);
   enable_interrupts(GLOBAL);//Habilito interrupciones globales

   envia_pc=0;
   //int linked_out=0;
   edo_str=envia_pc=0;
   re_bol=0;
   //leyendo validacion de pago
   int vigencia;
   vigencia=read_ext_eeprom(10);
   if(vigencia!=1) {
      write_ext_eeprom(10,1);
      write_ext_eeprom(11,0);
      write_ext_eeprom(12,0);
      write_ext_eeprom(13,0);
   }
   pago=read_ext_eeprom(11);//
   dias_prueba=read_ext_eeprom(12);//dias de prueba
   dia_temp=read_ext_eeprom(13);//
   lcd_gotoxy(1,1);
   printf(lcd_putc,"Dias:%u PAGO:%d",dias_prueba,pago);
   lcd_gotoxy(1,2);
   printf(lcd_putc,"Restan:%u ",(dia_limite-dias_prueba) );
   delay_ms(1000);
   setup_wdt(WDT_ON);
   //////////////////
   hrs2=espera=fmensaje=0;
   reset();
   while(TRUE) {
      restart_wdt();
      StackTask();
      MyTCPTask();
      if( (dias_prueba>dia_limite)&&(pago!=1) ){//pruebas
         lcd_gotoxy(1,1);
         lcd_putc("TIEMPO DE PRUEBA");
         lcd_gotoxy(1,2);
         lcd_putc("  HA EXPIRADO   ");
         if(edo_str){
            edo_str=0;
            strcpy(XX,rxtcp);
            if( (XX[0]=='P')&&(XX[1]=='A')&&(XX[2]=='G')&&(XX[3]=='A')&&(XX[4]=='D')&&(XX[5]=='O') ){
               pago=1;
               write_ext_eeprom(11,pago);//
               strcpy (txtcp, "BOLETERA_PAGADO");
               size_tx_tcp=strlen(txtcp);
               enviar_tcp();
            }
         }
      }
      else{
         wiegand_read_card();
         //sensores();
         if(edo_str){//bandera de dato recibido en socket actual
            edo_str=0;
            //fprintf(DEBUG,"\r\nrxtcp:%s",rxtcp);
            //printf(lcd_putc,"\frxtcp:%s",rxtcp);
            strcpy(XX,rxtcp);
            //fprintf(U1PRINTER,"\frxtcp:%s",XX);
            rd_eeprom();
            //lcd_gotoxy(1,3);
            //printf(lcd_putc,"rxtcp:%s",rxtcp);
            //lcd_gotoxy(1,4);
            //printf(lcd_putc,"b:%Ld                ",b);
            switch (b) {
              case 1: {//IMPRIMIR BOLETO
                  espera=1;
                  printf(lcd_putc,"\f\n   TOME SU BOLETO\n Y AVANCE POR FAVOR");
                  ticket();
                  output_high(salida01);//abre entrada
                  output_high(salida02);//abre entrada
                  relay1=1;
                  time_relay1=0;
                  flag_pluma=1;
                  //sprintf(txtcp,barra_codi);//CONFIRMACION DE BOLETO IMPRESO
                  //size_tx_tcp=strlen(txtcp);
                  //enviar_tcp();
                  cta_lcd=0;
                  re_bol=1;
                  break;}
              case 2: {//VIGENCIA POR VENCER O VENCIDA
                  break;}
              case 3: {//no activa en entrada
                  break;}
              case 4: {//ABRIR ENTRADA
                  //lcd_putc("\f\n     BIENVENIDO");
                  output_high(salida01);//abre entrada
                  output_high(salida02);//abre entrada
                  flag_pluma=1;
                  relay1=1;
                  time_relay1=0;
                  b=0;
                  break;}
              case 5: {//CUPO LLENO
                  break;}
              case 6: {//ACTIVA BOLETOS
                  break;}
              case 7: {//MENSAJES DE LCD
                     memset(lcd_men, 0, sizeof(lcd_men) );
                     for(i=0;i<strlen(XX);i++) lcd_men[i]=XX[instruccion_size+i];
                     lcd_putc("\f");
                     for(i=0;i<strlen(lcd_men);i++){
                        if(i==0)lcd_gotoxy(1,1);
                        else if(i==20) lcd_gotoxy(1,2);
                        else if(i==40) lcd_gotoxy(1,3);
                        else if(i==60) lcd_gotoxy(1,4);
                        printf(lcd_putc,"%c",lcd_men[i]);
                     }
                     espera=1;
                     cta_lcd=0;
                     break;}//
              case 8: {//MOITOREO DESACTIVADO
                     break;}//
              case 9: {//CONSULTA ENTRADAS SIN CONEXION
                  break;}
              case 10:{//CONFIGURA FECHA Y HORA
                  fecha();
                  break;}
              case 11:{//ultima conexion
                  break;}
              case 13:{//RESET DE FOLIOS
                  break;}
              case 14:{//PAGO EFECTUADO
                  pago=1;
                  write_ext_eeprom(11,pago);//
                  strcpy (txtcp, "BOLETERA_PAGADO");
                  size_tx_tcp=strlen(txtcp);
                  enviar_tcp();
                  break;}
            }//end switch
         }//end dato recibido tcp
         //////////////////////////////////////
         if( (relay1)&&(time_relay1>=21) ){
            relay1=0;
            output_low(salida01);
            output_low(salida02);
         }
         //if( (time_boleto>60)&&(re_bol) )  re_bol=0;
         
         if( (espera)&&(cta_lcd>=100) ) espera=0;
         
         if( (cuenta>=20)&&(espera==0) ){//CADA 2 SEGUNDOS VERIFICA FECHA Y HORA
             ds1307_get_time(hrs,min,sec);
             ds1307_get_date(day,month,yr,dow);
             if(hrs2!=hrs){
               dia=31;//ultimo dia del mes
               mes2=10;//octubre, mes en el que cambia horario invierno
               zeller();
               hrs2=hrs;
               if( (dia_temp!=day)&&(pago!=1) ){
                  dia_temp=day;
                  dias_prueba++;
                  write_ext_eeprom(12,dias_prueba);//dias de prueba
                  write_ext_eeprom(13,dia_temp);//
               }
             }//FIN CONFIGURA AUTMATICO FECHA Y HORA
             //cuenta_lcd=0;
             puntos=!puntos;
             if( (!espera)&&(!en1) ){
               lcd_gotoxy(1,1);
               lcd_putc("                    ");
               lcd_gotoxy(1,2);//printf(lcd_putc,"  %02d %02d %02d %02d/%02d/%02d   ",hrs_e,min_e,sec_e,day_e,month,yr);
               if(puntos==0)   printf(lcd_putc,"   %02d %02d %02d/%02d/%02d    ",hrs,min,day,month,yr);
               else   printf(lcd_putc,"   %02d:%02d %02d/%02d/%02d    ",hrs,min,day,month,yr);
               cambio_msj++;
               lcd_gotoxy(1,3);
               if(cambio_msj>=10) lcd_putc("   WWW.ACCESA.ME    ");//COMPLEJO CITY ANGELÓPOLIS  
               else lcd_putc("       ACCESA       ");
               lcd_gotoxy(1,4);
               lcd_putc("                    ");
               if(cambio_msj>=20) cambio_msj=0;
             }
             cuenta=0;
         }//END SEGUNDO
      }
      //if( (MACIsLinked()==0)&&(inicializado)&&(linked_out==0) ){//cable off
      if( (MACIsLinked()==0)&&(inicializado) ){//cable off
         //linked_out=1;
         TCPDisconnect(socket2);
      }
      //////////////////////////////////////
   }//end true
}//end main

int convertir_to_entero(char *cadena){
   int valor = 0;
        if(cadena=='0') valor=0;
   else if(cadena=='1') valor=1;
   else if(cadena=='2') valor=2;
   else if(cadena=='3') valor=3;
   else if(cadena=='4') valor=4;
   else if(cadena=='5') valor=5;
   else if(cadena=='6') valor=6;
   else if(cadena=='7') valor=7;
   else if(cadena=='8') valor=8;
   else if(cadena=='9') valor=9;
   return valor;
}

void fecha(){//falta revisar si se deshabilitan las int globales
   disable_interrupts(INT_RDA);
   //delay_ms(1000);
//   lcd_gotoxy(1,2);
//   lcd_putc("\f     CONFIGURA      ");//
//   lcd_gotoxy(1,3);
//   lcd_putc("     FECHA/HORA     ");
   
   setup_wdt(WDT_ON);
   restart_wdt();
   
   day=(convertir_to_entero(XX[instruccion_size])*10)+convertir_to_entero(XX[instruccion_size+1]);
   hrs=(convertir_to_entero(XX[instruccion_size+7])*10)+convertir_to_entero(XX[instruccion_size+8]);
   min=(convertir_to_entero(XX[instruccion_size+9])*10)+convertir_to_entero(XX[instruccion_size+10]);
   sec=(convertir_to_entero(XX[instruccion_size+11])*10)+convertir_to_entero(XX[instruccion_size+12]);
   
   month=(convertir_to_entero(XX[instruccion_size+2])*10)+convertir_to_entero(XX[instruccion_size+3]);
   yr= (convertir_to_entero(XX[instruccion_size+4])*10)+convertir_to_entero(XX[instruccion_size+5]);
   dow= convertir_to_entero(XX[instruccion_size+6]);
   /////////////////////
   ds1307_set_date_time(day,month,yr,dow,hrs,min,sec); //dia,mes,año(2 digitos), ,hora,min,seg
   //lcd_gotoxy(1,1);
   //printf(lcd_putc,"      %02d:%02d:%02d      ",hrs,min,sec);
   //lcd_gotoxy(1,2);
   //printf (lcd_putc,"     %02d/%02d/2%03d     ",day,month,yr);
   //lcd_gotoxy(1,3);
   //lcd_putc("  DATOS GUARDADOS   ");
   lcd_gotoxy(1,4);
   lcd_putc("  Hora Actualizada  ");
   delay_ms(2500);
   restart_wdt();
   //if(demo==1) printf("\r\nXX:%s",XX);
   enable_interrupts(INT_RDA);
}

void wiegand_read_card(){
 if(wieg_full){
   //disable_interrupts(GLOBAL);//Deshabilito las interrupciones globales
   deci=0;
   for(i=1;i<sub_indice-1;i++) deci = (deci<<1)|data[i];
   sprintf(YY,"%08LX",deci);// imprime en un arreglo (printf en ram)
   sprintf(txtcp,"SA1-%08LX",deci);// imprime en un arreglo (printf en ram)
   //fprintf(DEBUG,"TA:%s\r\n",txtcp);
   //lcd_gotoxy(1,3);
   //printf(lcd_putc,"TA:%s",txtcp);
   size_tx_tcp=strlen(txtcp);
   enviar_tcp();
   //validar_tag_salida();//FUNCION PARA ABRIR AUTOMATICAMENTE Y GUARDAR ESTADO DE ANTIPASS
   wieg_full=0;
   wieg=sub_indice=0;
   wiegand_cuenta=0;
   memset(data, 0, sizeof(data) );
 }//END TARJETA LEIDA
 if( (wieg==1)&&(wiegand_cuenta>=3) ) {
      wigmal++;
      //fprintf(DEBUG,"CAPTURA INC. %u",wigmal );
      //lcd_gotoxy(1,4);
      //printf(lcd_putc,"CAPTURA INC. %u",wigmal );
      //lcd_putc("CAPTURA INCOMPLETA  ");
      wieg_full=0;
      deci=0;
      ///////////UNIFICANDO EL TAMAÑO DEL DATO LEIDO/////////////////
      deteccion_nulo=0;
      pre_cuenta=0;
      //////////////////////
      wieg=sub_indice=i=0;
      wiegand_cuenta=0;
      for(i=0;i<wieg_size;i++) data[i]=0;
 }
}

void enviar_tcp(){
   envia_pc=1;
   StackTask();
   MyTCPTask();
}
////////////////////
void zeller(){
int16 dia_s1,dia_s2,dia_s3,diac,mesc,dia_fecha;//(day,month,yr,dow,hrs,min,sec
   int siglo,asiglo,resultado,di2;
   diac=31;
   mesc=10;
   siglo=20;
   asiglo=yr;
   //fprintf(U1PRINTER,"Dia:%Ld mes:%Ld año:%d siglo:%d asiglo:%d\r\n",diac,mesc,asiglo,siglo,asiglo);
   if(mesc<3){
      mesc+=12;
      asiglo--;
   }
   dia_s1=(siglo/4)+5*siglo;
   dia_s2=dia_s1+asiglo+(asiglo/4);
   dia_s3=dia_s2+((mesc+1)*26)/10;
   dia_fecha=(dia_s3+diac)%7;//dia de fin de mes
   if(dia_fecha==0) dia_fecha=7;
   //fprintf(U1PRINTER,"Dia:%Ld ",dia_fecha);
   resultado=31-(dia_fecha-1);
   //fprintf(U1PRINTER,"Dia:%d ",resultado);
///////////revisar cambio de horario////////////
ds1307_get_date(day,month,yr,dow);
ds1307_get_time(hrs,min,sec);
di2=read_ext_eeprom(14);
if((month==4)&&(dow==1)&&(hrs>=2)&&(di2!=1)){//cambia a horario de verano
 hrs=hrs+1;
 ds1307_set_date_time(day,month,yr,dow,hrs,min,sec); //dia,mes,año(2 digitos),diasemana,hora,min,seg
 di2=1;
 write_ext_eeprom(14,di2);//(1,0)horario de invierno
}
//if((month==10)&&(dow==1)&&(day==resultado)&&(hrs==3)&&(di2!=day)){//cambia a horario de invierno
else if((month==10)&&(day==resultado)&&(hrs>=3)&&(hrs<=5)&&(di2!=day)){//cambia a horario de invierno
 hrs=hrs-1;
 ds1307_set_date_time(day,month,yr,dow,hrs,min,sec); //dia,mes,año(2 digitos),diasemana,hora,min,seg
 di2=day;
 write_ext_eeprom(14,di2);
 }
}
////////////////////
void sensores(){
if(en1!=input(entrada01) ){//SENSOR 1 bobina DE ENTRADA
   delay_ms(200);
   if(en1!=input(entrada01)){
      en1=input(entrada01);
      //fen4=0;
      if(en1==1){
         //lcd_putc("\fEN1-ENTRADA");
         re_bol=0;
         //strcpy(txtcp,"EN1-ENTRADA");
         //size_tx_tcp=strlen(txtcp);
         //enviar_tcp();
      }
      else {
         output_low(salida04);
         re_bol=0;
         //lcd_putc("\fEN1-SINDETECCION");
         //strcpy(txtcp,"EN1-SINDETECCION");
         //size_tx_tcp=strlen(txtcp);
         //enviar_tcp();
      }
   }
}//end S1

if(en4!=input(entrada04) ){//BOTON DE TICKET
   //delay_ms(10);
   if(en4!=input(entrada04)){
      en4=input(entrada04);
      //printf(lcd_putc,"\fbot:%d au:%d hbot:%d cpo:%d "en3,en1,fen3,cupo );
      //if( (en4)&&(en1)&&(fen4!=1)&&(cupo!=1) ){
      if( (en4)&&(en1)&&(!re_bol) ){
      //if( (en4)&&(en1) ){
         time_boleto=0;
         //re_bol=1;
         sprintf(txtcp,"BOTON_BOLETO");//01-id de camion 
         size_tx_tcp=strlen(txtcp);
         enviar_tcp();
      }//end boton activado
      /*else if( (en4==1)&&(cupo) ){
            lcd_gotoxy(1,2);
            lcd_putc("     CUPO LLENO     ");
      }*/
   }//verificacion boton
}//en boton ticket
/////////////////////
/*if(en5!=input(entrada05) ){//SENSOR DE BARRERA ENTRADA
   delay_ms(200);
   if(en5!=input(entrada05) ){
      en5=input(entrada05);
      if( (en5==0)&&(flag_pluma==0) ){
         espera_tcp();
         for(i=0;i<16;i++) tcp_XX[i]=barrera[i];//BARRERAABIERTAS2
         size_tx_tcp=i;
         enviar_tcp();//enviar a pc//avisar por tcp
      }
      else if(en5==1) flag_pluma=0;
   }
}*/
/////////////////////
}
//EPSON
void ticket(){//U1PRINTER //HHMMSSFFFFFFFFZ
   // 1-Direccion1 para Folio
   // 2-Direccion2 para Folio
   // 3-Direccion3 para Folio
   //int cta_l,cta_m,cta_h;
   char fol_bol[20];
   /*//leer
   cta_l=read_ext_eeprom(3);
   cta_m=read_ext_eeprom(2);
   cta_h=read_ext_eeprom(1);
   folio=0;//24bits 16,777,215
   folio=cta_h;//
   folio=(folio<<8)|cta_m;
   folio=(folio<<8)|cta_l;
   //fin de leer
   folio++;
   //printf("Total:%Ld\r\n",folio);
   write_ext_eeprom(3,folio);//graba
   write_ext_eeprom(2,folio>>8);//graba
   write_ext_eeprom(1,folio>>16);//graba
   */
   ////////////////////////////////
   ds1307_get_date(day,month,yr,dow);
   ds1307_get_time(hrs,min,sec);
   ////////texto//////
   //inicializa el envio de codigos esc/pos
   fputc(27, U1PRINTER);
   fputc('@', U1PRINTER);
   //linea de justificacion 1B "a" 0izquierda 1centrado 2derecha
   fputc(27, U1PRINTER);
   fputc(97, U1PRINTER);
   fputc(1, U1PRINTER);
   //////interlineado////////
   //fprintf(U1PRINTER,"\x1B");
   //fprintf(U1PRINTER,"3");
   fputc(27, U1PRINTER);
   fputc(51, U1PRINTER);
   fputc(40, U1PRINTER);
   ////////////
   /////negritas activas
   fprintf(U1PRINTER,"\x1B");
   fprintf(U1PRINTER,"G");
   fprintf(U1PRINTER,"\x01");
   /////Tipo de fuente
   fprintf(U1PRINTER,"\x1B");
   fprintf(U1PRINTER,"!");
   fputc(1, U1PRINTER);
   ///size
   fprintf(U1PRINTER,"\x1D");
   fprintf(U1PRINTER,"!");
   fputc(17, U1PRINTER);
   //printf("\x0A");//limpia la justificación
   //fprintf(U1PRINTER,"Folio:%08Ld",folio);
   memset(fol_bol, 0, sizeof(fol_bol));
   for(i=0;i<strlen(XX);i++) fol_bol[i]=XX[instruccion_size+i];//vaciando folio del tcp
   //for(i=0;i<8;i++) fol_bol[i]=XX[instruccion_size+i];//vaciando folio del tcp
   //for(i=0;i<8;i++) fol_bol[i]=XX[instruccion_size+i];//vaciando folio del tcp
   //folio = atoi32(fol_bol);
   //fprintf(U1PRINTER,"Folio:%08Ld",folio);
   fprintf(U1PRINTER,"Folio:%s",fol_bol);
   strcpy(barra_codi,fol_bol);
   fprintf(U1PRINTER,"\x0A");//limpia la justificación
   LF();
   ///////////////////////////////////////////////////////////
   fprintf(U1PRINTER,"MARIA TERESA BAEZ MONROY");
   //printf("\x0A");//limpia la justificación 
   ////desactiva negritas
   fprintf(U1PRINTER,"\x1B");
   fprintf(U1PRINTER,"G");
   fprintf(U1PRINTER,"\x00");
   ///////////////////////////////////////////////////////////
   LF();
   LF();
   /////size////////////
   fprintf(U1PRINTER,"\x1D");
   fprintf(U1PRINTER,"!");
   fputc(0, U1PRINTER);
   /////
   fprintf(U1PRINTER,"\x0A");//limpia la justificación
   fprintf(U1PRINTER,"RFC:BAMT97052884A\x0A");
   fprintf(U1PRINTER,"4 poniente 1302, colonia Centro  CP 72000\x0APuebla, Puebla\x0A");
   ///size
   fprintf(U1PRINTER,"\x1D");
   fprintf(U1PRINTER,"!");
   fputc(17, U1PRINTER);
   fprintf(U1PRINTER,"\x0A");//limpia la justificación
   fprintf(U1PRINTER,"%02d/%02d/20%02d %02d:%02d:%02d",day,month,yr,hrs,min,sec);
   LF();
   /////size////////////
   fprintf(U1PRINTER,"\x1D");
   fprintf(U1PRINTER,"!");
   fputc(0, U1PRINTER);
   /////
   //memset(barra_codi, 0, sizeof(barra_codi));//
   barra1();
   fprintf(U1PRINTER,"\x0A");//limpia la justificación
   fprintf(U1PRINTER,"TARIFA:$15 Hora o Fraccion, TOLERANCIA:5 min.  \x0A");
   fprintf(U1PRINTER,"Horarios:de 8:00 a 21:00hrs. de lunes a viernes\x0A");
   fprintf(U1PRINTER,"Sabados de 8:00 a 14:00hrs.                    \x0A");
   //fprintf(U1PRINTER,"ESTA  EMPRESA  Y SUS TRABAJADORES  NO SE  HACEN\x0A");
   //fprintf(U1PRINTER,"RESPONSABLES  POR  LOS OBJETOS  NO DECLARADOS Y\x0A");
   //fprintf(U1PRINTER,"MOSTRADOS  PARA  CUSTODIAR  AL  RESPONSABLE  DE\x0A");
   //fprintf(U1PRINTER,"ELABORAR  EL  BOLETO, Y  NO  SE  ENTREGUE  ESTE\x0A");
   //fprintf(U1PRINTER,"BOLETO  HASTA  QUE  RECIBA  SU  VEHICULO  A  SU\x0A");
   //fprintf(U1PRINTER,"ENTERA  SATISFACCION  IMPORTANTE LEA AL REVERSO\x0A");
   //fprintf(U1PRINTER,"DE ESTE  BOLETO  YA  QUE INDICA  LOS LIMITES DE\x0A");
   //fprintf(U1PRINTER,"NUESTRA RESPONSABILIDAD                        \x0A");
   fprintf(U1PRINTER,"\x0A");//limpia la justificación
   ////////////////////
   barra2();
   ////////////////////
   fprintf(U1PRINTER,"\x0A");//limpia la justificación
   fprintf(U1PRINTER,"\x0A");//limpia la justificación
   fprintf(U1PRINTER,"\"ACCESA\" automatizacion ");
   fprintf(U1PRINTER,"\x0A");//limpia la justificación
   fprintf(U1PRINTER,"www.accesa.me  automatizacion@accesa.me");
   fprintf(U1PRINTER,"\x0A");//limpia la justificación
   /////////feed para corte de papel//////////////
   fprintf(U1PRINTER,"\x1B");
   fprintf(U1PRINTER,"J");
   //fputc(190, U1PRINTER);
   fputc(250, U1PRINTER);
   fprintf(U1PRINTER,"\x0A");//limpia la justificación 
   LF();
   /////////
   //ESC i 69 Corte total de papel
   //ESC m 6D Corte parcial de papel
   fprintf(U1PRINTER,"\x1B");
   fprintf(U1PRINTER,"m");
   ////////////////////////////////////////
}

void LF(){
fprintf(U1PRINTER,"\x0A");
}

void CR(){
fprintf(U1PRINTER,"\x0D");//enter
}

void barra1(){
   //sprintf(con_barra,"%08Ld%02d%02d%02d%02d%02d%02d",folio,yr,month,day,hrs,min,sec);//
   //sprintf(con_barra,"%08Ld%d",folio,idbar);//01-id de camion
   
   //sprintf(con_barra,"%08Ld%02d%02d%02d%02d%02d%02d%d",folio,yr,month,day,hrs,min,sec,idbar);//01-id de camion
   //fprintf(U1PRINTER,"%08Ld/%02d/%02d/%02d/%02d/%02d/%02d/%d/",folio,yr,month,day,hrs,min,sec,idbar);//01-id de camion
   //encri();//barra_codi
   int size_bar;
   size_bar=strlen(barra_codi)+2;
   //printf (lcd_putc,"\fsize_bar:%u",size_bar);
   //delay_ms(2000);
//////bar code///////
   fprintf(U1PRINTER,"\x1D\x68\x8C");//h Setea el alto
   fprintf(U1PRINTER,"\x1D\x77\x01");//
   //imprimir numero 29 72 n
   fputc(29,U1PRINTER);//
   fputc(72,U1PRINTER);//
   fputc(2,U1PRINTER);//n=0 sin numero, 1 arriba, 2 abajo, 3 arriba y abajo
   //Fuente 29 102 n
   fputc(29,U1PRINTER);//
   fputc(102,U1PRINTER);//
   fputc(1,U1PRINTER);//0,1 Y 2
   ///////////////
   fputc(29,U1PRINTER);
   fputc('k',U1PRINTER);
   fputc(73,U1PRINTER); //69=code39//72=code93//73=code128//70=ITF (solo cantidades pares)//
   //fputc(23,U1PRINTER); //numero de caracteres+2 (solo code128)
   fputc(size_bar,U1PRINTER);//numero de caracteres+2 (solo code128)
   fputc(123,U1PRINTER);// d1 (solo code128)
   fputc(65,U1PRINTER); // d2 (solo code128)
   
   fprintf(U1PRINTER,"%s",barra_codi);
   fprintf(U1PRINTER,"\x00");//envia impresion de codigo
   ///////END BAR CODE//////////
}

void barra2(){
   int size_bar;
   size_bar=strlen(barra_codi);
   //sprintf(con_barra,"%08Ld%02d%02d%02d%02d%02d%02d%d",folio,yr,month,day,hrs,min,sec,idbar);//01-id de camion
   //encri();//barra_codi
   //CODIGO DE BARRAS
   fprintf(U1PRINTER,"\x1D\x68\x8C");//h Setea el alto
   fprintf(U1PRINTER,"\x1D\x77\x01");//setea ancho
   //imprimir numero 29 72 n
   fputc(29,U1PRINTER);//
   fputc(72,U1PRINTER);//
   fputc(2,U1PRINTER);//n=0 sin numero, 1 arriba, 2 abajo, 3 arriba y abajo
   //Fuente 29 102 n
   fputc(29,U1PRINTER);//
   fputc(102,U1PRINTER);//
   fputc(1,U1PRINTER);//0,1 Y 2
   ///////////////
   fputc(29,U1PRINTER);
   fputc('k',U1PRINTER);
   fputc(69,U1PRINTER);//69=code39//72=code93//73=code128//70=ITF(solo cantidades pares)//
   //fputc(21,U1PRINTER);//numero de caracteres
   fputc(size_bar,U1PRINTER);//numero de caracteres+2 (solo code128)
   fprintf(U1PRINTER,"%s",barra_codi);
   fprintf(U1PRINTER,"\x00");//envia impresion de codigo
   ///////END BAR CODE//////////
}

void encri(){
 int tem;
 //fprintf(U1PRINTER,"con_barra:%Lu\r\n",strlen(con_barra));
 
 for(tem=0;tem<=strlen(con_barra);tem++){
         if(con_barra[tem]=='0') barra_codi[tem]='5';
    else if(con_barra[tem]=='1') barra_codi[tem]='7';
    else if(con_barra[tem]=='2') barra_codi[tem]='8';
    else if(con_barra[tem]=='3') barra_codi[tem]='2';
    else if(con_barra[tem]=='4') barra_codi[tem]='9';
    else if(con_barra[tem]=='5') barra_codi[tem]='0';
    else if(con_barra[tem]=='6') barra_codi[tem]='6';
    else if(con_barra[tem]=='7') barra_codi[tem]='4';
    else if(con_barra[tem]=='8') barra_codi[tem]='1';
    else if(con_barra[tem]=='9') barra_codi[tem]='3';
 }//end for
}

/////////////////////
void llaves(){
   int tem;
   char texto[12];
   ////////////////llaves////////////
   ///TEXTO8 DIRECCIONES 0-8 YA NO SE USAN son para almacenar registros de cuentas
   for(tem=1;tem<=data_set;tem++){
      switch (tem) {
           case 1: {//
               strcpy(texto,texto1);
               break;}
           case 2: {//
               strcpy(texto,texto2);
               break;}
           case 3: {//
               strcpy(texto,texto3);
               break;}
           case 4: {//
               strcpy(texto,texto4);
               break;}
           case 5: {//
               strcpy(texto,texto5);
               break;}
           case 6: {//
               strcpy(texto,texto6);
               break;}
           case 7: {//
               strcpy(texto,texto7);
               break;}
           case 8: {//
               strcpy(texto,texto8);
               break;}
           case 9: {//
               strcpy(texto,texto9);
               break;}
           case 10: {//
               strcpy(texto,texto10);
               break;}
           case 11: {//
               strcpy(texto,texto11);
               break;}
           case 12: {//
               strcpy(texto,texto12);
               break;}
           case 13: {//
               strcpy(texto,texto13);
               break;}
           case 14: {//
               strcpy(texto,texto14);
               break;}
      }
      a=i=0;
      while (i < instruccion_size) {  //word_size=8
           memory[i] = read_eeprom( (tem*instruccion_size)+i);
           if (memory[i] != texto[i])  break;
           i++;
           if (i==instruccion_size) a=1;
      }
      //fprintf(U1PRINTER,"\r\n");
      if (a==0){
         i=0;
         while (texto[i] != 0x00){
            write_eeprom(i+(tem*instruccion_size),texto[i]);
            //fprintf(U1PRINTER,"%c",texto[i]);
            i++;
         }//end grabar
      }//END a
   }//end for
}

void rd_eeprom(){
n=a=b=0;
n=instruccion_size;//
i=0;
while ((b <=data_numbers)&&(a==0)){//NUMERO TOTAL DE TARJETAS
    i=0;
    b++;
    while (i < instruccion_size) {//WORD_SIZE_2=10
        memory[i] = read_eeprom(n+i);
        //fprintf(U1PRINTER,"m[%Ld]=%c",i,memory[i]);
        //fprintf(U1PRINTER,"X[%Ld]=%c",i,XX[i]);
        if (memory[i] != XX[i])
            break;
        i++;
        if (i==instruccion_size) a=1;
    }
    //fprintf(PRINTER," \r\n");
    //fprintf(PRINTER," n=%Ld ",n);
    n=n+instruccion_size;//WORD_SIZE=30
    restart_wdt();
    //fprintf(U1PRINTER," \r\n");
    //fprintf(U1PRINTER,"b=%Ld",b);
    }
}

void reset(){
 switch ( restart_cause() )
   {
      case WDT_TIMEOUT:
      {  //lcd_putc("REINICIO-WD");//
         fprintf(U1PRINTER,"\r\nREINICIO-WD");
         break;}
      case MCLR_FROM_RUN://avisa que reinicio por master clear
      {  
         fprintf(U1PRINTER,"\r\nREINICIO-MCLR");
         break;}
      case BROWNOUT_RESTART://avisa que el pic reinicio por un voltaje menor a 4v
      {
         fprintf(U1PRINTER,"\r\nREINICIO-VOLTAJE_BAJO");
         break;
      }
      case NORMAL_POWER_UP:{
         fprintf(U1PRINTER,"\r\nPOWER_UP");
         /*
         #define WDT_TIMEOUT       7     
         #define MCLR_FROM_SLEEP  11     
         #define MCLR_FROM_RUN    15     
         #define NORMAL_POWER_UP  12     
         #define BROWNOUT_RESTART 14     
         #define WDT_FROM_SLEEP    3     
         #define RESET_INSTRUCTION 0     */
         break;
      }
   }
}
