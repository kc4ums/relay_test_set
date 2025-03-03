void loop() {
  setFrequency(1000);  // Fixed 1kHz
  setPhase(0);        // 0°
  delay(2000);
  setPhase(90);       // 90°
  delay(2000);
  setPhase(180);      // 180°
  delay(2000);
  setPhase(270);      // 270°
  delay(2000);
}
