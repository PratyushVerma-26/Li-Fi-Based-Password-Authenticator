   /*=========================================================
   LiFi Auth  —  RECEIVER v4.5  TIMING FIX
   Root cause: fixed delay() per symbol caused drift.
   Fix: absolute timestamp locks each symbol to 380ms grid.
   TX D9(R)→A0  D10(G)→A1  D11(B)→A2  GND→GND
   LCD I2C: SDA→A4  SCL→A5
   Status LED: R→D5  G→D6  B→D3  (all PWM)
   ========================================================= */
 
#include <LiquidCrystal_I2C.h>
 
// ── Signal input pins ──────────────────────────────────
#define SIG_R      A0
#define SIG_G      A1
#define SIG_B      A2
#define THRESHOLD  300
 
// ── Status RGB LED pins (all PWM-capable) ─────────────
#define LED_R      5
#define LED_G      6
#define LED_B      3
 
// ── TX timing constants (must match TX exactly) ────────
#define SYMBOL_MS       300
#define GAP_MS           80
#define SYMBOL_PERIOD   (SYMBOL_MS + GAP_MS)  // = 380ms
#define START_FLASH_ON  150
#define START_FLASH_OFF 100
#define POST_START_MS   300
// Time from flash#1 RISING EDGE to symbol#1 RISING EDGE:
// 3 × (ON+OFF) + POST_START = 3×250 + 300 = 1050ms
#define TRISE_TO_DATA   1050
#define PASS_LEN           8
 
LiquidCrystal_I2C lcd(0x27, 16, 2);
// Change 0x27 to 0x3F if your I2C scanner shows 0x3F
 
const char  STORED[]  = "LIFI2024";
const char* CNAME[]   = {
  "RED","GREEN","BLUE","YELLOW",
  "CYAN","MAGENTA","WHITE","VIOLET"
};
int storedSeq[PASS_LEN];
 
// ── THE LOOKUP TABLE FIX ───────────────────────────────
// bits = (r<<2)|(g<<1)|b  where r/g/b = 0 or 1
// Maps each 3-bit combination to the correct color index
// matching the TX color lookup table (CR/CG/CB arrays).
const int RGB_TO_IDX[8] = {
  -1,  // 000 → no signal
   2,  // 001 → B only     → BLUE    (index 2)
   1,  // 010 → G only     → GREEN   (index 1)
   4,  // 011 → G+B        → CYAN    (index 4)
   0,  // 100 → R only     → RED     (index 0)
   5,  // 101 → R+B        → MAGENTA (index 5)
   3,  // 110 → R+G        → YELLOW  (index 3)
   6,  // 111 → R+G+B      → WHITE   (index 6)
};
// Note: VIOLET (index 7) is not used in LIFI2024 but
// would appear if a char with (ASCII mod 8)==7 is used.
 
// ── LED control ────────────────────────────────────────
void setLED(int r, int g, int b) {
  analogWrite(LED_R, r);
  analogWrite(LED_G, g);
  analogWrite(LED_B, b);
}
void ledOff()     { setLED(0,   0,   0); }
void ledIdle()    { setLED(0,   0,  35); }  // dim blue
void ledReading() { setLED(0,  35,  35); }  // dim cyan
void ledGrant()   { setLED(0, 255,   0); }  // solid green
 
int charIdx(char c) { return (int)c % 8; }
 
void buildStored() {
  for (int i = 0; i < PASS_LEN; i++)
    storedSeq[i] = charIdx(STORED[i]);
}
 
// ── ADC read with mux settling ─────────────────────────
int readCh(int pin) {
  analogRead(pin);           // dummy read — discharge mux
  delayMicroseconds(300);
  return analogRead(pin);
}
 
bool anyHigh() {
  return readCh(SIG_R) > THRESHOLD ||
         readCh(SIG_G) > THRESHOLD ||
         readCh(SIG_B) > THRESHOLD;
}
 
bool isCyan() {
  return readCh(SIG_R) < THRESHOLD &&
         readCh(SIG_G) > THRESHOLD &&
         readCh(SIG_B) > THRESHOLD;
}
 
// ── Windowed majority-vote read ─────────────────────────
// Samples all three channels repeatedly for windowMs ms.
// Returns the color index using RGB_TO_IDX lookup table.
int readIdxWindowed(int windowMs) {
  int rV = 0, gV = 0, bV = 0, n = 0;
  unsigned long t = millis();
  while (millis() - t < (unsigned long)windowMs) {
    if (readCh(SIG_R) > THRESHOLD) rV++;
    if (readCh(SIG_G) > THRESHOLD) gV++;
    if (readCh(SIG_B) > THRESHOLD) bV++;
    n++;
  }
  if (n == 0) return -1;
  // Majority: >50% of reads must be HIGH for channel = ON
  int bits = ((rV * 2 > n) ? 4 : 0) |
             ((gV * 2 > n) ? 2 : 0) |
             ((bV * 2 > n) ? 1 : 0);
  return RGB_TO_IDX[bits];
}
 
// ── START marker detection ─────────────────────────────
// Waits for CYAN (G+B high, R low) = start of flash #1.
// Returns true with millis() timestamped at flash#1 rise.
bool waitStart() {
  unsigned long t = millis();
  while (millis() - t < 10000) {
    if (isCyan()) {
      delay(20);          // confirm not a glitch
      if (isCyan()) {
        Serial.println("[RX] Flash#1 rising edge detected.");
        return true;
      }
    }
    delayMicroseconds(500);
  }
  return false;
}
 
bool ctCompare(int* a, int* b, int len) {
  int diff = 0;
  for (int i = 0; i < len; i++) diff |= (a[i] ^ b[i]);
  return diff == 0;
}
 
void showIdleScreen() {
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("LiFi Auth Ready");
  lcd.setCursor(0, 1); lcd.print("Waiting for TX..");
  ledIdle();
}
 
void grantAccess() {
  Serial.println("[RX] *** ACCESS GRANTED ***");
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(" ACCESS GRANTED ");
  lcd.setCursor(0, 1); lcd.print("  Welcome! :)   ");
  ledGrant();
  delay(3000);
  ledOff();
}
 
void denyAccess() {
  Serial.println("[RX] *** ACCESS DENIED ***");
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(" ACCESS DENIED  ");
  lcd.setCursor(0, 1); lcd.print(" Invalid key!   ");
  for (int i = 0; i < 6; i++) {
    setLED(255, 0, 0); delay(170);
    ledOff();          delay(130);
  }
  delay(1000);
}
 
void showProgress(int s, int idx) {
  lcd.setCursor(0, 0); lcd.print("Reading signal..");
  // Guard: idx can be -1 if no signal during window
  const char* name = (idx >= 0 && idx <= 7) ? CNAME[idx] : "???";
  String line = "Sym " + String(s + 1) + ": " + String(name);
  while (line.length() < 16) line += " ";
  lcd.setCursor(0, 1); lcd.print(line);
}
 
void setup() {
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  ledOff();
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0); lcd.print("LiFi Auth");
  lcd.setCursor(0, 1); lcd.print("Waiting for Tx");
  setLED(0, 0, 80);
  delay(1500);
  buildStored();
  showIdleScreen();
  Serial.println("[RX] Li-Fi Ready");
}
 
void loop() {
  showIdleScreen();
 
  // ── Phase 1: detect start marker ──────────────────────
  if (!waitStart()) {
    Serial.println("[RX] Timeout. Retrying.");
    return;
  }
 
  // ── Phase 2: deterministic sync ───────────────────────
  // waitStart() returns at flash#1 RISING EDGE.
  // From there, symbol#1 rising edge is EXACTLY 1050ms later.
  // Subtract the 20ms glitch-confirm delay inside waitStart().
  delay(TRISE_TO_DATA - 20);
 
  ledReading();
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Reading signal..");
  Serial.println("[RX] Synced. Reading symbols...");
 
  int  received[PASS_LEN];
  bool readOk = true;
 
  for (int i = 0; i < PASS_LEN; i++) {
 
    // ── Wait for rising edge (up to 80ms slip) ──────────
    unsigned long tEdge = millis();
    while (!anyHigh() && millis() - tEdge < 80);
 
    if (!anyHigh()) {
      Serial.print("[RX] Timeout waiting for symbol ");
      Serial.println(i + 1);
      readOk = false;
      break;
    }
 
    // ── ABSOLUTE TIMESTAMP ───────────────────────────────
    // Record when THIS symbol's rising edge was detected.
    // All subsequent delays for this symbol are calculated
    // relative to this timestamp — NOT accumulated from
    // previous symbols. This is what prevents drift.
    unsigned long symbolStart = millis();
 
    // ── Settle 50ms from rising edge, sample 160ms ──────
    delay(50);
    received[i] = readIdxWindowed(160);
 
    // ── Guard against -1 (no signal detected in window) ─
    // If -1, we were in a gap — mark as wrong but continue
    int safeIdx = (received[i] >= 0) ? received[i] : 0;
    received[i] = safeIdx;
 
    showProgress(i, received[i]);
    Serial.print("[RX] Symbol "); Serial.print(i + 1);
    Serial.print(": "); Serial.println(CNAME[received[i]]);
 
    // ── ABSOLUTE WAIT — THE KEY FIX ─────────────────────
    // Calculate exactly how many ms remain until the next
    // symbol's rising edge, based on symbolStart.
    // No matter how long readIdxWindowed() took, this always
    // arrives at the same point in the TX timing grid.
    long elapsed  = (long)(millis() - symbolStart);
    long toWait   = (long)SYMBOL_PERIOD - elapsed;
    if (toWait > 0) delay((unsigned long)toWait);
    // If toWait <= 0, processing was slow — move on immediately.
    // The 80ms slip window in the next iteration catches this.
  }
 
  if (readOk && ctCompare(received, storedSeq, PASS_LEN))
    grantAccess();
  else
    denyAccess();
 
  delay(500);
  showIdleScreen();
}
