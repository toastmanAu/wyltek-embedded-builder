/*
 * GT911.cpp — ISR implementation extracted from GT911.h
 *
 * The ISR must be in its own .cpp translation unit so the linker can correctly
 * place its literal pool in IRAM. Defining it inline in a .h causes a
 * "dangerous relocation: l32r: literal placed after use" error with xtensa-esp32.
 */
#include "GT911.h"

volatile bool GT911::_irq = false;

void IRAM_ATTR GT911::_isr() {
    _irq = true;
}
