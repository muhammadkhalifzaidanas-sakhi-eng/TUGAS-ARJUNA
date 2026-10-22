#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <SD.h>

// Definisi Pin LED (PB12 digunakan untuk LED Merah agar PB7 bebas untuk I2C SDA LCD)
const int PIN_LED_RED    = PB12; 
const int PIN_LED_YELLOW = PB8;  
const int PIN_LED_GREEN  = PB9;  

// Definisi Pin Sensor Analog
const int PIN_APPS     = PA0; 
const int PIN_BSE      = PA1; 
const int PIN_RPM      = PA2; 
const int PIN_SUHU     = PA3; 

// Definisi Pin MicroSD Chip Select
const int PIN_SD_CS    = PA4; 

// Inisialisasi Modul LCD I2C 20x4
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Variabel Status Sistem
bool interlockState = false;
bool sdReady = false;

void setup() {
  Serial.begin(115200);

  // Konfigurasi Mode Pin LED
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_YELLOW, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);

  // Kondisi Awal: Semua LED MATI
  digitalWrite(PIN_LED_RED, LOW);
  digitalWrite(PIN_LED_YELLOW, LOW);
  digitalWrite(PIN_LED_GREEN, LOW);

  // Inisialisasi Bus I2C & LCD 20x4
  Wire.begin();
  lcd.init();
  lcd.backlight();
  lcd.clear();

  // Mengaktifkan bus SPI1 (PA4=CS, PA5=SCK, PA6=MISO, PA7=MOSI)
  SPI.begin();

  // Inisialisasi MicroSD (.txt)
  if (SD.begin(PIN_SD_CS)) {
    sdReady = true;
    File logFile = SD.open("datalog.txt", FILE_WRITE);
    if (logFile) {
      logFile.println("Time_ms\tAPPS_pct\tBSE_pct\tRPM\tSuhu_C\tInterlock");
      logFile.close();
      Serial.println("MicroSD Siap! File datalog.txt berhasil dibuat.");
    }
  } else {
    Serial.println("Peringatan: MicroSD Gagal / Tidak Terdeteksi!");
  }
}

void loop() {
  // 1. Membaca nilai ADC dari simulator (0 - 1023)
  int rawAPPS = analogRead(PIN_APPS);
  int rawBSE  = analogRead(PIN_BSE);
  int rawRPM  = analogRead(PIN_RPM);
  int rawSuhu = analogRead(PIN_SUHU);

  // 2. Konversi Nilai Sensor (Pembagi 1023.0 sesuai skala simulator)
  float appsPct = (rawAPPS / 1023.0) * 100.0; //[cite: 1]
  float bsePct  = (rawBSE  / 1023.0) * 100.0; //[cite: 1]
  float rpmVal  = (rawRPM  / 1023.0) * 3000.0;
  float suhuVal = (rawSuhu / 1023.0) * 150.0;

  // 3. Logika Kontrol Interlock & LED[cite: 1]
  if (appsPct >= 25.0 && bsePct >= 10.0) {
    interlockState = true;   // Interlock Aktif[cite: 1]
  } else if (appsPct <= 5.0 && bsePct < 10.0) {
    interlockState = false;  // Interlock Non-Aktif[cite: 1]
  }

  // Kontrol LED Kuning (Menyala jika BSE >= 10%)[cite: 1]
  digitalWrite(PIN_LED_YELLOW, (bsePct >= 10.0) ? HIGH : LOW);

  // Kontrol LED Merah & Hijau
  if (interlockState) {
    digitalWrite(PIN_LED_RED, HIGH);   // LED Merah ON[cite: 1]
    digitalWrite(PIN_LED_GREEN, LOW);  // LED Hijau OFF[cite: 1]
  } else {
    digitalWrite(PIN_LED_RED, LOW);    // LED Merah OFF[cite: 1]
    if (appsPct > 5.0 && bsePct < 10.0) {
      digitalWrite(PIN_LED_GREEN, HIGH); // LED Hijau ON[cite: 1]
    } else {
      digitalWrite(PIN_LED_GREEN, LOW);  // LED Hijau OFF[cite: 1]
    }
  }

  // 4. Proses Penyimpanan Data ke MicroSD (.txt)
  bool writeSuccess = false;
  if (sdReady) {
    File logFile = SD.open("datalog.txt", FILE_WRITE);
    if (logFile) {
      logFile.print(millis());   logFile.print("\t");
      logFile.print(appsPct, 1); logFile.print("\t");
      logFile.print(bsePct, 1);  logFile.print("\t");
      logFile.print(rpmVal, 0);  logFile.print("\t");
      logFile.print(suhuVal, 1); logFile.print("\t");
      logFile.println(interlockState ? 1 : 0);
      logFile.close();
      writeSuccess = true;
    }
  }

  // 5. Output ke Serial Monitor
  Serial.print("APPS: "); Serial.print(appsPct, 1); Serial.print("% | ");
  Serial.print("BSE: "); Serial.print(bsePct, 1); Serial.print("% | ");
  Serial.print("RPM: "); Serial.print(rpmVal, 0); Serial.print(" | ");
  Serial.print("Suhu: "); Serial.print(suhuVal, 1); Serial.print(" C | ");
  Serial.print("Interlock: "); Serial.print(interlockState ? "ACTIVE" : "INACTIVE");
  Serial.println(writeSuccess ? " | SD: Saved OK" : " | SD: Error");

  // 6. Tampilan di LCD 20x4
  // Baris 0: APPS & BSE
  lcd.setCursor(0, 0);
  lcd.print("APPS:"); lcd.print((int)appsPct); lcd.print("%   ");
  lcd.setCursor(10, 0);
  lcd.print("BSE:"); lcd.print((int)bsePct); lcd.print("%   ");

  // Baris 1: RPM & Suhu
  lcd.setCursor(0, 1);
  lcd.print("RPM :"); lcd.print((int)rpmVal); lcd.print("   ");
  lcd.setCursor(10, 1);
  lcd.print("SUHU:"); lcd.print((int)suhuVal); lcd.print("C  ");

  // Baris 2: Status Interlock dalam bahasa Inggris
  lcd.setCursor(0, 2);
  lcd.print("INTERLOCK: ");
  lcd.print(interlockState ? "ACTIVE  " : "INACTIVE");

  // Baris 3: Status SD Card
  lcd.setCursor(0, 3);
  lcd.print("SD  :");
  lcd.print(writeSuccess ? "SAVED OK      " : "ERROR / NONE  ");

  delay(200);
}