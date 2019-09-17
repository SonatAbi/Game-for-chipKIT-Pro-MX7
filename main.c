////////////////////////////////////////////////////////////////////////////////
// ECE 2534:        Lab 3 Spring 2017
//
// File name:       main 
// Writer:          John Mert


#include <stdio.h>
#include <plib.h>
#include "PmodOLED.h"
#include "OledChar.h"
#include "OledGrph.h"
#include "delay.h"
#include "myDebug.h"
#include "myBoardConfigFall2016.h"

// Use preprocessor definitions for program constants
// The use of these definitions makes your code more readable!
#define NUMBER_OF_MILLISECONDS_PER_OLED_UPDATE 100
#define LEDS_MASK (0xf<<12)
#define VCC (1 << 6)
#define BUTTON1 (1 << 6)
#define BUTTON1BIT 6
#define BUTTON2 (1 << 7)
#define LED1_MASK (1<<12)
#define LED2_MASK (1<<13)
#define LED3_MASK (1<<14)
#define LED4_MASK (1<<15)


// Global variable to count number of times in timer2 ISR
volatile unsigned int timer2_ms_value = 0;

// Global variable to pass the ADC reading to main
volatile int UD_reading ;
volatile int LR_reading;
unsigned int check = 0;

BYTE Hero[8] = {0x70, 0xff, 0x7d, 0x7f, 0xfd, 0x30, 0x7f, 0x10};
char Hero_char = 0x00;
BYTE Heart[8] = {0x0e, 0x3f, 0x7f, 0xfe, 0xfe, 0x7f, 0x3f, 0x0e};
char Heart_char = 0x01;
BYTE Trap[8] = {0xff, 0xf9, 0x89, 0xc7, 0xc7, 0x89, 0xf9, 0xff};
char Trap_char = 0x02;
BYTE blank[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
char blank_char = 0x03;

// The interrupt handler for the ADC
// IPL7 highest interrupt priority
void __ISR(_ADC_VECTOR, IPL7SRS) _ADCHandler(void) {
    LR_reading = ReadADC10(0);
    UD_reading = ReadADC10(1);
    INTClearFlag(INT_AD1);
}

// The interrupt handler for timer2
// IPL4 medium interrupt priority
void __ISR(_TIMER_2_VECTOR, IPL4AUTO) _Timer2Handler(void) {
    timer2_ms_value++; // Increment the millisecond counter.
    INTClearFlag(INT_T2); // Acknowledge the interrupt source by clearing its flag.
}

//***********************************************
//* IMPORTANT: THE ADC CONFIGURATION SETTINGS!! *
//***********************************************

// ADC MUX Configuration
// Only using MUXA, AN2 as positive input, VREFL as negative input
#define AD_MUX_CONFIG ADC_CH0_POS_SAMPLEA_AN2 | ADC_CH0_POS_SAMPLEB_AN4 | ADC_CH0_NEG_SAMPLEA_NVREF | ADC_CH0_NEG_SAMPLEB_NVREF

// ADC Config1 settings
// Data stored as 16 bit unsigned int
// Internal clock used to start conversion
// ADC auto sampling (sampling begins immediately following conversion)
#define AD_CONFIG1 ADC_FORMAT_INTG | ADC_CLK_AUTO | ADC_AUTO_SAMPLING_ON

// ADC Config2 settings
// Using internal (VDD and VSS) as reference voltages
// Do not scan inputs
// One sample per interrupt
// Buffer mode is one 16-word buffer
// Alternate sample mode off (use just MUXA)
#define AD_CONFIG2 ADC_VREF_AVDD_AVSS | ADC_SCAN_OFF | \
                  ADC_SAMPLES_PER_INT_2 | \
                  ADC_BUF_16 | ADC_ALT_INPUT_ON

// ADC Config3 settings
// Autosample time in TAD = 8
// Prescaler for TAD:  the 20 here corresponds to a
// ADCS value of 0x27 or 39 decimal => (39 + 1) * 2 * TPB = 8.0us = TAD
// NB: Time for an AD conversion is thus, 8 TAD for aquisition +
//     12 TAD for conversion = (8+12)*TAD = 20*8.0us = 160us.
#define AD_CONFIG3 ADC_SAMPLE_TIME_8 | ADC_CONV_CLK_20Tcy

// ADC Port Configuration (PCFG)
// Not scanning, so nothing need be set here..
// NB: AN2 was selected via the MUX setting in AD_MUX_CONFIG which
// sets the AD1CHS register (true, but not that obvious...)
#define AD_CONFIGPORT ENABLE_AN2_ANA | ENABLE_AN4_ANA


// ADC Input scan select (CSSL) -- skip scanning as not in scan mode
#define AD_CONFIGSCAN SKIP_SCAN_ALL

// Initialize the ADC using my definitions
// Set up ADC interrupts
void initADC() {

    // Configure and enable the ADC HW
    SetChanADC10(AD_MUX_CONFIG);
    OpenADC10(AD_CONFIG1, AD_CONFIG2, AD_CONFIG3, AD_CONFIGPORT, AD_CONFIGSCAN);
    EnableADC10();

    // Set up, clear, and enable ADC interrupts
    INTSetVectorPriority(INT_ADC_VECTOR, INT_PRIORITY_LEVEL_7);
    INTClearFlag(INT_AD1);
    INTEnable(INT_AD1, INT_ENABLED);
}

// Initialize timer2 and set up the interrupts
void initTimer2() {
    // Configure Timer 2 to request a real-time interrupt once per millisecond.
    // The period of Timer 2 is (16 * 625)/(10 MHz) = 1ms.
    OpenTimer2(T2_ON | T2_IDLE_CON | T2_SOURCE_INT | T2_PS_1_16 | T2_GATE_OFF, 624);
    
    // Setup Timer 2 interrupts
    INTSetVectorPriority(INT_TIMER_2_VECTOR, INT_PRIORITY_LEVEL_4);
    INTClearFlag(INT_T2);
    INTEnable(INT_T2, INT_ENABLED);
}


void initINT()
{
    // This is a multi-vector setup
    INTConfigureSystem(INT_SYSTEM_CONFIG_MULT_VECTOR);
    
    // Let the interrupts happen
    INTEnableInterrupts();
}

// All the hardware initializations 
void initALL(){
    
    TRISBCLR = VCC;
    // Initialize GPIO for LEDs
    TRISGCLR = LEDS_MASK; // For LEDs: configure PortG pins for output
    ODCGCLR = LEDS_MASK; // For LEDs: configure as normal output (not open drain)
    PORTGCLR = LEDS_MASK; // Turn all LEDs off
    TRISGSET = BUTTON1 | BUTTON2; 

    // Initialize Timer1 and OLED for display
    DelayInit();
    OledInit();

    // Initial Timer2 and ADC
    initTimer2();
    initADC();
    
    // Initial interrupt controller
    initINT();
    //int successful_definition = 0;

    // Set up our user-defined characters for the OLED
    //successful_definition = OledDefUserChar(blank_char, blank);
}

void MyOledDrawGlyph(int x_start, int y_start, BYTE *MyGlyphArray)
{
    int i, j;
    
    // i is the index of MyGlyphArray
    for (i = 0; i< 8; i++)
        // We are processing MyGlyphArray[i], which is a BYTE
        
        // j is the index of bits inside each BYTE
        for (j = 0; j<8; j++)
        {
            // find the coordinates of the pixel we are drawing
            int x = x_start + i;
            int y = y_start - j;
            OledMoveTo(x, y);
            
            // When j is 0, we are dealing with MSB
            // When j is 7, we are dealing with LSB
            // For other js, we are dealing with a bit in between
            // The mask needs to be created based on j
            int mask = 1 << (7 - j);
            
            // If this condition holds, it means this bit is 1
            if (MyGlyphArray[i] & mask)
                //make the pen white
                OledSetDrawColor(1);
            else 
                //make the pen black
                OledSetDrawColor(0);
            
            //draw the pixel. 
            //If pen is black, it basically wipes what was there before.
            OledDrawPixel();
        }
}

void moveUD(volatile int UD_reading, unsigned int *star, int *up, int *down){
    //moving main menu
    if(UD_reading > 900 && *star != 0 && *up == 0){
        OledSetCursor(0, *star);
        OledPutString(" ");
        (*star)--;
        OledSetCursor(0, *star);
        OledPutString("*");
        *up = 1;
    }
    else if(UD_reading < 100 && *star != 3 && *down == 0){
        OledSetCursor(0, *star);
        OledPutString(" ");
        (*star)++;
        OledSetCursor(0, *star);
        OledPutString("*");
        *down = 1;
    }
    if(UD_reading < 700 && UD_reading >300){
        *up = 0;
        *down = 0;
    }
}
void moveDiff(volatile int UD_reading, unsigned int *star){
    if(UD_reading > 900 && *star != 0){
        OledSetCursor(0, *star);//moving diff menu
        OledPutString(" ");
        (*star)--;
        OledSetCursor(0, *star);
        OledPutString("*");
    }
    else if(UD_reading < 100 && *star != 1){
        OledSetCursor(0, *star);
        OledPutString(" ");
        (*star)++;
        OledSetCursor(0, *star);
        OledPutString("*");
    }
}
void moveHERO(volatile int UD_reading, volatile int LR_reading, unsigned int *glyph_pixel_x, unsigned int *glyph_pixel_y){
    //moving avatar
    if(check == 0){
        MyOledDrawGlyph(*glyph_pixel_x, *glyph_pixel_y, Hero);
        OledUpdate();
        check = 1;
    }
    
    MyOledDrawGlyph(*glyph_pixel_x, *glyph_pixel_y, blank);
    
    if(LR_reading > 900 && *glyph_pixel_x != 120 ){
        if(*glyph_pixel_x == 112 && *glyph_pixel_y < 15 )
        {
            
        }
        else
        {
            (*glyph_pixel_x)++;
            MyOledDrawGlyph(*glyph_pixel_x, *glyph_pixel_y, Hero);
            OledUpdate();
        }
    }
    else if(LR_reading < 200 && *glyph_pixel_x != 0){
        (*glyph_pixel_x)--;
        MyOledDrawGlyph(*glyph_pixel_x, *glyph_pixel_y, Hero);
        OledUpdate();
    }
    else if(UD_reading > 900 && *glyph_pixel_y != 7){
        if(*glyph_pixel_y == 15 && *glyph_pixel_x > 112 )
        {
            
        }
        else
        {
        (*glyph_pixel_y)--;
        MyOledDrawGlyph(*glyph_pixel_x, *glyph_pixel_y, Hero);
        OledUpdate();
        }
    }
    else if(UD_reading < 200 && *glyph_pixel_y != 31){
        (*glyph_pixel_y)++;
        MyOledDrawGlyph(*glyph_pixel_x, *glyph_pixel_y, Hero);
        OledUpdate();
    }
    MyOledDrawGlyph(*glyph_pixel_x, *glyph_pixel_y, Hero);
}

void showScore(int *score){
    OledSetCursor(15, 0);//showing score during play
    char scorestr[2];
    sprintf(scorestr, "%d", *score);
    OledPutString(scorestr);
}

void mainMenu(){
    OledSetCursor(1, 0);//Main menu
    OledPutString("PLAY");
    OledSetCursor(1, 1);
    OledPutString("DIFFICULTY");
    OledSetCursor(1, 2);
    OledPutString("SCOREBOARD");
    OledSetCursor(1, 3);
    OledPutString("HOW TO PLAY");
}
void HTPMenu(){
    OledSetCursor(0,0);//How to play menu
    OledPutString("Collect Prizes");
    OledSetCursor(0,1);
    OledPutString("Avoid Traps");
    OledSetCursor(0,2);
    OledPutString("Hero Reward Trap");
    MyOledDrawGlyph(0, 31, Hero);
    MyOledDrawGlyph(40, 31, Heart);
    MyOledDrawGlyph(96, 31, Trap);
}

void diffMenu(){
    
    OledSetCursor(1, 0);//difficulty menu
    OledPutString("Don't hurt me");
    OledSetCursor(1, 1);
    OledPutString("Bring 'em on!");
    
}

void scoreDisplay(unsigned int h_score1, unsigned int h_score2, unsigned int h_score3){
    OledSetCursor(0, 0);//scoreboard display
    OledPutString("1.");
    OledSetCursor(3, 0);
    char first[4];
    sprintf(first, "%d", h_score1);
    OledPutString(first);
                    
    OledSetCursor(0, 1);
    OledPutString("2.");
    OledSetCursor(3, 1);
    char sec[4];
    sprintf(sec, "%d", h_score2);
    OledPutString(sec);
                    
    OledSetCursor(0, 2);
    OledPutString("3.");
    OledSetCursor(3, 2);
    char third[4];
    sprintf(third, "%d", h_score3);
    OledPutString(third);
}

void colCheckRew(unsigned int herox, unsigned int heroy, unsigned int obx, unsigned int oby, int *curscore, int *checkcol ){
    //checking collisions with rewards
    int xh1 = herox + 3;
    int xh2 = xh1+2;
    int yh1 = heroy - 3;
    int yh2 = yh1 - 1;
    
    int xr1 = obx + 3;
    int xr2 = xr1+2;
    int yr1 = oby - 3;
    int yr2 = yr1 - 1;
    
    if(xh1 == xr1 && yh1 == yr1)
        *checkcol = 1;
    if(xh1 == xr1 && yh1 == yr2)
        *checkcol = 1;
    if(xh1 == xr2 && yh1 == yr2)
        *checkcol = 1;
    if(xh1 == xr2 && yh1 == yr1)
        *checkcol = 1;
    ///////////////////////////////
    if(xh1 == xr1 && yh2 == yr1)
        *checkcol = 1;
    if(xh1 == xr1 && yh2 == yr2)
        *checkcol = 1;
    if(xh1 == xr2 && yh2 == yr2)
        *checkcol = 1;
    if(xh1 == xr2 && yh2 == yr1)
        *checkcol = 1;
    ///////////////////////////////
    if(xh2 == xr1 && yh2 == yr1)
        *checkcol = 1;
    if(xh2 == xr1 && yh2 == yr2)
        *checkcol = 1;
    if(xh2 == xr2 && yh2 == yr2)
        *checkcol = 1;
    if(xh2 == xr2 && yh2 == yr1)
        *checkcol = 1;
    ///////////////////////////////
    if(xh2 == xr1 && yh1 == yr1)
        *checkcol = 1;
    if(xh2 == xr1 && yh1 == yr2)
        *checkcol = 1;
    if(xh2 == xr2 && yh1 == yr2)
        *checkcol = 1;
    if(xh2 == xr2 && yh1 == yr1)
        *checkcol = 1;
    if(*checkcol == 1)
        (*curscore)++;
    
}

void colCheckTrap(unsigned int herox, unsigned int heroy, unsigned int obx, unsigned int oby, int *curscore, int *checkcol ){
    //Checking collisons with traps
    int xh1 = herox + 3;
    int xh2 = xh1+2;
    int yh1 = heroy - 3;
    int yh2 = yh1 - 1;
    
    int xr1 = obx + 3;
    int xr2 = xr1+2;
    int yr1 = oby - 3;
    int yr2 = yr1 - 1;
    
    if(xh1 == xr1 && yh1 == yr1)
        *checkcol = 1;
    if(xh1 == xr1 && yh1 == yr2)
        *checkcol = 1;
    if(xh1 == xr2 && yh1 == yr2)
        *checkcol = 1;
    if(xh1 == xr2 && yh1 == yr1)
        *checkcol = 1;
    ///////////////////////////////
    if(xh1 == xr1 && yh2 == yr1)
        *checkcol = 1;
    if(xh1 == xr1 && yh2 == yr2)
        *checkcol = 1;
    if(xh1 == xr2 && yh2 == yr2)
        *checkcol = 1;
    if(xh1 == xr2 && yh2 == yr1)
        *checkcol = 1;
    ///////////////////////////////
    if(xh2 == xr1 && yh2 == yr1)
        *checkcol = 1;
    if(xh2 == xr1 && yh2 == yr2)
        *checkcol = 1;
    if(xh2 == xr2 && yh2 == yr2)
        *checkcol = 1;
    if(xh2 == xr2 && yh2 == yr1)
        *checkcol = 1;
    ///////////////////////////////
    if(xh2 == xr1 && yh1 == yr1)
        *checkcol = 1;
    if(xh2 == xr1 && yh1 == yr2)
        *checkcol = 1;
    if(xh2 == xr2 && yh1 == yr2)
        *checkcol = 1;
    if(xh2 == xr2 && yh1 == yr1)
        *checkcol = 1;
    if(*checkcol == 1)
        (*curscore)--;
    
}

int main() {
    
    initALL();
    //unsigned int timer2_ms_value_save;
    unsigned int last_oled_update = 0;
    unsigned int ms_since_last_oled_update;
    PORTGCLR = LED1_MASK;
    PORTGCLR = LED2_MASK;
    PORTGCLR = LED3_MASK;
    PORTGCLR = LED4_MASK;
    
    int button1cur = 0;//button1 checkers
    int button1prev = 0;     
    int button1pressed = 0;
    //control variables
    int up = 0;
    int down = 0;
    
    int checktrap1 = 0;
    int checkheart1 = 0;
    
    int checktrap2 = 0;
    int checkheart2 = 0;
    //coordinates for hero traps and rewards
    unsigned int glyph_pixel_x = 0, glyph_pixel_y = 7;
    
    unsigned int trap_1_x = rand() % (112 - 16 + 1) + 16;
    unsigned int trap_1_y = rand() % (31 - 8 + 1) + 8;
    
    unsigned int trap_2_x = rand() % (112 - 16 + 1) + 16;
    unsigned int trap_2_y = rand() % (31 - 8 + 1) + 8;
    
    unsigned int heart_1_x = rand() % (112 - 16 + 1) + 16;
    unsigned int heart_1_y = rand() % (31 - 8 + 1) + 8;
    
    unsigned int heart_2_x = rand() % (112 - 16 + 1) + 16;
    unsigned int heart_2_y = rand() % (31 - 8 + 1) + 8;
    
    unsigned int star = 0;
    
    int curscore = 0;
    unsigned int h_score1 = 0;
    unsigned int h_score2 = 0;
    unsigned int h_score3 = 0;
    
    int difficulty = 1;
    
    OledClearBuffer();
    OledSetCursor(0, 0);
    OledPutString("*");
    
    enum {MAIN, DIFF, HTPM, SBOARD, PLAY} mode = MAIN; 

    while (1) {
        
        // Update OLED
        ms_since_last_oled_update = timer2_ms_value - last_oled_update;
        if (ms_since_last_oled_update >= NUMBER_OF_MILLISECONDS_PER_OLED_UPDATE) 
        {
            //timer2_ms_value_save = timer2_ms_value;
            last_oled_update = timer2_ms_value;
            switch(mode)
            {
                
                case MAIN:
                    mainMenu();//displaying menu
                    moveUD(UD_reading, &star, &up, &down);//moving in menu
                    if(LR_reading > 900){//main menu selections
                        if(star == 0){
                            star = 0;
                            mode = PLAY;
                            OledClearBuffer();
                        }
                        if(star == 1){
                            star = 0;
                            mode = DIFF;
                            OledClearBuffer();
                            OledSetCursor(0, 0);
                            OledPutString("*");
                        }
                        if(star == 2){
                            star = 0;
                            mode = SBOARD;
                            OledClearBuffer();
                        }
                        if(star == 3){
                            star = 0;
                            OledClearBuffer();
                            mode = HTPM;
                        }
                    }
                    
                    break;
///////////////////////////////////////////////////////////////////////////////////////////////                    
                case PLAY:
                    moveHERO(UD_reading, LR_reading, &glyph_pixel_x, &glyph_pixel_y );//moving avatar
                    
                    if(checkheart1 == 0){//Checking Collisions with traps and rewards
                        MyOledDrawGlyph(heart_1_x, heart_1_y, Heart);
                        colCheckRew(glyph_pixel_x, glyph_pixel_y, heart_1_x, heart_1_y, &curscore, &checkheart1 );
                    }
                    if(checkheart1 == 1){
                        MyOledDrawGlyph(heart_1_x, heart_1_y, blank);
                        checkheart1++;
                    }
                    
                    if(checktrap1 == 0){
                        
                        MyOledDrawGlyph(trap_1_x, trap_1_y, Trap);
                        colCheckTrap(glyph_pixel_x, glyph_pixel_y, trap_1_x, trap_1_y, &curscore, &checktrap1 );
                    }
                     if(checktrap1 == 1){
                        
                        MyOledDrawGlyph(trap_1_x, trap_1_y, blank);
                        checktrap1++;
                    }
                    
                    if(difficulty == 2){//Checking Collisions with traps and rewards
                        if(checkheart2 == 0){
                            MyOledDrawGlyph(heart_2_x, heart_2_y, Heart);
                            colCheckRew(glyph_pixel_x, glyph_pixel_y, heart_2_x, heart_2_y, &curscore, &checkheart2 );
                        }
                        if(checkheart2 == 1){
                            MyOledDrawGlyph(heart_2_x, heart_2_y, blank);
                            checkheart2++;
                        }
                    
                        if(checktrap2 == 0){
                        
                            MyOledDrawGlyph(trap_2_x, trap_2_y, Trap);
                            colCheckTrap(glyph_pixel_x, glyph_pixel_y, trap_2_x, trap_2_y, &curscore, &checktrap2 );
                        }
                        if(checktrap2 == 1){
                        
                            MyOledDrawGlyph(trap_2_x, trap_2_y, blank);
                            checktrap2++;
                        }
                    }
                    
                    showScore(&curscore);//Displaying Score
                    
                    //Checking for Buttons
                    if ((PORTG & BUTTON1) >> BUTTON1BIT)//if button 1 is pressed
                        button1cur = 1;             
                    else                 
                        button1cur = 0;                         
                    // button1cur = ((PORTG & BUTTON1) >> BUTTON1BIT);
                    button1pressed = ((!button1cur) && (button1prev));
                    button1prev = button1cur;
                    if(button1pressed)
                    {                         
                        int ss = 0;
                        while (ss == 0)
                        {
                            if ((PORTG & BUTTON1) >> BUTTON1BIT)
                            button1cur = 1;             
                            else                 
                            button1cur = 0;                         
                            // button1cur = ((PORTG & BUTTON1) >> BUTTON1BIT);
                            button1pressed = ((!button1cur) && (button1prev));
                            button1prev = button1cur;
                            if(button1pressed){                         
                                ss = 1;                  
                            }
                            if(PORTG & BUTTON2){
                                //reinitializing all variables
                                check = 0;
                                curscore = 0;
                                checktrap1 = 0;
                                checkheart1 = 0;
                                checktrap2 = 0;
                                checkheart2 = 0;
                                glyph_pixel_x = 0;
                                glyph_pixel_y = 8;
                                OledClearBuffer();
                                OledSetCursor(0, 0);
                                OledPutString("*");
                                star = 0;
                                mode = MAIN;
                                ////////////////////////
                                trap_1_x = rand() % (112 - 16 + 1) + 16;
                                trap_1_y = rand() % (31 - 8 + 1) + 8;
                                trap_2_x = rand() % (112 - 16 + 1) + 16;
                                trap_2_y = rand() % (31 - 8 + 1) + 8;
                                heart_1_x = rand() % (112 - 16 + 1) + 16;
                                heart_1_y = rand() % (31 - 8 + 1) + 8;
                                heart_2_x = rand() % (112 - 16 + 1) + 16;
                                heart_2_y = rand() % (31 - 8 + 1) + 8;
                                break;
                            }
                        }
                    }
                    
                    if(PORTG & BUTTON2)
                    {
                        //reinitializing all variables
                        check = 0;
                        curscore = 0;
                        checktrap1 = 0;
                        checkheart1 = 0;
                        checktrap2 = 0;
                        checkheart2 = 0;
                        glyph_pixel_x = 0;
                        glyph_pixel_y = 8;
                        OledClearBuffer();
                        OledSetCursor(0, 0);
                        OledPutString("*");
                        star = 0;
                        mode = MAIN;
                        ////////////
                        trap_1_x = rand() % (112 - 16 + 1) + 16;
                        trap_1_y = rand() % (31 - 8 + 1) + 8;
                        trap_2_x = rand() % (112 - 16 + 1) + 16;
                        trap_2_y = rand() % (31 - 8 + 1) + 8;
                        heart_1_x = rand() % (112 - 16 + 1) + 16;
                        heart_1_y = rand() % (31 - 8 + 1) + 8;
                        heart_2_x = rand() % (112 - 16 + 1) + 16;
                        heart_2_y = rand() % (31 - 8 + 1) + 8;
                    }
                    
                    // If Score is lower than 0 you lose
                    if(curscore < 0){
                        OledClearBuffer();
                        int wait = 0;
                        while(wait < 200){//waiting 3 seconds
                            wait++;
                            OledSetCursor(6,2);
                            OledPutString("Lost");
                        }
                        //reinitializing all variables
                        check = 0;
                        curscore = 0;
                        checktrap1 = 0;
                        checkheart1 = 0;
                        checktrap2 = 0;
                        checkheart2 = 0;
                        glyph_pixel_x = 0;
                        glyph_pixel_y = 8;
                        OledClearBuffer();
                        OledSetCursor(0, 0);
                        OledPutString("*");
                        star = 0;
                        mode = MAIN;
                        //////////////////////////////////
                        trap_1_x = rand() % (112 - 16 + 1) + 16;
                        trap_1_y = rand() % (31 - 8 + 1) + 8;
                        trap_2_x = rand() % (112 - 16 + 1) + 16;
                        trap_2_y = rand() % (31 - 8 + 1) + 8;
                        heart_1_x = rand() % (112 - 16 + 1) + 16;
                        heart_1_y = rand() % (31 - 8 + 1) + 8;
                        heart_2_x = rand() % (112 - 16 + 1) + 16;
                        heart_2_y = rand() % (31 - 8 + 1) + 8;
                    }
                    //If you win, Updating the scoreboard and returning to main menu
                    if(difficulty == 1){
                        if(checkheart1 == 2){
                            OledClearBuffer();
                            int wait = 0;
                            while(wait < 200){//Waiting 3 seconds
                                wait++;
                                OledSetCursor(6,2);
                                OledPutString("Win");
                            }
                            //Changing Score
                            if(curscore > h_score1){
                                h_score3 = h_score2;
                                h_score2 = h_score1;
                                h_score1 = curscore;
                            }
                            else if(curscore == h_score1){
                                h_score3 = h_score2;
                                h_score2 = curscore;
                            }
                            else if(curscore > h_score2){
                                h_score3 = h_score2;
                                h_score2 = curscore;
                            }
                            else if(curscore == h_score2){
                                h_score3 = curscore;
                            }
                            else if(curscore > h_score3){
                                h_score3 = curscore;
                                
                            }
                            //reinitializing all variables
                            check = 0;
                            curscore = 0;
                            checktrap1 = 0;
                            checkheart1 = 0;
                            checktrap2 = 0;
                            checkheart2 = 0;
                            glyph_pixel_x = 0;
                            glyph_pixel_y = 8;
                            OledClearBuffer();
                            OledSetCursor(0, 0);
                            OledPutString("*");
                            star = 0;
                            mode = MAIN;
                            ///////////////////////
                            trap_1_x = rand() % (112 - 16 + 1) + 16;
                            trap_1_y = rand() % (31 - 8 + 1) + 8;
                            trap_2_x = rand() % (112 - 16 + 1) + 16;
                            trap_2_y = rand() % (31 - 8 + 1) + 8;
                            heart_1_x = rand() % (112 - 16 + 1) + 16;
                            heart_1_y = rand() % (31 - 8 + 1) + 8;
                            heart_2_x = rand() % (112 - 16 + 1) + 16;
                            heart_2_y = rand() % (31 - 8 + 1) + 8;
                        }
                    }
                    else if(difficulty == 2){
                        if(checkheart1 == 2 && checkheart2 == 2){
                            OledClearBuffer();
                            int wait = 0;
                            while(wait < 200){//waiting 3 seconds
                                wait++;
                                OledSetCursor(6,2);
                                OledPutString("Win");
                            }
                            //updating scoreboard
                            if(curscore > h_score1){
                                h_score3 = h_score2;
                                h_score2 = h_score1;
                                h_score1 = curscore;
                            }
                            else if(curscore == h_score1){
                                h_score3 = h_score2;
                                h_score2 = curscore;
                            }
                            else if(curscore > h_score2){
                                h_score3 = h_score2;
                                h_score2 = curscore;
                            }
                            else if(curscore == h_score2){
                                h_score3 = curscore;
                            }
                            else if(curscore > h_score3){
                                h_score3 = curscore;
                                
                            }
                            //reinitializing all variables
                            check = 0;
                            curscore = 0;
                            checktrap1 = 0;
                            checkheart1 = 0;
                            checktrap2 = 0;
                            checkheart2 = 0;
                            glyph_pixel_x = 0;
                            glyph_pixel_y = 8;
                            OledClearBuffer();
                            OledSetCursor(0, 0);
                            OledPutString("*");
                            star = 0;
                            mode = MAIN;
                            ///////////////////
                            trap_1_x = rand() % (112 - 16 + 1) + 16;
                            trap_1_y = rand() % (31 - 8 + 1) + 8;
                            trap_2_x = rand() % (112 - 16 + 1) + 16;
                            trap_2_y = rand() % (31 - 8 + 1) + 8;
                            heart_1_x = rand() % (112 - 16 + 1) + 16;
                            heart_1_y = rand() % (31 - 8 + 1) + 8;
                            heart_2_x = rand() % (112 - 16 + 1) + 16;
                            heart_2_y = rand() % (31 - 8 + 1) + 8;
                        }
                    }
                    
                    break;
//////////////////////////////////////////////////////////////////////////////////////////
                case SBOARD:
                    scoreDisplay(h_score1, h_score2, h_score3);//display scoreboard
                    if(PORTG & BUTTON2)
                    {
                        check = 0;
                        glyph_pixel_x = 0;
                        glyph_pixel_y = 8;
                        OledClearBuffer();
                        OledSetCursor(0, 0);
                        OledPutString("*");
                        star = 0;
                        mode = MAIN;
                    }
                    break;
///////////////////////////////////////////////////////////////////////////////////////////
                case DIFF:
                    diffMenu();//difficulty menu
                    moveDiff(UD_reading, &star);//selection function
                    if(LR_reading > 900){
                        if(star == 0){
                            difficulty = 1;
                        }
                        if(star == 1){
                            difficulty = 2;
                        }
                    }
                    if(PORTG & BUTTON2)
                    {
                        check = 0;
                        glyph_pixel_x = 0;
                        glyph_pixel_y = 8;
                        OledClearBuffer();
                        OledSetCursor(0, 0);
                        OledPutString("*");
                        star = 0;
                        mode = MAIN;
                    }
                    break;
/////////////////////////////////////////////////////////////////////////////////////////////
                case HTPM:
                    HTPMenu();//How to play menu
                    if(PORTG & BUTTON2)
                    {
                        check = 0;
                        glyph_pixel_x = 0;
                        glyph_pixel_y = 8;
                        OledClearBuffer();
                        OledSetCursor(0, 0);
                        OledPutString("*");
                        star = 0;
                        mode = MAIN;
                    }
                    break;
            }
        }
    
    }
    return (EXIT_SUCCESS);
}

