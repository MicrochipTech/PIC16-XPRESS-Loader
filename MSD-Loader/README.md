PIC16 XPRESS Loader
===================

USB-MSD Programmer and CDC Serial Port Emulator
-----------------------------------------------

This application is designed to act as a fast on-board programmer for PIC
microcontrollers.

A serial to USB bridge function is simultaneously available.

Additional information about the MPLAB Xpress project can be found at
<https://mplabxpress.microchip.com>

License
-------

This application is licensed under the Apache v2 license (see "LICENSE" file in
root directory of the repository).

To request to license this code under the MLA license
(https://www.microchip.com/mla/license), please contact
*mla\_licensing\@microchip.com*.

Product Support
---------------

-   The input (file) parsing algorithm is compatible with all PIC INTEL "Hex
    files" produced by MPLAB XC compilers.

-   The programming algorithm is currently supporting only the low voltage
    LVP-ICSP protocol and a selected subset of 8 and 16-bit microcontrollers.

-   The default serial interface does not support hardware handshake although
    this feature can be enabled if required.

Folder Structure
----------------

-   *MPLAB.X* - contains the main application source files

-   *framework* - elements of the MLA - USB and File System open source
    libraries (note: the MSD portion has been customised to reduce considerably
    RAM usage)

-   *utilities* - contains the Windows signed drivers for the Virtual COM port
    (OS X and Linux users do not need it)

-   *bsp* - board support package (currently only the XPRESS evaluation board)

 

Implementation Details
----------------------

1.  This project uses a USB MSD class implementation that is derived from the
    MLA 2013-08 rev. where the direct.c module operates on individual 64 byte
    segments of each sector. This is done to reduce the RAM usage and improve
    the transfer efficiency by matching the BULK transfer payload size.

2.  The USB framework implements "polling mode" to reduce the stack usage as the
    PIC16 architecture allows for a maximum call depth of 16.

3.  Serial communication and programming interface can be multiplexed on the
    same pair of I/Os. This can be used to reduce the I/O usage on targets that
    are capable of PPS.

4.  Different pin assignments can be supported by selecting/editing different
    *pinout.h* files.

5.  Multiple target architectures (200K, 250K, 290K, PIC18K) are supported by
    selecting a custom implementation of the *lvp-xx.c* file (make the selection
    in the MPLAB project configuration)

Firmware Upgrades
-----------------

See the "PIC16 MSD Loader DFU" project for a compact (512 words) DFU bootloader
that is used to enable field upgrades of this tool.

The bootloader can be activated by powering the device/board while the RESET
button is pressed.

Use the (open source) *dfu-util* from the command line to upgrade the XPRESS
Loader to a new firmware revision.

Example:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

dfu-util -d 04d8:0057 -t 64 -D newfirmware.dfu

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

 

 
