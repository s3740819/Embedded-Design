#include <stdio.h>
#include <stdbool.h>
#include "NUC100Series.h"
#include "MCU_init.h"
#include <time.h>
#include <stdlib.h>
#include "SYS_init.h"
#include "snake.h"
#include "LCD.h"
#define WIDTH 123
#define HEIGHT 59
typedef enum {welcome_screen, diff_mode, how_to_play, how_to_play2, main_game, end_game, open_screen} STATE;
volatile int x,y,fruitX,fruitY,bombX = 0,bombY = 0,bomb_freq= 0;
volatile char difficulty = 'E';

volatile int Fruit_Duration; 

volatile uint8_t pressed_key = 0; 

typedef enum {R, L, U, D} STEP;

volatile int snake_length = 3;
volatile int body[30][4]; // array to update the coordinates of the snake body 
													// given the function fill_rectangle(x, y, x', y', fg, bg)
													// the column elements of the array are the snake's number of body blocks (max: 20 blocks)
													// the row elements from 0 to 3 of the array are used to update x, y, x' and y' value, respectively

volatile int tempx, tempy;

volatile bool isControlled = false;

const int spawnX[20] = {1,7,13,19,25,31,37,43,49,55,61,67,73,79,85,91,97,103,109,115,121}; // Precise fruit or bomb spawning coordinates on the LCD screen
const int spawnY[6] = {17,23,29,35,41,47};

volatile bool isBomb = false;

volatile int snake_speed = 8000; // Define Timer 0 count of the interval between every snake's movement

volatile int score = 0;
volatile int high_score = 0;
volatile char difficulty_s[5] = " ";
volatile char score_s[2] =" ";

volatile int penalty = 0;
volatile int penaltyTime = 0;

volatile bool resetRequest = false;

volatile int min = 0;
volatile int second = 0;
volatile char min_s[] = "00";
volatile char sec_s[] = "00";

volatile 	STATE state = open_screen;
volatile STEP step = R;

int main(void)
{
	resetGame();
 //--------------------------------
 //System initialization
 //--------------------------------
 System_Config();
 //--------------------------------
 //SPI3 initialization
 //--------------------------------
 SPI3_Config();
 //--------------------------------
 //LCD initialization
 //--------------------------------
 LCD_start();
 LCD_clear();
 //--------------------------------
 //LCD static content
 //--------------------------------
 
 Timer_Config();

while(1){
	LCD_clear(); 
	switch (state) {
		case open_screen:
			draw_LCD(snake_128x64);
			for (int i=0;i<6;i++) CLK_SysTickDelay(1000000);
			LCD_clear();
			state = welcome_screen;
			break;
		case welcome_screen:
			welcome_LCD();
			getDecision:
			while (pressed_key==0) pressed_key = KeyPadScanning(); // scan for key press activity
			CLK_SysTickDelay(500000); // to eliminate key press bouncing
			switch(pressed_key){
			case 1: state = how_to_play; break;
			case 2: state = diff_mode; break;
			default: pressed_key = 0; goto getDecision; // continue scanning for key press activity if there is not any key press activity
			}
			CLK_SysTickDelay(500000);
			pressed_key = 0;
			break;
		case diff_mode: 
			difficult_LCD(); // function to display "Difficulty" screen
		
			getDifficult:
			while (pressed_key==0) pressed_key = KeyPadScanning(); // scan for key press activity
			switch(pressed_key){
				case 1: difficulty = 'E';  CLK_SysTickDelay(500000);state = welcome_screen; pressed_key =0; break;
				case 2: difficulty = 'M'; CLK_SysTickDelay(500000); state = welcome_screen; pressed_key =0; break;
				case 3: difficulty = 'H'; CLK_SysTickDelay(500000); state = welcome_screen; pressed_key =0; break;
				default: pressed_key = 0 ; goto getDifficult;
			}	
			break;
		case how_to_play:
			howToPlay_Screen(); // function to display "How to play" screen
			while (pressed_key==0){ 
				if(resetRequest == 1){
					state = welcome_screen;
					resetRequest = 0;
					break;
				}
				pressed_key = KeyPadScanning(); // scan for key press activity
			}
			if (pressed_key != 0){
				state = how_to_play2;
				CLK_SysTickDelay(500000);	
				CLK_SysTickDelay(500000);	
			}
			pressed_key =0;
			break;
		case how_to_play2:
			howToPlay_Screen_cont(); // function to display the next page of the "How to play" screen
			while (pressed_key==0){ 
				if(resetRequest == 1){
					state = welcome_screen;
					resetRequest = 0;
					break;
				}
				pressed_key = KeyPadScanning(); // scan for key press activity
			}
			if (pressed_key != 0){ // if any key is pressed on the keypad, the program moves on to the "Main game" stage
				CLK_SysTickDelay(500000);	
				CLK_SysTickDelay(500000);	
				state = main_game;
				SetupBomb(); // function to set up a random position to spawn a bomb which is different from the snake position
				SetupFruit(); // similar to the function above but to spawn a fruit
			}
			pressed_key =0;
			break;
		case main_game:
			printS_5x7(40,0,"00");
			printS_5x7(65,0,"00");
			while(1) {
				setDuration(); // function to set the duration of fruit spawning, or both bomb and fruit spawning
				if (bomb_freq != 1) { // if "bomb_freq" variable's random value is not 1, reset the bomb coordinates -> wait until the variable's value is 1 to set the coordinates and display the bomb on the screen!
					bombX = 0;
					bombY = 0;
				}
				statistic_GameLCD(); // function to display the current penalty for eating a bomb, the current playing time and the score on the top of the LCD screen
				if (isBomb == true){ // if the snake eats the bomb, apply a corresponding penalty (slow/fast/reverse)
					getPenalty();
				}
				if(penaltyTime == 3){ // after 3 turns of penalty, the game turns normal (no slow/fast/reverse)
						turnNormal();
				}
				while(!(TIMER0->TDR == Fruit_Duration)){ // Wait for a certain interval defined by the difficulty selected (Easy/Medium/Hard)
					if (TIMER0->TDR % 32768 == 0){ // Update playing time displayed on the LCD screen every 1 second
						timeUpdate();
					}
					if(TIMER0->TDR == 2000) {	// define the duration of the buzzer sound
						PB->DOUT |= (1 << 11); 
					}
					if	(isControlled == false){ // Keep scanning for suitable key press controlling activity if there is not any controlling activity
						control();
					}
					if(resetRequest == 1){ // If the PB15 button is pressed, an GPIO interrupt is generated, resetting the game to the welcome screen
						resetGame();
						resetRequest = 0;
						state = welcome_screen;
						goto end;
					}
					if ((TIMER0->TDR % snake_speed == 0)){ // Update snake movement every predefined interval count value stored in the "snake_speed" variable
						printS_5x7(0,8,"~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"); // draw the upper/ lower border on the screen
						printS_5x7(0,56,"~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
						if (!snakeMove()){ // if the snake hits the upper/ lower border, "snakeMove" returns FALSE and the game is over, otherwise the snake moves normally
							state = end_game;
							goto end;
						}
						isControlled = false; // after there has been a controlling activity, continue to scan for another suitable key press activity 
					}
					if(snakeEat()){ // function to increase the score and the length of the snake or decrease the score depending on what it eats
						break;
					}
					if(snakeEatItself()){
						state = end_game;
						goto end;
					}
					if (score>=90){
						state = end_game;
						goto end;
					}
				}
				FruitBombSetting(); // function to prepare for the display of fruit or both bomb and fruit (but not yet display them)
				fill_Rectangle(body[0][0], body[0][1], body[0][2], body[0][3], 1, 0); 
				} // end of while(1)
				end: // game is over: turn off the display of the snake, fruit and bomb
				for (int i = 0; i < snake_length;i++){
					fill_Rectangle(body[i][0], body[i][1], body[i][2], body[i][3], 0, 0);	
				}
				FruitBombSetting();
			break;
		case end_game:
				CLK_SysTickDelay(500000); // to eliminate key press bouncing
				CLK_SysTickDelay(500000); // to eliminate key press bouncing
				LCD_clear();
				endGameLCD();					
				resetGame();

				getChoice:
				while (pressed_key==0) pressed_key = KeyPadScanning(); // scan for key press activity
					CLK_SysTickDelay(500000); // to eliminate key press bouncing
					CLK_SysTickDelay(500000); // to eliminate key press bouncing
					switch(pressed_key){
					case 1: state = main_game;  break;
					case 2: state = welcome_screen; break;
					default: pressed_key = 0; goto getChoice;
				}
				pressed_key = 0;
				break;
		}
}
}
//------------------------------------------------------------------------------------------------------------------------------------
// Functions definition
//------------------------------------------------------------------------------------------------------------------------------------

void statistic_GameLCD(){
	printS_5x7(55,0,":");
	printS_5x7(80,0,"|Score:");
	printS_5x7(0,0," N/A  |");
	printS_5x7(115,0,"  ");
	Draw();
	sprintf(score_s,"%d",score);
	printS_5x7(115,0,score_s);
	printS_5x7(0,8,"~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
	printS_5x7(0,56,"~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
}

void timeUpdate(){
	second ++;
	if (second > 59){
		second = 0;
		min++;
	}
	if (min >= 0){
		if (min < 10){
			min_s[0] = '0';
			if (min == 1) min_s[1] = '1';
			else if (min == 2) min_s[1] = '2';
			else if (min == 3) min_s[1] = '3';
			else if (min == 4) min_s[1] = '4';
			else if (min == 5) min_s[1] = '5';
			else if (min == 6) min_s[1] = '6';
			else if (min == 7) min_s[1] = '7';
			else if (min == 8) min_s[1] = '8';
			else if (min == 9) min_s[1] = '9';
			else min_s[1] = '0';
		}
		else sprintf(min_s,"%d",min);
	}
	if (second >= 0){
		if (second < 10){
			sec_s[0] = '0';
			if (second == 1) sec_s[1] = '1';
			else if (second == 2) sec_s[1] = '2';
			else if (second == 3) sec_s[1] = '3';
			else if (second == 4) sec_s[1] = '4';
			else if (second == 5) sec_s[1] = '5';
			else if (second == 6) sec_s[1] = '6';
			else if (second == 7) sec_s[1] = '7';
			else if (second == 8) sec_s[1] = '8';
			else if (second == 9) sec_s[1] = '9';
			else sec_s[1] = '0';
		}
		else sprintf(sec_s,"%d",second);
	}
	printS_5x7(40,0,min_s);
	printS_5x7(65,0,sec_s);
}

void endGameLCD(){
	printS(30,0,"Game Over");
	printS_5x7(10,16,"_____________________");
	printS_5x7(30,24,"Your Score:");
	printS_5x7(25,32,"Game Time:");
	printS_5x7(25+5*11,32,min_s);
	printS_5x7(25+5*13,32,":");
	printS_5x7(25+5*14,32,sec_s);
	printS_5x7(30+5*12,24,score_s);
	printS_5x7(30+5*12+10,24,"  ");

	if (score > high_score) {
		high_score = score;
		printS_5x7(20,41,"New High Score !!!");
	} else {
		printS_5x7(0,41,"Better Luck Next Time <3");
	}
	printS_5x7(0,56,"1.Play Again");
	printS_5x7(5*14,56,"2.Main Menu");
}

void snakeGrowUp(){ // increase the length of the snake by 1 block from its tail
	snake_length++;
	if (step == R) {
		tempx = body[snake_length-2][0]; // take the x and y values of the previously-last block
		tempy = body[snake_length-2][1];
		body[snake_length-1][0] =tempx-6; // create new x, y, x' and y' values for the new empty block next to the previously-last block, an empty slot still available in the body array
		body[snake_length-1][1] =tempy;
		body[snake_length-1][2] =tempx-1;
		body[snake_length-1][3] =tempy + 5;
	} else if (step == L) {
		tempx = body[snake_length-2][0];
		tempy = body[snake_length-2][1];
		body[snake_length-1][0] =tempx+6;
		body[snake_length-1][1] =tempy;
		body[snake_length-1][2] =tempx+11;
		body[snake_length-1][3] =tempy + 5;
	} else if (step == U) {
		tempx = body[snake_length-2][0];
		tempy = body[snake_length-2][1];
		body[snake_length-1][0] =tempx;
		body[snake_length-1][1] =tempy+6;
		body[snake_length-1][2] =tempx+5;
		body[snake_length-1][3] =tempy+11;
	} else if (step == D) {
		tempx = body[snake_length-2][0];
		tempy = body[snake_length-2][1];
		body[snake_length-1][0] =tempx;
		body[snake_length-1][1] =tempy-6;
		body[snake_length-1][2] =tempx+5;
		body[snake_length-1][3] =tempy-1;
	}
}

void turnNormal(){
	isBomb = false;
	printS_5x7(0,0," N/A  |");
	snake_speed = 8000;
	penalty = 0;
}

void howToPlay_Screen(){
	LCD_clear();
	printS_5x7(20,0,"|How to play| (1/2)");
	printS_5x7(0,20,"Press 2/4/6/8 to move!");
	printS_5x7(0,30,"1 Fruit spawns randomly!");
	printS_5x7(0,37,"2 Fruits -> 1 bomb!");
	printS_5x7(0,44,"+/- point for fruit/bomb!");
	printS_5x7(40,56,"-> Next");
}

void howToPlay_Screen_cont() {
	LCD_clear();
	printS_5x7(20,0,"|How to play| (2/2)");
	printS_5x7(0,20,"Bomb: fast/slow/reverse!");
	printS_5x7(0,30,"Touch the banks -> dead!");
	printS_5x7(0,37,"Boundary wrap around on X!");
	printS_5x7(30,44,"Good Luck !!!");
	printS_5x7(10,56,"Press any key to play!");
}

void FruitBombSetting(){ // function to prepare for the display of fruit or both bomb and fruit
	fill_Rectangle(bombX,bombY,bombX+5,bombY+5,0,0); // Turn off bomb spawning display whenever it is going to be spawned at a different location on the LCD screen
	bombX = 0; // reset bomb's origin coordinates 
	bombY = 0;
	bomb_freq = rand()%4; // generate a random value to decide whether a bomb is spawned or not  
	if (bomb_freq == 1){ // Only spawn a bomb if the random value stored in "bomb_freq" is equal to 1
		SetupBomb();
	}	
	fill_Rectangle(fruitX,fruitY,fruitX+5,fruitY+5,0,0); // Turn off bomb spawning display
	SetupFruit(); // set up another random coordinates for fruit 
}

bool snakeEatItself(){
	if (snake_length > 4) {
		for (int i = 4; i < snake_length; i++) {
			if ((abs(body[i][0] - x) < 5) && (abs(body[i][2] - x - 5) < 5) && (abs(body[i][1] - y) < 5) && (abs(body[i][3] - y - 5) < 5)) {
				return true;
			}
		}
	}
	return false;
}

void difficult_LCD(){
	LCD_clear();
	printS(20,0,"Difficulty");
	printS_5x7(0,26,"1. Easy (7s)");
	printS_5x7(0,41,"2. Medium (5s)");
	printS_5x7(0,56,"3. Hard (2s)");
}

void welcome_LCD(){
	printS(45,0,"Snake");
	printS_5x7(25,26,"1. Start game");
	printS_5x7(25,41,"2. Difficulty:");
	sprintf(difficulty_s,"%c",difficulty);
	printS_5x7(25+5*15,41,difficulty_s);
	printS_5x7(30,56,"High score:");
	sprintf(score_s,"%d",high_score);
	printS_5x7(30+5*12,56,score_s);
}

bool snakeEat(){
	if ((x <= fruitX+5) && (x >= fruitX) && (y >= fruitY) && (y <= fruitY+5)) {
		PB->DOUT &= ~(1 << 11); 
		if(snake_length < 20){
			snakeGrowUp();
		}
		pointHandler(true); // increase the score
		return true;
	} else if ((x+5 >= fruitX) && (x+5 <= fruitX+5) && (y+5 >= fruitY) && (y+5 <= fruitY+5)) {
		PB->DOUT &= ~(1 << 11);
		if(snake_length < 20){
			snakeGrowUp();
		}
		pointHandler(true);
		return true;
	} else if ((x <= bombX+5) && (x >= bombX) && (y >= bombY) && (y <= bombY+5)) {
			PB->DOUT &= ~(1 << 11);
			pointHandler(false);
			if (penalty == 1) {
				if (step == R) { step = L;
			}else if (step == U) { step = D;
			}else if (step == L) { step = R;
			}else if (step == D) { step = U; }
			}
			
			// reset penalty and get the new one, penalty type will be chosen randomly.
			penalty =0;
			while (penalty == 0){
				penalty = rand()%4;
			}
			isBomb = true;
			snake_speed = 8000;
			penaltyTime = 0;
			return true;
	} else if ((x+5 >= bombX) && (x+5 <= bombX+5) && (y+5 >= bombY) && (y+5 <= bombY+5)) {
			PB->DOUT &= ~(1 << 11);
			pointHandler(false); // decrease the score
			if (penalty == 1) {
				if (step == R) { step = L;
				}else if (step == U) { step = D;
				}else if (step == L) { step = R;
				}else if (step == D) { step = U; }
			}
			penalty =0;
			while (penalty == 0){
				penalty = rand()%4;
			}	
			isBomb = true;
			snake_speed = 8000;
			penaltyTime = 0;
		return true;
	}
	return false;
}

void pointHandler(bool isFruit){ // function to increase score depending on different difficulty levels if a fruit is eaten by the snake
	if (isFruit == true){
		if (difficulty == 'E') {
			score++;
		} else if (difficulty == 'M') {
			score+=2;
		} else if (difficulty == 'H') {
			score+=3;
		}
	}
	else{
		if (difficulty == 'E') {
			score--;
		} else if (difficulty == 'M') {
			score-=2;
		} else if (difficulty == 'H') {
			score-=3;
		}
	}
}

void getPenalty(){		//Start applying penalty
	if (penalty == 2){
		penaltyTime ++;
		snake_speed = 14000;
		printS_5x7(0,0,"Slow B|");
	}
	else if (penalty == 3){
		penaltyTime ++;
		printS_5x7(0,0,"Fast B|");
		snake_speed = 4000;
	}
	else if (penalty == 1){
		penaltyTime ++;
		printS_5x7(0,0,"Revrs |");
		if ( (penaltyTime ==1 || penaltyTime == 3)){
			if (step == R) { step = L;
			}else if (step == U) { step = D;
			}else if (step == L) { step = R;
			}else if (step == D) { step = U; }
		}
		snake_speed = 8000;
	}
}

bool snakeMove(){ // function to move a snake whose each block is 6x6 bits
	for (int i = 0; i < snake_length;i++){
		fill_Rectangle(body[i][0], body[i][1], body[i][2], body[i][3], 0, 0); // turn off the display of the snake based on x, y, x' and y' values for every body block of the snake (max: 20 blocks) 
	}
	update_Bodysnake(); // update the body array based on x, y, x' and y' values for every body block of the snake  
	if (step == R){ // if the player controls the snake to turn right, or by default step is also set to R
		if (penalty == 1){ // if the snake eats reverse bomb, reverse the control
			x-=6;
			if (x < 1){ // wrap boundary around on x 
				x = 121;
			} 
		}
		else {
			x+=6;
			if (x > 121){
				x = 1;
			} 
		} 
	}
	else if (step == L){
		if (penalty == 1){
			x+=6;
			if (x > 121){
				x = 1;
			} 
		}
		else{
			x-=6;
			if (x < 1){
				x = 121;
			} 
		}
	}
	else if (step == U){ // if the player controls the snake to go up
		if( penalty == 1){
			y+= 6;
			if (y > 52){ // while the reverse penalty is applied, if the snake hits the lower border, game is over
				state = end_game;
				return false;						
			}
		}
		else{
			y= y-6;
			if (y < 15){ // while there is not any penalty, if the snake hits the upper border, game is over
				state = end_game;
				return false;						
			} 
		}
	}
	else{ 
		if ( penalty == 1){
			y= y -6;
			if (y < 15){
				state = end_game;
				return false;						
			} 
		}
		else{
			y+= 6;
			if (y > 52){
				state = end_game;
				return false;						
			}
		}
	}
	update_Headsnake(); // update the body array based on x, y, x' and y' values for the first block of the snake
	for (int i = 0; i < snake_length;i++){
		fill_Rectangle(body[i][0], body[i][1], body[i][2], body[i][3], 1, 0); // turn on the display of the snake based on x, y, x' and y' values for every body block of the snake (max: 30 blocks)	
	}
	Draw();	// Debug LCD display when snake move under the fruit/bomb and cause to delete thems.					
	return true;
}
void EINT1_IRQHandler(void) {
	CLK_SysTickDelay(50000);
	resetRequest = 1;
	PB->ISRC |= 1ul << 15;
}

void resetGame(){		//Reset everything 
	score = 0;
	snake_length = 3;
	x = WIDTH/2;
	y = HEIGHT/2;
	update_Headsnake();
	step = R;
	isBomb = false;
	min = 0;
	min_s[0] = "0";
	min_s[1] = "0";
	sec_s[0] = "0";
	sec_s[1] = "0";
	second = 0; 
	snake_speed = 8000;
	penalty = 0;
	
	for (int i = 1; i<snake_length; i++){ // update the snake's size of 6x6 bits starting from its first block (initially snake_length = 3) 
		tempx = body[i-1][0];
		tempy = body[i-1][1];
		body[i][0] =tempx-6;
		body[i][1] =tempy;
		body[i][2] =tempx-1;
		body[i][3] =tempy+5;
	}
}

void update_Headsnake(){	
	body[0][0] = x;
	body[0][1] = y;
	body[0][2] = x+ 5; 
	body[0][3] = y+ 5;
}

void update_Bodysnake(){ // fucntion to update the snake body starting from the last body block, the updated coordinates are taken from the body block right before the updated block
// starting from the last block to the second block, the values in the previous block will be mapped to the following block (max: 30 blocks)
	for (int i = snake_length - 1; i>0; i--){
		body[i][0] = body[i-1][0];
		body[i][1] = body[i-1][1];
		body[i][2] = body[i-1][2];
		body[i][3] = body[i-1][3];
	}
}

void control(){
	pressed_key= KeyPadScanning();
	//erase the objects
	//check changes
	switch(pressed_key) {
		case 2: 
			if(step == R || step == L){
				step = U;
				isControlled = true;
			}
			break;
		case 4: 
			if(step == U || step == D){
				isControlled = true;
				step = L;
			}
			break;
		case 8: 
			if(step == R || step == L){
				isControlled = true;						
				step = D;
			}
			break;
		case 6: 
	 		if(step == U || step == D){
				step = R;
				isControlled = true;
			}
			break;
		default: 
			break;
	}
	pressed_key = 0;	
}


void SetupFruit(){
	random:
	fruitX = spawnX[rand()% 21]; // generate random spawning position
	fruitY = spawnY[rand()% 6];
	for (int i = 0; i < snake_length; i++) { // re-generate other random coordinates if the spawning position matches with the snake body 
		if ((body[i][0] == fruitX)  && (body[i][2] == fruitY)) {
			goto random;
		}
	}
}

void SetupBomb(){
	random:
	bombX = spawnX[rand()% 21];
	bombY = spawnY[rand()% 6];
	for (int i = 0; i < snake_length; i++) {
		if ((body[i][0] == bombX)  && (body[i][2] == bombY) && ((fruitX == bombX) && (fruitY == bombY))) {
			goto random;
		}
	}
}

void Draw()
{ 
	if ( bomb_freq == 1){
		fill_Rectangle(bombX,bombY,bombX+5,bombY+5,1,0); 
	}		
	fill_Rectangle(fruitX,fruitY,fruitX+5,fruitY+5,1,0);
}

void setDuration(){
	if (difficulty == 'E') {
			Fruit_Duration = 229376; //7s
		} else if (difficulty == 'M') {
			Fruit_Duration = 163840; //5s
		} else {
			Fruit_Duration = 65536; //2s
		}
	TIMER0->TCMPR = Fruit_Duration;
}

void Timer_Config(void) {
	CLK->CLKSEL1 &= ~(0x07ul << 8);
	CLK->CLKSEL1 |= (0x01ul << 8);
	CLK->APBCLK |= (0x01ul << 2);
	TIMER0->TCSR &= ~(0xFFul << 0);
	//reset Timer 0
	TIMER0->TCSR |= (0x01ul << 26);
	//define Timer 0 operation mode
	TIMER0->TCSR &= ~(0x03ul << 27);
	TIMER0->TCSR |= (0x01ul << 27);
	TIMER0->TCSR &= ~(0x01ul << 24);
	//TDR to be updated continuously while timer bomb_freqer is bomb_freqing
	TIMER0->TCSR |= (0x01ul << 16);
	//TimeOut = 8s --> Counter's TCMPR = 8s / (1/(32768 Hz) = 262144
	TIMER0->TCMPR = 229376;
	//start bomb_freqing
	TIMER0->TCSR |= (0x01ul << 30);
	
	CLK->CLKSEL1 &= ~(0x07ul << 12);
	CLK->CLKSEL1 |= (0x01ul << 12);
	CLK->APBCLK |= (0x01ul << 3);
	TIMER1->TCSR &= ~(0xFFul << 0);
	//reset Timer 1
	TIMER1->TCSR |= (0x01ul << 26);
	//define Timer 1 operation mode
	TIMER1->TCSR &= ~(0x03ul << 27);
	TIMER1->TCSR |= (0x01ul << 27);
	TIMER1->TCSR &= ~(0x01ul << 24);
	//TDR to be updated continuously while timer bomb_freqer is bomb_freqing
	TIMER1->TCSR |= (0x01ul << 16);
	//TimeOut = 0.1s --> Counter's TCMPR = 0.1s / (1/(32768 Hz) = 3276
	TIMER1->TCMPR = 3276;
	//start bomb_freqing
	TIMER1->TCSR |= (0x01ul << 30);
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
 GPIO_SetMode(PC, BIT12, GPIO_MODE_OUTPUT);
 GPIO_SetMode(PB, BIT11, GPIO_MODE_OUTPUT);
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
 CLK->PWRCON |= (0x01ul << 1);
 while(!(CLK->CLKSTATUS & (1ul << 1)));
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
	PB->PMD |= (1<<30);
	PB->DBEN &= ~(1<<15);
	PB->DBEN |= (1<<15);
	PB->IMD &= (~1ul << 15);
	PB->IEN |= (1ul << 15);

	NVIC->ISER[0] |= 1ul << 3;
	NVIC->IP[0] &= (~(3ul << 30));
	
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