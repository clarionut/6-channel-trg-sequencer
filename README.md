# 6 channel trigger sequencer
A reworked version of [Hagiwo's 6 channel trigger sequencer](https://note.com/solder_state/n/n17c69afd484d).

This release contains the reworked Arduino code for the same hardware as the original Hagiwo module (I2C OLED and the same Arduino pins) so it should run on an existing sequencer.

The code has been massively reworked. Source code is now ~500 lines instead of the original 1658, but has all the original functionality. It also uses direct port manipulation for speed and the closest possible synchronisation of the trigger outputs.

This version (uploaded 16/09/2025) has some bug fixes (some mine, some from the original code) and also includes a screensaver which blanks the OLED after a user-defined period (30s by default). The screensaver can be disabled by commenting out the #define line - see the source code for details.

### Alternative Hardware
In addition to the I2C version, I have now created a version of the code to run on the same hardware as [my version of the Euclidean Rhythms module](https://github.com/clarionut/Euclidean_Rhythms). There's a speedup of ~35ms (2-3ms vs. 37ms per upload) per display refresh for SPI vs. I2C, which significantly improves performance. I'd recommend anyone interested in building one of these modules to use an SPI display. This version of the code is in the [6_chnlTrgSeq_SPI](https://github.com/clarionut/6-channel-trg-sequencer/tree/main/6_chnlTrgSeq_SPI) folder.

The hardware for the SPI version uses different Arduino pins from the Hagiwo original but is identical to my Euclidean Rhythms module (except the potentiometers and socket for the probability control can be omitted from the panel). Of course it can be used as an alternative firmware on the Euclidean Rhythms hardware, but the Probability controls will do nothing in this case.

To build a version of my Eulidean Rhythms PCB without the probability controls, omit R24, R25, R26, R31, R32, R35, D6, D7 & D8 and add wire links in place of D7 & R31.

### Updates
16-09-2025 - fixed bug in the code for setting user-defined patterns
