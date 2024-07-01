#include <8051.h>

#define TRUE 1
#define FALSE 0
#define TH_0 226 //mov TH0, #226
#define DIODA P1_7  //TEST bit P1.7
#define KLAW_BIT P3_5  //KBDS bit P3.5   ;stan klawisza klawiatury sekwencyjnej


//to jest z dsm_IO str. 26
#define ENTER 0b000001
#define ESC 0b000010
#define GORA 0b001000
#define PRAWO 0b000100
#define DOL 0b010000
#define LEWO 0b100000

__xdata unsigned char *wyswietlacz = (__xdata unsigned char *)0xFF30; //CSDS equ 30h  	;adres zatrzasku wybieraj¹cego wyœwietlacz
__xdata unsigned char *segment = (__xdata unsigned char *)0xFF38; //CSDB equ 38h    ;adres zatrzasku wybieraj¹cego segmenty

unsigned char SS = 56;   //sekundy
unsigned char MM = 20;   //minuty
unsigned char HH = 14;   //godziny

unsigned char CZAS[6] = {0, 0, 0, 0, 0, 0};      //CZAS equ 79h
unsigned char KLAWIATURA[4];   //KSTT equ 72h ; 4 ostatnie stany klawiatury sekwencyjnej
__code unsigned char WZOR[10] = {0b0111111, 0b0000110, 0b1011011, 0b1001111,     //0-3
                                 0b1100110, 0b1101101, 0b1111101,                //4-6
                                 0b0000111, 0b1111111, 0b1101111};               //7-9

__bit __at(0x96) SEG_OFF;
__bit t0_flaga = FALSE;		//flaga przerwania
__bit flaga_sekunda = FALSE;     //flaga czy sekunda

int licznik = 0;
unsigned char a_wyswietlacz = 1;
unsigned char a_segment = 0;

void t0_int(void) __interrupt(1)   //;procedura obs³ugi przerwania od TIMER0
{
    TH0 = TH_0;   //mov TH0, #226         ;nastêpne przerwanie za 1/960 sekundy
    t0_flaga = TRUE; //flaga przerwania
}

void przelicz()
{
     //mov A, SS, mov B, #10, div AB
    CZAS[0] = SS % 10;
    CZAS[1] = SS / 10;
    CZAS[2] = MM % 10;
    CZAS[3] = MM / 10;
    CZAS[4] = HH % 10;
    CZAS[5] = HH / 10;
}

void aktualizuj()
{
    if (flaga_sekunda == TRUE)   //jeœli minê³a sekunda
    {
    	licznik = licznik - 960;   //zmniejszamy licznik
        flaga_sekunda = FALSE;     //zerujemy flagê sekund
        SS++;
        if (SS >= 60)
        {
            SS = 0;
            MM++; 
            if (MM >= 60)
            {
                MM = 0;
                HH++;
                if (HH >= 24)
                {
                    HH = 0;
                }
            }
        }
    } 
}



void init()
{
    TH0 = TH_0;  //bajt na przerwanie co 1ms
    ET0 = TRUE; //aktywuj przerwanie od licznika T0
    ES = TRUE;  //aktywnij przerwanie od UART
    EA = TRUE;  //aktywuj wszystkie przerwania
    TR0 = TRUE; //uruchom licznik TR0
    t0_flaga = FALSE;  //zeruj flagê przerwania
}


void drukuj(){
    SEG_OFF = TRUE;
    *wyswietlacz = a_wyswietlacz;
    *segment = WZOR[CZAS[a_segment]];
    SEG_OFF = FALSE;
}

void klawiatura_niestabilna(){
	//	mov KSTT+3, KSTT+2
    	//	mov KSTT+2, KSTT+1
	//	mov KSTT+1, KSTT

	//mov KSTT, #0 		;reinicjalizujemy stan klawiatury
    KLAWIATURA[3] = KLAWIATURA [2];
    KLAWIATURA[2] = KLAWIATURA[1];
    KLAWIATURA[1] = KLAWIATURA[0];
    KLAWIATURA[0] = 0;
}

void obsluga_klawiatury(){
        if (KLAWIATURA[0] != KLAWIATURA[1] && KLAWIATURA[0] != KLAWIATURA[2] && KLAWIATURA[0] != KLAWIATURA[3]) //boucing key
        {
	 //ca³a klawiatura 'naœladuje' tryb edycji, tylko jeœli mamy nacisniête 2 klawisze jednoczeœnie
	 //w zale¿noœci co chcemy zrobiæ naciskamy ENTER/ESC + PRAWO/GORA/LEWO
	 //gdzie:
	 //ESC - odpowiada za dekrementacjê wartoœci reprezentuj¹cej czas (SS, MM lub HH)
	 //ENTER - odpowiada za inkrementacjê 
	 //PRAWO - odpowiada za zmianê wartoœci sekund
	 //GORA - odpowiada za zmianê wartoœci minut
	 //LEWO - odpowiada za zmianê wartoœci godzin
	 if(KLAWIATURA[0] == (PRAWO | ENTER))    //or, ¿eby coœ siê sta³o po naciœniêciu 2 klawiszy
	 {
  	 //inc SS
 	 //mov R6, SS
	 //cjne R6, #60, jeszczeNie60
	 //mov SS, #0

          if(SS < 59)
	 {
	 	SS++;
	 }
         else if(SS == 59) {
	 	SS = 0;
		}
		//jeszczeNie60:
;	//lcall przelicz

         przelicz();
 	 }

 	 if(KLAWIATURA[0] == (PRAWO | ESC ))
	 {
         if(SS != 0)
	 {
	 	SS--;
	 }
         else if(SS == 0) {
	 	SS = 59;
		}
         przelicz();
 	 }

        if(KLAWIATURA[0] == (GORA | ENTER))
	 {
         if(MM < 59)
	 {
	 	MM++;
	 }
         else if(MM == 59) {
	 	MM = 0;
		}
         przelicz();
 	 }

 	 if(KLAWIATURA[0] == (GORA | ESC ))
	 {
         if(MM != 0)
	 {
	 	MM--;
	 }
         else if(MM == 0) {
	 	MM = 59;
		}
         przelicz();
 	 }
 	 if(KLAWIATURA[0] == (LEWO | ENTER))
	 {
         if(HH < 23)
	 {
	 	HH++;
	 }
         else if(HH == 23) {
	 	HH = 0;
		}
         przelicz();
 	 }

 	 if(KLAWIATURA[0] == (LEWO | ESC ))
	 {
         if(HH != 0)
	 {
	 	HH--;
	 }
         else if(HH == 0) {
	 	HH = 23;
		}
         przelicz();
 	}
        }
        klawiatura_niestabilna();
}







void main()
{
    init();
    while(TRUE) {
	    while (!t0_flaga);  //obs³uga przerwania
            t0_flaga = FALSE; //zeruj flagê
		licznik++;
    	if(licznik >= 960){     //jeœli licznik przekroczy 960 to wtedy czy sekunda na true
    		flaga_sekunda = TRUE;
	    }
        przelicz();
//       for(a_segment = 0, a_wyswietlacz = 1; a_segment <= 5; a_segment++, a_wyswietlacz += a_wyswietlacz) {
    	drukuj();
    	if(KLAW_BIT){
             KLAWIATURA[0] = (KLAWIATURA[0] | a_wyswietlacz);       //orl KSTT, A     ;wpr orujemy maskê wybieraj¹c¹ klawisz do KSTT
     	     }
    	a_segment++;
	a_wyswietlacz += a_wyswietlacz;
        if ( a_segment == 6 ){
            a_segment = 0;
	    a_wyswietlacz = 1;                                                                                           

	    if (KLAWIATURA[0] != 0) //obs³uga klawiatury jeœli stan klawiszy niezero
     	    {
     	     obsluga_klawiatury();
     	     }

        }
           aktualizuj();


    }


}
