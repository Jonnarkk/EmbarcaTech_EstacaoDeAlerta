#ifndef SIRENE_H
#define  SIRENE_H

#include "hardware/pwm.h"
#include "hardware/clocks.h"

void sirene(uint freq_grave, uint freq_agudo, uint duration);

#endif