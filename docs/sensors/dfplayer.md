# DFPlayer Mini — MP3 Player Module
**Driver:** `WyDFPlayer.h`

---

## What it does
Plays MP3/WAV/WMA files from a micro-SD card. Built-in amplifier drives a small speaker directly — no external amp needed. Controlled via 9600 baud UART. The DFPlayer Mini is the go-to module for adding audio feedback, sound effects, or music to any ESP32 project.

- Files from micro-SD (FAT32, up to 32GB)
- Built-in amplifier: 3W into 4Ω
- Volume control: 30 levels
- Hardware EQ: Normal, Pop, Rock, Jazz, Classic, Bass
- Loop, shuffle, folder, advertise modes
- BUSY pin goes LOW while playing

## ⚠️ SD card file naming — the biggest gotcha

Files must be named with exactly 4 leading zeros: `0001.mp3`, `0002.mp3`, etc.

**The file order is the FAT32 write order, not alphabetical.** If you drag-and-drop files onto the SD card in Windows Explorer, the order may not match what you expect. For guaranteed order:

1. Format the SD card (FAT32, default allocation size)
2. Copy files **one at a time** in the order you want them
3. Don't rename files after copying — the FAT entry order is locked

**Folder mode** — more reliable for organised playback:
```
/01/001.mp3    → playFolder(1, 1)
/01/002.mp3    → playFolder(1, 2)
/02/001.mp3    → playFolder(2, 1)
```

**Special folders:**
```
/MP3/0001.mp3  → playMP3(1)    — up to 9999 files
/ADVERT/0001.mp3 → playAdvertise(1)  — interrupts playback, then resumes
```

## Wiring
```
Module VCC  → 5V (recommended — 3.3V works but max volume is lower)
Module GND  → GND
Module TX   → 1kΩ → ESP32 RX pin
Module RX   → ESP32 TX pin (direct, no resistor)
Module SPK_1 → Speaker positive
Module SPK_2 → Speaker negative
Module BUSY  → ESP32 GPIO (optional — LOW while playing)
```

### ⚠️ The 1kΩ resistor on TX is not optional
The DFPlayer TX line can hold the ESP32 UART RX low during boot, preventing firmware upload. A 1kΩ series resistor between DFPlayer TX and ESP32 RX fixes this. Takes 5 seconds to add, saves hours of debugging.

### ⚠️ Headphones go on L_OUT/R_OUT, not SPK
SPK_1 and SPK_2 are the amplifier output — they'll distort and possibly damage headphones. Use the L_OUT and R_OUT pins with a 1kΩ series resistor for line-level or headphone output.

### ⚠️ Decouple the power rail
The built-in amplifier causes voltage spikes. Add **100µF + 100nF** across module VCC/GND. Without this, I2C sensors on the same 5V or 3.3V rail may glitch during loud playback.

## Basic usage
```cpp
#include "sensors/drivers/WyDFPlayer.h"

WyDFPlayer mp3;

void setup() {
    mp3.setBusyPin(GPIO_NUM_27);   // optional — LOW while playing
    mp3.begin(GPIO_NUM_17, GPIO_NUM_16);  // TX, RX
    mp3.setVolume(20);             // 0–30
    delay(500);
    mp3.play(1);                   // play 0001.mp3
}
```

Or via WySensors registry:
```cpp
auto* mp3 = sensors.addUART<WyDFPlayer>("mp3", TX_PIN, RX_PIN, 9600);
sensors.begin();
sensors.get<WyDFPlayer>("mp3")->play(1);
```

## API

### Playback
```cpp
mp3.play(1);              // global track 1 (0001.mp3)
mp3.playFolder(1, 3);     // /01/003.mp3
mp3.playMP3(5);           // /MP3/0005.mp3
mp3.playAdvertise(2);     // /ADVERT/0002.mp3, then resume
mp3.pause();
mp3.resume();
mp3.stop();
mp3.next();               // next track
mp3.prev();
```

### Volume & EQ
```cpp
mp3.setVolume(20);        // 0–30 (default 20)
mp3.volumeUp();
mp3.volumeDown();
mp3.setEQ(DFP_EQ_BASS);  // NORMAL/POP/ROCK/JAZZ/CLASSIC/BASS
```

### Loop modes
```cpp
mp3.loopAll();            // loop all tracks continuously
mp3.loopCurrent();        // loop current track
mp3.loopFolder(1);        // loop all files in folder 01
mp3.shuffle();            // random shuffle all
```

### Status
```cpp
bool playing = mp3.isPlaying();     // LOW on BUSY pin, or UART query
uint16_t track = mp3.currentTrack();
uint16_t total = mp3.totalFiles();
uint8_t  vol   = mp3.currentVolume();

// Block until track finishes (up to 60 seconds timeout):
mp3.play(1);
mp3.waitDone(30000);   // wait up to 30s
Serial.println("Done playing");
```

## BUSY pin — recommended
Wire the BUSY pin to a GPIO and call `setBusyPin()`. This lets `isPlaying()` work without UART round-trips — much faster and works on clone modules that don't respond to queries.

```cpp
mp3.setBusyPin(27);
while (mp3.isPlaying()) delay(100);  // efficient play-complete wait
```

## Clone compatibility
There are several different chips used in DFPlayer Mini clones:
- **YX5200** — original DFRobot, full protocol support
- **MH2024K-24SS** — very common clone, **doesn't respond to feedback queries**
- **GD3200B** — another clone, mostly compatible

If your module works (plays audio) but queries return nothing, call `setFeedback(false)`. BUSY pin detection still works with feedback disabled.

```cpp
mp3.setFeedback(false);   // for MH2024K-24SS clones
```

## Advertise mode — the killer feature
`/ADVERT/` folder files interrupt current playback, then resume where it left off. Perfect for:
- "Low battery" warning during music playback
- Alert sounds while ambient audio plays
- Real-time announcements over background music

```cpp
mp3.loopAll();           // play music continuously
// When alert needed:
mp3.playAdvertise(1);    // plays /ADVERT/0001.mp3, then resumes music
```

## Sleep mode (battery projects)
```cpp
mp3.dacOff();    // disable DAC — reduces quiescent current
mp3.sleep();     // put module in sleep mode

// Wake:
mp3.dacOn();
mp3.play(1);
```

## The 2-second init delay
The DFPlayer takes ~1.5–2 seconds to boot and initialise after power-on. The driver waits `WY_DFP_INIT_DELAY_MS` (default 2000ms) in `begin()`. Don't shorten this — sending commands before init completes causes the module to ignore them silently.

## Speaker selection
- **4Ω 3W** — correct match for the built-in amplifier (small round or oval speakers)
- **8Ω** — also works, slightly lower max volume
- **Larger speakers** — need external amplifier via L_OUT/R_OUT

Good cheap options: 28mm round 8Ω 0.5W (for beeps/notification), 40mm 4Ω 3W (for music quality).
