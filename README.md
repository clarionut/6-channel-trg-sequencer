# 6 channel trigger sequencer
A reworked version of [Hagiwo's 6 channel trigger sequencer](https://note.com/solder_state/n/n17c69afd484d).

This initial pre-alpha release just contains the reworked Arduino code but uses the same hardware as the original (I2C OLED and the same Arduino pins) so it should run
on an existing sequencer.

The code has been massively reworked. Source code is now 552 lines instead of the original 1658, but has all the original functionality. It also uses direct
port manipulation for speed and the closest possible synchronisation of the trigger outputs.

If this version proves to work satisfactorily I will create a version to run on the same hardware as [my version of the Euclidean Rhythms module](https://github.com/clarionut/Euclidean_Rhythms). This will use interrupts to monitor the input clock, display on a faster SPI OLED and have all the output channels on the same Arduino port.
