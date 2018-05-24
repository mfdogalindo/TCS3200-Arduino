#include "TCS3200.h"
RGB color = {0, 0, 0};

void setup() {
  Serial.println("TCS3200 Selector de color");
  pinMode(LED_BUILTIN, OUTPUT);
  colorimetro_iniciar();
}

void loop() {
  color = colorimetro();
  if (!color.TO) {
    print_color(color);
    Serial.print("Color detectado:\t");
    Serial.println(selector_color(color);
  }
  else {
    Serial.println("Error de sensor");
  }
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  delay(100);
}
