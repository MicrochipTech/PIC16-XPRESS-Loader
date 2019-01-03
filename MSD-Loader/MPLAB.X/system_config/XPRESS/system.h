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
#include <stdbool.h>
#include <stdint.h>

#include "bsp.h"
#include "fixed_address_memory.h"   // required for fix PIC16F145X USB RAM addresses
#include "pinout.h"

// semantic versioning
#define MAJOR   1       // change only when incompatible changes are made
#define MINOR   3       // change when adding functionality or fixing bugs

// this release date
#define YEAR            2018
#define MONTH           4       // January=1, February=2 ..
#define DAY             22       // Day:1..31

#define MAIN_RETURN void

/*********************************************************************
 * Function: void SYSTEM_Initialize(void)
 *
 * Overview: Initializes the system.
 *
 ********************************************************************/
void SYSTEM_init(void);
