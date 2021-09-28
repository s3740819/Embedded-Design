#include <stdbool.h>
#include "NUC100Series.h"

void System_Config(void);
void SPI3_Config(void);
void TIMER0_Config (void);
void LCD_start(void);
void LCD_command(unsigned char temp);
void LCD_data(unsigned char temp);
void LCD_clear(void);
void LCD_SetAddress(uint8_t PageAddr, uint8_t ColumnAddr);
bool LCD_Display(uint32_t distance);
void LED_Buzzer_Alarm(uint32_t distance);
uint32_t get_Duration(const int trigger_pin, const int echo_pin, const int TMR_CLK_FREQ);
void displayTitle(void);