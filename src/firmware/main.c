#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/sleep.h>

#include "usbdrv.h"
#include "util.h"

#define F_CPU 12000000L

#include <util/delay.h>
#include "dht.h"

//USB command
#define USB_LED_OFF	0
#define USB_LED_ON	1
#define USB_READ	2

#define PORT_LED	PORTD
#define PIN_LED		PIND3
#define DDR_LED		DDRD

static uchar dataReceived = 0;
static uchar dataLength = 0;
unsigned char dhtBuf[5];

static uchar replyBuf[5];

void setup(void);
void setup_watchdog(void);
void blink_led(void);

USB_PUBLIC uchar usbFunctionSetup(uchar data[8])
{
	usbRequest_t *rq = (void *)data; // custom command is in bRequest field
	DHT_ERROR_t errorCode = 0;
		
	switch(rq->bRequest)
	{
		case USB_LED_ON:
			sbi(PORT_LED, PIN_LED); //turn LED on
			return 0;
		
		case USB_LED_OFF:
			cbi(PORT_LED, PIN_LED); //turn LED off
			return 0;
		
		case USB_READ:
		{
			errorCode = readDHT(&dhtBuf);
			
			replyBuf[0] = errorCode;
			replyBuf[1] = dhtBuf[0];
			replyBuf[2] = dhtBuf[1];
			replyBuf[3] = dhtBuf[2];
			replyBuf[4] = dhtBuf[3];

			usbMsgPtr = replyBuf;
			return sizeof(replyBuf);
		}
	}
	return 0;
}

USB_PUBLIC uchar usbFunctionWrite(uchar *data, uchar len)
{
	uchar i;
	
	for(i=0; dataReceived < dataLength && i < len; i++, dataReceived++)
		replyBuf[dataReceived] = data[i];
	
	return (dataReceived == dataLength);
}


int main(void)
{
	cli();
	setup();
	setup_watchdog(); 
	usbInit();
	usbDeviceDisconnect(); //enforce re-enumeration
		
	_delay_ms(500);
		
	usbDeviceConnect();
	
	set_sleep_mode(SLEEP_MODE_IDLE);    // Set Sleep Mode: Power Down
		
	sei();
	
	cbi(PORT_LED, PIN_LED);
	sleep_enable();

	while(1)
	{
		usbPoll();
		sbi(WDTCSR, WDIE); 
		sleep_enable();
	}
}

ISR(WDT_OVERFLOW_vect)
{
  sleep_disable();
  blink_led();
}

void setup(void)
{
	sbi(DDR_LED, PIN_LED); //set led pin as ouput
	sbi(PORT_LED, PIN_LED); 
	DHT_PORT = 0x0;
	ACSR = (1<<ACD); //Turn off Analog Comparator
}

void setup_watchdog()
{
	
	MCUSR &= ~(1<<WDRF); // clear the watchdog reset
	// set up watchdog timer
	WDTCSR |= (1 << WDCE ) | ( 1 << WDE );
	WDTCSR |= (1 << WDIE );
	WDTCSR |= (1 << WDP2) | (1<< WDP1); // timer goes off every 1 seconds

}

void blink_led(void)
{
	sbi(PORT_LED, PIN_LED);
	_delay_ms(100);
	cbi(PORT_LED, PIN_LED);
}