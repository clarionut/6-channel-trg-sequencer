# 6 channel trigger sequencer
A reworked version of [Hagiwo's 6 channel trigger sequencer](https://note.com/solder_state/n/n17c69afd484d).

This release just contains the reworked Arduino code but uses the same hardware as the original (I2C OLED and the same Arduino pins) so it should run on an existing sequencer.

The code has been massively reworked. Source code is now ~500 lines instead of the original 1658, but has all the original functionality. It also uses direct port manipulation for speed and the closest possible synchronisation of the trigger outputs.

The version uploaded 16/09/2025 has some bug fixes (some min, some from the original code) and also includes a screensaver which blanks the OLED after a user-defined period (30s by default). The screensaver can be disabled by commenting out the #define line - see the source code for details.

In addition to the I2C version previously uploaded, I have now created a version of the code to run on the same hardware as [my version of the Euclidean Rhythms module](https://github.com/clarionut/Euclidean_Rhythms). Somewhat to my surprise I discovered that the Adafruit SSD1306 library runs about twice as fast as either u8glib or u8g2. The compiled code is larger, but the speedup is well worthwhile. This version of the code is in the 

### Hardware
The SPI version runs much faster than I2C due to the difference in upload time to the OLED (2-3ms per upload for SPI vs 37ms for I2C), so I recommend anyone interested in building one of these modules to use an SPI display. The hardware is identical to my version of the Euclidean Rhythms module except the potentiometers and socket for the probability control can be omitted from the panel.
