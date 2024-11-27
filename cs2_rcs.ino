#include <Mouse.h>
#include <usbhub.h>
#include <hidboot.h>
#include <usbhid.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h> // LCD library for smoothing display

// USB Host Shield setup
USB Usb;
HIDBoot<HID_PROTOCOL_MOUSE> Mouse1(&Usb); // HIDBoot for mouse input

// LCD setup (16x2 I2C LCD display)
LiquidCrystal_I2C lcd(0x27, 16, 2); // Set up LCD at address 0x27

// Define the recoil patterns for different rifles
int ak47Recoil[][2] = {
  {0, 3},   // 1
  {1, 6},   // 2
  {-1, 7},  // 3
  {0, 8},   // 4
  {2, 9},   // 5
  {1, 9},   // 6
  {0, 10},  // 7
  {1, 8},   // 8
  {0, 9}    // 9
};

int m4a4Recoil[][2] = {
  {0, 3},   // 1
  {1, 4},   // 2
  {0, 5},   // 3
  {-1, 6},  // 4
  {1, 7},   // 5
  {0, 8},   // 6
  {0, 8},   // 7
  {-1, 7},  // 8
  {0, 9}    // 9
};

int m4a1sRecoil[][2] = {
  {0, 2},   // 1
  {1, 5},   // 2
  {-1, 6},  // 3
  {0, 7},   // 4
  {1, 8},   // 5
  {0, 9},   // 6
  {0, 10},  // 7
  {0, 8},   // 8
  {-1, 9}   // 9
};

int famasRecoil[][2] = {
  {0, 4},   // 1
  {1, 5},   // 2
  {0, 6},   // 3
  {0, 7},   // 4
  {1, 8},   // 5
  {0, 8},   // 6
  {-1, 9},  // 7
  {0, 8},   // 8
  {0, 10}   // 9
};

int sg553Recoil[][2] = {
  {0, 4},   // 1
  {1, 5},   // 2
  {-1, 6},  // 3
  {0, 7},   // 4
  {1, 8},   // 5
  {0, 8},   // 6
  {1, 9},   // 7
  {0, 9},   // 8
  {0, 10}   // 9
};

int augRecoil[][2] = {
  {0, 3},   // 1
  {1, 5},   // 2
  {0, 6},   // 3
  {-1, 7},  // 4
  {1, 8},   // 5
  {0, 8},   // 6
  {1, 9},   // 7
  {0, 9},   // 8
  {0, 10}   // 9
};

// Array of recoil patterns for easy switching
int (*recoilPatterns[])[2] = {
  ak47Recoil,
  m4a4Recoil,
  m4a1sRecoil,
  famasRecoil,
  sg553Recoil,
  augRecoil
};

// Define an array of pattern names
const char* patternNames[] = {
  "AK-47",
  "M4A4",
  "M4A1-S",
  "FAMAS",
  "SG553",
  "AUG"
};

int patternSizes[] = {
  sizeof(ak47Recoil) / sizeof(ak47Recoil[0]),
  sizeof(m4a4Recoil) / sizeof(m4a4Recoil[0]),
  sizeof(m4a1sRecoil) / sizeof(m4a1sRecoil[0]),
  sizeof(famasRecoil) / sizeof(famasRecoil[0]),
  sizeof(sg553Recoil) / sizeof(sg553Recoil[0]),
  sizeof(augRecoil) / sizeof(augRecoil[0])
};

// Fire rates in milliseconds
int fireRates[] = {
  100,  // AK-47: 600 RPM
  90,   // M4A4: 666 RPM
  100,  // M4A1-S: 600 RPM
  90,   // FAMAS: 666 RPM
  90,   // SG553: 666 RPM
  90    // AUG: 666 RPM
};

// Timing variables for updating the LCD with the character sequence
const char* charPatterns[] = {
  "/)*-*/) ",
  "/)0-*/) ",
  "/)0-0/) ",
  "(\0-*(\ ",
  "(\*-*(\ ",
  "(\*-*(\" "
};

// Timing variables
unsigned long lastPatternChangeTime = 0;
const unsigned long patternChangeInterval = 1000; // 1 second interval
int currentCharPatternIndex = 0;

// Global variables
int currentPattern = 0;    // Index of the current recoil pattern
int stepDelay = 50;        // Delay between recoil steps
int smoothing = 5;         // Smoothing factor for recoil compensation
bool leftMouseButtonPressed = false; // Flag for detecting left mouse button press

// Mouse event handler
class MouseRptParser : public MouseReportParser {
  void OnMouseMove(MOUSEINFO *mi) override {
    // Not used here, but available for mouse movement handling
  }

  void OnLeftButtonUp(MOUSEINFO *mi) override {
    leftMouseButtonPressed = false;  // Left mouse button released
  }

  void OnLeftButtonDown(MOUSEINFO *mi) override {
    leftMouseButtonPressed = true;   // Left mouse button pressed
  }
};

MouseRptParser mouseParser;  // Mouse report parser instance

// Button configuration
const int buttonPin = 2;     // Pin for the pattern switch button
bool lastButtonState = LOW;   // Previous state of the button
unsigned long lastDebounceTime = 0; // Last time the button state changed
unsigned long debounceDelay = 50; // Debounce delay

void setup() {
  Serial.begin(115200);
  
  // Initialize the USB Host Shield
  if (Usb.Init() == -1) {
    Serial.println("USB Host Shield initialization failed");
    while (1); // Halt if initialization fails
  }
  
  // Set up mouse report parser
  Mouse1.SetReportParser(0, &mouseParser);
  
  // Initialize LCD
  lcd.begin();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Smoothing:");
  displaySmoothing();  // Display initial smoothing value
  
  // Display the initial pattern name
  displayPatternName(); // Display initial pattern name on LCD
  
  // Initialize button pin
  pinMode(buttonPin, INPUT);
  
  delay(3000); // Delay before starting
}

void loop() {
  Usb.Task(); // Handle USB Host Shield tasks
  
  // Check if the left mouse button is pressed
  if (leftMouseButtonPressed) {
    // Call simulateRecoil with the currently selected pattern, size, and fire rate
    simulateRecoil(recoilPatterns[currentPattern], patternSizes[currentPattern], fireRates[currentPattern]);
  }
  
  // Read button state for switching patterns
  int buttonState = digitalRead(buttonPin);
  
  // Check for button state change
  if (buttonState != lastButtonState) {
    lastDebounceTime = millis(); // Reset debounce timer
  }

  // Handle LCD character pattern update every second
  if (millis() - lastPatternChangeTime >= patternChangeInterval) {
    lastPatternChangeTime = millis(); // Reset the timer
    displayCharPattern(); // Update the LCD with the current character pattern

    // Move to the next pattern in the array
    currentCharPatternIndex = (currentCharPatternIndex + 1) % (sizeof(charPatterns) / sizeof(charPatterns[0]));
  }

  // Check if the button is pressed
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (buttonState == HIGH) {
      // Switch to the next pattern
      currentPattern = (currentPattern + 1) % (sizeof(recoilPatterns) / sizeof(recoilPatterns[0]));
      Serial.print("Switched to: ");
      Serial.println(patternNames[currentPattern]); // Print selected pattern to console
      displayPatternName(); // Update LCD with the new pattern name
    }
  }

  lastButtonState = buttonState; // Update last button state
}

// Function to simulate recoil
void simulateRecoil(int recoilPattern[][2], int patternSize, int fireRate) {
  for (int i = 0; i < patternSize; i++) {
    int moveX = recoilPattern[i][0]; // Horizontal recoil
    int moveY = recoilPattern[i][1]; // Vertical recoil
    
    // Apply smoothing to the recoil
    for (int j = 0; j < smoothing; j++) {
      int smoothedX = moveX / smoothing;
      int smoothedY = moveY / smoothing;

      Mouse.move(smoothedX, smoothedY);  // Move mouse smoothly
      delay(fireRate / smoothing); // Smooth delay based on fire rate
    }

    // Delay between each shot according to fire rate
    delay(fireRate);  
  }
}

// Function to display the current smoothing value on the LCD
void displaySmoothing() {
  lcd.setCursor(0, 1);
  lcd.print("Smoothing: ");
  lcd.print(smoothing);
}

// Function to display the current recoil pattern name on the LCD
void displayPatternName() {
  lcd.setCursor(0, 1);
  lcd.print("Pattern:    "); // Clear previous pattern name
  lcd.setCursor(0, 1);
  lcd.print(patternNames[currentPattern]); // Print the current pattern name
}

// Function to display character patterns on the LCD
void displayCharPattern() {
  lcd.setCursor(0, 1);
  lcd.print(charPatterns[currentCharPatternIndex]); // Print character pattern on LCD
}