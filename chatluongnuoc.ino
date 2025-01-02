#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Khai báo LCD I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Khai báo DS18B20
#define ONE_WIRE_BUS 2
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Khai báo TDS
#define TdsSensorPin A1
#define VREF 5.0                   // Điện áp tham chiếu của ADC
#define SCOUNT 30                  // Số lượng mẫu đo
#define SAMPLE_INTERVAL 40         // Thời gian giữa các lần lấy mẫu (ms)
#define DISPLAY_INTERVAL 800       // Thời gian giữa các lần hiển thị dữ liệu (ms)

// Biến toàn cục
int analogBuffer[SCOUNT];
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0, copyIndex = 0;
float averageVoltage = 0, tdsValue = 0, waterTemperature = 25.0; // Nhiệt độ mặc định

void setup() {
  Serial.begin(115200);
  pinMode(TdsSensorPin, INPUT);

  // Khởi tạo LCD
  lcd.begin(16, 2);
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("-Dang khoi tao-");
  delay(2000);
  lcd.clear();

  // Khởi tạo cảm biến DS18B20
  sensors.begin();
}

void loop() {
  // Đọc dữ liệu cảm biến
  readSensorData();
  waterTemperature = getTemperature();

  static unsigned long displayTimepoint = millis();
  if (millis() - displayTimepoint > DISPLAY_INTERVAL) {
    displayTimepoint = millis();
    tdsValue = calculateTDS(waterTemperature);
    displayData(tdsValue, waterTemperature);
  }
}

// Hàm đọc nhiệt độ từ DS18B20
float getTemperature() {
  sensors.requestTemperatures();                     // Yêu cầu cảm biến đo nhiệt độ
  float temp = sensors.getTempCByIndex(0);           // Lấy giá trị nhiệt độ từ cảm biến đầu tiên
  if (temp == DEVICE_DISCONNECTED_C) {               // Kiểm tra nếu cảm biến không hoạt động
    return 25.0; // Giá trị mặc định nếu cảm biến bị lỗi
  }
  return temp;
}

// Hàm đọc dữ liệu từ cảm biến TDS
void readSensorData() {
  static unsigned long sampleTimepoint = millis();
  if (millis() - sampleTimepoint > SAMPLE_INTERVAL) {
    sampleTimepoint = millis();
    analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);
    analogBufferIndex++;
    if (analogBufferIndex == SCOUNT)
      analogBufferIndex = 0;
  }
}

// Hàm tính toán giá trị TDS
float calculateTDS(float temperature) {
  for (copyIndex = 0; copyIndex < SCOUNT; copyIndex++)
    analogBufferTemp[copyIndex] = analogBuffer[copyIndex];

  averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * (float)VREF / 1024.0;
  float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);
  float compensationVoltage = averageVoltage / compensationCoefficient;
  return (133.42 * compensationVoltage * compensationVoltage * compensationVoltage
          - 255.86 * compensationVoltage * compensationVoltage
          + 857.39 * compensationVoltage) * 0.5;
}

// Hàm hiển thị dữ liệu lên LCD
void displayData(float tdsValue, float temperature) {
  lcd.setCursor(0, 0);
  lcd.print("TDS:");
  lcd.print(tdsValue, 0); // Hiển thị giá trị TDS
  lcd.print(" ppm");
  lcd.setCursor(11, 0);
  if (tdsValue < 300) {
    lcd.print("GOOD");
  } else if (tdsValue < 600) {
    lcd.print("OK  ");
  } else {
    lcd.print("BAD ");
  }

  lcd.setCursor(0, 1);
  lcd.print("Temp:");
  lcd.print(temperature, 1); // Hiển thị nhiệt độ nước
  lcd.print("C");
}

// Hàm lọc trung vị
int getMedianNum(int bArray[], int iFilterLen) {
  int bTab[iFilterLen];
  for (byte i = 0; i < iFilterLen; i++)
    bTab[i] = bArray[i];

  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++) {
    for (i = 0; i < iFilterLen - j - 1; i++) {
      if (bTab[i] > bTab[i + 1]) {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }

  if ((iFilterLen & 1) > 0)
    bTemp = bTab[(iFilterLen - 1) / 2];
  else
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;

  return bTemp;
}
