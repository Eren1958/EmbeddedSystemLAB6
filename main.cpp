#include "mbed.h"
#include "arm_book_lib.h"
#include "display.h"
#include "matrix_keypad.h"
#include <string.h> 
#include <stdio.h>  

#define SYSTEM_TIME_INCREMENT_MS 10
#define ALARM_REPORT_INTERVAL_MS 60000 
#define SAFE_TEMP_LIMIT 35.0           

DigitalOut buzzer(D7);         
AnalogIn myTempSensor(A1);     

typedef enum {
    SYSTEM_SAFE,   
    SYSTEM_DANGER, 
    SYSTEM_MUTED   
} SystemState;

static SystemState currentState = SYSTEM_SAFE;

static int accumulatedReportTime = 0; 
static char inputBuffer[6] = "";       
static int inputIndex = 0;
static const char DEACTIVATION_CODE[] = "11111"; 

extern bool gasDetectorStateRead();

static void systemInit()
{
    displayInit(DISPLAY_CONNECTION_I2C_PCF8574_IO_EXPANDER);
    matrixKeypadInit(SYSTEM_TIME_INCREMENT_MS);
    
    buzzer = 0; 

    displayCharPositionWrite(0, 0);
    displayStringWrite("Smart Home Sys  ");
    displayCharPositionWrite(0, 1);
    displayStringWrite("System Initial  ");
}

static void systemLoop()
{
    float currentTemp = myTempSensor.read() * 3.3f * 100.0f; 
    bool isGasDetected = gasDetectorStateRead();
    bool limitExceeded = (currentTemp > SAFE_TEMP_LIMIT || isGasDetected);

    switch (currentState) {
        case SYSTEM_SAFE:
            if (limitExceeded) {
                currentState = SYSTEM_DANGER;
                displayCharPositionWrite(0, 3);
                displayStringWrite("WARN: LIMIT HIT!"); 
            }
            break;

        case SYSTEM_DANGER:
            if (!limitExceeded) {
                currentState = SYSTEM_SAFE;
                displayCharPositionWrite(0, 3);
                displayStringWrite("                "); 
            }
            break;

        case SYSTEM_MUTED:
            if (!limitExceeded) {
                currentState = SYSTEM_SAFE; 
                displayCharPositionWrite(0, 3);
                displayStringWrite("                ");
            }
            break;
    }

    char key = matrixKeypadUpdate(); 
    
    if (key != '\0') {
        if (key == '4') {
            displayCharPositionWrite(0, 0);
            displayStringWrite(isGasDetected ? "Gas: DETECTED!  " : "Gas: Safe       ");
        } 
        else if (key == '5') {
            char tempString[17]; 
            sprintf(tempString, "Temp: %.1f C     ", currentTemp);
            displayCharPositionWrite(0, 0);
            displayStringWrite(tempString);
        } 
        else if (key >= '0' && key <= '9') {
            if (inputIndex < 5) {
                inputBuffer[inputIndex++] = key;
                inputBuffer[inputIndex] = '\0';
                
                displayCharPositionWrite(0, 1);
                displayStringWrite("Code:           ");
                displayCharPositionWrite(6, 1);
                displayStringWrite(inputBuffer);
                
                if (inputIndex == 5) {
                    if (strcmp(inputBuffer, DEACTIVATION_CODE) == 0) {
                        currentState = SYSTEM_MUTED; 
                        displayCharPositionWrite(0, 0);
                        displayStringWrite("Alarm OFF       ");
                    } else {
                        displayCharPositionWrite(0, 0);
                        displayStringWrite("Wrong Code!     ");
                    }
                    inputIndex = 0;
                    inputBuffer[0] = '\0';
                }
            }
        }
    }

    if (currentState == SYSTEM_DANGER) {
        buzzer = 0; 
    } else {
        buzzer = 1; 
    }

    if (accumulatedReportTime >= ALARM_REPORT_INTERVAL_MS) {
        displayCharPositionWrite(0, 2);
        if (currentState == SYSTEM_DANGER) {
            displayStringWrite("Alarm is: ON    ");
        } else {
            displayStringWrite("Alarm is: OFF   ");
        }
        accumulatedReportTime = 0; 
    } else {
        accumulatedReportTime += SYSTEM_TIME_INCREMENT_MS;
    }
}

int main()
{
    systemInit();
    while (true) {
        systemLoop();
        delay(SYSTEM_TIME_INCREMENT_MS); 
    }
}
