# stm32f4-FatFs

This application uses a FatFs middleware module on a STM32F4 Discovery board. The code creates two FAT file systems: one for the on-chip RAM and the other for a USB drive.
After a Link Driver phase, the application writes periodically some XYZ values (read from the accelerometer) on the RAM and then, after a USB Drive attach, copy the written values on a new file in the
USB drive.

To use this project, you have to modify the paths in the makefile in order to find the project's folder.
Use the .PHONY command 'project' to execute all the necessary commands to run this project.
