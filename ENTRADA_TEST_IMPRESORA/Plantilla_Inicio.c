// Controlador ETHERNET
// Fecha: 2018/02/19.
// Versi�n del compilador: v5.070
// Versi�n del programa: v0.1
// Revisi�n del programa: 0.00

// Definimos el microcontrolador utilizado.
#include <18F4680.h> // Definici�n de registros internos del PIC18F4520.
// Conversor de 10 bits con justificaci�n a la derecha.
//#device ADC=10
// Configuramos velocidad de operaci�n.
#use delay(clock=40000000) // Trabajamos a 20.00Mhz. 

// Configuramos fusibles de programaci�n.

#FUSES NOWDT                    // No utilizamos el perro guard�an.
#FUSES H4                       // Oscilador de alta velocidad 40Mhz.
#FUSES FCMEN                    // Monitor de reloj activado.
#FUSES PUT                      // Temporizador de encendido.
#FUSES NOBROWNOUT               // No activamos el reset por bajo voltaje.
#FUSES NOPBADEN                 // Deshabilitamos el m�dulo conversor ADC del puerto B.
#FUSES NOLPT1OSC                // Timer 1 configurado para una alta potencia de operaci�n.
//#FUSES NOMCLR                   // Pin Master Clear deshabilitado.
#FUSES MCLR                     // Pin Master Clear habilitado.
#FUSES STVREN                   // Si se rebalsa o llena el stack el microcontrolador se resetea.
#FUSES NOLVP                    // No utilizamos bajo voltaje para programaci�n.
#FUSES NOXINST                  // Set de instruccciones ampliado, desactivado.
#FUSES NODEBUG                  // No utilizamos c�digo para debug.
#FUSES PROTECT                  // C�digo protejido contra lecturas.
#FUSES NOCPB                    // Sector de booteo no protejido.
#FUSES NOCPD                    // Sin protecci�n de c�digo en la EEPROM.
#FUSES NOWRT                    // Memoria de programa no protejida contra escrituras.
#FUSES NOWRTC                   // Registros de configuraci�n no protegido contra escritura.
#FUSES NOWRTB                   // Bloque de booteo no protejido contra escritura.
#FUSES NOWRTD                   // Memoria EEPROM no protejida contra escritura.
#FUSES NOEBTR                   // Memoria no protejida contra lectuas de tablas de memoria.
#FUSES NOEBTRB                  // Bloque de booteo no protejido contra lectura de tablas de memoria.

// Configuramos los puertos RS232 utilizados.
#use RS232(uart1, baud=38400,RESTART_WDT,stream=U1PRINTER,TIMEOUT=40,DISABLE_INTS)

const int data_set=14;//CANTIDAD DE INSTRUCCIONES GRABADAS
const int port_size=90;
const int tag_size = 8;
const int word_size_reg =12;//TAMA�O DE REGISTRO
const int16 lim_reg=300;
const int16 data_numbers_ext=2000;//maximo de tags en eeprom
const int instruccion_size = 10;

int envia_pc,a,b2,relay1,tcp_recibe,time_relay1,falla_grabar,f_graba;
char memory[30];
int16 i,n,b,c_envia;
int size_tx_tcp,con_activas;
unsigned int16 m,dir;

int edo_str, reintento_envio,inicializado,socket2;
char XX[port_size];// ARREGLO GENERICO(ALMACENA LECTURAS TARJETAS Y DATOS RECIBIDOS DE TCP)
char YY[port_size];// ARREGLO GENERICO(ALMACENA LECTURAS TARJETAS Y DATOS RESPALDO EN RAM)
char txtcp[port_size];//ARREGLO DE TRANSMICION
char rxtcp[port_size];//1536->112 tags,se direcciona mal la ram con valores mas grandes
//500-alcanza para 54 tags, mas no alcanza la ram, 700au cabe
char lcd_men[port_size-10];
////////VARIABLES WIEGAND/////////
const int wieg_size=26;//34;//
int deteccion_nulo,pre_cuenta;
int wieg,wigmal,tecla;
int sub_indice=0;
int wieg_full=0;
int wiegand_cuenta;
unsigned int32 deci;
char data[wieg_size];
char con_barra[30],barra_codi[30];
int32 folio;
const int idbar=1;
const int data_numbers=14;
//RELOJ/CALENDARIO
int day,month,yr,hrs,min,sec,dow,puntos,cambio_msj;
int dias_prueba,dia_temp,pago,cuenta,espera,hrs2;
int16 mes2,dia;
const int dia_limite=90;
//variables de entradas
int en1,en2,en3,en4,en5,flag_pluma;
//boleto
int time_boleto,re_bol,cta_lcd;
// Definimos macros hardware:
#include "LCD_4x20.c"
#include "24256_SEGUINT.c"
#include "ds1307_3.c" //reloj
// Entradas.
#define entrada01 PIN_D4//
#define entrada02 PIN_B0//DATA1 
#define entrada03 PIN_B1//DATA0
#define entrada04 PIN_D0//
#define entrada05 PIN_D1//SENSOR BARRERA

// Salidas.
#define salida01 PIN_A5//abre entrada
#define salida02 PIN_E0//abre entrada
#define salida03 PIN_D2//abre salida
#define salida04 PIN_D3//indicador de cupo lleno
#define salida05 PIN_D7//
#define salida06 PIN_C2//indicador de conexion
// Macros de entrada:
char texto1[12]= "BOLETO_FOL";//
char texto2[12]= "BORRARTODO";//
char texto3[12]= "BORRAR_TAG";//
char texto4[12]= "ABRIR_ENTR";//
char texto5[12]= "CUPO_LLENO";//CANCELA BOLETOS
char texto6[12]= "CUPO_DISPO";//ACTIVA BOLETOS
char texto7[12]= "MENSAJEEN1";//
char texto8[12]= "          ";//
char texto9[12]= "CONSULTA01";//
char texto10[12]="CONFIGDATE";//
char texto11[12]="CONFIGURAB";//
char texto12[12]="BORRA_CON1";//
char texto13[12]="FOLIORESET";//REINICIAR FOLIOS
char texto14[12]="PAGADO0000";//
char boleto[]= "BOLETO"; //
char barrera[] ="BARRERAABIERTAE1";
int fmensaje;
int16 time_lcd;
//int en5,flag_pluma;

