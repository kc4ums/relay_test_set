#include <SPI.h>

// Pin definitions
const int FSYNC = 10;  // Chip Select pin (active low)
const int SCLK = 13;   // SPI Clock pin
const int SDATA = 11;  // SPI Data pin (MOSI)

// AD9833 Register Addresses
#define CTRL_REG   0x0000
#define FREQ0_REG  0x4000
#define FREQ1_REG  0x8000
#define PHASE0_REG 0xC000
#define PHASE1_REG 0xE000

// Control Register Bits
#define B28    (1 << 13)    // 28-bit frequency words
#define HLB    (1 << 12)    // High/Low byte control
#define FSEL   (1 << 11)    // Frequency register select
#define PSEL   (1 << 10)    // Phase register select
#define RESET  (1 << 8)     // Reset bit
#define SINE   (0 << 1)     // Sine wave output
#define TRI    (1 << 1)     // Triangle wave output
#define SQUARE (2 << 1)     // Square wave output

// Master clock frequency (25 MHz for typical AD9833 crystal)
const unsigned long MCLK = 25000000;

void setup() {
  // Initialize SPI pins
  pinMode(FSYNC, OUTPUT);
  digitalWrite(FSYNC, HIGH);
  
  // Start SPI
  SPI.begin();
  SPI.setDataMode(SPI_MODE2);  // AD9833 uses Mode 2
  
  // Initialize AD9833
  initAD9833();
  
  // Set frequency to 1kHz and phase to 0 degrees as an example
  setFrequency(1000);
  setPhase(0);
}

void loop() {
  // Example: Sweep phase from 0 to 360 degrees
  for(int phase = 0; phase <= 360; phase += 45) {
    setPhase(phase);
    delay(1000);
  }
}

// Initialize AD9833
void initAD9833() {
  writeRegister(CTRL_REG | RESET);    // Reset the device
  delay(10);
  writeRegister(CTRL_REG | B28);      // Set to 28-bit frequency mode, sine wave
}

// Set output frequency in Hz
void setFrequency(unsigned long frequency) {
  // Calculate frequency register value
  unsigned long freqReg = (unsigned long)(((double)frequency * (1UL << 28)) / MCLK);
  
  // Split into two 14-bit words
  uint16_t freqLSB = (uint16_t)(freqReg & 0x3FFF);
  uint16_t freqMSB = (uint16_t)((freqReg >> 14) & 0x3FFF);
  
  // Add register address bits
  freqLSB |= FREQ0_REG;
  freqMSB |= FREQ0_REG;
  
  // Write to AD9833
  writeRegister(CTRL_REG | B28);      // Set control register
  writeRegister(freqLSB);            // Write LSB
  writeRegister(freqMSB);            // Write MSB
  writeRegister(CTRL_REG | B28);      // Take out of reset, start generating
}

// Set phase angle in degrees (0-360)
void setPhase(float phaseDeg) {
  // Convert degrees to 12-bit phase value (0-4095)
  // AD9833 phase register uses 12 bits for 0-2π radians
  uint16_t phaseVal = (uint16_t)((phaseDeg * 4096) / 360) & 0x0FFF;
  
  // Add phase register address
  phaseVal |= PHASE0_REG;
  
  // Write phase value
  writeRegister(phaseVal);
  writeRegister(CTRL_REG | B28);      // Apply settings
}

// Write 16-bit data to AD9833 via SPI
void writeRegister(uint16_t data) {
  digitalWrite(FSYNC, LOW);          // Select chip
  SPI.transfer16(data);             // Send 16-bit data
  digitalWrite(FSYNC, HIGH);         // Deselect chip
}

// Set waveform type
void setWaveform(uint8_t waveform) {
  uint16_t control = CTRL_REG | B28;
  switch(waveform) {
    case 0:  // Sine
      control |= SINE;
      break;
    case 1:  // Triangle
      control |= TRI;
      break;
    case 2:  // Square
      control |= SQUARE;
      break;
  }
  writeRegister(control);
}
