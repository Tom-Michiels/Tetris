// Tom Michiels' Tetris (github.com/Tom-Michiels/Tetris), ported from the Arduino Nano to a Seeed XIAO ESP32-C3,
// plus a menu with snake, dodge, bricks and memory.
#include <Arduino.h>
#include <Adafruit_NeoPixel.h> 
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <WebServer.h>
#include <Preferences.h>
#include <WiFiManager.h>
#include <esp_sleep.h>
#include <driver/gpio.h>

// Wifi only for flashing over the air (Arduino IDE / arduino-cli: network port "tetris"), the game never waits for it.
// The network is remembered in flash. New location: after 20 s without wifi the board opens the access point
// "tetris-setup"; join it with a phone and pick the network. Hold up + down while powering on to forget the network.
#ifndef OTA_PASS
#define OTA_PASS "change-me"  // pick your own, or set it with build_flags = -DOTA_PASS=\"...\"
#endif
#define PORTAL_AFTER_MS 20000

// Battery: the XIAO charges a LiPo on its BAT pads but cannot measure it. Solder 2x 220k as a divider
// BAT+ -> A0 (D0) -> GND, then set BATTERY_SENSE to 1. Below BATTERY_EMPTY_MV the board sleeps to protect the cell.
#define BATTERY_SENSE 0
#define BATTERY_PIN A0
#define BATTERY_EMPTY_MV 3300
#define BATTERY_FULL_MV 4150

// No button pressed for this long: matrix off, deep sleep. Left or right wakes it (only GPIO0-5 can wake a C3).
#define IDLE_SLEEP_MS (10UL * 60 * 1000)

#define PIXELPIN D10   // was Nano D6
#define BUZZERPIN D1   // was Nano D11

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

// games, picked in the menu. Tetris score: 1/3/5/8 points for 1/2/3/4 lines at once.
// The best score of every game survives power-off (NVS)
#define NR_GAMES 5
const char* const GAME_NAMES[NR_GAMES] = {"tetris", "snake", "dodge", "bricks", "memory"};
int game = -1;  // -1 = menu
unsigned long score = 0, lines = 0, hiscore[NR_GAMES];
Preferences prefs;

const char* hiKey(int g) {  // tetris keeps the "hi" key it had before the other games existed
  static char key[4];
  snprintf(key, sizeof key, g ? "hi%d" : "hi", g);
  return key;
}


// Buttons
#define BUTTON_LEFT D2   // was Nano D7
#define BUTTON_RIGHT D3  // was Nano D8
#define BUTTON_UP D4     // was Nano D10
#define BUTTON_DOWN D5   // was Nano D9

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

// tone() queues every beep behind the previous one (the core plays them one after another, each for its full
// length), so quick presses sounded late. This drives the buzzer directly: a new beep replaces the old one at once.
#define BEEP_CHANNEL 0
unsigned long beepUntil = 0;

void beep(unsigned int freq, unsigned long ms) {
  ledcWriteTone(BEEP_CHANNEL, freq);
  beepUntil = millis() + ms;
}

void beepUpdate() {  // called every loop, ends the beep in time
  if (beepUntil && (long)(millis() - beepUntil) >= 0) {
    ledcWriteTone(BEEP_CHANNEL, 0);
    beepUntil = 0;
  }
}

// flashclean - special effect used to clean up the screen
void flashclean() {
  const int tail = 5;
  // note that this actually does not work as originally intended... the neopixel library will disables timer interupts
  // so this sounds weird.   But I declared that to be a feature, not a bug. 
  beep(6000, 200);
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

// 7-bag: every shape once per 7 blocks, in random order, so no long wait for the long bar
int bag[NR_SHAPES];
int bagLeft = 0;

int nextShape() {
  if (bagLeft == 0) {
    for (int i = 0; i < NR_SHAPES; i++) bag[i] = i;
    bagLeft = NR_SHAPES;
  }
  int i = random(bagLeft);
  int s = bag[i];
  bag[i] = bag[--bagLeft];
  return s;
}

// initialize a random block, random color at the middle/top of the screen
void initBlock() {
  block_x = WIDTH/2 - 2;
  block_y = 0;
  block_color = ((block_color-1) % NR_COLORS) + 2;  // plus two because the first two are black and white
  block_shape = nextShape();
  block_orientation = 0;
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
  for (int y = d;  y > 0; y--) {
    for (int x = 0;  x < WIDTH; x++) {
      grid[y][x] = grid[y-1][x]; // shift all lines down 
    }  
  }
  for (int x = 0;  x < WIDTH; x++) {
    grid[0][x] = 0;
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
  prefs.begin("tetris");
  for (int g = 0; g < NR_GAMES; g++) hiscore[g] = prefs.getULong(hiKey(g), 0);
  gpio_hold_dis((gpio_num_t)BUTTON_LEFT);  // held during deep sleep to keep the pullups
  gpio_hold_dis((gpio_num_t)BUTTON_RIGHT);
  pinMode(BUTTON_LEFT, INPUT_PULLUP);
  pinMode(BUTTON_RIGHT, INPUT_PULLUP);
  pinMode(BUTTON_UP, INPUT_PULLUP);
  pinMode(BUTTON_DOWN, INPUT_PULLUP);
  ledcSetup(BEEP_CHANNEL, 2000, 10);
  ledcAttachPin(BUZZERPIN, BEEP_CHANNEL);
  WiFi.mode(WIFI_STA);
  WiFi.setHostname("tetris");
  delay(5);  // let the pullups settle before reading the reset combo
  if (!digitalRead(BUTTON_UP) && !digitalRead(BUTTON_DOWN)) {
    WiFi.disconnect(false, true);  // forget the saved network, keep the radio on
    beep(2000, 500);
    delay(600);
  }
  WiFi.begin();  // saved network, if any
  // just some random beep to show we are awake! 
  beep(5000, 200);
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
// A pin that reads "pressed" for more than 3 s is treated as stuck (bad wire) and ignored until it is released,
// so one shorted button no longer pushes every block to the side.
const unsigned long STUCK_MS = 3000;
unsigned long lowSince[40];
unsigned long lastInput = 0;  // last button or web press, for the idle sleep
int batteryMv = 0;            // 0 = not measured

void goToSleep() {
  Serial.println("slapen");
  pixels.clear();
  pixels.show();
  gpio_hold_en((gpio_num_t)BUTTON_LEFT);
  gpio_hold_en((gpio_num_t)BUTTON_RIGHT);
  esp_deep_sleep_enable_gpio_wakeup((1ULL << BUTTON_LEFT) | (1ULL << BUTTON_RIGHT), ESP_GPIO_WAKEUP_GPIO_LOW);
  esp_deep_sleep_start();  // wakes as a fresh boot
}

void checkPower() {
  if (millis() - lastInput > IDLE_SLEEP_MS) goToSleep();
  static unsigned long checked = 0;
  if (!BATTERY_SENSE || (checked && millis() - checked < 10000)) return;
  checked = millis();
  uint32_t mv = 0;
  for (int i = 0; i < 16; i++) mv += analogReadMilliVolts(BATTERY_PIN);
  batteryMv = mv / 16 * 2;  // 1:2 divider
  if (batteryMv > 1000 && batteryMv < BATTERY_EMPTY_MV) goToSleep();  // < 1 V: no cell or divider, ignore
}

int batteryPercent() {
  return constrain(map(batteryMv, BATTERY_EMPTY_MV, BATTERY_FULL_MV, 0, 100), 0, 100);
}
unsigned long nLeft = 0, nRight = 0, nUp = 0, nDown = 0, nGlitch = 0;  // shown on http://tetris.local/

bool pinDown(int pin) {
  if (digitalRead(pin)) { lowSince[pin] = 0; return false; }
  if (!lowSince[pin]) lowSince[pin] = millis() | 1;
  return (long)(millis() - lowSince[pin]) < (long)STUCK_MS;  // signed: the |1 can put lowSince 1 ms ahead
}

bool buttonPressed(int pin, int& state, bool repeat = false) {
  const int treshold = 4;
  if (!pinDown(pin)) {  // button is up (or stuck)
      if (pin == BUTTON_LEFT && state > 0 && state < treshold) nGlitch++;
      state = 0;
      return false;
  } else { // button down
      state ++;
      // Pressed once, and when repeat is on: again every 15 loops after holding 50 loops
      // shortcut: counts loops (~4-5 ms each, pixels.show() dominates), switch to millis() if the loop gets slower
      bool p = (state == treshold) || (repeat && state > 50 && state % 15 == 0);
      if (p) lastInput = millis();
      return p;
  }
}

bool buttonLeftPressed() {
  bool p = buttonPressed(BUTTON_LEFT, previousLeft, true);
  nLeft += p;
  return p;
}

bool buttonRightPressed() {
  bool p = buttonPressed(BUTTON_RIGHT, previousRight, true);
  nRight += p;
  return p;
}

bool buttonUpPressed() {
  bool p = buttonPressed(BUTTON_UP, previousUp);
  nUp += p;
  return p;
}

bool buttonDownPressed() {
  bool p = buttonPressed(BUTTON_DOWN, previousDown);
  nDown += p;
  return p;
}

bool otaStarted = false;
WebServer web(80);

// http://tetris.local/ : live state of every button pin, for checking the wiring
void handleStatus() {
  const int pins[] = {BUTTON_LEFT, BUTTON_RIGHT, BUTTON_UP, BUTTON_DOWN};
  const char* names[] = {"links D2", "rechts D3", "boven D4", "onder D5"};
  String s = "{";
  for (int i = 0; i < 4; i++) {
    bool low = !digitalRead(pins[i]);
    bool stuck = low && lowSince[pins[i]] && millis() - lowSince[pins[i]] >= STUCK_MS;
    s += String(i ? "," : "") + "\"" + names[i] + "\":\"" + (stuck ? "VAST (genegeerd)" : low ? "ingedrukt" : "los") + "\"";
  }
  s += ",\"telling\":\"links " + String(nLeft) + " rechts " + String(nRight) + " boven " + String(nUp) +
       " onder " + String(nDown) + " ruis-links " + String(nGlitch) + " x " + String(block_x) + "\"";
  s += ",\"score\":" + String(score) + ",\"lijnen\":" + String(lines) + ",\"record\":" + String(game < 0 ? 0 : hiscore[game]) +
       ",\"spel\":\"" + (game < 0 ? "menu" : GAME_NAMES[game]) + "\"";
  if (batteryMv) s += ",\"batterij_mv\":" + String(batteryMv) + ",\"batterij_pct\":" + String(batteryPercent());
  web.send(200, "application/json", s + "}");
}

// http://tetris.local/play : the board in the browser, playable with the buttons or the arrow keys
bool webLeft = false, webRight = false, webUp = false, webDown = false, webMenu = false;

bool take(bool& flag) {
  bool v = flag;
  flag = false;
  return v;
}

void handleKey() {
  char b = web.arg("b")[0];
  if (b == 'l') webLeft = true;
  if (b == 'r') webRight = true;
  if (b == 'u') webUp = true;
  if (b == 'd') webDown = true;
  if (b == 'm') webMenu = true;
  lastInput = millis();
  web.send(204);
}

// what the LEDs show right now, as 128 hex colors row by row, plus the score of the game being played
void handleState() {
  String s = "{\"p\":\"";
  char hex[7];
  for (int y = 0; y < HEIGHT; y++)
    for (int x = 0; x < WIDTH; x++) {
      snprintf(hex, sizeof hex, "%06lx", (unsigned long)pixels.getPixelColor(HEIGHT * WIDTH - 1 - y * WIDTH - x));
      s += hex;
    }
  s += String("\",\"n\":\"") + (game < 0 ? "menu" : GAME_NAMES[game]) + "\",\"s\":" + String(score) +
       ",\"h\":" + String(game < 0 ? 0 : hiscore[game]) + "}";
  web.send(200, "application/json", s);
}

const char PLAY_PAGE[] = R"(<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no"><title>Games</title>
<style>body{margin:0;background:#111;color:#eee;font:16px system-ui;display:flex;flex-direction:column;align-items:center}
canvas{margin:12px;border:2px solid #444;height:60vh;image-rendering:pixelated}
#b{display:flex;gap:8px}button{font-size:28px;width:72px;height:64px;border-radius:12px;border:0;background:#333;color:#eee}</style></head>
<body><p id="s">...</p><canvas id="c" width="8" height="16"></canvas>
<div id="b"><button data-k="m">&#9776;</button><button data-k="l">&#9664;</button><button data-k="u">&#10227;</button><button data-k="d">&#9660;</button><button data-k="r">&#9654;</button></div>
<script>
const x=document.getElementById('c').getContext('2d');
const k=b=>fetch('/k?b='+b);
document.querySelectorAll('button').forEach(e=>e.onpointerdown=ev=>{ev.preventDefault();k(e.dataset.k)});
const A={ArrowLeft:'l',ArrowRight:'r',ArrowUp:'u',ArrowDown:'d'};
onkeydown=e=>{if(A[e.key]){e.preventDefault();k(A[e.key])}};
async function f(){try{const r=await(await fetch('/state')).json();
for(let i=0;i<128;i++){const c=[0,2,4].map(j=>Math.min(255,4*parseInt(r.p.substr(i*6+j,2),16)));x.fillStyle=`rgb(${c})`;x.fillRect(i%8,i>>3,1,1)}
document.getElementById('s').textContent=r.n=='menu'?'kies een spel':`${r.n} · score ${r.s} · record ${r.h}`}catch(e){}setTimeout(f,150)}
f();
</script></body></html>)";

WiFiManager wm;
bool portalOpen = false;

void handleOta() {
  if (WiFi.status() != WL_CONNECTED) {
    // never connected since boot: open the setup access point (the portal uses port 80 itself, so only before web.begin)
    if (!otaStarted && !portalOpen && millis() > PORTAL_AFTER_MS) {
      wm.setConfigPortalBlocking(false);
      wm.startConfigPortal("tetris-setup");
      portalOpen = true;
    }
    if (portalOpen) wm.process();
    return;
  }
  if (portalOpen) {  // connected (portal or saved network came back): free port 80
    wm.stopConfigPortal();
    WiFi.mode(WIFI_STA);
    portalOpen = false;
  }
  if (!otaStarted) {
    ArduinoOTA.setHostname("tetris");
    ArduinoOTA.setPassword(OTA_PASS);
    // green bar filling up from the bottom while flashing, instead of a frozen game
    ArduinoOTA.onStart([]() { pixels.clear(); pixels.show(); });
    ArduinoOTA.onProgress([](unsigned int done, unsigned int total) {
      static int lit = -1;
      int n = (uint64_t)done * NUMPIXELS / total;
      if (n == lit) return;
      lit = n;
      for (int i = 0; i < n; i++) setPixelRGB(HEIGHT - 1 - i / WIDTH, i % WIDTH, 0, 40, 0);
      pixels.show();
    });
    ArduinoOTA.begin();
    web.on("/", handleStatus);
    web.on("/play", []() { web.send(200, "text/html", PLAY_PAGE); });
    web.on("/state", handleState);
    web.on("/k", handleKey);
    web.begin();
    otaStarted = true;
    Serial.print("OTA klaar op ");
    Serial.println(WiFi.localIP());
  }
  ArduinoOTA.handle();
  web.handleClient();
}

// ---- input: every button is read exactly once per loop (the debounce counts loops), web presses count too
bool keyL, keyR, keyU, keyD;

void readKeys() {
  keyL = buttonLeftPressed() | take(webLeft);  // | not ||: always run the debounce
  keyR = buttonRightPressed() | take(webRight);
  keyU = buttonUpPressed() | take(webUp);
  keyD = buttonDownPressed() | take(webDown);
}

void startGame(int g);

void gameOver() {
  flashclean();
  Serial.println("game over");
  if (score > hiscore[game]) {
    hiscore[game] = score;
    prefs.putULong(hiKey(game), score);  // only on a new record, so no flash wear
  }
  startGame(game);
}

// ---- tetris

void tetrisStart() {
  clearGrid();
  drawGrid();
  // reset the interval back to 300ms 
  level = 300;
  falling_down = false;
  previousMoveTime = millis();
  initBlock();
}

void tetrisStep() {
  eraseBlock();
  if (keyL && canMoveLeft()) {
    moveLeft();
  }
  if (keyR && canMoveRight()) {
    moveRight();
  }
  if (keyU) {
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
  if (keyD) {
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
        gameOver();
        return;
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

  int cleared = 0;
  for (int y=0; y < HEIGHT; y++)  {
    if (isFullLine(y)) {
      highlightLine(y);
      drawGrid();
      previousFullLineTime = millis();
      beep(6000,5);
      // make blocks drop a little (2ms) faster
      if (level > 50) level = level - 2;  // unsigned: would wrap to ~49 days per drop
      cleared++;
    }
  }
  const int points[] = {0, 1, 3, 5, 8};
  score += points[cleared];
  lines += cleared;

  drawBlock(); 
}

// ---- snake: the four buttons steer, eat the red dot to grow, don't hit the wall or yourself

int snakeX[NUMPIXELS], snakeY[NUMPIXELS];  // [0] is the head
int snakeLen, snakeDx, snakeDy, snakeLastDx, snakeLastDy, foodX, foodY;
unsigned long snakeMoved;

bool onSnake(int x, int y, int n) {  // checks the first n segments
  for (int i = 0; i < n; i++) {
    if (snakeX[i] == x && snakeY[i] == y) return true;
  }
  return false;
}

void placeFood() {
  do {
    foodX = random(WIDTH);
    foodY = random(HEIGHT);
  } while (onSnake(foodX, foodY, snakeLen));
}

void snakeStart() {
  snakeLen = 3;
  for (int i = 0; i < snakeLen; i++) {
    snakeX[i] = WIDTH / 2;
    snakeY[i] = HEIGHT - 4 + i;
  }
  snakeDx = snakeLastDx = 0;
  snakeDy = snakeLastDy = -1;  // going up
  placeFood();
  snakeMoved = millis();
}

void snakeStep() {
  // compare with the last real move, so two quick presses can't turn the snake back into itself
  if (keyL && snakeLastDx != 1)  { snakeDx = -1; snakeDy = 0; }
  if (keyR && snakeLastDx != -1) { snakeDx = 1;  snakeDy = 0; }
  if (keyU && snakeLastDy != 1)  { snakeDx = 0;  snakeDy = -1; }
  if (keyD && snakeLastDy != -1) { snakeDx = 0;  snakeDy = 1; }

  if (millis() - snakeMoved >= (unsigned long)max(90, 250 - snakeLen * 5)) {
    snakeMoved = millis();
    snakeLastDx = snakeDx;
    snakeLastDy = snakeDy;
    int nx = snakeX[0] + snakeDx;
    int ny = snakeY[0] + snakeDy;
    bool eat = (nx == foodX && ny == foodY);
    // the tail moves out of the way this step, unless the snake grows
    if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT || onSnake(nx, ny, eat ? snakeLen : snakeLen - 1)) {
      gameOver();
      return;
    }
    if (eat) {
      snakeLen++;
      score++;
      beep(3000, 20);
    }
    for (int i = snakeLen - 1; i > 0; i--) {
      snakeX[i] = snakeX[i - 1];
      snakeY[i] = snakeY[i - 1];
    }
    snakeX[0] = nx;
    snakeY[0] = ny;
    if (eat) {
      if (snakeLen == NUMPIXELS) {  // board full: nothing left to eat
        gameOver();
        return;
      }
      placeFood();
    }
  }

  pixels.clear();
  for (int i = snakeLen - 1; i >= 0; i--) setPixelRGB(snakeY[i], snakeX[i], 0, i ? 30 : 90, 0);
  setPixelRGB(foodY, foodX, 80, 0, 0);
}

// ---- dodge: walls come down, steer left/right through the gaps. Hold down to go faster

int carX, gapX, dodgeRows;
unsigned long dodgeMoved;

void dodgeStart() {
  clearGrid();
  carX = WIDTH / 2;
  gapX = WIDTH / 2 - 1;
  dodgeRows = 0;
  dodgeMoved = millis();
}

void dodgeStep() {
  if (keyL && carX > 0) carX--;
  if (keyR && carX < WIDTH - 1) carX++;

  unsigned long speed = max(80, 250 - (int)score * 4);
  if (previousDown > 0) speed /= 3;  // down held
  if (millis() - dodgeMoved >= speed) {
    dodgeMoved = millis();
    bool wallLeaving = false;
    for (int x = 0; x < WIDTH; x++) wallLeaving |= grid[HEIGHT - 1][x] != 0;
    for (int y = HEIGHT - 1; y > 0; y--) {
      for (int x = 0; x < WIDTH; x++) grid[y][x] = grid[y - 1][x];
    }
    for (int x = 0; x < WIDTH; x++) grid[0][x] = 0;
    // a wall every 5 rows with a 3 wide gap, at most 2 steps away from the last one so it can always be reached
    if (++dodgeRows % 5 == 0) {
      gapX = constrain(gapX + (int)random(5) - 2, 0, WIDTH - 3);
      for (int x = 0; x < WIDTH; x++) grid[0][x] = (x < gapX || x >= gapX + 3) ? 4 : 0;
    }
    if (wallLeaving) {
      score++;
      beep(4000, 5);
    }
  }
  if (grid[HEIGHT - 1][carX] || grid[HEIGHT - 2][carX]) {
    gameOver();
    return;
  }

  drawGrid();
  setPixelRGB(HEIGHT - 2, carX, 0, 40, 80);
  setPixelRGB(HEIGHT - 1, carX, 0, 40, 80);
}

// ---- bricks: keep the ball up with the 3 wide paddle and clear all bricks. Up or down launches the ball

int padX, ballX, ballY, ballDx, ballDy;
bool ballLaunched;
unsigned long ballMoved;
unsigned int ballSpeed;

void fillBricks() {
  clearGrid();
  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < WIDTH; x++) grid[y][x] = 2 + y;
  }
  ballLaunched = false;
}

void bricksStart() {
  fillBricks();
  padX = WIDTH / 2 - 1;
  ballSpeed = 160;
}

void bricksStep() {
  if (keyL && padX > 0) padX--;
  if (keyR && padX < WIDTH - 3) padX++;

  if (!ballLaunched) {  // ball rides on the paddle
    ballX = padX + 1;
    ballY = HEIGHT - 2;
    ballDx = random(2) ? 1 : -1;
    ballDy = -1;
    if (keyU || keyD) {
      ballLaunched = true;
      ballMoved = millis();
    }
  } else if (millis() - ballMoved >= ballSpeed) {
    ballMoved = millis();
    if (ballX + ballDx < 0 || ballX + ballDx >= WIDTH) ballDx = -ballDx;
    if (ballY + ballDy < 0) ballDy = -ballDy;
    if (ballY + ballDy == HEIGHT - 1) {  // reaching the paddle row
      int hit = ballX + ballDx - padX;
      if (hit < -1 || hit > 3) {  // one pixel of mercy past each end
        gameOver();
        return;
      }
      ballDy = -1;
      // the ends steer the ball, the middle sends it straight up. Only diagonal moves would keep x + y even or odd
      // forever, and then half of the bricks could never be hit
      ballDx = hit < 1 ? -1 : hit > 1 ? 1 : 0;
      if (ballX + ballDx < 0 || ballX + ballDx >= WIDTH) ballDx = -ballDx;
      beep(2000, 5);
    }
    int nx = ballX + ballDx;
    int ny = ballY + ballDy;
    if (grid[ny][nx]) {  // brick: break it and bounce back
      grid[ny][nx] = 0;
      score++;
      ballDy = -ballDy;
      beep(5000, 5);
    } else {
      ballX = nx;
      ballY = ny;
    }
    bool left = false;
    for (int y = 0; y < 4; y++) {
      for (int x = 0; x < WIDTH; x++) left |= grid[y][x] != 0;
    }
    if (!left) {  // cleared: new wall, a bit faster
      fillBricks();
      if (ballSpeed > 70) ballSpeed -= 20;
    }
  }

  drawGrid();
  for (int x = padX; x < padX + 3; x++) setPixelRGB(HEIGHT - 1, x, 0, 40, 80);
  setPixelRGB(ballY, ballX, 120, 120, 120);
}

// ---- memory: watch the zones light up, then press the same buttons in the same order.
// The zones sit like the buttons: up = top, left / right = the two middle halves, down = bottom

const int ZONE_RGB[4][3] = {{0, 0, 60}, {0, 50, 0}, {50, 0, 0}, {40, 30, 0}};  // up, left, right, down
const int ZONE_TONE[4] = {440, 554, 659, 880};
uint8_t seq[64];
int seqLen, seqPos, memLit;
bool memShowing;
unsigned long memTime, memLitUntil;

bool inZone(int z, int y, int x) {
  if (z == 0) return y < 5;
  if (z == 3) return y >= HEIGHT - 5;
  return y >= 5 && y < HEIGHT - 5 && (z == 1 ? x < WIDTH / 2 : x >= WIDTH / 2);
}

void memAddStep() {
  seq[seqLen++] = random(4);
  memShowing = true;
  memTime = millis() + 600;  // short pause before the sequence plays
}

void memoryStart() {
  seqLen = 0;
  memLit = -1;
  memAddStep();
}

void memoryStep() {
  // the sequence plays faster as it gets longer
  const unsigned long STEP_MS = max(300, 500 - seqLen * 15), ON_MS = STEP_MS * 3 / 4;
  if (memShowing) {
    long t = (long)(millis() - memTime);
    if (t < 0 && memLit >= 0 && millis() > memLitUntil) memLit = -1;  // the last press fades during the pause
    if (t >= 0) {
      int i = t / STEP_MS;
      if (i >= seqLen) {
        memShowing = false;
        seqPos = 0;
        memLit = -1;
      } else {
        int lit = (t % STEP_MS < ON_MS) ? seq[i] : -1;
        if (lit >= 0 && memLit < 0) beep(ZONE_TONE[lit], ON_MS);
        memLit = lit;
      }
    }
  } else {
    int z = keyU ? 0 : keyL ? 1 : keyR ? 2 : keyD ? 3 : -1;
    if (z >= 0) {
      if (z != seq[seqPos]) {
        gameOver();
        return;
      }
      memLit = z;
      memLitUntil = millis() + 200;
      beep(ZONE_TONE[z], 150);
      if (++seqPos == seqLen) {
        score = seqLen;
        if (seqLen == (int)sizeof seq) {  // remembered all of it
          gameOver();
          return;
        }
        memAddStep();
      }
    }
    if (memLit >= 0 && millis() > memLitUntil) memLit = -1;
  }

  for (int y = 0; y < HEIGHT; y++) {
    for (int x = 0; x < WIDTH; x++) {
      for (int z = 0; z < 4; z++) {
        if (!inZone(z, y, x)) continue;
        int k = (z == memLit) ? 4 : 1;
        const int* c = ZONE_RGB[z];
        setPixelRGB(y, x, min(255, c[0] * k / 3), min(255, c[1] * k / 3), min(255, c[2] * k / 3));
      }
    }
  }
}

// ---- menu: left / right picks a game, up or down starts it. Hold up + down in a game to come back here

const uint8_t ICONS[NR_GAMES][8] = {  // 8x8, bit 7 is the left column
  {0x00, 0x70, 0x20, 0x00, 0x06, 0x0C, 0xC0, 0xE6},  // tetris
  {0x00, 0x7C, 0x04, 0x3C, 0x20, 0x3E, 0x00, 0x48},  // snake
  {0xE7, 0x00, 0x00, 0x00, 0xC7, 0x00, 0x08, 0x08},  // dodge
  {0xFF, 0xFF, 0x00, 0x00, 0x10, 0x00, 0x00, 0x38},  // bricks
  {0x3C, 0x3C, 0x00, 0xE7, 0xE7, 0x00, 0x3C, 0x3C},  // memory
};
int menuPick = 0;

void menuStep() {
  if (keyL) menuPick = (menuPick + NR_GAMES - 1) % NR_GAMES;
  if (keyR) menuPick = (menuPick + 1) % NR_GAMES;
  if (keyL || keyR) beep(3000, 10);
  if (keyU || keyD) {
    startGame(menuPick);
    return;
  }
  pixels.clear();
  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < WIDTH; x++) {
      if (ICONS[menuPick][y] & (0x80 >> x)) setPixel(y + 1, x, 2 + menuPick % NR_COLORS);
    }
  }
  for (int i = 0; i < NR_GAMES; i++) setPixel(HEIGHT - 3, (WIDTH - NR_GAMES) / 2 + i, i == menuPick ? 1 : 2);
}

void startGame(int g) {
  game = g;
  score = 0;
  lines = 0;
  pixels.clear();
  switch (game) {
    case 0: tetrisStart(); break;
    case 1: snakeStart(); break;
    case 2: dodgeStart(); break;
    case 3: bricksStart(); break;
    case 4: memoryStart(); break;
  }
}

void loop() {
  beepUpdate();
  handleOta();
  checkPower();
  readKeys();
  bool toMenu = take(webMenu) | (previousUp > 150 && previousDown > 150);  // ~0.7 s holding both
  if (game >= 0 && toMenu) {
    game = -1;
    beep(1500, 50);
  }
  switch (game) {
    case -1: menuStep(); break;
    case 0: tetrisStep(); break;
    case 1: snakeStep(); break;
    case 2: dodgeStep(); break;
    case 3: bricksStep(); break;
    case 4: memoryStep(); break;
  }
  pixels.show();
}
