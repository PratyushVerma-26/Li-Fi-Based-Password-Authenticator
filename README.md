### LiFi-Based Password Authentication System — Direct-Wire Arduino Implementation
 
[![Arduino](https://img.shields.io/badge/Platform-Arduino_Nano_(ATmega328P)-00979D?logo=arduino)](https://www.arduino.cc/)
[![Status](https://img.shields.io/badge/Status-Working_Hardware_Verified-brightgreen)]()
[![Authors](https://img.shields.io/badge/Authors-Pratyush_Verma-orange)]()
 
---
 
## 📋 Table of Contents
 
- [Overview](#-overview)
- [How It Works](#-how-it-works)
- [Password Encoding](#-password-encoding)
- [Transmission Protocol](#-transmission-protocol)
- [Repository Structure](#-repository-structure)
- [Hardware Required](#-hardware-required)
- [Wiring Guide](#-wiring-guide)
- [Software Setup](#-software-setup)
- [Upload Instructions](#-upload-instructions)
- [Serial Monitor Output](#-serial-monitor-output)
- [Troubleshooting](#-troubleshooting)
- [Version History](#-version-history)
- [Authors](#-authors)
---
 
## 🔍 Overview
 
This project implements a **LiFi (Light Fidelity) inspired password authentication system** using two Arduino Nano microcontrollers. An 8-character password is encoded as a time-sequenced series of **RGB color symbols** and transmitted via PWM output pins. The receiver decodes the signal through direct analog input readings and grants or denies access.
 
### Why LiFi?
 
| RF Systems (RFID / BLE) | This System (LiFi-based) |
|---|---|
| ❌ Signals pass through walls | ✅ Light / electrical signal is directional |
| ❌ Interceptable from meters away | ✅ Requires physical proximity |
| ❌ Vulnerable to replay attacks | ✅ Timed sequence is harder to replay |
| ❌ Clonable credentials | ✅ Dynamic color sequence |
 
### Direct-Wire Architecture
 
> The optical channel (LED → sensor) has been replaced with a **direct electrical wire connection** (TX PWM pin → RX analog pin) for hardware reliability. The full LiFi protocol is preserved at the software layer — the RGB LED still visually emits the color sequence.
 
```
TX Arduino D9  ──────────────────►  RX Arduino A0  (Red channel)
TX Arduino D10 ──────────────────►  RX Arduino A1  (Green channel)
TX Arduino D11 ──────────────────►  RX Arduino A2  (Blue channel)
TX Arduino GND ──────────────────►  RX Arduino GND (Shared reference)
```
 
---
 
## ⚙️ How It Works
 
```
┌─────────────────────────────────────────────────────────────────────────┐
│  TRANSMITTER                          RECEIVER                          │
│                                                                         │
│  [Button Press]                       [Idle — Blue LED, "Auth Ready"]  │
│       │                                           │                     │
│  [Encode Password]                    [Poll A0/A1/A2 for CYAN start]   │
│  ASCII(char) mod 8                               │                     │
│       │                               [Lock timing to flash #1 edge]  │
│  [Send 3× CYAN Start Flashes]                    │                     │
│  150ms ON / 100ms OFF / 300ms gap     [Delay 1030ms → Symbol 1 edge]  │
│       │                                           │                     │
│  [Send 8 Color Symbols]               [Read 8 symbols — 200ms window]  │
│  300ms ON / 80ms OFF each             [Windowed majority-vote ADC]     │
│       │                                           │                     │
│  [Send CYAN End Marker]               [ctCompare() — constant-time XOR]│
│                                                   │                     │
│                                       [LCD + Status LED output]         │
└─────────────────────────────────────────────────────────────────────────┘
```
 
---
 
## 🎨 Password Encoding
 
Each character of the 8-character password maps to a color using:
 
```
color_index = ASCII(character) mod 8
```
 
| Index | Color    | R (D9) | G (D10) | B (D11) | PWM Values |
|-------|----------|--------|---------|---------|------------|
| 0     | RED      | HIGH   | LOW     | LOW     | 255, 0, 0  |
| 1     | GREEN    | LOW    | HIGH    | LOW     | 0, 255, 0  |
| 2     | BLUE     | LOW    | LOW     | HIGH    | 0, 0, 255  |
| 3     | YELLOW   | HIGH   | HIGH    | LOW     | 255, 255, 0|
| 4     | CYAN     | LOW    | HIGH    | HIGH    | 0, 255, 255|
| 5     | MAGENTA  | HIGH   | LOW     | HIGH    | 255, 0, 255|
| 6     | WHITE    | HIGH   | HIGH    | HIGH    | 255, 255, 255|
| 7     | VIOLET   | HIGH   | LOW     | HIGH    | 148, 0, 211|
 
**Example — Password `LIFI2024`:**
 
```
L → ASCII 76  → 76 mod 8 = 4 → CYAN
I → ASCII 73  → 73 mod 8 = 1 → GREEN
F → ASCII 70  → 70 mod 8 = 6 → WHITE
I → ASCII 73  → 73 mod 8 = 1 → GREEN
2 → ASCII 50  → 50 mod 8 = 2 → BLUE
0 → ASCII 48  → 48 mod 8 = 0 → RED
2 → ASCII 50  → 50 mod 8 = 2 → BLUE
4 → ASCII 52  → 52 mod 8 = 4 → CYAN
 
Sequence: CYAN → GREEN → WHITE → GREEN → BLUE → RED → BLUE → CYAN
Indices:  4       1       6       1       2      0     2      4
```
 
---
 
## 📡 Transmission Protocol
 
Every authentication attempt follows this exact frame:
 
```
[START MARKER]  3 × CYAN flashes  (150ms ON / 100ms OFF each)
[SYNC GAP]      300ms all LOW
[SYMBOL 1]      Color ON 300ms → all OFF 80ms
[SYMBOL 2]      Color ON 300ms → all OFF 80ms
...
[SYMBOL 8]      Color ON 300ms → all OFF 80ms
[END MARKER]    CYAN ON 600ms
```
 
> **Why CYAN for start marker?** CYAN uses only D10 and D11 (G+B channels). It leaves D9 (R) LOW, making the start marker detectable even if the D9→A0 wire has a contact issue. This is a fault-tolerant synchronization design.
 
**Receiver timing model:**  
The receiver timestamps the rising edge of Start Flash #1 and delays exactly **1030 ms** (3 × 250 ms flash cycles + 300 ms gap − 20 ms confirmation delay) to land precisely at Symbol 1's rising edge.
 
---
 
## 📁 Repository Structure
 
```
LiFi-Auth/
│
├── tx/
│   └── LiFi_TX_v4.ino          ← Transmitter firmware (upload to TX Arduino)
│
├── rx/
│   └── LiFi_RX_v4.ino          ← Receiver firmware (upload to RX Arduino)
│
├── README.md                    ← This file
└── LICENSE
```
 
---
 
## 🔧 Hardware Required
 
### Transmitter (TX)
 
| Component | Qty | Notes |
|---|---|---|
| Arduino Nano (ATmega328P) | 1 | TX brain |
| RGB LED — Common Cathode | 1 | Flashes the color password |
| 220 Ω resistor | 3 | One per R, G, B channel |
| Tactile push button 4×4 mm | 1 | Triggers transmission |
| Half-size breadboard | 1 | |
| 9V battery + clip or USB | 1 | Power source |
| Jumper wires Male-Male | 15 | Including 3 signal + 1 GND to RX |
 
### Receiver (RX)
 
| Component | Qty | Notes |
|---|---|---|
| Arduino Nano (ATmega328P) | 1 | RX brain |
| 16×2 LCD with I2C backpack (PCF8574) | 1 | Shows GRANTED / DENIED |
| RGB LED — Common Cathode | 1 | Status indicator |
| 220 Ω resistor | 3 | One per R, G, B status LED channel |
| Half-size breadboard | 1 | |
| USB Mini-B cable | 1 | Power + Serial Monitor |
| Jumper wires Male-Male | 20 | Including the 4 inter-Arduino wires |
 
**Total cost estimate: ~₹800–1200 for both units**
 
---
 
## 🔌 Wiring Guide
 
### Transmitter Wiring
 
```
TX Arduino D9  ──► 220Ω ──► RGB LED R leg  (also wire to RX A0)
TX Arduino D10 ──► 220Ω ──► RGB LED G leg  (also wire to RX A1)
TX Arduino D11 ──► 220Ω ──► RGB LED B leg  (also wire to RX A2)
RGB LED cathode (LONGEST leg) ──► TX Arduino GND   ← NO resistor here
TX Arduino D2  ──► Push button leg 1
Push button leg 2 ──► TX Arduino GND
```
 
> ⚠️ **Common LED mistake:** The LONGEST leg is the CATHODE (GND). Only R, G, B legs get resistors.
 
### Inter-Arduino Wires (Most Important)
 
```
TX Arduino D9  ──────────────────────────►  RX Arduino A0   (Red)
TX Arduino D10 ──────────────────────────►  RX Arduino A1   (Green)
TX Arduino D11 ──────────────────────────►  RX Arduino A2   (Blue)
TX Arduino GND ──────────────────────────►  RX Arduino GND  (SHARED GROUND ← critical!)
```
 
> ⚠️ **The shared GND wire is NOT optional.** Without it, analogRead() returns random values and authentication always fails.
 
### Receiver Wiring
 
```
LCD VCC ──► RX 5V
LCD GND ──► RX GND
LCD SDA ──► RX A4
LCD SCL ──► RX A5
 
Status LED R leg ──► 220Ω ──► RX D5   ✓ PWM
Status LED G leg ──► 220Ω ──► RX D6   ✓ PWM
Status LED B leg ──► 220Ω ──► RX D3   ✓ PWM
Status LED cathode (LONGEST) ──► RX GND
```
 
> ⚠️ **Do NOT use D7 or D8 for the status LED.** Arduino Nano PWM pins are: **D3, D5, D6, D9, D10, D11** (marked with `~` on the board). D7 and D8 are not PWM — `analogWrite()` on them does nothing silently.
 
### Status LED Color Meanings
 
| LED State | Color | Meaning |
|---|---|---|
| Idle | 🔵 Dim BLUE | System ready, waiting for TX |
| Start detected | 🩵 Dim CYAN | Reading password symbols |
| Each symbol | ⬜ Brief WHITE | One symbol received |
| Granted | 🟢 Solid GREEN | Correct password — 3 seconds |
| Denied | 🔴 Flashing RED | Wrong password — 6 flashes |
 
---
 
## 💻 Software Setup
 
### 1. Install Arduino IDE
Download from [arduino.cc/en/software](https://www.arduino.cc/en/software) (v2.x recommended)
 
### 2. Install the Only Required Library
 
Open Arduino IDE → **Sketch → Include Library → Manage Libraries**
 
Search: `LiquidCrystal I2C`  
Install: **LiquidCrystal_I2C** by Frank de Brabander
 
> That's it. No other libraries needed. SoftwareSerial is not used. No sensor libraries.
 
### 3. Find Your LCD I2C Address (first time only)
 
Upload this quick scanner to the **RX Arduino**:
 
```cpp
#include <Wire.h>
void setup() {
  Wire.begin();
  Serial.begin(9600);
  Serial.println("Scanning I2C...");
  for (byte a = 8; a < 120; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found: 0x");
      Serial.println(a, HEX);
    }
  }
}
void loop() {}
```
 
- If you see `0x27` → keep the code as-is
- If you see `0x3F` → change `0x27` to `0x3F` in `LiFi_RX_v4.ino` line:  
  `LiquidCrystal_I2C lcd(0x27, 16, 2);`
---
 
## 📤 Upload Instructions
 
### Upload TX Firmware
 
1. Connect **TX Arduino** to laptop via USB
2. Arduino IDE → **Tools → Board → Arduino Nano**
3. **Tools → Processor → ATmega328P** (try Old Bootloader if upload fails)
4. **Tools → Port** → select the TX COM port
5. Open `tx/LiFi_TX_v4.ino`
6. Click **Upload** (→)
7. Open Serial Monitor (9600 baud) — you should see:
   ```
   [TX] LiFi Transmitter v4.0 Ready.
   [TX] Press button to transmit password.
   ```
 
### Upload RX Firmware
 
1. Connect **RX Arduino** to laptop via USB
2. Change port to RX COM port
3. Open `rx/LiFi_RX_v4.ino`
4. Click **Upload**
5. Open Serial Monitor (9600 baud) — you should see:
   ```
   [RX] LiFi Receiver v4.5 Ready.
   [RX] Waiting for signal on A0/A1/A2.
   ```
6. LCD should show `LiFi Auth Ready` and status LED should glow **dim blue**
### Changing the Password
 
Both files have a single line to change the password:
 
**In `tx/LiFi_TX_v4.ino`:**
```cpp
const char PASSWORD[] = "LIFI2024";  // ← change this
```
 
**In `rx/LiFi_RX_v4.ino`:**
```cpp
const char STORED[]  = "LIFI2024";  // ← change this to match TX
```
 
> ⚠️ Both must be **identical, exactly 8 characters**.
 
---
 
## 📊 Serial Monitor Output
 
### Expected TX Output (after button press)
```
[TX] Transmitting password...
[TX] Symbol 1: CYAN
[TX] Symbol 2: GREEN
[TX] Symbol 3: WHITE
[TX] Symbol 4: GREEN
[TX] Symbol 5: BLUE
[TX] Symbol 6: RED
[TX] Symbol 7: BLUE
[TX] Symbol 8: CYAN
[TX] Done. Waiting for next button press.
```
 
### Expected RX Output (correct password)
```
[RX] LiFi Receiver v4.5 Ready.
[RX] Waiting for signal on A0/A1/A2.
[RX] Flash#1 rising edge detected.
[RX] Synced. Reading symbols...
[RX] Symbol 1: CYAN
[RX] Symbol 2: GREEN
[RX] Symbol 3: WHITE
[RX] Symbol 4: GREEN
[RX] Symbol 5: BLUE
[RX] Symbol 6: RED
[RX] Symbol 7: BLUE
[RX] Symbol 8: CYAN
[RX] *** ACCESS GRANTED ***
```
 
### Expected RX Output (wrong password)
```
[RX] Symbol 1: ...
...
[RX] Symbol 8: ...
[RX] *** ACCESS DENIED ***
```
 
---
 
## 🔧 Troubleshooting
 
| Problem | Likely Cause | Fix |
|---|---|---|
| LCD shows nothing / blank | Wrong I2C address | Run I2C scanner; change `0x27` to `0x3F` |
| LCD stays on "Waiting for TX.." | TX button not pressed, or start marker not detected | Check D10→A1 and D11→A2 wires; check GND wire |
| Status LED doesn't light | Wrong pins or common anode LED | Use D3/D5/D6 only; verify common **cathode** |
| Status LED wrong color (red when should be blue) | R/G/B legs swapped | R leg→D5, G leg→D6, B leg→D3 |
| Always ACCESS DENIED with correct password | GND wire missing, or wire swapped | Verify TX GND → RX GND; check D9→A0 not D9→A1 |
| RX sees "Timeout — no start signal" | Start marker not detected | Run ADC diagnostic below; check D10→A1, D11→A2 |
| Symbol 4 (or any symbol) reads blank/wrong | Timing drift | Ensure absolute-timestamp loop is in firmware (v4.5) |
| Upload fails | Wrong bootloader | Try Tools → Processor → ATmega328P **(Old Bootloader)** |
 
### ADC Diagnostic
 
Upload this to RX to check raw pin values:
 
```cpp
void setup() { Serial.begin(9600); }
void loop() {
  Serial.print(analogRead(A0)); Serial.print("\t");
  Serial.print(analogRead(A1)); Serial.print("\t");
  Serial.println(analogRead(A2));
  delay(100);
}
```
 
**Expected readings:**
 
| State | A0 | A1 | A2 |
|---|---|---|---|
| Idle (no TX signal) | 0–5 | 0–5 | 0–5 |
| TX sending CYAN | 0–5 | 970–1023 | 970–1023 |
| TX sending RED | 970–1023 | 0–5 | 0–5 |
| TX sending WHITE | 970–1023 | 970–1023 | 970–1023 |
 
If active channels read below 500, check the GND wire between the two Arduinos.
 
---
 
## 📚 Version History
 
| Version | Key Change | Issue Fixed |
|---|---|---|
| v1.0 | GY-33 I2C + Adafruit TCS34725 library | — Initial build |
| v2.0 | GY-33 UART SoftwareSerial on D2/D3 | I2C library incompatible with UART-mode sensor |
| v3.0 | Single RGB status LED replaces separate red/green LEDs | Simpler output; buzzer removed |
| v4.0 | Direct wire TX D9/D10/D11 → RX A0/A1/A2; LCD bug fix | Sensor contact failure; LCD stuck on "Waiting" |
| v4.1 | Mid-symbol sampling; majority vote; CYAN start marker | Edge transient misreads |
| v4.2 | Threshold lowered to 400; ADC settling dummy-read | Marginal threshold causing false LOW |
| v4.3 | Deterministic timing anchor to flash #1 rising edge | Symbols 1–2 missed; cumulative drift |
| v4.4 | Pin mapping verified; threshold 300; 200ms windowed read | Wrong color reads |
| **v4.5** | **RGB_TO_IDX lookup table; absolute-timestamp symbol loop** | **Symbol 4 dropout; bit-weight formula error** |
 
---
 
## 👥 Authors
 
**Pratyush Verma** — 23BCI0261  
School of Computer Science and Engineering  
VIT Vellore
 
 
*"Light cannot pass through walls — your password shouldn't have to either."*
