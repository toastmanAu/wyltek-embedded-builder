# HS-QR204 — Thermal Receipt Printer
**Driver:** `WyHSQR204.h`

---

## What it does
Drives a thermal receipt printer using the ESC/POS protocol (the industry standard originating from Epson). Print text, barcodes, QR codes, and receipts. The HS-QR204 is a small 58mm or 80mm thermal printer module commonly used in POS systems, ticket printers, and IoT receipt printers.

## Wiring

| HS-QR204 Pin | ESP32 |
|-------------|-------|
| TX | ESP32 RX pin |
| RX | ESP32 TX pin |
| GND | GND (common ground!) |
| VCC | 5–9V (see note below) |

**Power is critical.** Most thermal printer modules need 5V at 1–2A peak (during printing). Some modules need 7.4V (a single LiPo cell) or 9V. Check your specific module's datasheet. **Do not power from the ESP32's 3.3V or USB 5V — the current spike when printing will brown-out your ESP32.**

Use a dedicated 5V power supply rated at 2A or more, with a common GND between the supply, printer, and ESP32.

## Default settings

| Setting | Value |
|---------|-------|
| Baud rate | 9600 (some modules: 19200 or 115200) |
| Format | 8N1 |
| Protocol | ESC/POS |
| Paper width | 58mm (most modules) |

## Code Example

```cpp
#include <WySensors.h>
#include <sensors/drivers/WyHSQR204.h>

WySensors sensors;
WyHSQR204* printer;

void setup() {
    // TX=17, RX=16
    printer = sensors.addUART<WyHSQR204>("receipt", 17, 16);
    sensors.begin();

    printer->init();
    printer->header("WYLTEK STORE");
    printer->receiptRow("Item 1", "12.50", "$");
    printer->receiptRow("Item 2", "8.00",  "$");
    printer->divider();
    printer->setAlign(WY_PRINTER_RIGHT);
    printer->setBold(true);
    printer->println("TOTAL: $20.50");
    printer->setBold(false);
    printer->feed(3);
    printer->cut();
}

void loop() {}
```

## Print QR code

```cpp
printer->setAlign(WY_PRINTER_CENTER);
printer->printQR("https://example.com/order/12345", 4, WY_QR_EC_MEDIUM);
printer->feed(2);
```

## Print barcode

```cpp
printer->printBarcode("ABC-12345", WY_BARCODE_CODE128, 80, 2);
```

## Text formatting

```cpp
printer->setBold(true);
printer->setUnderline(1);     // 1=thin, 2=thick, 0=off
printer->setAlign(WY_PRINTER_CENTER);  // LEFT, CENTER, RIGHT
printer->setSize(WY_PRINTER_DOUBLE);   // double size
printer->setUpsideDown(true);          // upside-down text
printer->resetFormat();                // back to normal
```

## ESC/POS commands used

| Method | ESC/POS | Effect |
|--------|---------|--------|
| `init()` | `ESC @` | Reset printer |
| `println(str)` | raw bytes + LF | Print + newline |
| `feed(n)` | `ESC d n` | Feed n lines |
| `cut()` | `GS V 0/1` | Full or partial cut |
| `setAlign(n)` | `ESC a n` | Left/Center/Right |
| `setBold(bool)` | `ESC E n` | Bold on/off |
| `setUnderline(n)` | `ESC - n` | Underline |
| `setSize(n)` | `ESC ! n` | Double height/width |
| `printBarcode(...)` | `GS k` | 1D barcode |
| `printQR(...)` | `GS ( k` | QR code |

## Alignment constants

| Constant | Value | Effect |
|---------|-------|--------|
| `WY_PRINTER_LEFT` | 0 | Left justify |
| `WY_PRINTER_CENTER` | 1 | Center |
| `WY_PRINTER_RIGHT` | 2 | Right justify |

## Gotchas

**Uses Serial2.** Like the GM861S, this driver hardcodes `Serial2`. Only one of them can be active at a time unless you modify the driver.

**Power supply is the biggest issue.** Thermal printers need a current surge when the heating element fires. Inadequate power causes garbled characters, incomplete prints, or ESP32 resets. A good 2A supply solves most problems.

**Baud rate may not be 9600.** Some modules ship at 19200 or 115200. Try them if 9600 doesn't work. Some have DIP switches or a configuration mode — check the manual.

**Slow printing with large QR codes.** The `printQR()` method adds a `delay(len/4 + 200)` after the QR data. For long URLs this could be 500ms or more. Don't print QR codes from time-sensitive loops.

**Paper jams and cuts.** `cut(false)` does a full cut. `cut(true)` does a partial cut (connected by a thin strip). Partial cuts are more reliable mechanically — the paper doesn't fall and jam.

**ESC/POS compatibility.** ESC/POS is a de-facto standard but printer implementations vary. Some commands may not work on all modules. If a command does nothing, check your printer's manual for ESC/POS support level.
