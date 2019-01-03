# PIC16-XPRESS-Loader
A PIC16 implementation of the XPRESS-Loader (Drag'n Drop Programmer + USB-Serial Bridge) 

This is composed of two projects: 
* The MSD-Loader project proper containing several configurations for different target device families
* A DFU bootloader to allow updates of the MSD-Loader itself via the standard DFU-util tool
  * NOTE: this is based on the great work of Matt Sarnoff and Peter Lawrence [USB-DFU Booloader for PIC16F145x](https://github.com/majbthrd/PIC16F1-USB-DFU-Bootloader)
