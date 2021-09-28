#include <stdio.h>
#include <stdbool.h>
#include "NUC100Series.h"
#include "MCU_init.h"
#include "SYS_init.h"
#include "LCD.h"
#include "door_lock_system.h"


// global variables declaration
typedef enum{welcome_screen, login_screen, change_password} STATE;
char display(int number);
volatile char pw[] = "______";
volatile count = 0;
volatile int validPassword[] ={7,4,1,8,1,9};
volatile STATE state = welcome_screen;
volatile uint8_t pressed_key = 0;
int main(void)
{
	uint8_t pressed_key = 0;
	int password[7];
	bool passwordChange = false;
	//--------------------------------
	//System initialization
	//--------------------------------
	System_Config();
	TMR1_Config();
	//--------------------------------
	//SPI3 initialization
	//--------------------------------
	SPI3_Config();
	//--------------------------------
	//LCD initialization
	//--------------------------------
	LCD_start();
	LCD_clear();

	while(1){
		// Case for each state of the system
		switch(state){
				//----------------------------------------------------------//
			case welcome_screen:
				displayTitle();
				printS_5x7(20, 40, "1. Unlock");
				printS_5x7(20, 56, "2. Change Password");
			
				while (pressed_key==0) pressed_key = KeyPadScanning(); // scan for key press activity
				switch(pressed_key){
					case 1: state = login_screen; passwordChange = false; break;
					case 2: state = change_password; passwordChange = true;break;
					default: state = welcome_screen; break;
				}
				CLK_SysTickDelay(150000); // to eliminate key press bouncing
				pressed_key = 0;
				break;
				
				
				//----------------------------------------------------------//
			case login_screen:
				login:
				displayTitle();
				printS_5x7(40, 40, "Password: ");
				printS_5x7(45, 55, pw);
				if (count < 6){
					if(!getPassword(password)){
						goto end;
					}
				}
				else if (count == 6){
					if (isValidPassword(password)){	// Correct
						displayTitle();
						printS_5x7(20, 40, "Valid Password !!!");
						for(int i = 0; i < 2; i++) CLK_SysTickDelay(300000);
						if(!passwordChange){
							displayTitle();
							while (PB->PIN & (1<<15)){		// Until the user ask to return back to main interface, the system will hello its owner forever.
								printS_5x7(38, 40, "Hello Sir!");
								printS_5x7(33, 56, "Welcome Home!");
							}
							// Reset
							state = welcome_screen;
							resetPasswordGetter();
							pressed_key=0;
							break;
						}
						else {			// Otherwise
							goto changePassword;
						}
					}
					else{
						displayTitle();
						printS_5x7(30, 40, "Wrong Password!");
						printS_5x7(15, 48, "Restart in 1 second.");
						resetPasswordGetter();
						PC->DOUT &= ~(1ul << 12);
						for(int i = 0; i < 3; i++) CLK_SysTickDelay(300000); // delay for a total of 1 second
						CLK_SysTickDelay(100000);
						PC->DOUT |= (1ul << 12);
					}
				}
				pressed_key=0;
				end:
				break;
				
			//----------------------------------------------------------//
			case change_password:
				state = login_screen;
				goto login;
			
				changePassword:
				resetPasswordGetter();
				pressed_key = 0;
				while(passwordChange){	// Taking a new password
					displayTitle();
					printS_5x7(30, 40, "New Password: ");
					printS_5x7(45, 55, pw);
													
					if (count < 6){
						if(!getPassword(validPassword)){
							passwordChange = false;
							goto end;
						}
					}
					else if (count == 6){			// Confirm to the user
						displayTitle();
						printS_5x7(20, 40, "Change successfully!");
						printS_5x7(40, 55, "Thank you!");
						for(int i = 0; i < 3; i++) CLK_SysTickDelay(300000);

						passwordChange = false;						
						resetPasswordGetter();
					}
					pressed_key=0;
				}
				state = welcome_screen;
				break;			
		}
	}
}
bool getPassword(int password[]){	
	// scan for key press activity
	while (pressed_key==0){
		if (!(PB->PIN & (1<<15))) {	// If the button PB15 is pressed, return to the main interface
			state = welcome_screen;
			resetPasswordGetter();
			pressed_key=0;
			CLK_SysTickDelay(150000);
			return false;
		}
		pressed_key = KeyPadScanning();
	}
	// Take the input of the user and store the elements of the password to the system's memory for checking if password is correct
	password[count] = pressed_key; count++;
	// display the input number before it becomes a "*"
	passwordPerformance();
	// Tracking
	pw[count - 1] = '*';
	pressed_key = 0;
	return true;
}

bool isValidPassword(int password[]){
	for(int i = 0; i < count; i++){
		if (password[i] != validPassword[i]){
			return false;
		}
	}
	return true;
}

void displayTitle(){
	LCD_clear();
	printS_5x7(2, 0, "EEET2481 Final Assignment");
	printS_5x7(10, 16, "Act1:Door Lock System");
	printS_5x7(15, 25, "___________________");
}
void passwordPerformance(){
	pw[count -1] = display(pressed_key);
	displayTitle();
	printS_5x7(45, 40, "Input :");
	printS_5x7(45, 55, pw);
	
	TIMER1->TCSR |= (0x01ul << 30); //start counting
	while(TIMER1->TDR != 6554);
	TIMER1->TCSR |= (0x01ul << 26); // reset timer
	//CLK_SysTickDelay(200000);
}
char display(int number){
	switch(number){
		case 1: return '1';
		case 2:	return '2';
		case 3: return '3';
		case 4: return '4';
		case 5: return '5';
		case 6: return '6';
		case 7: return '7';
		case 8: return '8';
		case 9: return '9';
		default: return '0';
	}
}


void resetPasswordGetter(){
	for (int i =0; i < 6; i++){
		pw[i] = '_';
	}
	count = 0;
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
	LCD_command(0x10 | (ColumnAddr>>4)&0xF);
	LCD_command(0x00 | (ColumnAddr & 0xF));
 }

void KeyPadEnable(void){
	GPIO_SetMode(PA, BIT0, GPIO_MODE_QUASI);
	GPIO_SetMode(PA, BIT1, GPIO_MODE_QUASI);
	GPIO_SetMode(PA, BIT2, GPIO_MODE_QUASI);
	GPIO_SetMode(PA, BIT3, GPIO_MODE_QUASI);
	GPIO_SetMode(PA, BIT4, GPIO_MODE_QUASI);
	GPIO_SetMode(PA, BIT5, GPIO_MODE_QUASI);
}
uint8_t KeyPadScanning(void){
	PA0=1; PA1=1; PA2=0; PA3=1; PA4=1; PA5=1;
	if (PA3==0) return 1;
	if (PA4==0) return 4;
	if (PA5==0) return 7;
	PA0=1; PA1=0; PA2=1; PA3=1; PA4=1; PA5=1;
	if (PA3==0) return 2;
	if (PA4==0) return 5;
	if (PA5==0) return 8;
	PA0=0; PA1=1; PA2=1; PA3=1; PA4=1; PA5=1;
	if (PA3==0) return 3;
	if (PA4==0) return 6;
	if (PA5==0) return 9;
	return 0;
}
void System_Config (void){
	SYS_UnlockReg(); // Unlock protected registers
	CLK->PWRCON |= (0x01ul << 0);
	while(!(CLK->CLKSTATUS & (1ul << 0)));
	CLK->PWRCON |= (1 << 1);
	while(!(CLK->CLKSTATUS & (1 << 1)));
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
	PB->PMD &= ~(3<<30);
	
	PC->PMD &= ~(3<<24);
	PC->PMD |= (1<<24); // LED5 used for timing verification
	PB->DBEN &= ~(1<<15);
	PB->DBEN |= (1<<15);
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
	SPI3->DIVIDER = 0; // SPI clock divider. SPI clock = HCLK / ((DIVIDER+1)*2). HCLK = 50 MHz
}
void TMR1_Config(void) {
	SYS_UnlockReg(); // Unlock protected registers
	//Timer 1 Clock selection and configuration
	CLK->CLKSEL1 &= ~(0x07ul << 12);
	CLK->CLKSEL1 |= (0x01ul << 12); // TMR1 clock = LXT
	CLK->APBCLK |= (0x01ul << 3); // Enable propagation
	TIMER1->TCSR &= ~(0xFFul << 0); // No prescale

	//reset Timer 1
	TIMER1->TCSR |= (0x01ul << 26);
	
	//define Timer 1 operation mode
	TIMER1->TCSR &= ~(0x03ul << 27);
	TIMER1->TCSR |= (0x03ul << 27); // continuous
	TIMER1->TCSR &= ~(0x01ul << 24); // disable timer counter mode

	//TDR to be updated continuously while timer counter is counting
	TIMER1->TCSR |= (0x01ul << 16);
	SYS_LockReg(); // Lock protected registers
}