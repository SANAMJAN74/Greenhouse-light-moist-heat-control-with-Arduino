#include <EEPROM.h>
#include <LiquidCrystal.h>
#include <LM35.h>

LM35 temper(A1);               // LM35 Temp Sensor on A1
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);  // LCD pins (RS, EN, D4, D5, D6, D7)

// Button definitions
#define btnRIGHT   0
#define btnUP      1
#define btnDOWN    2
#define btnLEFT    3
#define btnSELECT  4
#define btnNONE    5

// Hardware pins
#define Mois       A3    // Moisture sensor
#define Pump       3     // Pump control (PWM)
#define Fan        11    // Fan control (PWM)
#define LED        12    // Light control
#define Relay      13    // (Unused in this code)
#define photoCellValue A2  // Photocell (LDR)

// EEPROM addresses for saving settings
#define addressTemp     0
#define addressFan      5
#define addressLight    10
#define addressPump     15
#define addressMoisture 25

// System parameters
struct parameter {
  int speed_fan;
  int speed_pump;
  float temp_value;
  int mois_value;
  int light_value;
} lcdParametr;

// Menu items
char menu[7][13] = {
  "Set TEMP ",
  "Set PUMP ",
  "Set Cell ",
  "Set Fan",
  "Set Moisture",
  "About ",
  "  "
};

int key = btnNONE;
int i = 0;
int Temp;
int Moisture;
int speedFan;
int lightLED;
int speedPump;
int START = 0;

void setup() {
  lcd.begin(16, 2);        // Initialize LCD
  Serial.begin(9600);      // Start serial communication

  // Load saved settings from EEPROM
  EEPROM.get(addressTemp, Temp);
  EEPROM.get(addressFan, speedFan);
  EEPROM.get(addressLight, lightLED);
  EEPROM.get(addressPump, speedPump);
  EEPROM.get(addressMoisture, Moisture);

  // Set pin modes
  pinMode(Relay, OUTPUT);
  pinMode(Pump, OUTPUT);
  pinMode(Fan, OUTPUT);
  pinMode(LED, OUTPUT);
}

void loop() {
  // Check button input
  if (getKey() == btnRIGHT) {
    displayMenu();
  }

  // Update sensor readings and control actuators
  showParametrer();
  checkParameter();

  // Serial control (for debugging/remote commands)
  if (Serial.available() > 0) {
    char ch = Serial.read();
    switch (ch) {
      case '1':
        digitalWrite(LED, HIGH);
        Serial.println("LED ON");
        break;
      case '2':
        digitalWrite(LED, LOW);
        Serial.println("LED OFF");
        break;
      case '3':
        Serial.print("Light Level: ");
        Serial.println(lightLED);
        break;
    }
  }
}

//===============================================
// Core Functions
//===============================================

void checkParameter() {
  // Read temperature and control fan
  float tempValue = temper.cel();
  lcdParametr.temp_value = tempValue;
  if (tempValue > Temp) {
    startFan(tempValue);
  } else {
    startFan(0);
    lcdParametr.speed_fan = 0;
  }

  // Read moisture and control pump
  int moisValue = getMois();
  lcdParametr.mois_value = moisValue;
  if (Moisture > moisValue) {
    startPump(moisValue);
  } else {
    startPump(0);
    lcdParametr.speed_pump = 0;
  }

  // Read light and control LED
  int cellValue = getPhotoCell();
  lcdParametr.light_value = cellValue;
  if (cellValue < lightLED) {
    pwmLED(1);
  } else {
    pwmLED(0);
  }
}

void startFan(int value) {
  if (value != 0) {
    int temp = value - Temp;
    if (START == 0) {
      analogWrite(Fan, 200);
      delay(2000);
    }
    analogWrite(Fan, (temp + 10) + speedFan);
    lcdParametr.speed_fan = (temp + 10) + speedFan;
    START = 1;
  } else {
    analogWrite(Fan, 0);
    START = 0;
  }
}

void startPump(int value) {
  if (value != 0) {
    analogWrite(Pump, value + speedPump);
    lcdParametr.speed_pump = value + speedPump;
  } else {
    analogWrite(Pump, 0);
    lcdParametr.speed_pump = 0;
  }
}

void pwmLED(int pwm) {
  if (pwm) {
    digitalWrite(LED, HIGH);
  } else {
    digitalWrite(LED, LOW);
  }
}

int getMois() {
  int value = analogRead(Mois);
  return map(value, 0, 1023, 100, 0);  // Convert to percentage (0-100%)
}

int getPhotoCell() {
  int value = analogRead(photoCellValue);
  return map(value, 0, 1023, 0, 100);   // Convert to percentage (0-100%)
}

//===============================================
// LCD & Menu Functions
//===============================================

void showParametrer() {
  lcd.clear();
  lcd.print("Temp: ");
  lcd.print(lcdParametr.temp_value);
  lcd.print("C");
  
  lcd.setCursor(0, 1);
  lcd.print("Mois: ");
  lcd.print(lcdParametr.mois_value);
  lcd.print("%");
  delay(1000);

  lcd.clear();
  lcd.print("Fan: ");
  lcd.print(lcdParametr.speed_fan);
  lcd.print(" Pump: ");
  lcd.print(lcdParametr.speed_pump);
  
  lcd.setCursor(0, 1);
  lcd.print("Light: ");
  lcd.print(lcdParametr.light_value);
  lcd.print("%");
  delay(1000);
}

void displayMenu() {
  i = 0;
  showMenu(i);
  do {
    key = getKey();
    if (key == btnDOWN && i <= 4) {
      showMenu(i + 1);
    } else if (key == btnUP && i <= 4) {
      showMenu(i - 1);
    } else if (key == btnSELECT) {
      runMenu(i);
      key = btnLEFT;
    }
  } while (key != btnLEFT);
}

void showMenu(int index) {
  i = index;
  lcd.clear();
  lcd.print(">");
  lcd.print(menu[index]);
  if (index <= 5) {
    lcd.setCursor(1, 1);
    lcd.print(menu[index + 1]);
  }
}

void runMenu(int option) {
  switch (option) {
    case 0: setTemp(); break;
    case 1: setPump(); break;
    case 2: setCell(); break;
    case 3: setFan(); break;
    case 4: setMoisture(); break;
    case 5: about(); break;
  }
}

void setTemp()      { setValue("Set Temp:", addressTemp, 0); }
void setPump()      { setValue("Set Pump:", addressPump, Pump); }
void setCell()      { setValue("Set Light:", addressLight, LED); }
void setFan()       { setValue("Set Fan:", addressFan, Fan); }
void setMoisture()  { setValue("Set Moisture:", addressMoisture, 0); }

void setValue(char *title, int address, int pin) {
  int value;
  EEPROM.get(address, value);
  lcd.clear();
  lcd.print(title);
  
  do {
    key = getKey();
    if (key == btnUP) value++;
    else if (key == btnDOWN) value--;
    
    lcd.setCursor(0, 1);
    lcd.print(value);
    lcd.print("  ");
    analogWrite(pin, value);  // Preview changes in real-time
  } while (key != btnSELECT);
  
  EEPROM.put(address, value);
  updateSystem();
  lcd.clear();
  lcd.print("Saved!");
  delay(1200);
}

void about() {
  lcd.clear();
  lcd.print("Plant Control Sys");
  lcd.setCursor(0, 1);
  lcd.print("By Your Name");
  delay(3000);
}

void updateSystem() {
  EEPROM.get(addressTemp, Temp);
  EEPROM.get(addressFan, speedFan);
  EEPROM.get(addressLight, lightLED);
  EEPROM.get(addressPump, speedPump);
  EEPROM.get(addressMoisture, Moisture);
}

int read_LCD_buttons() {
  int adc_key_in = analogRead(0);
  if (adc_key_in > 1000) return btnNONE;
  if (adc_key_in < 50)   return btnRIGHT;
  if (adc_key_in < 250)  return btnUP;
  if (adc_key_in < 450)  return btnDOWN;
  if (adc_key_in < 650)  return btnLEFT;
  if (adc_key_in < 850)  return btnSELECT;
  return btnNONE;
}

int getKey() {
  int lcd_key = read_LCD_buttons();
  delay(250);  // Debounce delay
  return lcd_key;
}
