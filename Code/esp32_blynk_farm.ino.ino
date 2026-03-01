#define BLYNK_TEMPLATE_ID "Your BLYNK TEMPLATE ID"
#define BLYNK_TEMPLATE_NAME "Your BLYNK TEMPLATE NAME"
#define BLYNK_AUTH_TOKEN "Your BLYNK AUTH TOKEN"

#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>

#define LDR 4
#define TH 5
#define Rain 36
#define SoilSensor 34
#define RelayPin 27

LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(TH, DHT11);
BlynkTimer timer;
WidgetLED led(V4);

char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "Your wifi name";
char pass[] = "Your wifi password";

bool manualRelayControl = false; // Flag for manual relay control

void setup() {
  Serial.begin(115200);
  Blynk.begin(auth, ssid, pass);

  dht.begin();
  lcd.init();
  lcd.backlight();

  pinMode(LDR, INPUT);
  pinMode(Rain, INPUT);
  pinMode(SoilSensor, INPUT);
  pinMode(RelayPin, OUTPUT);

  analogReadResolution(12);

  lcd.setCursor(0, 0);
  lcd.print("System");
  lcd.setCursor(4, 1);
  lcd.print("Loading..");
  delay(4000);
  lcd.clear();

  timer.setInterval(2000L, sendSensorData); // every 2 seconds

  Serial.println("Setup done");
}

// Main sensor data send function
void sendSensorData() {
  DHT11sensor();
  rainSensor();
  soilSensorAndRelay();
  LDRsensor();
}

// Read DHT11 sensor
void DHT11sensor() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  Blynk.virtualWrite(V0, t);
  Blynk.virtualWrite(V1, h);

  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(t, 1);
  lcd.print("C ");

  lcd.setCursor(8, 0);
  lcd.print("H:");
  lcd.print(h, 1);
  lcd.print("% ");

  Serial.print("Temp: ");
  Serial.print(t);
  Serial.print(" C, Humidity: ");
  Serial.print(h);
  Serial.println(" %");
}

// Read rain sensor
void rainSensor() {
  int Rvalue = analogRead(Rain);
  Rvalue = map(Rvalue, 0, 4095, 0, 100);
  Rvalue = (Rvalue - 100) * -1;
  Blynk.virtualWrite(V2, Rvalue);

  lcd.setCursor(0, 1);
  lcd.print("R:");
  lcd.print(Rvalue);
  lcd.print("% ");

  Serial.print("Rain: ");
  Serial.print(Rvalue);
  Serial.println("%");
}

// Read soil moisture and control relay automatically
void soilSensorAndRelay() {
  int soilValue = analogRead(SoilSensor);  // Raw analog soil sensor value (0-4095)
  int soilPercent = map(soilValue, 4095, 0, 0, 100); // Map to percentage (dry=100%, wet=0%)

  Blynk.virtualWrite(V5, soilPercent);

  lcd.setCursor(8, 1);
  lcd.print("Soil:");
  lcd.print(soilPercent);
  lcd.print("% ");

  Serial.print("Soil Moisture: ");
  Serial.print(soilValue);
  Serial.print(" raw, ");
  Serial.print(soilPercent);
  Serial.println("%");

  if (!manualRelayControl) {
    if (soilPercent < 40) {   // Soil dry below 40%
      digitalWrite(RelayPin, HIGH); // Turn ON relay
      Serial.println("Relay ON (Auto: Soil Dry)");
    } else {                  // Soil wet 40% or above
      digitalWrite(RelayPin, LOW);  // Turn OFF relay
      Serial.println("Relay OFF (Auto: Soil Wet)");
    }
  }
}

// Read LDR sensor and update LED widget
void LDRsensor() {
  bool value = digitalRead(LDR);
  if (value == 1) {
    led.on();
  } else {
    led.off();
  }
}

// Blynk button handler for manual relay control
BLYNK_WRITE(V6) {
  int buttonState = param.asInt();
  Serial.print("Manual Relay Control Button: ");
  Serial.println(buttonState);

  if (buttonState == 1) {
    manualRelayControl = true;
    digitalWrite(RelayPin, HIGH);
    Serial.println("Relay ON (Manual)");
  } else {
    manualRelayControl = false; // Release manual control; auto control resumes
    Serial.println("Manual control OFF; Auto control resumes");
  }
}

void loop() {
  Blynk.run();
  timer.run();
}
