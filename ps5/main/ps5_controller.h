#pragma once

#include "ps5.h"

typedef void (*callback_t)();

bool ps5_begin(const char *mac);
bool ps5_isConnected();

void ps5_try_pairing();
void ps5_set_led(uint8_t r, uint8_t g, uint8_t b);
void ps5_set_rumble(uint8_t small, uint8_t large);
void ps5_trigger_effect(uint8_t effect, uint8_t left, uint8_t right);

void ps5_setFlashRate(uint8_t onTime, uint8_t offTime);
void ps5_attach(callback_t callback);
void ps5_attachOnConnect(callback_t callback);
void ps5_attachOnDisconnect(callback_t callback);
uint8_t *ps5_LatestPacket();
void ps5_disable_lightbar();
ps5_t *ps5_get_data();
ps5_event_t *ps5_get_event();

bool ps5_Right();
bool ps5_Down();
bool ps5_Up();
bool ps5_Left();

bool ps5_Square();
bool ps5_Cross();
bool ps5_Circle();
bool ps5_Triangle();

bool ps5_UpRight();
bool ps5_DownRight();
bool ps5_UpLeft();
bool ps5_DownLeft();

bool ps5_L1();
bool ps5_R1();
bool ps5_L2();
bool ps5_R2();

bool ps5_Share();
bool ps5_Options();
bool ps5_L3();
bool ps5_R3();

bool ps5_PSButton();
bool ps5_Touchpad();

uint8_t ps5_L2Value();
uint8_t ps5_R2Value();

int8_t ps5_LStickX();
int8_t ps5_LStickY();
int8_t ps5_RStickX();
int8_t ps5_RStickY();

uint8_t ps5_Battery();
bool ps5_Charging();
bool ps5_Audio();
bool ps5_Mic();
