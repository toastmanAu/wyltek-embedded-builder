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
#include "display/WyDisplay.h"
#include "touch/WyTouch.h"
#include "settings/WySettings.h"
#include "net/WyNet.h"
