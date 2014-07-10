hello_disting
=============

This is a minimal code project to start up the disting, configure the codec, 
and run a simple processing loop.

Preserving the calibration
==========================

If you want to preserve the factory-written calibration data, 
do not overwrite memory between 0x1d01fe00-0x1d01ffff.

This is done in the project properties dialog. 
Select "PICkit 3" in the configuration section, enable "Preserve Program Memory",
and enter the addresses 0x1d01fe00 & 0x1d01ffff.
