/*******************************************************************************
Copyright 2016 Microchip Technology Inc. (www.microchip.com)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

 *******************************************************************************/
#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "pinout.h"

#ifndef LVP_H
#define	LVP_H

void ICSP_slaveReset(void);
void ICSP_slaveRun(void);
void LVP_enter(void);
void LVP_exit(void);
bool LVP_inProgress(void);
void LVP_packRow(uint32_t address, uint8_t *data, uint8_t data_count);
void LVP_programLastRow(void);

#endif	/* LVP_H */

