#include <stdio.h>
#include <stdbool.h>
#include "NUC100Series.h"
#include "MCU_init.h"
#include <time.h>
#include <stdlib.h>
#include "SYS_init.h"
#include "picture.h"
#include "LCD.h"
void System_Config(void);
void SPI3_Config(void);
void howToPlay_Screen_cont();
void LCD_start(void);
void LCD_command(unsigned char temp);
void LCD_data(unsigned char temp);
void LCD_clear(void);
void LCD_SetAddress(uint8_t PageAddr, uint8_t ColumnAddr);
void KeyPadEnable(void);
void SetupFruit(void);
void SetupBomb(void);
void Draw(void);
void statistic_GameLCD();
void EINT1_IRQHandler(void);
void timeUpdate();
void resetGame();
void endGameLCD();
void snakeMoveControl();
void snakeGrowUp();
void turnNormal();
void control();
void setDuration();
void howToPlay_Screen();
void FruitBombSetting();
void update_Bodysnake(void);
void update_Headsnake(void);
bool snakeEatItself();
void difficult_LCD();
void welcome_LCD();
bool snakeEat();
void Timer_Config(void);
void pointHandler(bool isFruit);
void getPenalty();
bool snakeMove();
uint8_t KeyPadScanning(void);