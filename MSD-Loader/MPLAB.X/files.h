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

#include "system.h"
#include "direct.h"

#ifndef FILES_H
#define	FILES_H

#define ROOT_ENTRY_SIZE             32  // standard root entry size
#define ENTRY_FILE_SIZE_OFFSET      28  // offset to entry.file_size field
#define ENTRY_CLUSTER               26  // offset of entry.cluster

#define DATEH(y, m, d)    (((y-1980) << 1) + (m >> 3))  // y:1980..2099, m:1..12
#define DATEL(y, m, d)    ((m << 5) + d)                // d: 1..31
#define TIMEH(h, m, s)    ((h << 3) + (m >> 3))         // h:0..23, m:0..59
#define TIMEL(h, m, s)    ((m << 5) + s)           // s = seconds/2 (0-29)

extern const char readme[];

/**
 */
uint8_t readme_size(void);

/**
 */
void MasterBootRecordGet(uint8_t* buffer, uint8_t seg);

/**
 *
 * @param buffer
 */
void VolumeBootRecordGet(uint8_t*buffer, uint8_t seg);

/**
 *
 * @param buffer
 */
void FATRecordGet(uint8_t* buffer, uint8_t seg);

/**
 *
 * @param buffer
 */
void FATRecordSet(uint8_t* buffer, uint8_t seg);

/**
 *
 * @param buffer
 */
void RootRecordGet(uint8_t* buffer, uint8_t seg);

/**
 *
 * @param buffer
 */
void RootRecordSet(uint8_t* buffer, uint8_t seg);

/**
 * Initializes the ROOT directory in RAM
 */
void RootRecordInit(void);

/**
 Initializes the FAT table in RAM
 */
void FATRecordInit(void);

#endif	/* FILES_H */

