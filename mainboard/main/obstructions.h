#pragma once
#include "stdbool.h"
// if an object is detected closer than 20cm from the machine, we consider it an obstruction
#define MIN_OBSTRUCTION_DISTANCE 20
#define COLLISION_NONE 0
#define COLLISION_FRONT 1
#define COLLISION_REAR 2

void obstruction_init_detection();
bool obstruction_is_too_close();
void obstruction_enable_sweep(bool enable);
int obstruction_get_colision_status();