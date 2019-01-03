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

#ifndef _FS_DEF_
#define _FS_DEF_

#include <stdint.h>

// Macro indicating how many drives can be mounted simultaneously.
#define FILEIO_CONFIG_MAX_DRIVES        1

// Delimiter for directories.
#define FILEIO_CONFIG_DELIMITER '/'

// Macro defining the maximum supported sector size for the FILEIO module.  This value should always be 512 , 1024, 2048, or 4096 bytes.
// Most media uses 512-byte sector sizes.
#define FILEIO_CONFIG_MEDIA_SECTOR_SIZE 		512

/* *******************************************************************************************************/
/************** Compiler options to enable/Disable Features based on user's application ******************/
/* *******************************************************************************************************/

// Uncomment FILEIO_CONFIG_FUNCTION_SEARCH to disable the functions used to search for files.
#define FILEIO_CONFIG_SEARCH_DISABLE

// Uncomment FILEIO_CONFIG_FUNCTION_FORMAT to disable the function used to format drives.
#define FILEIO_CONFIG_FORMAT_DISABLE

// Uncomment FILEIO_CONFIG_FUNCTION_DIRECTORY to disable use of directories on your drive.  Disabling this feature will
// limit you to performing all file operations in the root directory.
//#define FILEIO_CONFIG_DIRECTORY_DISABLE

// Uncomment FILEIO_CONFIG_FUNCTION_PROGRAM_MEMORY_STRINGS to disable functions that accept ROM string arguments.
// This is only necessary on PIC18 parts.
#define FILEIO_CONFIG_PROGRAM_MEMORY_STRINGS_DISABLE

//---------------------------------------------------------------------------------------
// The size (in number of sectors) of the desired usable data portion of the MSD volume
//---------------------------------------------------------------------------------------
// Note1: Windows 7 appears to require a minimum capacity of at least 13 sectors.
// Note2: Drive (re)formatting is NOT applicable
#define DRV_CONFIG_DRIVE_CAPACITY 4096          // * 512 byte = useable drive volume

//--------------------------------------------------------------------------
// Number of Sectors per Cluster
//--------------------------------------------------------------------------
#define DRV_SECTORS_PER_CLUSTER     8           // * 512 byte = cluster size (4kB)

//--------------------------------------------------------------------------
// Maximum files supported
//--------------------------------------------------------------------------
// DRV_MAX_NUM_FILES_IN_ROOT must be a multiple of 16
// Note: Even if DRV_MAX_NUM_FILES_IN_ROOT is 16, this does not
// necessarily mean the drive will always work with 16 files.  The drive will
// suppport "up to" 16 files, but other limits could be hit first, even before
// the drive is full.  The RootDirectory0[] sector could get full with less
// files, especially if the files are using long filenames.
#define DRV_MAX_NUM_FILES_IN_ROOT 16

#endif
