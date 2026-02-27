/*
 * POSExample — wyltek-embedded-builder
 * ======================================
 * Demonstrates GM861S barcode scanner + HS-QR204 thermal printer
 * working together as a point-of-sale peripheral pair.
 *
 * This is the exact hardware used in the CKB ESP32 POS system:
 *   - Scan a QR/barcode → get product code or payment reference
 *   - Print a receipt with QR code for CKB payment
 *
 * Wiring (adjust pins to your board — these match Elecrow HMI 3.5"):
 *   GM861S:  TX→RX2(16), RX→TX2(17), TRIG→GPIO4
 *   QR204:   TX→RX1(9),  RX→TX1(10)   (uses Serial1)
 *
 * Note: This example uses two UART ports. Adjust _serial in drivers
 * if your board only has one free UART.
 */

#include <Arduino.h>
#include "sensors/WySensors.h"
#include "sensors/drivers/WyGM861S.h"
#include "sensors/drivers/WyHSQR204.h"

/* ── Pin config ────────────────────────────────────────────────── */
#define SCANNER_TX   17   /* ESP32 TX → GM861S RX */
#define SCANNER_RX   16   /* ESP32 RX ← GM861S TX */
#define SCANNER_TRIG  4   /* GM861S TRIG (active LOW) */

#define PRINTER_TX   10   /* ESP32 TX → QR204 RX */
#define PRINTER_RX    9   /* ESP32 RX ← QR204 TX (status only) */

/* ── Registry ──────────────────────────────────────────────────── */
WySensors sensors;
WyGM861S*  scanner = nullptr;
WyHSQR204* printer = nullptr;

/* ── CKB payment address to print on every receipt ────────────── */
const char* CKB_ADDRESS = "ckb1qzda0cr08m85hc8jlnfp3zer7xulejywt49kt2rr0vthywaa50xws";

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("[POS] booting...");

    scanner = sensors.addUART<WyGM861S>("scanner",
        SCANNER_TX, SCANNER_RX, 9600);
    printer = sensors.addUART<WyHSQR204>("printer",
        PRINTER_TX, PRINTER_RX, 9600);

    sensors.begin();
    sensors.list();

    /* Point scanner trig pin (set after begin() to avoid reinit) */
    if (scanner) {
        /* trigger mode: pull TRIG LOW to scan */
        pinMode(SCANNER_TRIG, OUTPUT);
        digitalWrite(SCANNER_TRIG, HIGH);
    }

    /* Print startup receipt */
    if (printer) {
        printer->init();
        printer->header("CKB POS SYSTEM");
        printer->setAlign(WY_PRINTER_CENTER);
        printer->println("Ready to scan");
        printer->feed(2);
    }

    Serial.println("[POS] ready — scan a barcode to print receipt");
}

void loop() {
    /* Non-blocking scan poll */
    if (scanner && scanner->available()) {
        const char* barcode = scanner->lastBarcode();
        Serial.printf("[POS] scanned: %s\n", barcode);

        /* Print receipt for scanned item */
        printReceipt(barcode);

        /* Beep to confirm */
        scanner->beep();
        scanner->clear();
    }
}

void printReceipt(const char* productCode) {
    if (!printer) return;

    printer->init();

    /* Header */
    printer->header("RECEIPT");

    /* Item details */
    printer->receiptRow("Product", productCode);
    printer->receiptRow("Amount", "1.00", " CKB");
    printer->divider();

    /* QR code for CKB payment */
    printer->setAlign(WY_PRINTER_CENTER);
    printer->println("Scan to pay with CKB:");
    printer->printQR(CKB_ADDRESS, 4, WY_QR_EC_MEDIUM);

    /* Address text (smaller) */
    printer->setSize(WY_PRINTER_NORMAL);
    printer->println(CKB_ADDRESS);
    printer->feed(1);

    /* Also print the scanned barcode */
    printer->println("Item barcode:");
    printer->printBarcode(productCode, WY_BARCODE_CODE128, 60, 2);

    /* Footer */
    printer->divider();
    printer->setAlign(WY_PRINTER_CENTER);
    printer->println("Thank you!");
    printer->feed(3);
    printer->cut();
}
