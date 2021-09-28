#include <stdio.h>
#include <stdbool.h>
#include "NUC100Series.h"
#include "LCD.h"
#include "distance_measurement_system.h"

int main(void)
{
	System_Config();
	SPI3_Config();
	TIMER0_Config();
	LCD_start();
	LCD_clear();
	
	const int trigger_pin = 10, echo_pin = 11; // Port C
	const int TMR_CLK_FREQ = 12000000; // Hz
	uint32_t distance = 2, duration = 0;

	PC->PMD &= ~(0x03ul << (trigger_pin*2));
	PC->PMD |= (0x01ul << (trigger_pin*2)); // PC Output pin used to supply a short pulse (10 us) to HC-SR04’s Trigger pin 
	PC->PMD &= ~(0x03ul << (echo_pin*2)); // PC Input pin used to read high-level time of HC-SR04’s Echo pin
	
	displayTitle();

	while(1) {
		start:
		duration = get_Duration(trigger_pin, echo_pin, TMR_CLK_FREQ);
		
		distance = (340*duration*100)/(2*1000000); // d = v*t then divided by 2, in centimeters, with -1 precision  
		
		if (!LCD_Display(distance)){
			goto start; // re-trigger the sensor before it sends an unnecessary short echo pulse
		}
		else{
			CLK_SysTickDelay(60000); // delay at least 60 ms before re-triggering the sensor
			LED_Buzzer_Alarm(distance);
		}
	}
}

//------------------------------------------------------------------------------------------------------------------------------------
// Functions definition
//------------------------------------------------------------------------------------------------------------------------------------

void displayTitle(void){
	printS_5x7(43, 0, "EEET2481");
	printS_5x7(2, 8, "Distance Measurement Sys.");
	printS_5x7(8, 32, "Current Distance (cm):"); 
}

uint32_t get_Duration(const int trigger_pin, const int echo_pin, const int TMR_CLK_FREQ){
		// The sensor is triggered by a short pulse of 10 microseconds.
		// Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
		PC->DOUT &= ~(1ul << trigger_pin);
		CLK_SysTickDelay(2);
		PC->DOUT |= (1ul << trigger_pin);
		CLK_SysTickDelay(10);
		PC->DOUT &= ~(1ul << trigger_pin);
		
		while(!(PC->PIN & (1ul << echo_pin)));
		TIMER0->TCSR |= (0x01ul << 30); //start counting
		
		while(PC->PIN & (1ul << echo_pin)); // Wait until low echo signal 
		// It is worth noting that even when the actual distance exceeds the sensor's max distance, echo pin still returns repeatedly a long pulse then a short pulse but results in an incorrect distance.
		TIMER0->TCSR &= ~(0x01ul << 30); //stop counting
		uint32_t duration = (TIMER0->TDR & 0xFFFFFFFF)/(TMR_CLK_FREQ/1000000); // duration of HIGH pulse in microseconds
		TIMER0->TCSR |= (0x01ul << 26); // reset Timer 0
		return duration;
}

bool LCD_Display(uint32_t distance){
	if (distance >= 400){
		printS_5x7(58, 40, "400"); // display the max distance on LCD
		return false;
	}
	else{
		char distance_s[3]="000";
		sprintf(distance_s, "%d", distance);
		printS_5x7(58, 40, "   "); // clear the 3 chars before overwriting
		printS_5x7(58, 40, distance_s); // display the distance on LCD
		return true;
	}
}

void LED_Buzzer_Alarm(uint32_t distance){
	if(distance >= 40) {
			PC->DOUT &= ~(1ul << 12);
			PB->DOUT &= ~(1ul << 11);
			CLK_SysTickDelay(100000);
			PB->DOUT |= (1ul << 11);
			PC->DOUT |= (1ul << 12);
			for(int i = 0; i < 3; i++) CLK_SysTickDelay(300000); // 1s + 60ms interval
	} else if(distance >= 25) {
			PC->DOUT &= ~(1ul << 12);
			PB->DOUT &= ~(1ul << 11);
			CLK_SysTickDelay(100000);
			PB->DOUT |= (1ul << 11);
			PC->DOUT |= (1ul << 12);
			for(int i = 0; i < 2; i++) CLK_SysTickDelay(300000); // 0.7s + 60ms interval
	} else if(distance >= 10) {
			PC->DOUT &= ~(1ul << 12);
			PB->DOUT &= ~(1ul << 11);
			CLK_SysTickDelay(100000);
			PB->DOUT |= (1ul << 11);
			PC->DOUT |= (1ul << 12);
			CLK_SysTickDelay(300000); // 0.4s + 60ms interval
	} else {
			PC->DOUT &= ~(1ul << 12);
			PB->DOUT &= ~(1ul << 11);
			CLK_SysTickDelay(100000); // 0.1s + 60ms interval
			PC->DOUT |= (1ul << 12);
			PB->DOUT |= (1ul << 11);
	}
}

void LCD_start(void)
{
	LCD_command(0xE2); // Set system reset
	LCD_command(0xA1); // Set Frame rate 100 fps
	LCD_command(0xEB); // Set LCD bias ratio E8~EB for 6~9 (min~max)
	LCD_command(0x81); // Set V BIAS potentiometer
	LCD_command(0xA0); // Set V BIAS potentiometer: A0 ()
	LCD_command(0xC0);
	LCD_command(0xAF); // Set Display Enable
}

void LCD_command(unsigned char temp)
{
	SPI3->SSR |= 1ul << 0;
	SPI3->TX[0] = temp;
	SPI3->CNTRL |= 1ul << 0;
	while(SPI3->CNTRL & (1ul << 0));
	SPI3->SSR &= ~(1ul << 0);
}

void LCD_data(unsigned char temp)
{
	SPI3->SSR |= 1ul << 0;
	SPI3->TX[0] = 0x0100+temp;

	SPI3->CNTRL |= 1ul << 0;
	while(SPI3->CNTRL & (1ul << 0));

	SPI3->SSR &= ~(1ul << 0);
}

void LCD_clear(void) {
	int16_t i;
	LCD_SetAddress(0x0, 0x0);
	for (i = 0; i < 132 *8; i++) {
		LCD_data(0x00);
	}
}

void LCD_SetAddress(uint8_t PageAddr, uint8_t ColumnAddr) {
	LCD_command(0xB0 | PageAddr);
	LCD_command(0x10 | ((ColumnAddr>>4)&0xF));
	LCD_command(0x00 | (ColumnAddr & 0xF)); 
}

void System_Config (void){
	SYS_UnlockReg(); // Unlock protected registers
	CLK->PWRCON |= (0x01ul << 0);
	while(!(CLK->CLKSTATUS & (1ul << 0)));
	//PLL configuration starts
	CLK->PLLCON &= ~(1ul<<19); //0: PLL input is HXT
	CLK->PLLCON &= ~(1ul<<16); //PLL in normal mode
	CLK->PLLCON &= (~(0x01FFul << 0));
	CLK->PLLCON |= 48;
	CLK->PLLCON &= ~(1ul<<18); //0: enable PLLOUT
	while(!(CLK->CLKSTATUS & (0x01ul << 2)));
	//PLL configuration ends
	//clock source selection
	CLK->CLKSEL0 &= (~(0x07ul << 0));
	CLK->CLKSEL0 |= (0x02ul << 0);
	//clock frequency division
	CLK->CLKDIV &= (~0x0Ful << 0);
	//enable clock of SPI3
	CLK->APBCLK |= 1ul << 15;
	
	PC->PMD &= ~(0x03ul << 24);
	PC->PMD |= (0x01ul << 24); // Set LED1 PC.12
	PB->PMD &= ~(0x03ul << 22);
	PB->PMD |= (0x01ul << 22); // Set Buzzer PB.11
	
	SYS_LockReg(); // Lock protected registers
}

void SPI3_Config (void){
	SYS->GPD_MFP |= 1ul << 11; //1: PD11 is configured for alternative function
	SYS->GPD_MFP |= 1ul << 9; //1: PD9 is configured for alternative function
	SYS->GPD_MFP |= 1ul << 8; //1: PD8 is configured for alternative function
	SPI3->CNTRL &= ~(1ul << 23); //0: disable variable clock feature
	SPI3->CNTRL &= ~(1ul << 22); //0: disable two bits transfer mode
	SPI3->CNTRL &= ~(1ul << 18); //0: select Master mode
	SPI3->CNTRL &= ~(1ul << 17); //0: disable SPI interrupt
	SPI3->CNTRL |= 1ul << 11; //1: SPI clock idle high
	SPI3->CNTRL &= ~(1ul << 10); //0: MSB is sent first
	SPI3->CNTRL &= ~(3ul << 8); //00: one transmit/receive word will be executed in one data transfer
	SPI3->CNTRL &= ~(31ul << 3); //Transmit/Receive bit length
	SPI3->CNTRL |= 9ul << 3; //9: 9 bits transmitted/received per data transfer
	SPI3->CNTRL |= (1ul << 2); //1: Transmit at negative edge of SPI CLK
	SPI3->DIVIDER = 0; // SPI clock divider. SPI clock = HCLK / ((DIVIDER+1)*2). HCLK = 50 MHz -> SPICLK = 25 MHz
}

void TIMER0_Config (void){
	SYS_UnlockReg();
	//TM0 Clock selection and configuration
	CLK->CLKSEL1 &= ~(0x07ul << 8);
	CLK->CLKSEL1 |= (0x00ul << 8); // TMR0 clock = HXT
	CLK->APBCLK |= (0x01ul << 2); // Enable propagation
	TIMER0->TCSR &= ~(0xFFul << 0); // No prescale
	
	//reset Timer 0
	TIMER0->TCSR |= (0x01ul << 26);
	
	//define Timer 0 operation mode
	TIMER0->TCSR &= ~(0x03ul << 27);
	TIMER0->TCSR |= (0x03ul << 27); // continuous mode
	TIMER0->TCSR &= ~(0x01ul << 24); // disable timer counter mode

	//TDR to be updated continuously while timer counter is counting
	TIMER0->TCSR |= (0x01ul << 16);
	
	SYS_LockReg();
}
