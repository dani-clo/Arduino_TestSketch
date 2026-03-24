#include <SPI.h>
#include <Wire.h>

// UNO Q (STM32U5) mapping from board DTS:
// Serial1 TX/RX -> D1/D0 (PB6/PB7)
// SPI (default bus) -> D13/D12/D11 with CS on D10
// A4/A5 -> PC1/PC0 and I2C3 on the same pins
static const uint8_t UART1_TX_PIN = 1;
static const uint8_t SPI_CLK_PIN = 13;
static const uint8_t SPI_MOSI_PIN = 11;
static const uint8_t DIGITAL_TEST_PIN = 4;

static void print_banner() {
  Serial.println("\n=== UNO Q Dynamic PINCTRL test ===");
  Serial.println("Setup:");
  Serial.println("  - CH1: D1 (Serial1 TX -> GPIO transition)");
  Serial.println("  - CH2: D13 (SPI SCK -> GPIO -> SPI re-acquire)");
  Serial.println("  - A4/A5 (PC1/PC0): ADC <-> I2C runtime switch");
  Serial.println("  - Optional: D4 digital stress toggle");
}

static void test_digital_toggle() {
  Serial.println("\n[0] Digital GPIO toggle on D4");

  pinMode(DIGITAL_TEST_PIN, OUTPUT);
  for (int i = 0; i < 8; i++) {
    digitalWrite(DIGITAL_TEST_PIN, (i & 0x1) ? HIGH : LOW);
    delay(40);
  }
}

static void test_uart_gpio() {
  Serial.println("\n[1] UART (Serial1) -> GPIO on D1");
  Serial.println("  Expectation: D1 active while Serial1 TX runs, then GPIO pulse on D1");

  Serial1.begin(115200);
  Serial1.println("UNO Q Serial1 active");
  Serial1.println("Transition to GPIO in 50ms");
  delay(50);
  Serial1.end();

  pinMode(UART1_TX_PIN, OUTPUT);
  digitalWrite(UART1_TX_PIN, HIGH);
  delay(80);
  digitalWrite(UART1_TX_PIN, LOW);
}

static void test_spi_gpio_spi() {
  Serial.println("\n[2] SPI -> GPIO -> SPI on D13 path");
  Serial.println("  Expectation: SPI clocks on D13, then manual GPIO pulse, then SPI clocks again");

  SPI.begin();
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  for (uint16_t i = 0; i < 10; i++) {
    (void)SPI.transfer((uint8_t)(0x30 + (i & 0x0F)));
  }
  SPI.endTransaction();
  SPI.end();

  Serial.println("SPI 1 Done");
  delay(2000);

  pinMode(SPI_MOSI_PIN, OUTPUT);
  digitalWrite(SPI_MOSI_PIN, HIGH);
  delay(100);
  digitalWrite(SPI_MOSI_PIN, LOW);

  delay(100);

  SPI.begin();
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  for (uint16_t i = 0; i < 10; i++) {
    (void)SPI.transfer((uint8_t)(0xC0 + (i & 0x0F)));
  }
  SPI.endTransaction();
  SPI.end();

  delay(2000);

  pinMode(SPI_MOSI_PIN, OUTPUT);
  digitalWrite(SPI_MOSI_PIN, HIGH);
}

static void test_a4a5_analog_i2c() {
  Serial.println("\n[3] A4/A5 (PC1/PC0) ADC -> I2C -> ADC");
  delay(1000);

  int a4_before = analogRead(A4);
  int a5_before = analogRead(A5);
  Serial.print("  Before I2C: A4=");
  Serial.print(a4_before);
  Serial.print(" A5=");
  Serial.println(a5_before);

  Serial.println("\nRemove A4/A5 connections and start I2C scan on I2C3 bus...");
  delay(15000);

  Wire2.begin();
  Wire2.setClock(100000);

  int found = 0;
  for (uint8_t addr = 8; addr < 120; addr++) {
    Wire2.beginTransmission(addr);
    uint8_t err = Wire2.endTransmission();
    if (err == 0) {
      found++;
      Serial.print("  I2C device @0x");
      if (addr < 16) Serial.print('0');
      Serial.println(addr, HEX);
    }
  }
  Serial.println("\nI2C devices found on A4/A5 bus: ");
  Serial.println(found);

  Serial.println("\nRe-attach A4/A5 connections and check ADC levels again...");
  delay(15000);

  int a4_after = analogRead(A4);
  int a5_after = analogRead(A5);
  Serial.print("  After I2C:  A4=");
  Serial.print(a4_after);
  Serial.print(" A5=");
  Serial.println(a5_after);

  bool a4_ok = (a4_after > 900);
  bool a5_ok = (a5_after < 100);
  Serial.println((a4_ok && a5_ok)
                     ? "  PASS: A4/A5 tornati in ADC dopo I2C su PC1/PC0"
                     : "  WARN: livelli ADC inattesi; verificare cablaggio/manual setup");
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {};
  print_banner();
}

void loop() {
  test_digital_toggle();
  delay(500);

  test_uart_gpio();
  delay(500);

  test_spi_gpio_spi();
  delay(500);

  test_a4a5_analog_i2c();
  delay(500);

  Serial.println("\n=== Test cycle complete ===");

  while (1) {};
}
