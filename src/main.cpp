#define T85

#include <Arduino.h>
#ifdef T85
    #include <avr/eeprom.h>
#endif


// Configuration Constants
#define BASELINE_BRIGHTNESS 0.4 // Baseline brightness percentage (40%)
#define MODULATION_SPEED 8      // Speed of brightness modulation
#ifndef T85
    #define LED_PIN 9               // PWM pin connected to the LED driver
    #define BATTERY_PIN A0          // Analog pin for battery voltage monitoring
#else
    #define LED_PIN 1               // PWM pin on PB1 for ATtiny
    #define BATTERY_PIN A1          // Battery pin on PB2 for ATtiny
#endif

#define LED_BLINK_DELAY 250     // Delay in milliseconds for LED blinks

// Battery voltage configuration
#define BATTERY_DIVIDER_RATIO 2.0    // Voltage divider ratio (2S LiPo through a /2 resistive divider)
#define MAX_BATTERY_VOLTAGE 8.4      // Maximum voltage for a fully charged 2S LiPo
#define MIN_BATTERY_VOLTAGE 6.6      // Minimum safe voltage for a 2S LiPo
#define ADC_REF_VOLTAGE 5.0          // Reference voltage for ADC (MCU supply voltage)
#define ADC_RESOLUTION 1024          // ADC resolution (10-bit for most MCU)


// Global variables

// Brightness Levels
const float brightnessLevels[4] = {0.1, 0.2, 0.6, 1.0}; // Brightness levels as fractions of maximum
const uint8_t numBrightnessLevels = sizeof(brightnessLevels) / sizeof(brightnessLevels[0]);
// EEPROM address to store the current brightness level index
uint8_t EEMEM eepromBrightnessIndex = 0;
uint8_t currentBrightnessIndex = 0;

// Variables for heart-beat simulation
int modulationCounter = 0;       // Tracks modulation steps
int modulationInterval = MODULATION_SPEED; // Modulation interval (adjusted dynamically)
bool isSystolePhase = false;     // Heartbeat phase (diastole or systole)
uint8_t phaseStep = 0;           // Current position in the sine table
uint8_t pwmValue = 0;            // Calculated PWM value for the LED

// Sine wave table in PROGMEM for smooth brightness transitions
PROGMEM const uint8_t sineTable[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 3, 3, 4, 5, 6, 7, 8, 9, 10, 12, 13, 15, 16, 18, 19, 21, 23, 25, 26, 28, 
    30, 33, 35, 37, 39, 41, 44, 46, 49, 51, 54, 56, 59, 61, 64, 67, 70, 72, 75, 78, 81, 84, 87, 90, 93, 96, 99, 102, 
    105, 108, 111, 115, 118, 121, 124, 127, 130, 133, 136, 139, 143, 146, 149, 152, 155, 158, 161, 164, 167, 170, 173, 
    176, 179, 182, 184, 187, 190, 193, 195, 198, 200, 203, 205, 208, 210, 213, 215, 217, 219, 221, 224, 226, 228, 229, 
    231, 233, 235, 236, 238, 239, 241, 242, 244, 245, 246, 247, 248, 249, 250, 251, 251, 252, 253, 253, 254, 254, 254, 
    254, 254, 255, 254, 254, 254, 254, 254, 253, 253, 252, 251, 251, 250, 249, 248, 247, 246, 245, 244, 242, 241, 239, 
    238, 236, 235, 233, 231, 229, 228, 226, 224, 221, 219, 217, 215, 213, 210, 208, 205, 203, 200, 198, 195, 193, 190, 
    187, 184, 182, 179, 176, 173, 170, 167, 164, 161, 158, 155, 152, 149, 146, 143, 139, 136, 133, 130, 127, 124, 121, 
    118, 115, 111, 108, 105, 102, 99, 96, 93, 90, 87, 84, 81, 78, 75, 72, 70, 67, 64, 61, 59, 56, 54, 51, 49, 46, 44, 
    41, 39, 37, 35, 33, 30, 28, 26, 25, 23, 21, 19, 18, 16, 15, 13, 12, 10, 9, 8, 7, 6, 5, 4, 3, 3, 2, 1, 1
};

// Function to blink the LED a specified number of times
void blink_LED(uint8_t count, uint16_t delay_ms) {
    for (uint8_t i = 0; i < count; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(delay_ms);
        digitalWrite(LED_PIN, LOW);
        delay(delay_ms);
    }
}

void setup() {
    // Delay to ensure power supply is stable
    delay(200);


    // Initialize the LED pin and battery voltage pin
    pinMode(LED_PIN, OUTPUT);
    pinMode(BATTERY_PIN, INPUT);

    // Read battery voltage
    int rawADC = analogRead(BATTERY_PIN);
    float batteryVoltage = (rawADC * ADC_REF_VOLTAGE / ADC_RESOLUTION) * BATTERY_DIVIDER_RATIO;

    // Calculate battery percentage
    float batteryPercentage = (batteryVoltage - MIN_BATTERY_VOLTAGE) / 
                              (MAX_BATTERY_VOLTAGE - MIN_BATTERY_VOLTAGE) * 100.0;
    batteryPercentage = constrain(batteryPercentage, 0, 100);

    // Determine the number of blinks based on battery percentage
    uint8_t blinkCount = 0;
    if (batteryPercentage > 75) {
        blinkCount = 4;
    } else if (batteryPercentage > 50) {
        blinkCount = 3;
    } else if (batteryPercentage > 25) {
        blinkCount = 2;
    } else {
        blinkCount = 1;
    }

    // Read the current brightness index from EEPROM
    currentBrightnessIndex = eeprom_read_byte(&eepromBrightnessIndex);
    if (currentBrightnessIndex >= numBrightnessLevels) {
        currentBrightnessIndex = 0;  // Reset if out of bounds
    }

    // Save the next brightness level index to EEPROM
    eeprom_write_byte(&eepromBrightnessIndex, (currentBrightnessIndex + 1) % numBrightnessLevels);



    // Blink the LED to indicate battery percentage
    blink_LED(blinkCount, LED_BLINK_DELAY);
    
    // Delay before indicating preset brightness level
    delay(1000);

    // Blink the LED to indicate the current brightness level
    blink_LED(currentBrightnessIndex+1, LED_BLINK_DELAY/2);

    // Small delay to separate setup indication from the main loop
    delay(500);
}

void loop() {
    modulationCounter++;

    // Update brightness at the end of the modulation interval
    if (modulationCounter == modulationInterval) {
        // Calculate sine wave value (scale for diastole:systole ratio of 2:3)
        uint8_t sineValue = (isSystolePhase) 
                            ? pgm_read_byte(&sineTable[phaseStep]) * 0.6 
                            : pgm_read_byte(&sineTable[phaseStep]);

        // Adjust brightness with baseline and current level
        pwmValue = BASELINE_BRIGHTNESS * 255 + sineValue * (1 - BASELINE_BRIGHTNESS);
        pwmValue *= brightnessLevels[currentBrightnessIndex];

        // Update phase and check for phase reset
        phaseStep++;
        if (phaseStep == 0) {
            isSystolePhase = !isSystolePhase; // Toggle heartbeat phase
            // Model diastole:systole duration ratio of 1:2
            modulationInterval = (isSystolePhase) ? MODULATION_SPEED : 2 * MODULATION_SPEED;
        }

        modulationCounter = 0; // Reset modulation counter
    }

    // Output the calculated PWM value to the LED
    analogWrite(LED_PIN, pwmValue);

    delay(1); // Small delay for smoother transitions
}
