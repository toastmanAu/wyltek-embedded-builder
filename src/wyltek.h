/*
 * wyltek-embedded-builder — top-level convenience include
 * ========================================================
 * Include individual modules as needed for minimum flash usage,
 * or include this file to pull in everything.
 *
 * Typical: include only what you need
 *   #include <display/WyDisplay.h>
 *   #include <touch/WyTouch.h>
 *   #include <settings/WySettings.h>
 *   #include <net/WyNet.h>
 */

#pragma once

/* Auth & signing */
#include "auth/WyAuth.h"
#include "display/WyDisplay.h"
#include "display/WyKeyDisplay.h"
#include "touch/WyTouch.h"
#include "settings/WySettings.h"
#include "net/WyNet.h"
#include "sensors/WySensors.h"
#include "ui/WyScreenshot.h"
#include "ui/WySerialDisplay.h"
