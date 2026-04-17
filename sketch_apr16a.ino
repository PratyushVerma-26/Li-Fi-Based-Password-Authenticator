/* =========================================================
   LiFi Auth  —  TRANSMITTER  v4.0  (Direct-Wire Edition)
   RGB LED on D9 (R)  D10 (G)  D11 (B)
   Push button on D2
   D9 / D10 / D11 also wired directly to RX Arduino
   ========================================================= */

#define PIN_R      9
#define PIN_G     10
#define PIN_B     11
#define BTN_PIN    2

#define SYMBOL_MS  300
#define GAP_MS      80
#define START_FLASH_ON  150
#define START_FLASH_OFF 100
#define START_FLASHES     3
#define END_ON          600

const char PASSWORD[] = "LIFI";

const int CR[] = {255,   0,   0, 255,   0, 255, 255, 148};
const int CG[] = {  0, 255,   0, 255, 255,   0, 255,   0};
const int CB[] = {  0,   0, 255,   0, 255, 255, 255, 211};
const char* CNAME[] = {
  "RED","GREEN","BLUE","YELLOW",
  "CYAN","MAGENTA","WHITE","VIOLET"
};

int charIdx(char c) { return (int)c % 8; }

void setRGB(int r, int g, int b) {
  analogWrite(PIN_R, r);
  analogWrite(PIN_G, g);
  analogWrite(PIN_B, b);
}

void allOff() { setRGB(0, 0, 0); }

// ── START marker: 3 CYAN flashes (G+B only, no red channel) ─
void sendStart() {
  for (int i = 0; i < START_FLASHES; i++) {
    setRGB(0, 255, 255);    // CYAN — does not use D9/A0
    delay(START_FLASH_ON);
    allOff();
    delay(START_FLASH_OFF);
  }
  delay(300);
}

void sendEnd() {
  setRGB(0, 255, 255);      // CYAN end marker too
  delay(END_ON);
  allOff();
}

void transmit() {
  Serial.println("[TX] Transmitting password...");
  sendStart();

  for (int i = 0; i < (int)strlen(PASSWORD); i++) {
    int idx = charIdx(PASSWORD[i]);
    Serial.print("[TX] Symbol ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.println(CNAME[idx]);

    setRGB(CR[idx], CG[idx], CB[idx]);
    delay(SYMBOL_MS);
    allOff();
    delay(GAP_MS);
  }

  sendEnd();
  Serial.println("[TX] Done. Waiting for next button press.");
}

bool lastBtn = HIGH;

void setup() {
  pinMode(PIN_R,   OUTPUT);
  pinMode(PIN_G,   OUTPUT);
  pinMode(PIN_B,   OUTPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);
  allOff();
  Serial.begin(9600);
  Serial.println("[TX] LiFi Transmitter v4.0 Ready.");
  Serial.println("[TX] Press button to transmit password.");
}

void loop() {
  bool btn = digitalRead(BTN_PIN);
  if (lastBtn == HIGH && btn == LOW) {
    delay(20);
    transmit();
  }
  lastBtn = btn;
}