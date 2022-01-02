#include <Arduino.h>
#include <Adafruit_NeoPixel.h> 

#define PIXELPIN 6
#define BUZZERPIN 11 

#define WIDTH 8
#define HEIGHT 16
#define NUMPIXELS (WIDTH * HEIGHT) 

Adafruit_NeoPixel pixels(NUMPIXELS, PIXELPIN, NEO_GRB + NEO_KHZ800);

#define NR_COLORS 6

const int g_colors[NR_COLORS + 2][3] =  { 
  {   0,  0,  0}, // color 0 is black (blank)
  { 255,255,255}, // color 1 is white 
  {   0,  0, 60}, // rest of the NR_COLORS are used for the blocks  
  {   0, 50,  0},
  {  50,  0,  0},
  {   0, 30, 60},
  {  30, 50,  0},
  {  30,  0, 30},
};

#define NR_SHAPES 7

// there are 7 different shapes, we put them in a constant array in four orientations 
// [shape_id][orientation][y][x]
const char g_shapes[NR_SHAPES][4][4][4] =  {  
  { 
    { { 1, 1, 1, 1}, { 0, 0, 0, 0}, { 0, 0, 0, 0}, { 0, 0, 0, 0}, }, 
    { { 0, 1, 0, 0}, { 0, 1, 0, 0}, { 0, 1, 0, 0}, { 0, 1, 0, 0}, }, 
    { { 1, 1, 1, 1}, { 0, 0, 0, 0}, { 0, 0, 0, 0}, { 0, 0, 0, 0}, }, 
    { { 0, 1, 0, 0}, { 0, 1, 0, 0}, { 0, 1, 0, 0}, { 0, 1, 0, 0}, }
  }, {   
    { { 0, 1, 1, 0}, { 0, 1, 1, 0}, { 0, 0, 0, 0}, { 0, 0, 0, 0}, }, 
    { { 0, 1, 1, 0}, { 0, 1, 1, 0}, { 0, 0, 0, 0}, { 0, 0, 0, 0}, }, 
    { { 0, 1, 1, 0}, { 0, 1, 1, 0}, { 0, 0, 0, 0}, { 0, 0, 0, 0}, }, 
    { { 0, 1, 1, 0}, { 0, 1, 1, 0}, { 0, 0, 0, 0}, { 0, 0, 0, 0}, },
  }, {
    { { 1, 1, 1, 0}, { 0, 1, 0, 0}, { 0, 0, 0, 0}, { 0, 0, 0, 0}, }, 
    { { 0, 0, 1, 0}, { 0, 1, 1, 0}, { 0, 0, 1, 0}, { 0, 0, 0, 0}, }, 
    { { 0, 1, 0, 0}, { 1, 1, 1, 0}, { 0, 0, 0, 0}, { 0, 0, 0, 0}, },
    { { 0, 1, 0, 0}, { 0, 1, 1, 0}, { 0, 1, 0, 0}, { 0, 0, 0, 0}, }, 
  }, {
    { { 1, 1, 1, 0}, { 1, 0, 0, 0}, { 0, 0, 0, 0}, { 0, 0, 0, 0}, },
    { { 0, 1, 1, 0}, { 0, 0, 1, 0}, { 0, 0, 1, 0}, { 0, 0, 0, 0}, },
    { { 0, 0, 1, 0}, { 1, 1, 1, 0}, { 0, 0, 0, 0}, { 0, 0, 0, 0}, },
    { { 0, 1, 0, 0}, { 0, 1, 0, 0}, { 0, 1, 1, 0}, { 0, 0, 0, 0}, },
  }, {
    { { 1, 1, 1, 0}, { 0, 0, 1, 0}, { 0, 0, 0, 0}, { 0, 0, 0, 0}, },
    { { 0, 0, 1, 0}, { 0, 0, 1, 0}, { 0, 1, 1, 0}, { 0, 0, 0, 0}, },
    { { 1, 0, 0, 0}, { 1, 1, 1, 0}, { 0, 0, 0, 0}, { 0, 0, 0, 0}, },
    { { 0, 1, 1, 0}, { 0, 1, 0, 0}, { 0, 1, 0, 0}, { 0, 0, 0, 0}, },
  }, { 
    { { 1, 1, 0, 0}, { 0, 1, 1, 0}, { 0, 0, 0, 0}, { 0, 0, 0, 0}, },
    { { 0, 0, 1, 0}, { 0, 1, 1, 0}, { 0, 1, 0, 0}, { 0, 0, 0, 0}, }, 
    { { 1, 1, 0, 0}, { 0, 1, 1, 0}, { 0, 0, 0, 0}, { 0, 0, 0, 0}, },
    { { 0, 0, 1, 0}, { 0, 1, 1, 0}, { 0, 1, 0, 0}, { 0, 0, 0, 0}, },
  }, {
    { { 0, 1, 1, 0}, { 1, 1, 0, 0}, { 0, 0, 0, 0}, { 0, 0, 0, 0}, },
    { { 0, 1, 0, 0}, { 0, 1, 1, 0}, { 0, 0, 1, 0}, { 0, 0, 0, 0}, },
    { { 0, 1, 1, 0}, { 1, 1, 0, 0}, { 0, 0, 0, 0}, { 0, 0, 0, 0}, },
    { { 0, 1, 0, 0}, { 0, 1, 1, 0}, { 0, 0, 1, 0}, { 0, 0, 0, 0}, },
  },
};

// position, color, orientation and shape_id of the block that is falling downs
int block_x;  
int block_y;  
int block_color = (NR_COLORS - 1);
int block_orientation;  
int block_shape;  

// the grid contains the pixels of the blocks that are already on the screen
// the falling block is not part of the grid 
char grid[HEIGHT][WIDTH];


// Buttons
#define BUTTON_LEFT 7
#define BUTTON_RIGHT 8
#define BUTTON_UP 10 
#define BUTTON_DOWN 9  

void setPixelRGB( int y, int x, int r, int g, int b) {
  if ((y >= 0) && (y < HEIGHT)) {
    pixels.setPixelColor(HEIGHT * WIDTH - 1 - y * WIDTH - x,r,g,b);
  }
}

// draws a pixel, using the RGB-table.   
// 0: black, 1: white, 2,3,4... are colors used for the blocks
void setPixel( int y, int x, int color) {
  int r = g_colors[color][0];   
  int g = g_colors[color][1];   
  int b = g_colors[color][2];   
  setPixelRGB(y,x,r,g,b); 
}

// flashclean - special effect used to clean up the screen
void flashclean() {
  const int tail = 5;
  // note that this actually does not work as originally intended... the neopixel library will disables timer interupts
  // so this sounds weird.   But I declared that to be a feature, not a bug. 
  tone(BUZZERPIN, 6000, 200);
  for (int y = 0 ; y < HEIGHT + 2; y++) {
    for (int x = 0; x < WIDTH; x++) {
       for (int t = 0; t < tail; t++)  {
          int c = 255>>t; 
          if (t == (tail-1)) {
            c = 0;
          }
          setPixelRGB(y-t,x,c,c,c);
       } 
    }
    pixels.show();
    delay(10);
  }
}

// set all pixels of the grid to black 
void clearGrid() { 
  for (int x = 0; x < WIDTH; x++) {
    for (int y = 0 ; y < HEIGHT; y++) {
      grid[y][x] = 0;
    }
  }
}

// initialize a random block, random color at the middle/top of the screen 
void initBlock() {
  block_x = WIDTH/2 - 2;
  block_y = 0; 
  block_color = ((block_color-1) % NR_COLORS) + 2;  // plus two because the first two are black and white
  block_shape = random(NR_SHAPES);
}

// helper function to see if the current block can be placed in position/orientation
bool canMoveTo(int x, int y, int orientation) {
  for (int dx =0; dx<4 ; dx++) { 
    for (int dy =0; dy<4 ; dy++) { 
      if (g_shapes[block_shape][orientation][dy][dx]) { 
        int xx = x + dx; 
        int yy = y + dy; 
        if ((xx < 0) || (xx >= WIDTH) || (yy < 0) || (yy >= HEIGHT)) {
          return false;
        }
        if (grid[yy][xx] != 0) {
          return false;
        }
      }
    }
  }
  return true;
}

bool canMoveDown() {
  return canMoveTo(block_x, block_y + 1, block_orientation);
}

bool canMoveLeft() {
  return canMoveTo(block_x - 1, block_y, block_orientation);
}

bool canMoveRight() {
  return canMoveTo(block_x + 1, block_y, block_orientation);
}

bool canRotate() {
  return canMoveTo(block_x, block_y, (block_orientation + 1) % 4);
}

// sometimes block can not rotate because they are stuck on the rigth/left
// so we try to see if we can rotate them afher shifting left
bool canRotateShift(int i) {
  return canMoveTo(block_x + i, block_y, (block_orientation + 1) % 4);
}

// put the block in current 
bool dropOnGrid() {
  for (int dx =0; dx<4 ; dx++) { 
    for (int dy =0; dy<4 ; dy++) { 
      if (g_shapes[block_shape][block_orientation][dy][dx]) { 
        int xx = block_x + dx; 
        int yy = block_y + dy; 
        grid[yy][xx] = block_color;
      }
    }
  }
  return false;
}

bool isFullLine(int y) {
  for (int x=0;  x < WIDTH; x++) {
     if (grid[y][x] < 2) { // zero means that this position is empty, one means hightlighted
       return false; 
     }
  }  
  return true; 
}

bool isHighlightedLine(int y) {
  for (int x=0;  x < WIDTH; x++) {
     if (grid[y][x] != 1) { // one means highlighted  
       return false; 
     }
  }  
  return true; 
}

void deleteLineFromGrid(int d) {
  for (int y = d;  y > 1; y--) {
    for (int x = 0;  x < WIDTH; x++) {
      grid[y][x] = grid[y-1][x]; // shift all lines down 
    }  
  }
} 

void drawGrid() {
  for (int x = 0; x < WIDTH; x++) {
    for (int y = 0; y < HEIGHT; y++) {
      setPixel(y,x,grid[y][x]); 
    }
  }
}

void drawBlockInColor(int color) {
  for (int dx =0; dx<4 ; dx++) { 
    for (int dy =0; dy<4 ; dy++) { 
      if (g_shapes[block_shape][block_orientation][dy][dx]) { 
        int xx = block_x + dx; 
        int yy = block_y + dy; 
        setPixel(yy,xx,color);
      }
    }
  }
}

void drawBlock() {
  drawBlockInColor(block_color);
}

void eraseBlock() {
  drawBlockInColor(0); // black
}

void highlightBlock() {
  drawBlockInColor(1); // bright white 
}

void highlightLine(int y) {
  for (int x =0; x < WIDTH ; x++) { 
     grid[y][x] = 1; // bright white
  }
}

void moveDown() {
  block_y += 1;
}

void moveLeft(int i = 1) {
  block_x -= i;
}

void moveRight(int i = 1) {
  block_x += i;
}

void rotate() {
  block_orientation = (block_orientation + 1) % 4;
}

void setup() {
  Serial.begin(9600);
  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.clear();
  randomSeed(analogRead(A1) + analogRead(A0));
  initBlock();
  pinMode(BUTTON_LEFT, INPUT_PULLUP); 
  pinMode(BUTTON_RIGHT, INPUT_PULLUP); 
  pinMode(BUTTON_UP, INPUT_PULLUP); 
  pinMode(BUTTON_DOWN, INPUT_PULLUP); 
  pinMode(BUZZERPIN, OUTPUT);
  // just some random beep to show we are awake! 
  tone(BUZZERPIN, 5000, 200);
  delay(300);
}


unsigned long previousMoveTime = 0;
unsigned long previousFullLineTime = 0; 

int previousLeft = 0;
int previousRight = 0;
int previousUp = 0;
int previousDown = 0;
unsigned int level = 300;  // start off with 300ms drops

int falling_down = false;

// this is some lazy way to debounce.    Since there is a lot of stuff in the loop, it kind of works. 
bool buttonPressed(int pin, int& state) {
  const int treshold = 4; 
  if (digitalRead(pin)) {  // button is up
      state = 0; 
      return false;
  } else { // button down
      state ++;
      return (state == treshold); // we only return the Pressed result once 
  }
}

bool buttonLeftPressed() {
  return buttonPressed(BUTTON_LEFT, previousLeft);
}

bool buttonRightPressed() {
  return buttonPressed(BUTTON_RIGHT, previousRight);
}

bool buttonUpPressed() {
  return buttonPressed(BUTTON_UP, previousUp);
}

bool buttonDownPressed() {
  return buttonPressed(BUTTON_DOWN, previousDown);
}

void loop() {
  eraseBlock();
  if (buttonLeftPressed() && canMoveLeft()) {
    moveLeft();
  }
  if (buttonRightPressed() && canMoveRight()) {
    moveRight();
  }
  if (buttonUpPressed()) {
    // first try to just rotate
    if (canRotate()) {
      rotate();
    } else {
      //  
      for (int i = 1; i > -3 ; i--) {
        if (canRotateShift(i)) {
          moveRight(i);
          rotate();
          break;
        }
      }
    } 
  }
  if (buttonDownPressed()) {
    falling_down = true;
  }

  if ((millis() - previousMoveTime >= level) || falling_down) {
    if (canMoveDown()) { 
      moveDown();
    } else {
      falling_down = false;
      dropOnGrid();
      drawGrid();
      if (block_y<4) {
        flashclean();   
        Serial.println("game over");
        // game over  
        clearGrid();
        drawGrid();
        // reset the interval back to 300ms 
        level = 300;
      }
      initBlock();  
    }
    previousMoveTime = millis(); 
  } 

  if (millis() - previousFullLineTime >= 50) {
    for (int y=0; y < HEIGHT; y++)  {
      if (isHighlightedLine(y)) {
        deleteLineFromGrid(y); 
        drawGrid();
      } 
    }
  }

  for (int y=0; y < HEIGHT; y++)  {
    if (isFullLine(y)) {
      highlightLine(y);
      drawGrid();
      previousFullLineTime = millis(); 
      tone(BUZZERPIN, 6000,5);
      // make blocks drop a little (2ms) faster 
      level = level - 2;
    } 
  }

  drawBlock(); 
  pixels.show();
}