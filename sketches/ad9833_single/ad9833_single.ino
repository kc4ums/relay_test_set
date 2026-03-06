#include <SPI.h>

// Pin definitions
const int FSYNC = 10;  // Chip Select (active low)
const int SCLK  = 13;  // SPI Clock
const int SDATA = 11;  // SPI Data (MOSI)

// AD9833 Register Addresses
#define CTRL_REG   0x0000
#define FREQ0_REG  0x4000
#define FREQ1_REG  0x8000
#define PHASE0_REG 0xC000
#define PHASE1_REG 0xE000

// Control Register Bits
#define B28   (1 << 13)  // 28-bit frequency word mode
#define HLB   (1 << 12)  // High/Low byte select
#define FSEL  (1 << 11)  // Frequency register select
#define PSEL  (1 << 10)  // Phase register select
#define RESET (1 << 8)   // Reset

// Waveform output modes
enum Waveform {
  WAVEFORM_SINE     = 0,
  WAVEFORM_TRIANGLE = (1 << 1),
  WAVEFORM_SQUARE   = (1 << 5) | (1 << 3)  // OPBITEN | DIV2
};

// Master clock frequency (25 MHz)
const unsigned long MCLK = 25000000UL;

// Function prototypes
void initAD9833();
void setFrequency(unsigned long frequency);
void setPhase(float phaseDeg);
void setWaveform(Waveform waveform);
void writeRegister(uint16_t data);

void setup() {
  pinMode(FSYNC, OUTPUT);
  digitalWrite(FSYNC, HIGH);

  SPI.begin();
  SPI.setDataMode(SPI_MODE2);

  initAD9833();
  setFrequency(1000);
  setPhase(0);
}

void loop() {
  // Step through 0, 90, 180, 270 degrees at 1 kHz
  const float phases[] = {0, 90, 180, 270};
  for (int i = 0; i < 4; i++) {
    setPhase(phases[i]);
    delay(2000);
  }
}

void initAD9833() {
  writeRegister(CTRL_REG | RESET);  // Reset device
  delay(10);
  writeRegister(CTRL_REG | B28);    // 28-bit frequency mode, sine wave
}

void setFrequency(unsigned long frequency) {
  unsigned long freqReg = (unsigned long)(((double)frequency * (1UL << 28)) / MCLK);

  uint16_t freqLSB = (uint16_t)(freqReg & 0x3FFF)         | FREQ0_REG;
  uint16_t freqMSB = (uint16_t)((freqReg >> 14) & 0x3FFF) | FREQ0_REG;

  writeRegister(CTRL_REG | B28);
  writeRegister(freqLSB);
  writeRegister(freqMSB);
  writeRegister(CTRL_REG | B28);
}

void setPhase(float phaseDeg) {
  uint16_t phaseVal = (uint16_t)((phaseDeg / 360.0f) * 4096) & 0x0FFF;
  writeRegister(PHASE0_REG | phaseVal);
  writeRegister(CTRL_REG | B28);
}

void setWaveform(Waveform waveform) {
  writeRegister(CTRL_REG | B28 | (uint16_t)waveform);
}

void writeRegister(uint16_t data) {
  digitalWrite(FSYNC, LOW);
  SPI.transfer16(data);
  digitalWrite(FSYNC, HIGH);
}
