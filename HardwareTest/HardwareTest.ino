// =============================================================================
//  RS-232 loopback test
//
//  1. Bridge pins 2 (RXD) and 3 (TXD) of the male RS-232 connector with a
//     jumper wire. Test the whole chain, including any cables or adapters you
//     will use with the DCX2496, because they can change the result.
//  2. Flash this sketch and open the serial monitor at 115200 baud. You should
//     see "OK" every two seconds.
//
//  "OK" proves the loop works, but the polarity can still be wrong. Set your
//  multimeter to 20 V DC and measure between RS-232 pin 5 (GND) and pin 3
//  (TXD): the reading should swing between roughly -5 V and +5 V. If you see
//  that between pin 5 and pin 2 (RXD) instead, RX and TX are swapped.
//
//  The pins match RX2_PIN / TX2_PIN in DuinoDCX/Config.h.
// =============================================================================

const int txPin = 17;
const int rxPin = 16;

void setup() {
  Serial.begin(115200);
  pinMode(txPin, OUTPUT);
  pinMode(rxPin, INPUT_PULLUP);
}

void loop() {
  digitalWrite(txPin, LOW);
  delay(1000);

  Serial.println(digitalRead(rxPin) ? "Error" : "OK");

  digitalWrite(txPin, HIGH);
  delay(1000);
}
