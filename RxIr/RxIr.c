/*
 * RxIr.c
 *
 * Created: 10/09/2016 6:34:23
 *  Author: Nelson Rodriguez
 */ 


#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <math.h>

// MACROS FOR EASY PIN HANDLING FOR ATMEL GCC-AVR
//these macros are used indirectly by other macros , mainly for string concatination
#define _SET(type,name,bit)          type ## name  |= _BV(bit)
#define _CLEAR(type,name,bit)        type ## name  &= ~ _BV(bit)
#define _TOGGLE(type,name,bit)       type ## name  ^= _BV(bit)
#define _GET(type,name,bit)          ((type ## name >> bit) &  1)
#define _PUT(type,name,bit,value)    type ## name = ( type ## name & ( ~ _BV(bit)) ) | ( ( 1 & (unsigned char)value ) << bit )

//these macros are used by end user
#define OUTPUT(pin)         _SET(DDR,pin)
#define INPUT(pin)          _CLEAR(DDR,pin)
#define HIGH(pin)           _SET(PORT,pin)
#define LOW(pin)            _CLEAR(PORT,pin)
#define TOGGLE(pin)         _TOGGLE(PORT,pin)
#define READ(pin)           _GET(PIN,pin)

#define pinLed D,7  //define pins like this
#define ir D,2
#define hv D,5
#define gas C,0
#define rf C,4

#define USART_BAUDRATE 38400
#define UBRR_VALUE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1)

int contador=0;
int pulso1=0;
int pulso2=0;
int pulso3=0;
int sw=0;
int timeout=0;
int errori=0;
signed int deltap1=0;signed int deltap2=0;signed int deltap3=0;
int isOrder=0;
int counterLed=0;

ISR (INT0_vect){
	cli();
	//EIFR=1<<INTF0;
	errori=0;
	pulso1=0;
	while (!READ(ir)){//pulso1=430
		pulso1++;	
		_delay_us(3);
		
	}
	_delay_us(50);
	timeout=0;
	while (READ(ir)){
		timeout++;
		_delay_us(3);	
		if (timeout>5500){
			errori=1;
			goto err;
		}
	}
	
	
	
	pulso2=0;
	while (!READ(ir)){//pulso2=688  pulso3=4301
		pulso2++;	
		_delay_us(10);
		
	}
	_delay_us(50);
	timeout=0;
	while (READ(ir)){
		timeout++;
		_delay_us(3);
		if (timeout>5500){
			errori=1;
			goto err;
		}
	}
	pulso3=0;
	while (!READ(ir)){// pulso3=4301
		pulso3++;	
		_delay_us(10);
		
	}
	_delay_us(50);
	
	HIGH(pinLed);_delay_ms(1);LOW(pinLed);
	
	
	//printf("pulso1:%u pulso2:%u pulso3:%u error:%u\r\n",pulso1,pulso2,pulso3,errori);
err:
	deltap1=206-pulso1;
	deltap2=172-pulso2;
	deltap3=259-pulso3;
	
	if (abs(deltap1)<30 && abs(deltap2)<30 && abs(deltap3)<30){
		isOrder=1;	
	}else {
		isOrder=0;
	}
	printf("pulso1:%u pulso2:%u pulso3:%u error:%u dp1:%d dp2:%d dp3:%d\r\n",pulso1,pulso2,pulso3,errori,deltap1,deltap2,deltap3);

	if (errori==1){
		//printf("Error en transmision\r\n");
	}	
	EIFR=1<<INTF0;
	_delay_ms(100);
	EIFR=1<<INTF0;
	sei();
}

void USART0Init(void)
{
	// Set baud rate
	//UBRRH=(uint8_t)(UBRR_VALUE>>8);
	UBRR0H = (uint8_t)(UBRR_VALUE>>8);
	UBRR0L = (uint8_t)UBRR_VALUE;
	//UBRRL = (uint8_t)UBRR_VALUE;
	// Set frame format to 8 data bits, no parity, 1 stop bit
	//UCSRC |= (1<<UCSZ1)|(1<<UCSZ0);
	UCSR0A &= ~(1<<U2X0);
	//UCSR0C|= (1<<UMSEL00)|(1<<UCSZ01)|(1<<UCSZ00);
	UCSR0C|=(1<<USBS0)|(1<<UCSZ01)|(1<<UCSZ00);
	//UCSRC |= (1<<URSEL)|(1<<USBS)|(3<<UCSZ0);
	//enable transmission and reception
	UCSR0B |= (1<<RXEN0)|(1<<TXEN0);
}
int USART0SendByte(char u8Data, FILE *stream)
{
	if(u8Data == '\n')
	{
		USART0SendByte('\r', stream);
	}
	//wait while previous byte is completed
	while(!(UCSR0A&(1<<UDRE0))){};
	// Transmit data
	UDR0 = u8Data;
	return 0;
}
//set stream pointer
FILE usart0_str = FDEV_SETUP_STREAM(USART0SendByte, NULL, _FDEV_SETUP_WRITE);



int main(void){
	
	OUTPUT(pinLed);
	OUTPUT(hv);
	OUTPUT(gas);
	INPUT(rf);
	LOW(hv);LOW(gas);LOW(pinLed);
	//DDRD &= ~(1 << DDD2);     // Clear the PD2 pin
	//HIGH(pinLed);_delay_ms(80);LOW(pinLed);_delay_ms(80);
	//HIGH(pinLed);_delay_ms(80);LOW(pinLed);_delay_ms(80);
	//HIGH(pinLed);_delay_ms(80);LOW(pinLed);_delay_ms(80);
	//
	//GI = 1<<INT0;					// Enable INT0
	//MCUCR = 1<<ISC01 | 1<<ISC00;	// Trigger INT0 on rising edge
	USART0Init();
	//assign our stream to standart I/O streams
	stdout=&usart0_str;
	EICRA|=1<<ISC01;
	EIMSK|=1<<INT0;
	
	sei();
	

	sw=0;
    while(1)
    {
       //printf("hola");
	   _delay_ms(10);
	   
	   counterLed++;
	   if (counterLed>100 && sw==0){
		   counterLed=0;
		   HIGH(pinLed);_delay_ms(20);LOW(pinLed);
	   }
	   
	   
	   
	   
	   if (isOrder==1 || READ(rf)){
		   counterLed=0;
		   isOrder=0;
		   printf("pulsook:%u pulso2:%u pulso3:%u error:%u dp1:%d dp2:%d dp3:%d\r\n\r\n\r\n",pulso1,pulso2,pulso3,errori,deltap1,deltap2,deltap3);
			if (sw==0){
				//Prendemos gas
				sw=1;
				HIGH(pinLed);
				HIGH(gas);
				_delay_ms(1500);
				HIGH(hv);_delay_ms(2000);LOW(hv);
			
			}else {
				//apagamos gas
				sw=0;
				LOW(gas);
				LOW(hv);
				LOW(pinLed);
			}
			_delay_ms(100);
			while (READ(rf)){
				
			}
			_delay_ms(300);	
			pulso1=0;pulso2=0;pulso3=0;
			deltap1=0;deltap2=0;deltap3=0;
	   }
			
    }
}