#include <stdbool.h>
#include "NUC100Series.h"
// Functions declaration as file header
void System_Config(void);
void TMR1_Config(void);
void SPI3_Config(void);
void LCD_start(void);
void LCD_command(unsigned char temp);
void LCD_data(unsigned char temp);
void LCD_clear(void);
void LCD_SetAddress(uint8_t PageAddr, uint8_t ColumnAddr);
void KeyPadEnable(void);
void resetPasswordGetter();
uint8_t KeyPadScanning(void);
void displayTitle();
void passwordPerformance(void);
bool isValidPassword(int password[]);
bool getPassword(int password[]);