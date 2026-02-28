#pragma once
/**
 * WyKY040.h — KY-040 360° Rotary Encoder driver
 * ================================================
 * Incremental rotary encoder with push button.
 * Polling-based (no interrupts required, but works with them too).
 *
 * Wiring:
 *   CLK (A) → any GPIO (with INPUT_PULLUP)
 *   DT  (B) → any GPIO (with INPUT_PULLUP)
 *   SW       → any GPIO (with INPUT_PULLUP) — active LOW
 *   VCC: 3.3V or 5V
 *   GND: GND
 *
 * Usage:
 *   WyKY040 enc(CLK_PIN, DT_PIN, SW_PIN);
 *   enc.begin();
 *   int delta = enc.read();   // +1 CW, -1 CCW, 0 no change
 *   bool pressed = enc.button();
 */
class WyKY040 {
public:
    WyKY040(uint8_t clk, uint8_t dt, uint8_t sw = 255)
        : _clk(clk), _dt(dt), _sw(sw) {}

    void begin() {
        pinMode(_clk, INPUT_PULLUP);
        pinMode(_dt,  INPUT_PULLUP);
        if (_sw != 255) pinMode(_sw, INPUT_PULLUP);
        _lastClk = digitalRead(_clk);
    }

    // Returns +1 (CW), -1 (CCW), or 0 (no movement)
    int8_t read() {
        uint8_t clkNow = digitalRead(_clk);
        if (clkNow == _lastClk) return 0;
        _lastClk = clkNow;
        if (clkNow == LOW) {
            return (digitalRead(_dt) == HIGH) ? 1 : -1;
        }
        return 0;
    }

    // Returns true if button pressed (active LOW, debounced ~20ms)
    bool button() {
        if (_sw == 255) return false;
        if (digitalRead(_sw) == LOW) {
            uint32_t now = millis();
            if (now - _lastBtn > 20) { _lastBtn = now; return true; }
        }
        return false;
    }

private:
    uint8_t  _clk, _dt, _sw;
    uint8_t  _lastClk = HIGH;
    uint32_t _lastBtn = 0;
};
