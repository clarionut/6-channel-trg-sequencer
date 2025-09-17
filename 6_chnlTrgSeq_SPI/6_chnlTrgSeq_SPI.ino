/*
  An updated version of the code for the Hagiwo 6-channel trigger sequencer (https://note.com/solder_state/n/n17c69afd484d)

  Code restructured to remove duplicate statements and make greater use of loops and arrays to reduce source size.
  Trigger output is via direct port manipulation rather than using digitalWrite();.

  Screen saver blanks display after specified period (can be disabled)
*/

// Screen saver timeout in seconds. Comment out the next line to disable screen saver
#define SS_TIMEOUT 30

#include <EEPROM.h>
#define  ENCODER_OPTIMIZE_INTERRUPTS
#include <Encoder.h>

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET 12

// Pin definitions
#define BTN_PIN 5
#define CLK_PIN 2

//----------------Defining variables---------------------------
uint8_t step_count = 0; // clock in when it reaches 16 it goes back to 0
uint8_t clock_in = 0;   // External clock input status H=1, L=0
uint8_t old_clock_in = 0;

typedef struct {
  uint16_t step;
  bool mute;
} channel_t;

channel_t chnl[6] = { { 0x8888, 0 },
                      { 0x0808, 0 },
                      { 0xCCCC, 0 },
                      { 0x2222, 0 },
                      { 0xFFFF, 0 },
                      { 0x0000, 0 } };

// Rotary Encoder Settings
Encoder myEnc(3, 4);
int oldPosition, newPosition;
uint8_t enc = 97; // Selected encoder. MANUAL is displayed when first started.

// Button Settings
uint8_t button = 0; // 0=OFF,1=ON
uint8_t old_button = 0; // Debounce
uint8_t button_on = 0; // Button state after debounce. 0=OFF, 1=ON

// Display constructor & variables
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, 10, 12, -1); // SCK = 13, COPI = 11, D/C = 10, RST = 12, CS not used
bool screenSave = 0;
unsigned long screenMillis;

// Screensaver
#ifdef SS_TIMEOUT
const unsigned long ssTimeout = SS_TIMEOUT * 1000;
#else
const unsigned long ssTimeout = 0;
#endif

// Options
uint8_t mode = 0; // 0 = MANUAL, 1 = AUTO
uint8_t count_reset = 0; // 1 makes the count 0
uint8_t save = 0; // Execute save at 1 and immediately return to 0

char genres[][7] = { "TECHNO", "DUBTEC", "HOUSE" }; // Pattern Genres 0=techno, 1=dub, 2=house
uint8_t genre = 0;  // Default to techno
uint8_t repeat = 2; // 0 = 4 times, 1 = 8 times, 2 = 16 times, 3 = 32 times, 4 = eternal
uint8_t fillin = 1; // 0 = OFF, 1 = ON
uint8_t sw = 0;     // 0 = 2, 1 = 4, 2 = 8, 3 = 16, 4 = eternal

// AUTO mode
int repeat_max = 4; // repeat_done Fill in when this value is reached
int repeat_done = 0; // step When it reaches 16, it is rounded up.
uint8_t change_bnk[] = { 1, 1, 1 }; // presets 1, 2 & 3
uint8_t sw_max = 1; // sw_done When this value is reached, the pattern changes (random)
uint8_t sw_done = 0; // When repeat_max is reached, it will be rounded up

uint8_t maskC;

// preset step patterns
// [Number of patterns] [Number of CHs * 2. The first 6 are normal, the last 6 are fillin]
// The 16 numbers indicate the drum pattern.
const static uint16_t bank_pattern[][8][12] PROGMEM = {
  // bank1 TECHNO
  {
    { 0x8888,  0x0808, 0xDDDD, 0x2222, 0x1000, 0x0022, 0x8888, 0x88AF, 0xDDDD, 0x2222, 0x1000, 0x0022 },
    { 0x8888,  0x0808, 0xFFFF, 0x2222, 0x1000, 0x0022, 0x8888, 0x88AF, 0xFFFF, 0x2222, 0x1000, 0x0022 },
    { 0x8888,  0x0808, 0xCCCC, 0x2222, 0x1000, 0x0022, 0x8888, 0x88AF, 0xCCCC, 0x2222, 0x1000, 0x0022 },
    { 0x8888,  0x0808, 0x4545, 0x2222, 0x1000, 0x0022, 0x88AF, 0x0808, 0x4545, 0x2222, 0x1000, 0x0022 },
    { 0x8888,  0x0808, 0xFFFF, 0x2222, 0x1000, 0x0022, 0x88AF, 0x0808, 0xFFFF, 0x2222, 0x1000, 0x0022 },
    { 0x8888,  0x0809, 0xDDDD, 0x2222, 0x1000, 0x0022, 0x0000, 0x0809, 0xDDDD, 0x2222, 0x1000, 0x0022 },
    { 0x8888,  0x0849, 0xDDDD, 0x2222, 0x1000, 0x0022, 0x0000, 0x0849, 0xDDDD, 0x2222, 0x1000, 0x0022 },
    { 0x8888,  0x0802, 0xDDDD, 0x2222, 0x1000, 0x0022, 0x8896, 0x0869, 0xDDDD, 0x2222, 0x1000, 0x0022 }
  },

  // bank2 DUBTECHNO
  {
    { 0x8888,  0x0808, 0xDDDD, 0x2222, 0x1240, 0x0022, 0x8888, 0x0809, 0xDDDD, 0x2222, 0x1240, 0x0022 },
    { 0x8888,  0x0808, 0xFFFF, 0x2222, 0x1240, 0x0022, 0x000A, 0x0849, 0xDDDD, 0x2222, 0x1000, 0x0022 },
    { 0x8889,  0x0808, 0xCCCC, 0x2222, 0x1240, 0x0022, 0x8888, 0x0000, 0xCCCC, 0x2222, 0x1240, 0x0022 },
    { 0x8889,  0x0808, 0x4545, 0x2222, 0x1240, 0x0022, 0x8888, 0x0809, 0x4545, 0x2222, 0x1240, 0x0022 },
    { 0x888C,  0x0808, 0xFFFF, 0x2222, 0x1240, 0x0022, 0x8888, 0x8888, 0xFFFF, 0x2222, 0x1240, 0x0022 },
    { 0x888C,  0x0809, 0xDDDD, 0x2222, 0x1240, 0x0022, 0x999F, 0x0000, 0xDDDD, 0x2222, 0x1240, 0x0022 },
    { 0x0000,  0x0849, 0xDDDD, 0x2222, 0x1240, 0x0022, 0x000A, 0x0849, 0xDDDD, 0x2222, 0x1000, 0x0022 },
    { 0x0000,  0x0802, 0xDDDD, 0x2222, 0x1240, 0x0022, 0x000A, 0x0802, 0xDDDD, 0x2222, 0x1000, 0x0022 }
  },

  // bank3 HOUSE
  {
    { 0x8888, 0x0808, 0x2222, 0x0000, 0x0040, 0x0101, 0x8888, 0x88AF, 0x2222, 0x0000, 0x0040, 0x0101 },
    { 0x888A, 0x0808, 0x2323, 0x0000, 0x0040, 0x0101, 0x8888, 0x88AF, 0x2323, 0x0000, 0x0040, 0x0101 },
    { 0x8888, 0x0808, 0xCCCC, 0x2222, 0x0040, 0x0101, 0x8888, 0x88AF, 0xCCCC, 0x2222, 0x0040, 0x0101 },
    { 0x888A, 0x0808, 0xCCCC, 0x2222, 0x0040, 0x0101, 0x8888, 0x88AF, 0xCCCC, 0x2222, 0x0040, 0x0101 },
    { 0x8888, 0x0808, 0xFFFF, 0x2222, 0x0040, 0x0101, 0x0000, 0x0808, 0xFFFF, 0x2222, 0x0040, 0x0101 },
    { 0x888A, 0x0808, 0xFFFF, 0x2222, 0x0040, 0x0101, 0x0000, 0x0808, 0xFFFF, 0x2222, 0x0040, 0x0101 },
    { 0x888A, 0x0808, 0xFFFF, 0x2222, 0x0040, 0x4112, 0x0000, 0x0808, 0xFFFF, 0x2222, 0x0040, 0x4112 },
    { 0x8888, 0x0808, 0xCCCC, 0x2222, 0x0040, 0x4112, 0x88AF, 0x0808, 0xCCCC, 0x2222, 0x0040, 0x4112 }
  }
};

void setup() {
  //Serial.begin(115200); // For debugging

  // Display initialization
  delay(50);
  display.begin(SSD1306_SWITCHCAPVCC);
  delay(50);

  // Use A0 - A5 as digital outputs 14 - 19 (port C)
  for (uint8_t j = 14; j <= 19; j++) {
    pinMode(j, OUTPUT);
  }
  //pinMode(CLK_PIN, INPUT_PULLUP);
  pinMode(BTN_PIN, INPUT_PULLUP);

  // Read saved data
  uint8_t EEadd = 1;
  for (uint8_t j = 0; j < 6; j++) {
    EEPROM.get(EEadd, chnl[j].step);
    EEadd += 2;
  }

  // Initialise the encoder position
  oldPosition = myEnc.read();

  OLED_display();
  screenMillis = millis();
}

void loop() {
  old_button = button;
  old_clock_in = clock_in;

  //-----------Rotary encoder reading----------------
  newPosition = myEnc.read();

  if (newPosition + 3 < oldPosition) { // Left turn by one detent
    oldPosition = newPosition;
    screenMillis = millis();

    if (screenSave) {
      screenSave = 0;
    } else {
      --enc;
    }
 
  } else if (newPosition - 3 > oldPosition) { // Right turn by one detent
    oldPosition = newPosition;
    screenMillis = millis();
    
    if (screenSave) {
      screenSave = 0;
    } else {
      ++enc;
    }
  }

  if (0 == enc) {
    // In manual mode, max=105 (16*6ch+3option+6mute). In AUTO mode, max=11 (MANUAL, genre, fillin, repeat, sw+mute6)
    enc = (mode) ? 11 : 105; //enc_max[mode]; // When the selection reaches the minimum value for each mode, it returns to the maximum value.
  
  } else if (enc > ((mode) ? 11 : 105)) { //enc_max[mode]) {
    enc = 1; // When the selection reaches the maximum value for each mode, it returns to the minimum value.
  }

  //----------BUTTON reading----------------
  if (button = !digitalRead(BTN_PIN)) // input_pullup for "1-"
    screenMillis = millis();

  if (screenSave && button) {
    screenSave = 0;
  } else {
    button_on = (!old_button && button); // 0→1 debounce
  }

  //-------------------MANUAL mode-------------------------
  if (0 == mode) {
    if (button_on) { // Press the button while any step is selected to switch the gate ON/OFF.
      switch (enc) {
        case 1 ... 96: // Toggle channel steps
          // (enc-1)/16 gives the channel number [0-5], (1<<(uint8_t)(16-enc)%16) gives the bit within that channel [0-15]
          chnl[(enc - 1) / 16].step ^= (1 << (uint8_t) (16 - enc) % 16);
          break;
  
        case 97: // AUTO mode selected
          mode = 1;
          change_step();
          break;

        case 98: // RESET
          step_count = 0;
          break;

        case 99: // SAVE to EEPROM
          save_data();
          step_count = 0;
          break;

        case 100 ... 105: // Toggle mute of selected channel
          chnl[enc - 100].mute = !chnl[enc - 100].mute;
          break;
      }
    }

  //-------------------AUTO MODE-------------------------
  } else if (1 == mode) {
    if (button_on) {
      // MANUAL option
      uint8_t EEadd;
      switch (enc) {
        case 1: // change to MANUAL
          mode = 0;
          enc = 97; // When returning from AUTO to MANUAL mode, 'MAN.' will be selected. Feel free to change it.
          // Read saved data
          EEadd = 1;
          for (uint8_t j = 0; j < 6; j++) {
            EEPROM.get(EEadd, chnl[j].step);
            EEadd += 2;
          }
          break;

        case 2: // Change GENRE
          ++genre;
          genre %= 3;
          break;

        case 3: // Toggle FILL-IN
          fillin = !fillin;
          break;

        case 4: // Change REPEAT: 0 = 4 times, 1 = 8 times, 2 = 16 times, 3 = 32 times, 4 = eternal
          ++repeat;
          repeat %= 5;
          break;

        case 5: // Change SW: 0 = 2, 1 = 4, 2 = 8, 3 = 16, 4 = eternal
          ++sw;
          sw %= 5;
          break;

        case 6 ... 11: // Mute relevant channel
          chnl[enc - 6].mute = !chnl[enc - 6].mute;
          break;
      }
    }
  }

  //-------------AUTO Mode Processing---------------
  repeat_max = 4;
  repeat_max <<= repeat;
  if (64 == repeat_max) repeat_max = 10000; // Eternal  

  sw_max = 2;
  sw_max <<= sw;
  if (32 == sw_max) sw_max = 255; // Eternal

  // Screen Saver
#ifdef SS_TIMEOUT
  if (!screenSave && (millis() - screenMillis > ssTimeout)) {
    screenSave = 1;
    display.clearDisplay();
    display.display();
  }
#endif

  //--------------External clock input and trigger output----------------------------------
  clock_in = digitalRead(CLK_PIN);
  if (clock_in != old_clock_in) {
    if (clock_in) {
      // set the trigger bits
      maskC = 0;
      for (uint8_t j = 0; j <= 5; j++) {
        // set port C bits
        if (!chnl[j].mute && (chnl[j].step & (0b1000000000000000 >> step_count)))
          maskC |= (1 << j);
      }
      PORTC |= maskC;
      // Note: The OLED is updated only when the clock is input to avoid the Arduino being busy.
      if (!screenSave) OLED_display();
      ++step_count;
      step_count %= 16;
   
      if (1 == mode) {
        ++repeat_done;

        if (fillin && (repeat_done == repeat_max - 1)) {
          fillin_step();

        } else if (repeat_done >= repeat_max) {
          ++sw_done;
          repeat_done = 0;
          if (sw_done >= sw_max) {
            change_step();
            sw_done = 0;
          }
        }
      }

    } else {
      // clear the trigger bits
      PORTC &= ~maskC;
    }
  }
}

//--------------OLED output----------------------------------
void OLED_display() {
  display.setTextSize(1); // Output character size
  display.setTextColor(WHITE); // Output text color
  display.clearDisplay(); // Clear Display

  // Channels display
  for (uint8_t j = 0; j < 6; j++) {
    uint8_t tmp = j * 9;
    display.setCursor(0, tmp);
    display.print("CH");
    display.print(j + 1);

    display.setCursor(30, tmp);
    if (!chnl[j].mute) {
      uint16_t mask = 0b1000000000000000;
      for (int8_t i = 15; i >= 0; i--) {
        display.print((chnl[j].step & mask) ? "*" : "_");
        mask >>= 1;
      }
    }
  }

  //-------------Encode Display---------------------------------
  display.setCursor(0, 54);

  if (0 == mode) {
    //MANUAL mode
    // Selected step
    if (97 > enc) {
      uint8_t chan = (enc - 1) / 16;
      display.drawRect((enc - 16 * chan) * 6 + 23, 9 * chan, 6, 8, WHITE);
    }

    // Optional items
    if (97 == enc) {
      display.setTextColor(BLACK, WHITE); // (BLACK, WHITE) prints black text on white
    }
    display.print("MAN.");
    display.setTextColor(WHITE);

    if (98 == enc) {
      display.setTextColor(BLACK, WHITE);
    }
    display.setCursor(48, 54);
    display.print("RESET");
    display.setTextColor(WHITE);

    if (99 == enc) {
      display.setTextColor(BLACK, WHITE);
    }
    display.setCursor(102, 54);
    display.print("SAVE");
    display.setTextColor(WHITE);

    for (uint8_t j = 0; j < 6; j++) {
      if (100 + j == enc) {
        display.setTextColor(BLACK, WHITE);
      }
      display.setCursor(0, 9 * j);
      display.print("CH");
      display.print(j + 1);
      display.setTextColor(WHITE);
    }

  } else {
    // AUTO mode
    if (enc <= 3 ) {
      if (1 == enc) {
        display.setTextColor(BLACK, WHITE);
      }
      display.print("AUTO");
      display.setTextColor(WHITE);

      display.print("  ");

      if (2 == enc) {
        display.setTextColor(BLACK, WHITE);
      }
      display.setCursor(39, 54);
      display.print(genres[genre]);
      display.setTextColor(WHITE);

      if (3 == enc) { // Selection of Fillin
        display.setTextColor(BLACK, WHITE);
      }
      display.setCursor(90, 54);
      display.print((fillin) ? "FILL:Y" : "FILL:N");
      display.setTextColor(WHITE);

    } else if (enc >= 4 ) {
      display.setCursor(0, 54);
      if (4 == enc) { // Repeat Selection
        display.setTextColor(BLACK, WHITE);
      }
      display.print("REP:");
      display.print(repeat_done + 1);
      display.print("/");
      if (repeat <= 3) {
        display.print(repeat_max);
      } else {
        display.print("ET");
      }
      display.setTextColor(WHITE);

      if (5 == enc) { // Selection of SW
        display.setTextColor(BLACK, WHITE);
      }
      display.setCursor(70, 54);
      display.print("SW:");
      display.print(sw_done + 1);
      display.print("/");
      if (sw <= 3) {
        display.print(sw_max);
      } else {
        display.print("ET");
      }
      display.setTextColor(WHITE);

      for (uint8_t j = 0; j < 6; j++) {
        if (6 + j == enc) {
          display.setTextColor(BLACK, WHITE);
        }
        display.setCursor(0, 9 * j);
        display.print("CH");
        display.print(j + 1);
        display.setTextColor(WHITE);
      }
    }
  }

  // Show current step
  display.drawRect(step_count * 6 + 30, 0 , 6, 53, WHITE);
  display.display();  // Display on the screen
}

void save_data() {
  uint8_t EEadd = 1;
  for (uint8_t j = 0; j < 6; j++) {
    EEPROM.put(EEadd, chnl[j].step);
    EEadd += 2;
  }
}

void change_step() {
  // Automatically change STEP in AUTO mode
  change_bnk[genre] = random(0, 8); // random max matches the number of alternative patterns.
  for (uint8_t j = 0; j < 6; j++) {
    chnl[j].step = pgm_read_word(&(bank_pattern[genre][change_bnk[genre]][j]));
  }
}

void fillin_step(void) {
  // Fill-in pattern settings
  for (uint8_t j = 0; j < 6; j++) { // loop over channels
    chnl[j].step = pgm_read_word(&(bank_pattern[genre][change_bnk[genre]][j + 6]));
  }
}
