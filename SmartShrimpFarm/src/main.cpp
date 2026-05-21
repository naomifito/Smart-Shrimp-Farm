#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHTesp.h"
#include <SPI.h>
#include <MFRC522.h>

// PIN DEFINITION
#define SS_PIN    5   // SDA RFID
#define RST_PIN   0   // Reset RFID
#define DHT_PIN   15
#define PH_PIN    34
#define SALT_PIN  35
#define LED_RED   2
#define BUZZER    4

// OBJECTS
LiquidCrystal_I2C lcd(0x27, 20, 4);
DHTesp dhtSensor;
MFRC522 rfid(SS_PIN, RST_PIN);

// CONFIGURATION
// Sudah disesuaikan dengan UID Green Card Wokwi Anda
String authorizedUID = "11:22:33:44";
bool isAuthorized = false;

// Function Prototypes
void handleRFID();
void updateSensors();
void showWelcomeMessage();

void setup() {
  Serial.begin(115200);
  SPI.begin();      
  rfid.PCD_Init();  
  Wire.begin(21, 22); 
  
  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);
  pinMode(LED_RED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  
  lcd.init();
  lcd.backlight();
  
  showWelcomeMessage();
}

void loop() {
  handleRFID();
  
  if (isAuthorized) {
    updateSensors();
  }
}

void showWelcomeMessage() {
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("SMART SHRIMP FARM");
  lcd.setCursor(2, 2);
  lcd.print("SCAN YOUR CARD...");
  digitalWrite(LED_RED, LOW);
  noTone(BUZZER);
}

void handleRFID() {
  // Cek apakah ada kartu baru ditempel
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }

  // Format UID agar cocok dengan 11:22:33:44
  String content = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    content.concat(String(rfid.uid.uidByte[i] < 0x10 ? "0" : ""));
    content.concat(String(rfid.uid.uidByte[i], HEX));
    if (i < rfid.uid.size - 1) content.concat(":");
  }
  content.toUpperCase();

  Serial.println("UID Terdeteksi: " + content);

  if (content == authorizedUID) {
    // Toggle Status (Jika sudah login, maka logout. Jika belum, maka login)
    isAuthorized = !isAuthorized; 
    
    lcd.clear();
    if (isAuthorized) {
      lcd.setCursor(3, 1);
      lcd.print("AKSES DITERIMA");
      tone(BUZZER, 1500, 200); // Bunyi bip sukses
    } else {
      lcd.setCursor(4, 1);
      lcd.print("LOGOUT BERHASIL");
      tone(BUZZER, 1000, 500); // Bunyi bip logout
    }
    
    delay(2000);
    if (!isAuthorized) showWelcomeMessage();
    lcd.clear();
  } else {
    // Jika kartu salah
    lcd.clear();
    lcd.setCursor(3, 1);
    lcd.print("AKSES DITOLAK!");
    lcd.setCursor(2, 2);
    lcd.print("ID: " + content);
    
    digitalWrite(LED_RED, HIGH);
    tone(BUZZER, 200, 800); // Bunyi peringatan
    delay(2500);
    digitalWrite(LED_RED, LOW);
    
    if (!isAuthorized) showWelcomeMessage();
  }
  
  // Berhenti membaca kartu yang sama
  rfid.PICC_HaltA();
}

void updateSensors() {
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  float suhu = data.temperature;
  float phValue = 3.5 * (analogRead(PH_PIN) * (3.3 / 4095.0));
  float saltValue = (analogRead(SALT_PIN) / 4095.0) * 50.0;

  lcd.setCursor(0, 0);
  lcd.print("Suhu Air  : "); lcd.print(suhu, 1); lcd.print(" C  ");
  lcd.setCursor(0, 1);
  lcd.print("Kadar pH  : "); lcd.print(phValue, 1); lcd.print("    ");
  lcd.setCursor(0, 2);
  lcd.print("Salinitas : "); lcd.print(saltValue, 1); lcd.print(" ppt ");

  lcd.setCursor(0, 3);
  if (phValue < 7.5 || phValue > 8.5 || suhu > 32.0 || suhu < 27.0) {
    digitalWrite(LED_RED, HIGH);
    tone(BUZZER, 1000); 
    lcd.print("STATUS    : BAHAYA  "); 
  } else {
    digitalWrite(LED_RED, LOW);
    noTone(BUZZER);
    lcd.print("STATUS    : AMAN    "); 
  }
  delay(500); // Refresh data lebih cepat
}