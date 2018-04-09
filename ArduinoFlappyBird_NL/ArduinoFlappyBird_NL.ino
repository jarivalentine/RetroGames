// WERKT ENKEL MET IDE 1.5.8 en 1.6.0
// parameters aangepast, zie comments
// NEDERLANDSE VERSIE
// WERKT MET 1.8TFT SPI 128x160 V1.1 DISPLAY
// http://www.ebay.com/itm/371462246346?_trksid=p2057872.m2749.l2649&ssPageName=STRK%3AMEBIDX%3AIT

// =============================================================================
//
// Arduino - Flappy Bird clone
// ---------------------------
// by Themistokle "mrt-prodz" Benetatos
//
// This is a Flappy Bird clone made for the ATMEGA328 and a Sainsmart 1.8" TFT
// screen (ST7735). It features an intro screen, a game over screen containing
// the player score and a similar gameplay from the original game.
//
// Developed and tested with an Arduino UNO and a Sainsmart 1.8" TFT screen.
//
// TODO: - debounce button ?
//
// Dependencies:
// - https://github.com/adafruit/Adafruit-GFX-Library
// - https://github.com/adafruit/Adafruit-ST7735-Library
//
// References:
// - http://www.tweaking4all.com/hardware/arduino/sainsmart-arduino-color-display/
// - http://www.koonsolo.com/news/dewitters-gameloop/
// - http://www.arduino.cc/en/Reference/PortManipulation
//
// --------------------
// http://mrt-prodz.com
// http://github.com/mrt-prodz/ATmega328-Flappy-Bird-Clone
//
// =============================================================================

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <EEPROM.h>

// initialize Sainsmart 1.8" TFT screen
// (connect pins accordingly or change these values)
#define TFT_DC            9     // Sainsmart RS/DC
#define TFT_RST           8     // Sainsmart RES
#define TFT_CS           10     // Sainsmart CS
// initialize screen with pins
static Adafruit_ST7735 TFT = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);
// instead of using TFT.width() and TFT.height() set constant values
// (we can change the size of the game easily that way)
#define TFTW            128     // screen width
#define TFTH            160     // screen height
#define TFTW2            64     // half screen width
#define TFTH2            80     // half screen height
// game constant
#define SPEED             1
#define GRAVITY         9.8
#define JUMP_FORCE     2.15
#define SKIP_TICKS     25.0     // 1000 / 50fps  // bepaalt de horizontale snelheid, STOND OP 20, geeft probleem met vertikale strepen.
#define MAX_FRAMESKIP     5
// bird size
#define BIRDW             8     // bird width
#define BIRDH             8     // bird height
#define BIRDW2            4     // half width
#define BIRDH2            4     // half height
// pipe size
#define PIPEW            12      // pipe width       was 12
#define GAPHEIGHT        36    // pipe gap height  was 36
// floor size
#define FLOORH           20     // floor height (from bottom of the screen)
// grass size
#define GRASSH            4     // grass height (inside floor, starts at floor y)

int maxScore = 0;
const int buttonPin = 3;
// background
const unsigned int BCKGRDCOL = TFT.Color565(138, 235, 244);
// bird
const unsigned int BIRDCOL = TFT.Color565(255, 254, 174);
// pipe
const unsigned int PIPECOL  = TFT.Color565(99, 255, 78);
// pipe highlight
const unsigned int PIPEHIGHCOL  = TFT.Color565(250, 255, 250);
// pipe seam
const unsigned int PIPESEAMCOL  = TFT.Color565(0, 0, 0);
// floor
const unsigned int FLOORCOL = TFT.Color565(246, 240, 163);
// grass (col2 is the stripe color)
const unsigned int GRASSCOL  = TFT.Color565(141, 225, 87);
const unsigned int GRASSCOL2 = TFT.Color565(156, 239, 88);

// bird sprite
// bird sprite colors (Cx name for values to keep the array readable)
#define C0 BCKGRDCOL
#define C1 TFT.Color565(195,165,75)
#define C2 BIRDCOL
#define C3 ST7735_WHITE
#define C4 ST7735_RED
#define C5 TFT.Color565(251,216,114)
static unsigned int birdcol[] =
{ C0, C0, C1, C1, C1, C1, C1, C0,
  C0, C1, C2, C2, C2, C1, C3, C1,
  C0, C2, C2, C2, C2, C1, C3, C1,
  C1, C1, C1, C2, C2, C3, C1, C1,
  C1, C2, C2, C2, C2, C2, C4, C4,
  C1, C2, C2, C2, C1, C5, C4, C0,
  C0, C1, C2, C1, C5, C5, C5, C0,
  C0, C0, C1, C5, C5, C5, C0, C0
};

// bird structure
static struct BIRD {
  unsigned char x, y, old_y;
  unsigned int col;
  float vel_y;
} bird;
// pipe structure
static struct PIPE {
  char x, gap_y;
  unsigned int col;
} pipe;

// score
int score;
// temporary x and y var
static short tmpx, tmpy;

// ---------------
// draw pixel
// ---------------
// faster drawPixel method by inlining calls and using setAddrWindow and pushColor
// using macro to force inlining
#define drawPixel(a, b, c) TFT.setAddrWindow(a, b, a, b); TFT.pushColor(c)

// ---------------
// initial setup
// ---------------
void setup() {
  // initialize the push button on pin 2 as an input
  //DDRD &= ~(1<<PD2);
  pinMode(buttonPin, INPUT);
  digitalWrite(buttonPin, HIGH);
  // resetMaxScore();      // verwijder uit commentaar om max score te resetten
  // initialize a ST7735S chip, black tab
  TFT.initR(INITR_BLACKTAB);
}

// ---------------
// main loop
// ---------------
void loop() {
  game_start();
  game_loop();
  game_over();
}

// ---------------
// game loop
// ---------------
void game_loop() {
  // ===============
  // prepare game variables
  // draw floor
  // ===============
  // instead of calculating the distance of the floor from the screen height each time store it in a variable
  unsigned char GAMEH = TFTH - FLOORH;
  // draw the floor once, we will not overwrite on this area in-game
  // black line
  TFT.drawFastHLine(0, GAMEH, TFTW, ST7735_BLACK);
  // grass and stripe
  TFT.fillRect(0, GAMEH + 1, TFTW2, GRASSH, GRASSCOL);
  TFT.fillRect(TFTW2, GAMEH + 1, TFTW2, GRASSH, GRASSCOL2);
  // black line
  TFT.drawFastHLine(0, GAMEH + GRASSH, TFTW, ST7735_BLACK);
  // mud
  TFT.fillRect(0, GAMEH + GRASSH + 1, TFTW, FLOORH - GRASSH, FLOORCOL);
  // grass x position (for stripe animation)
  char grassx = TFTW;
  // game loop time variables
  double delta, old_time, next_game_tick, current_time;
  next_game_tick = current_time = millis();
  int loops;
  // passed pipe flag to count score
  bool passed_pipe = false;
  // temp var for setAddrWindow
  unsigned char px;

  while (1) {
    loops = 0;
    while ( millis() > next_game_tick && loops < MAX_FRAMESKIP) {
      // ===============
      // input
      // ===============
      if ( (PIND & (1 << PD3)) ) {
        // if the bird is not too close to the top of the screen apply jump force
        if (bird.y > BIRDH2 * 0.5) bird.vel_y = -JUMP_FORCE;
        // else zero velocity
        else bird.vel_y = 0;
      }

      // ===============
      // update
      // ===============
      // calculate delta time
      // ---------------
      old_time = current_time;
      current_time = millis();
      delta = (current_time - old_time) / 1000;

      // bird
      // ---------------
      bird.vel_y += GRAVITY * delta;
      bird.y += bird.vel_y;

      // pipe
      // ---------------
      pipe.x -= SPEED;
      // if pipe reached edge of the screen reset its position and gap
      if (pipe.x < -PIPEW) {
        pipe.x = TFTW;
        pipe.gap_y = random(10, GAMEH - (10 + GAPHEIGHT));
      }

      // ---------------
      next_game_tick += SKIP_TICKS;
      loops++;
    }

    // ===============
    // draw
    // ===============
    // pipe
    // ---------------
    // we save cycles if we avoid drawing the pipe when outside the screen
    if (pipe.x >= 0 && pipe.x < TFTW) {
      // pipe color
      TFT.drawFastVLine(pipe.x + 3, 0, pipe.gap_y, PIPECOL);
      TFT.drawFastVLine(pipe.x + 3, pipe.gap_y + GAPHEIGHT + 1, GAMEH - (pipe.gap_y + GAPHEIGHT + 1), PIPECOL);
      // highlight
      TFT.drawFastVLine(pipe.x, 0, pipe.gap_y, PIPEHIGHCOL);
      TFT.drawFastVLine(pipe.x, pipe.gap_y + GAPHEIGHT + 1, GAMEH - (pipe.gap_y + GAPHEIGHT + 1), PIPEHIGHCOL);
      // bottom and top border of pipe
      drawPixel(pipe.x, pipe.gap_y, PIPESEAMCOL);
      drawPixel(pipe.x, pipe.gap_y + GAPHEIGHT, PIPESEAMCOL);
      // pipe seam
      drawPixel(pipe.x, pipe.gap_y - 6, PIPESEAMCOL);
      drawPixel(pipe.x, pipe.gap_y + GAPHEIGHT + 6, PIPESEAMCOL);
      drawPixel(pipe.x + 3, pipe.gap_y - 6, PIPESEAMCOL);
      drawPixel(pipe.x + 3, pipe.gap_y + GAPHEIGHT + 6, PIPESEAMCOL);
    }
    // erase behind pipe
    if (pipe.x <= TFTW) TFT.drawFastVLine(pipe.x + PIPEW, 0, GAMEH, BCKGRDCOL);

    // bird
    // ---------------
    tmpx = BIRDW - 1;
    do {
      px = bird.x + tmpx + BIRDW;
      // clear bird at previous position stored in old_y
      // we can't just erase the pixels before and after current position
      // because of the non-linear bird movement (it would leave 'dirty' pixels)
      tmpy = BIRDH - 1;
      do {
        drawPixel(px, bird.old_y + tmpy, BCKGRDCOL);
      } while (tmpy--);
      // draw bird sprite at new position
      tmpy = BIRDH - 1;
      do {
        drawPixel(px, bird.y + tmpy, birdcol[tmpx + (tmpy * BIRDW)]);
      } while (tmpy--);
    } while (tmpx--);
    // save position to erase bird on next draw
    bird.old_y = bird.y;

    // grass stripes
    // ---------------
    grassx -= SPEED;
    if (grassx < 0) grassx = TFTW;
    TFT.drawFastVLine( grassx    % TFTW, GAMEH + 1, GRASSH - 1, GRASSCOL);
    TFT.drawFastVLine((grassx + 64) % TFTW, GAMEH + 1, GRASSH - 1, GRASSCOL2);

    // ===============
    // collision
    // ===============
    // if the bird hit the ground game over
    if (bird.y > GAMEH - BIRDH) break;
    // checking for bird collision with pipe
    if (bird.x + BIRDW >= pipe.x - BIRDW2 && bird.x <= pipe.x + PIPEW - BIRDW) {
      // bird entered a pipe, check for collision
      if (bird.y < pipe.gap_y || bird.y + BIRDH > pipe.gap_y + GAPHEIGHT) break;
      else passed_pipe = true;
    }
    // if bird has passed the pipe increase score
    else if (bird.x > pipe.x + PIPEW - BIRDW && passed_pipe) {
      passed_pipe = false;
      // erase score with background color
      TFT.setTextColor(BCKGRDCOL);
      TFT.setCursor( TFTW2, 4);
      TFT.print(score);
      // set text color back to white for new score
      TFT.setTextColor(ST7735_WHITE);
      // increase score since we successfully passed a pipe
      score++;
    }

    // update score
    // ---------------
    TFT.setCursor( TFTW2, 4);
    TFT.print(score);
  }

  // add a small delay to show how the player lost
  delay(1200);
}

// ---------------
// game start
// ---------------
void game_start() {
  TFT.fillScreen(ST7735_BLACK);
  TFT.fillRect(10, TFTH2 - 20, TFTW - 20, 1, ST7735_WHITE);
  TFT.fillRect(10, TFTH2 + 32, TFTW - 20, 1, ST7735_WHITE);
  TFT.setTextColor(ST7735_MAGENTA);
  TFT.setTextSize(3);
  // half width - num char * char width in pixels
  TFT.setCursor( TFTW2 - (6 * 9), TFTH2 - 16);
  TFT.println("FLAPPY");
  TFT.setTextSize(3);
  TFT.setCursor( TFTW2 - (6 * 9), TFTH2 + 8);
  TFT.println("-BIRD-");
  TFT.setTextColor(ST7735_WHITE);
  TFT.setTextSize(0);
  TFT.setCursor( 10, TFTH2 - 48);
  TFT.println("Arduino UNO");
  TFT.setTextColor(ST7735_RED);
  TFT.setCursor( TFTW2 - (17 * 3) - 1, TFTH2 + 54);
  TFT.println("DRUK OP LINKSE KNOP");
  while (1) {
    // wait for push button
    if ( (PIND & (1 << PD3)) ) break;
  }

  // init game settings
  game_init();
}

// ---------------
// game init
// ---------------
void game_init() {
  // clear screen
  TFT.fillScreen(BCKGRDCOL);
  // reset score
  score = 0;
  // init bird
  bird.x = 20;
  bird.y = bird.old_y = TFTH2 - BIRDH;
  bird.vel_y = -JUMP_FORCE;
  tmpx = tmpy = 0;
  // generate new random seed for the pipe gape
  randomSeed(analogRead(3));
  // init pipe
  pipe.x = TFTW;
  pipe.gap_y = random(20, TFTH - 60);
}

// ---------------
// game over
// ---------------
void game_over() {
  TFT.fillScreen(ST7735_BLACK);
  EEPROM_Read(&maxScore, 0);
  if (score > maxScore)
  {
    EEPROM_Write(&score, 0);
    maxScore = score;
    TFT.setTextColor(ST7735_RED);
    TFT.setTextSize(0);
    TFT.setCursor(4, 120);
    TFT.println("NIEUWE HOOGSTE SCORE");
  }
  TFT.setTextColor(ST7735_BLUE);
  TFT.setTextSize(2);
  // half width - num char * char width in pixels
  TFT.setCursor( TFTW2 - (5 * 6), TFTH2 - 30);
  TFT.setTextColor(ST7735_YELLOW);
  TFT.println("EINDE");
  TFT.setTextColor(ST7735_WHITE);
  TFT.setTextSize(0);
  TFT.setCursor( 40, TFTH2 - 50);
  TFT.print("SCORE: ");
  TFT.print(score);
  TFT.setCursor( TFTW2 - (15 * 3), TFTH2 + 12);
  TFT.println("DRUK OP DE KNOP");
  TFT.setCursor( 10, 130);
  TFT.print("MAXIMUM SCORE:");
  TFT.print(maxScore);
  while (1) {
    // wait for push button
    if ( !(PIND & (1 << PD2)) ) break;
  }
}

void EEPROM_Write(int *num, int MemPos)
{
  byte ByteArray[2];
  memcpy(ByteArray, num, 2);
  for (int x = 0; x < 2; x++)
  {
    EEPROM.write((MemPos * 2) + x, ByteArray[x]);
  }
}

void EEPROM_Read(int *num, int MemPos)
{
  byte ByteArray[2];
  for (int x = 0; x < 2; x++)
  {
    ByteArray[x] = EEPROM.read((MemPos * 2) + x);
  }
  memcpy(num, ByteArray, 2);
}

void resetMaxScore()
{
  EEPROM_Write(&maxScore, 0);
}
