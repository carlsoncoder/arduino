#include <SPI.h>

#define CS_SPI_PIN_1 6
#define CS_SPI_PIN_2 7

void setup() {
  pinMode(CS_SPI_PIN_1, OUTPUT);
  pinMode(CS_SPI_PIN_2, OUTPUT);
  
  digitalWrite(CS_SPI_PIN_1, HIGH);
  digitalWrite(CS_SPI_PIN_2, HIGH);
  SPI.begin();
  Serial.begin(9600);
}

void writeToSpiPotentiometer(uint16_t value, int pinValue) {
  SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));
    
  digitalWrite(pinValue, LOW);
  delay(50);

  uint8_t bytes[3];

  // Since we're ONLY ever using ONE of the two pots on the SPI chips (for 25k and 250k ohm digi-pots), we can hard-code the cmd to "0xB0", and make sure we ONLY use Wiper 1 (RDAC1 register)
  // If we did want to use the other (Wiper 2 - RDAC2 register), it should just use "0xB1" for the first byte
  bytes[0] = 0xB0;
  bytes[1] = value >> 8;
  bytes[2] = value & 0xFF;

  for (int i = 0; i < 3; i++) {
    SPI.transfer(bytes[i]);
  }
  
  digitalWrite(pinValue, HIGH);
  SPI.endTransaction();
}

// Standard program loop
void loop() {
  Serial.println("Setting pots to 0 (0%)");
  writeToSpiPotentiometer(0, CS_SPI_PIN_1);
  writeToSpiPotentiometer(0, CS_SPI_PIN_2);
  delay(10000);

  Serial.println("Setting pots to 255 (25%)");
  writeToSpiPotentiometer(255, CS_SPI_PIN_1);
  writeToSpiPotentiometer(255, CS_SPI_PIN_2);
  delay(10000);

  Serial.println("Setting pots to 512 (50%)");
  writeToSpiPotentiometer(512, CS_SPI_PIN_1);
  writeToSpiPotentiometer(512, CS_SPI_PIN_2);
  delay(10000);

  Serial.println("Setting pots to 768 (75%)");
  writeToSpiPotentiometer(768, CS_SPI_PIN_1);
  writeToSpiPotentiometer(768, CS_SPI_PIN_2);
  delay(10000);

  Serial.println("Setting pots to 1023 (100%)");
  writeToSpiPotentiometer(1023, CS_SPI_PIN_1);
  writeToSpiPotentiometer(1023, CS_SPI_PIN_2);
  delay(10000);
}
