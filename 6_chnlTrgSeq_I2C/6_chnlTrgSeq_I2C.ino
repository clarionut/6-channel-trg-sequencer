//----------------Loading the Library---------------------------
// EEPROM library
#include <EEPROM.h>

// Encoder library
#define  ENCODER_OPTIMIZE_INTERRUPTS // Encoder noise countermeasures
#include <Encoder.h>

// OLED library
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_ADDRESS 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

//----------------Defining variables---------------------------
uint8_t step_count = 0; // clock in when it reaches 17 it goes back to 1
uint8_t clock_in = 0; // External clock input status H=1, L=0
uint8_t old_clock_in = 0; // Variables for recognizing 0 → 1

typedef struct {
  uint16_t step;
  bool mute;
} channel_t;

channel_t chnl[6] = { {0x8888, 0},
                      {0x0808, 0},
                      {0xCCCC, 0},
                      {0x2222, 0},
                      {0xFFFF, 0},
                      {0x0000, 0} };

// Rotary Encoder Settings
Encoder myEnc(3, 2); // For rotary encoder library
int oldPosition  = -999;
int newPosition = -999;
uint8_t enc = 96; // Selected encoder. MANUAL is displayed when first started.
unsigned int enc_bit = 0x00;//

// Button Settings
uint8_t button = 0; // 0=OFF,1=ON
uint8_t old_button = 0; // Debounce
uint8_t button_on = 0; // Button state after debounce. 0=OFF, 1=ON

// Display variable declaration
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Options
uint8_t mode = 0; // 0 = MANUAL, 1 = AUTO
uint8_t count_reset = 0; // 1 makes the count 0
uint8_t save = 0; // Execute save at 1 and immediately return to 0

uint8_t genre = 0; // Pattern Genres 0=techno, 1=dub, 2=house
uint8_t repeat = 2; // 0 = 4 times, 1 = 8 times, 2 = 16 times, 3 = 32 times, 4 = eternal
uint8_t fillin = 1; // 0 = OFF, 1 = ON
uint8_t sw = 0; // 0 = 2, 1 = 4, 2 = 8, 3 = 16, 4 = eternal

// AUTO mode
int repeat_max = 4; // repeat_done Fill in when this value is reached
int repeat_done = 0; // step When it reaches 16, it is rounded up.
uint8_t test = 0; // For development
uint8_t change_bnk[] = {1, 1, 1}; // presets 1, 2 & 3
uint8_t sw_max = 1; // sw_done When this value is reached, the pattern changes (random)
uint8_t sw_done = 0; // When repeat_max is reached, it will be rounded up


// preset step pattern
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
  //Development communication settings
  Serial.begin(115200);

  // Display initialization
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  pinMode(13, INPUT);
  pinMode(12, INPUT_PULLUP); //BUTTON
  pinMode(5,  OUTPUT); //CH1
  pinMode(6,  OUTPUT); //CH2
  pinMode(7,  OUTPUT); //CH3
  pinMode(8,  OUTPUT); //CH4
  pinMode(9,  OUTPUT); //CH5
  pinMode(10, OUTPUT); //CH6

  // Read saved data
  uint8_t EEadd = 0;
  for (uint8_t j = 0; j < 6; j++) {
    EEPROM.get(EEadd, chnl[j].step);
    EEadd += 2;
  }
 
  OLED_display();
}

void loop() {

  old_clock_in = clock_in;
  old_button = button;

  //-----------Rotary encoder reading----------------
  newPosition = myEnc.read();

  if (newPosition + 3 < oldPosition) { // Left turn by one detent
    oldPosition = newPosition;
    enc = enc - 1;
  } else if (newPosition - 3 > oldPosition) { // Right turn by one detent
    oldPosition = newPosition;
    enc = enc + 1;
  }

  if (enc <= 0) {
    // In manual mode, max=105 (16*6ch+3option+6mute). In AUTO mode, max=11 (MANUAL, genre, fillin, repeat, sw+mute6)
    enc = (mode) ? 11 : 105; //enc_max[mode]; // When the selection reaches the minimum value for each mode, it returns to the maximum value.
  } else if (enc > ((mode) ? 11 : 105)) { //enc_max[mode]) {
    enc = 1; // When the selection reaches the maximum value for each mode, it returns to the minimum value.
  }

  //----------BUTTON reading----------------
  button = 1 - digitalRead(12); // input_pullup for "1-"

  if (!old_button && button) { // 0→1 debounce
    button_on = 1;
  } else {
    button_on = 0;
  }

  //-------------------MANUAL mode-------------------------
  if (0 == mode) {
    // Bit management for the selected pattern
    enc_bit = 0;
    bitSet(enc_bit, abs(enc % 16 - 16));
    if (abs(enc % 16 - 16) == 16) {
      bitSet(enc_bit, 0);
    }

    if (button_on) { // Press the button while any step is selected to switch the gate ON/OFF.
    Serial.println(enc);
    uint8_t chan;
      switch (enc) {
        case 1 ... 96:
          // Toggle channel steps
          chan = (enc - 1) / 16;
          chnl[chan].step ^= enc_bit;
          break;
  
        case 97:
          // AUTO selected
          mode = 1;
          //enc_max = 11;
          change_step(); // Switch to AUTO mode pattern
          break;

        case 98:
          // RESET selected
          step_count = 1;
          break;

        case 99:
          // SAVE selected
          save_data(); // Save to EEPROM
          step_count = 1;
          break;

        case 100 ... 105:
          // Toggle mute of selected channel
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
        case 1:
          mode = 0; // MANUAL call
          //enc_max = 105;
          enc = 97; // When returning from AUTO to MANUAL, MANUAL will be selected. Feel free to change it.
          // Reading saved data
          EEadd = 0;
          for (uint8_t j = 0; j < 6; j++) {
            EEPROM.get(EEadd, chnl[j].step);
            EEadd += 2;
          }
          break;

        case 2:
          ++genre;
          genre %= 3;
          break;

        case 3:
          fillin = !fillin;
          break;

        case 4:
          // 0 = 4 times, 1 = 8 times, 2 = 16 times, 3 = 32 times, 4 = eternal
          ++repeat;
          repeat %= 5;
          break;

        case 5:
          // 0 = 2, 1 = 4, 2 = 8, 3 = 16, 4 = eternal
          ++sw;
          sw %= 5;
          break;

        case 6 ... 11:
          // Mute relevant channel
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

  //--------------External clock input detection and counting----------------
  clock_in = digitalRead(13);

  if (!old_clock_in && clock_in) {
    ++step_count;
  }

  if (step_count >= 17) {
    step_count = 1;

    if (1 == mode) {
      ++repeat_done;

      if (fillin && repeat_done == repeat_max - 1) {
        fillin_step();

      } else if (repeat_done >= repeat_max) {
        ++sw_done;
        repeat_done = 0;
        change_step();
      }
    }
  }

  if (sw_done >= sw_max)
    sw_done = 0;

  //--------------Output----------------------------------
  uint8_t maskB = 0, maskD = 0;
  uint8_t tmp = 16 - step_count;
  for (uint8_t j = 0; j <= 2; j++) {
    // set port D bits
    if (bitRead(chnl[j].step, tmp) && !chnl[j].mute)
      bitSet(maskD, j + 5);
  }
  for (uint8_t j = 3; j <= 5; j++) {
    //set port B bits
    if (bitRead(chnl[j].step, tmp) && !chnl[j].mute)
      bitSet(maskB, j - 3);
  }
  if (clock_in) {
    PORTB |= maskB;
    PORTD |= maskD;
  } else {
    PORTB &= ~maskB;
    PORTD &= ~maskD;
  }

 //--------------OLED output----------------------------------
 // Note: The OLED is updated only when the clock is input to avoid the Arduino being busy.
  if (old_clock_in == 0 && clock_in == 1) {
    OLED_display();
  }

  // For development
 //  Serial.print(repeat_done);
 //  Serial.print(",");
 //  Serial.print(sw_max);
 //  Serial.println("");
}
//--------------OLED output----------------------------------
void OLED_display() {

  display.clearDisplay(); // Clear Display
  display.setTextSize(1); // Output character size
  display.setTextColor(WHITE); // Output text color

  // Channels display
  for (uint8_t j = 0; j < 6; j++) {
    display.setCursor(0, j * 9); // The position of the character's extreme edge
    display.print("CH");
    display.print(j + 1);

    display.setCursor(30, j * 9);
    if (!chnl[j].mute) {
      uint16_t mask = 0b1000000000000000;
      for (int8_t i = 15; i >= 0; i--) {
        display.print((chnl[j].step & mask) ? "*" : "_");
        mask >>= 1;
      }
    }
  }

 //-------------Encode Display---------------------------------

 //MANUAL mode
  if (0 == mode) {

    // Selected step
    if (enc < 97) {
      uint8_t chan = (enc - 1) / 16;
      display.drawRect((enc - 16 * chan) * 6 + 24, 9 * chan, 6, 8, WHITE);
    }

    // Optional items
    if (enc == 97) {
      display.setTextColor(BLACK, WHITE); // (BLACK, WHITE) inverts the color of the output text
    } else {
      display.setTextColor(WHITE);
    }
    display.setCursor(0, 54);  // The position of the character's extreme edge
    display.print("MAN.");

    if (enc == 98) {
      display.setTextColor(BLACK, WHITE);
    } else {
      display.setTextColor(WHITE);
    }
    display.setCursor(48, 54);
    display.print("RESET");

    if (enc == 99) {
      display.setTextColor(BLACK, WHITE);
    } else {
      display.setTextColor(WHITE);
    }
    display.setCursor(102, 54);
    display.print("SAVE");

    for (uint8_t j = 0; j < 6; j++) {
      if (100 + j == enc) {
        display.setTextColor(BLACK, WHITE);
      } else {
        display.setTextColor(WHITE);
      }
      display.setCursor(0, 9 * j);
      display.print("CH");
      display.print(j + 1);
    }
  }

  // AUTO mode
  display.setCursor(0, 54);  // The position of the character's extreme edge
  if (1 == mode) {
    if (enc <= 3 ) {
      if (1 == enc) {
        display.setTextColor(BLACK, WHITE);
      } else {
        display.setTextColor(WHITE);
      }
      display.print("AUTO");

      display.setTextColor(WHITE);
      display.print("  ");

      if (2 == enc) {
        display.setTextColor(BLACK, WHITE);
      } else {
        display.setTextColor(WHITE);
      }
      switch (genre) { // Selecting a Preset
        case 0:
          display.print("TECHNO"); // Due to display area limitations, 6 characters are used.
          break;

        case 1:
          display.print("DUBTCN");
          break;

        case 2:
          display.print("HOUSE ");
          break;
      }

      display.setTextColor(WHITE);
      display.print("  ");

      if (enc == 3) { // Selection of Fillin
        display.setTextColor(BLACK, WHITE);
      } else {
        display.setTextColor(WHITE);
      }
      switch (fillin) {
        case 0:
          display.print("FILL: N");
          break;

        case 1:
          display.print("FILL: Y");
          break;
      }

    } else if (enc >= 4 ) {
      display.setCursor(0, 54);  // The position of the character's extreme edge
      if (4 == enc) { // Repeat Selection
        display.setTextColor(BLACK, WHITE);
      } else {
        display.setTextColor(WHITE);
      }
      display.print("REP:");
      display.print(repeat_done + 1);
      display.print("/");
      if (repeat <= 3) {
        display.print(repeat_max);
      } else if (repeat >= 4) {
        display.print("ET");
      }

      if (5 == enc) { // Selection of SW
        display.setTextColor(BLACK, WHITE);
      } else {
        display.setTextColor(WHITE);
      }
      display.setCursor(70, 54);
      display.print("SW:");
      display.print(sw_done + 1);
      display.print("/");
      if (sw <= 3) {
        display.print(sw_max);
      } else if (sw >= 4) {
        display.print("ET");
      }

      for (uint8_t j = 0; j < 6; j++) {
        if (6 + j == enc) {
          display.setTextColor(BLACK, WHITE);
        } else {
          display.setTextColor(WHITE);
        }
        display.setCursor(0, 9 * j);
        display.print("CH");
        display.print(j + 1);
      }
    }
  }

  // During output step
  display.drawRect(step_count * 6 + 24, 0 , 6, 53, WHITE);
  display.setCursor(0, 54);  // The position of the character's extreme edge

  display.display();  // Display on the screen
}

void save_data() {
  uint8_t EEadd = 0;
  for (uint8_t j = 0; j < 6; j++) {
    EEPROM.put(EEadd, chnl[j].step);
    EEadd += 2;
  }
}

void change_step() {
  // Automatically change STEP in AUTO mode

  // bank1
  if (0 == genre) {
    if (sw_done >= sw_max) { // Only when the SW reaches the specified value, a pattern is randomly selected from the presets.
      change_bnk[0] = random(0, 7); // random max matches the upper limit of the pattern.
    }
    for (uint8_t j = 0; j < 6; j++) {
      chnl[j].step = pgm_read_word(&(bank_pattern[0][change_bnk[0]][j]));
    }
  }

  // bank2
  if (1 == genre) {
    if (sw_done >= sw_max) { // SW Only when the specified value is reached, a pattern is randomly selected from the presets.
      change_bnk[1] = random(0, 4); // random max matches the upper pattern limit
    }
    for (uint8_t j = 0; j < 6; j++) {
      chnl[j].step = pgm_read_word(&(bank_pattern[1][change_bnk[1]][j]));
    }
  }

  // bank3
  if (2 == genre) {
    if (sw_done >= sw_max) { // Only when the SW reaches the specified value, a pattern is randomly selected from the presets.
      change_bnk[2] = random(0, 4); // random max matches the upper pattern limit
    }
    for (uint8_t j = 0; j < 6; j++) {
      chnl[j].step = pgm_read_word(&(bank_pattern[2][change_bnk[2]][j]));
    }
  }
}

void fillin_step() {
  // Fill-in pattern settings

    // bank1
    for (uint8_t j = 0; j < 6; j++) {
      chnl[j].step = pgm_read_word(&(bank_pattern[genre][change_bnk[genre]][j + 6]));
    }
}