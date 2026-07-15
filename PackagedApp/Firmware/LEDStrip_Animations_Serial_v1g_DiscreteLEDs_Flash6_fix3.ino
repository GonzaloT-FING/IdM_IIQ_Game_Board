/*
  LEDStrip_Animations_Serial_v1g_IDLE_Violet_Keyboard_DiscreteLEDs.ino
  --------------------------------------------------------------------
  Adds 4 discrete LEDs (via 2N2222) that react to app commands:
    - Default: ALL FOUR ON
    - While TF question is active (after MODE:TF, before ANS:*): only LED1 & LED2 ON (LED3/LED4 OFF)
    - On any answer (CORRECT/WRONG) or switching to a non-TF mode: ALL ON again

  Also includes:
    - LED strip animations + idle groups (with Violet 'V')
    - 4-button USB keyboard on pins 2,3,4,5 typing '1','2','3','4'
    - IDLE runtime config via Serial

  Board: Pro Micro / Leonardo (ATmega32U4)
*/

#include <Adafruit_NeoPixel.h>
#include <Keyboard.h>
#include <math.h> // cos()

// ====== USER SETTINGS ======
// NeoPixel strip
#define DATA_PIN      6
#define NUM_LEDS      213
#define BRIGHTNESS    255     //de 0 a 255
#define PIXEL_TYPE    NEO_GRB   // change to NEO_GRBW if RGBW
#define DUR_MODE_MS   3000
#define DUR_PULSE_MS  3000
#define DUR_WRONG_MS  3000
#define FLASH_COUNT   6    // number of flashes for WRONG animation
#define FLASH_WIDTH   0.1 // pulse width around each flash center (0..1 of total time)
#define FRAME_MS      16  // ~60 FPS

// Button keyboard
static const uint8_t BTN_PINS[4] = {2,3,4,5};
static const uint8_t BTN_KEYS[4] = {'1','2','3','4'};
#define DEBOUNCE_MS   25

// Discrete indicator LEDs (via 2N2222). Change pins if needed.
static const uint8_t DISC_PINS[4] = {8,9,10,11};
static const bool    LED_ACTIVE_HIGH = true; // If HIGH turns LED ON through transistor, keep true. Invert if needed.

// Trails (TF)
#define TRAILS_COUNT  3
#define TRAIL_TAIL    12
#define TRAIL_MIN_SPD 0.8f
#define TRAIL_MAX_SPD 1.6f

// Twinkle (FORTUNE)
#define TWINKLE_FADE  230
#define TWINKLE_BURST 4
#define TWINKLE_TICK  25
// ===========================

Adafruit_NeoPixel strip(NUM_LEDS, DATA_PIN, PIXEL_TYPE + NEO_KHZ800);

#if (PIXEL_TYPE == NEO_GRBW)
  #define COLOR4(r,g,b,w) strip.Color((r),(g),(b),(w))
#else
  #define COLOR4(r,g,b,w) strip.Color((r),(g),(b))
#endif

const uint32_t COL_BLUE   = COLOR4(0,   0, 255, 0);
const uint32_t COL_GREEN  = COLOR4(0, 255,   0, 0);
const uint32_t COL_RED    = COLOR4(255, 0,   0, 0);
const uint32_t COL_YELLOW = COLOR4(255,180,  0, 0);
const uint32_t COL_VIOLET = COLOR4(180,  0, 255, 0);
const uint32_t COL_WHITE  = COLOR4(255,255,255,0);

// -------- Idle configuration --------
#define IDLE_MAX_GROUPS 64
static uint8_t IDLE_DEFAULT_GROUPS[] = {14,8,12,12,11,12,11,11,11,12,12,12,11,8,17,7,8,9,8,7}; // 12*5 = 60
static char    IDLE_DEFAULT_COLORS[] = {'V','B','G','Y','R','B','Y','G','B','R','Y','R','B','Y','V','G','B','R','Y','G'};

bool     idleEnabled   = true;
uint8_t  idleCount     = 0;
uint16_t idleLen[IDLE_MAX_GROUPS];
char     idleCol[IDLE_MAX_GROUPS];

// -------- Browser-authoritative board snapshot --------
#define BOARD_MAX_PLAYERS 6
bool boardStateActive = false;
uint16_t boardTurn = 0;
uint8_t boardActivePlayer = 0;
uint8_t boardPlayerCount = 0;
uint8_t boardPlayerPos[BOARD_MAX_PLAYERS];
char boardPlayerBranch[BOARD_MAX_PLAYERS];
uint32_t boardPlayerColor[BOARD_MAX_PLAYERS];
String boardPhase;
uint8_t boardHintCount = 0;
uint8_t boardHintPos[2];

// -------- Animation system --------
enum AnimType : uint8_t { ANIM_NONE, ANIM_PULSE, ANIM_TWINKLE, ANIM_FLASH3, ANIM_TRAILS };
struct Anim {
  AnimType type;
  uint32_t color;
  unsigned long start;
  uint16_t duration;
  unsigned long lastTick;
};
Anim anim = { ANIM_NONE, 0, 0, 0, 0 };

unsigned long lastFrame = 0;

// Trails state
float trailPos[TRAILS_COUNT];
float trailVel[TRAILS_COUNT];

// Keyboard state
struct BtnState { bool last; unsigned long lastChange; };
BtnState btn[4];
bool keyboardEnabled = true;

// Track TF state for discrete LEDs
bool tfActive = false;

// -------- Utils --------
static inline uint8_t clamp8(int v){ return (v<0)?0:((v>255)?255:v); }
static inline void   setAll(uint32_t c){ for(uint16_t i=0;i<NUM_LEDS;i++) strip.setPixelColor(i,c); }
static inline void   clearAll(){ strip.clear(); }
uint32_t scaleColor(uint32_t c, uint8_t s){
  uint8_t r = (c >> 16) & 0xFF;
  uint8_t g = (c >>  8) & 0xFF;
  uint8_t b = (c      ) & 0xFF;
  r = (uint8_t)((uint16_t)r * s / 255);
  g = (uint8_t)((uint16_t)g * s / 255);
  b = (uint8_t)((uint16_t)b * s / 255);
  #if (PIXEL_TYPE == NEO_GRBW)
    return strip.Color(r,g,b,0);
  #else
    return strip.Color(r,g,b);
  #endif
}

uint32_t colorFromChar(char c){
  switch(toupper(c)){
    case 'B': return COL_BLUE;
    case 'G': return COL_GREEN;
    case 'R': return COL_RED;
    case 'Y': return COL_YELLOW;
    case 'V': return COL_VIOLET;
    default:  return COLOR4(25,25,25,0);
  }
}

// -------- Discrete LEDs control --------
inline void discWrite(uint8_t pin, bool on){
  if (LED_ACTIVE_HIGH) digitalWrite(pin, on ? HIGH : LOW);
  else                 digitalWrite(pin, on ? LOW  : HIGH);
}
void discAll(bool on){
  for (uint8_t i=0;i<4;i++) discWrite(DISC_PINS[i], on);
}
void discUpdate(){
  if (tfActive){
    // Only 1 & 2 ON, 3 & 4 OFF
    discWrite(DISC_PINS[0], true);
    discWrite(DISC_PINS[1], true);
    discWrite(DISC_PINS[2], false);
    discWrite(DISC_PINS[3], false);
  } else {
    // All ON
    discAll(true);
  }
}

// -------- Idle helpers --------
void idleResetToDefaults(){
  const uint8_t ndef = sizeof(IDLE_DEFAULT_GROUPS)/sizeof(IDLE_DEFAULT_GROUPS[0]);
  idleCount = (ndef > IDLE_MAX_GROUPS) ? IDLE_MAX_GROUPS : ndef;
  for(uint8_t i=0;i<idleCount;i++){
    idleLen[i] = IDLE_DEFAULT_GROUPS[i];
    idleCol[i] = IDLE_DEFAULT_COLORS[i % (sizeof(IDLE_DEFAULT_COLORS)/sizeof(IDLE_DEFAULT_COLORS[0]))];
  }
}

void renderIdle(){
  if(!idleEnabled || idleCount==0){ strip.clear(); return; }
  uint16_t p=0;
  for(uint8_t g=0; g<idleCount; g++){
    uint32_t c = colorFromChar(idleCol[g]);
    uint16_t len = idleLen[g];
    for(uint16_t k=0; k<len && p<NUM_LEDS; k++, p++){
      strip.setPixelColor(p, c);
    }
    if (p>=NUM_LEDS) break;
  }
  for(; p<NUM_LEDS; p++) strip.setPixelColor(p, 0);
}

void renderBoardState(){
  strip.clear();
  uint16_t first = 0;
  bool whitePhase = ((millis() / 350UL) % 2UL) == 0;
  for(uint8_t g=0; g<idleCount && g<20; g++){
    uint16_t len = idleLen[g];
    bool hinted = false;
    for(uint8_t h=0; h<boardHintCount; h++) if(boardHintPos[h] == g+1) hinted = true;
    uint32_t color = (hinted && whitePhase) ? COL_WHITE : colorFromChar(idleCol[g]);
    for(uint16_t k=0; k<len && first+k<NUM_LEDS; k++){
      strip.setPixelColor(first+k, color);
    }
    first += len;
    if(first >= NUM_LEDS) break;
  }
  strip.show();
}

// Start an animation (uint8_t to avoid prototype ordering issues)
void startAnim(uint8_t t, uint32_t color, uint16_t duration){
  anim.type = (AnimType)t;
  anim.color = color;
  anim.duration = duration;
  anim.start = millis();
  anim.lastTick = anim.start;

  if (anim.type == ANIM_TRAILS){
    for (int i=0;i<TRAILS_COUNT;i++){
      trailPos[i] = (float)(random(NUM_LEDS));
      float dir = (random(2)==0) ? 1.0f : -1.0f;
      float spd = TRAIL_MIN_SPD + (float)random(1000)/1000.0f*(TRAIL_MAX_SPD-TRAIL_MIN_SPD);
      trailVel[i] = dir * spd;
    }
  }
}

// --- Renderers ---
void renderPulse(unsigned long now){
  unsigned long dt = now - anim.start;
  if (dt >= anim.duration){ anim.type = ANIM_NONE; clearAll(); return; }
  float t = (float)dt / (float)anim.duration; // 0..1
  float s = 0.5f - 0.5f * (float)cos(2.0 * 3.14159265358979323846 * (double)t); // smooth in/out
  uint8_t scale = (uint8_t)(s * 255.0f);
  setAll(scaleColor(anim.color, scale));
}

void renderTrails(unsigned long now){
  unsigned long dt = now - anim.start;
  if (dt >= anim.duration){ anim.type = ANIM_NONE; clearAll(); return; }
  // Fade trail
  for(uint16_t i=0;i<NUM_LEDS;i++){
    uint32_t c = strip.getPixelColor(i);
    uint8_t r=(c>>16)&0xFF, g=(c>>8)&0xFF, b=c&0xFF;
    r = (uint8_t)((uint16_t)r * 235 / 255);
    g = (uint8_t)((uint16_t)g * 235 / 255);
    b = (uint8_t)((uint16_t)b * 235 / 255);
    strip.setPixelColor(i, strip.Color(r,g,b));
  }
  // Advance and draw heads
  for(int t=0; t<TRAILS_COUNT; t++){
    trailPos[t] += trailVel[t];
    while (trailPos[t] < 0) trailPos[t] += NUM_LEDS;
    while (trailPos[t] >= NUM_LEDS) trailPos[t] -= NUM_LEDS;
    int head = (int)trailPos[t];
    for(int k=0;k<=TRAIL_TAIL;k++){
      int idx = head - k;
      while (idx < 0) idx += NUM_LEDS;
      uint8_t s = clamp8(255 - (k * (255/(TRAIL_TAIL+1))));
      strip.setPixelColor((uint16_t)idx, scaleColor(anim.color, s));
    }
  }
}

void renderTwinkle(unsigned long now){
  unsigned long dt = now - anim.start;
  if (dt >= anim.duration){ anim.type = ANIM_NONE; clearAll(); return; }
  // Fade all
  for(uint16_t i=0;i<NUM_LEDS;i++){
    uint32_t c = strip.getPixelColor(i);
    uint8_t r=(c>>16)&0xFF, g=(c>>8)&0xFF, b=c&0xFF;
    r = (uint8_t)((uint16_t)r * TWINKLE_FADE / 255);
    g = (uint8_t)((uint16_t)g * TWINKLE_FADE / 255);
    b = (uint8_t)((uint16_t)b * TWINKLE_FADE / 255);
    strip.setPixelColor(i, strip.Color(r,g,b));
  }
  if (now - anim.lastTick >= TWINKLE_TICK){
    anim.lastTick = now;
    for (int n=0;n<TWINKLE_BURST;n++){
      uint16_t idx = random(NUM_LEDS);
      strip.setPixelColor(idx, scaleColor(anim.color, 255));
    }
  }
}

static inline float fabsf_local(float x){ return x >= 0 ? x : -x; }
void renderFlash3(unsigned long now){
  unsigned long dt = now - anim.start;
  if (dt >= anim.duration){ anim.type = ANIM_NONE; clearAll(); return; }
  float f = (float)dt / (float)anim.duration; // 0..1

  bool on = false;
  for (int i = 1; i <= FLASH_COUNT; i++) {
    float center = (float)i / (FLASH_COUNT + 1);
    if (fabsf_local(f - center) < FLASH_WIDTH) { on = true; break; }
  }

  if (on) setAll(anim.color);
  else    clearAll();
}
void renderAnim(){
  unsigned long now = millis();
  if (now - lastFrame < FRAME_MS) return;
  lastFrame = now;

  if (anim.type == ANIM_NONE){
    if (boardStateActive){
      renderBoardState();
    }else if (idleEnabled){
      renderIdle();
      strip.show();
    }else{
      strip.clear();
      strip.show();
    }
    return;
  }

  switch(anim.type){
    case ANIM_PULSE:   renderPulse(now);   break;
    case ANIM_TWINKLE: renderTwinkle(now); break;
    case ANIM_FLASH3:  renderFlash3(now);  break;
    case ANIM_TRAILS:  renderTrails(now);  break;
    default: break;
  }
  strip.show();
}

// -------- Button Keyboard --------
void initButtons(){
  for (uint8_t i=0;i<4;i++){
    pinMode(BTN_PINS[i], INPUT_PULLUP);
    btn[i].last = (digitalRead(BTN_PINS[i]) == LOW); // pressed?
    btn[i].lastChange = millis();
  }
  Keyboard.begin();
}

void pollButtons(){
  unsigned long now = millis();
  for (uint8_t i=0;i<4;i++){
    bool cur = (digitalRead(BTN_PINS[i]) == LOW); // active low
    if (cur != btn[i].last){
      if (now - btn[i].lastChange >= DEBOUNCE_MS){
        btn[i].lastChange = now;
        btn[i].last = cur;
        if (cur){ // pressed edge
          if (keyboardEnabled){
            Keyboard.write(BTN_KEYS[i]); // one-shot keypress
          }
        }
      }
    }
  }
}

// -------- Serial parsing (commands) --------
String line;

bool parseIdleGroupsCSV(const String& csv){
  uint8_t cnt=0;
  int start=0;
  while (start < (int)csv.length() && cnt < IDLE_MAX_GROUPS){
    int comma = csv.indexOf(',', start);
    String tok = (comma>=0) ? csv.substring(start, comma) : csv.substring(start);
    tok.trim();
    if (tok.length()){
      int val = tok.toInt();
      if (val<=0) val=0;
      idleLen[cnt++] = (uint16_t)val;
    }
    if (comma<0) break;
    start = comma+1;
  }
  if (cnt==0) return false;
  idleCount = cnt;
  return true;
}
bool parseIdleColorsCSV(const String& csv){
  uint8_t cnt=0;
  int start=0;
  while (start < (int)csv.length() && cnt < IDLE_MAX_GROUPS){
    int comma = csv.indexOf(',', start);
    String tok = (comma>=0) ? csv.substring(start, comma) : csv.substring(start);
    tok.trim(); tok.toUpperCase();
    char c = 0;
    if (tok.length()==0){ /*skip*/ }
    else if (tok.startsWith("B")) c='B';
    else if (tok.startsWith("G")) c='G';
    else if (tok.startsWith("R")) c='R';
    else if (tok.startsWith("Y")) c='Y';
    else if (tok.startsWith("V")) c='V';
    else c = 'B';
    if (c) idleCol[cnt++] = c;
    if (comma<0) break;
    start = comma+1;
  }
  if (cnt==0) return false;
  for(uint8_t i=0;i<idleCount;i++){
    idleCol[i] = idleCol[i % cnt];
  }
  return true;
}

uint32_t parseHexColor(String hex){
  hex.trim();
  if(hex.startsWith("#")) hex = hex.substring(1);
  if(hex.length() != 6) return COL_VIOLET;
  unsigned long rgb = strtoul(hex.c_str(), NULL, 16);
  return COLOR4((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF, 0);
}

void parseBoardPlayers(const String& csv){
  boardPlayerCount = 0;
  int start = 0;
  while(start < (int)csv.length() && boardPlayerCount < BOARD_MAX_PLAYERS){
    int comma = csv.indexOf(',', start);
    String token = (comma >= 0) ? csv.substring(start, comma) : csv.substring(start);
    int a = token.indexOf('@');
    int b = (a >= 0) ? token.indexOf('@', a+1) : -1;
    int c = (b >= 0) ? token.indexOf('@', b+1) : -1;
    if(a >= 0 && b >= 0 && c >= 0){
      uint8_t idx = boardPlayerCount++;
      boardPlayerPos[idx] = (uint8_t)constrain(token.substring(a+1, b).toInt(), 1, 20);
      String branch = token.substring(b+1, c); branch.toUpperCase();
      boardPlayerBranch[idx] = branch.length() ? branch.charAt(0) : 'N';
      boardPlayerColor[idx] = parseHexColor(token.substring(c+1));
    }
    if(comma < 0) break;
    start = comma+1;
  }
}

void parseBoardHints(const String& csv){
  boardHintCount = 0;
  int start = 0;
  while(start < (int)csv.length() && boardHintCount < 2){
    int comma = csv.indexOf(',', start);
    String token = (comma >= 0) ? csv.substring(start, comma) : csv.substring(start);
    token.trim();
    int pos = token.toInt();
    if(pos >= 1 && pos <= 20) boardHintPos[boardHintCount++] = (uint8_t)pos;
    if(comma < 0) break;
    start = comma+1;
  }
}

bool parseBoardState(const String& payload){
  boardHintCount = 0;
  int start = 0;
  while(start < (int)payload.length()){
    int semi = payload.indexOf(';', start);
    String field = (semi >= 0) ? payload.substring(start, semi) : payload.substring(start);
    field.trim();
    String upper = field; upper.toUpperCase();
    if(upper.startsWith("T=")) boardTurn = (uint16_t)field.substring(2).toInt();
    else if(upper.startsWith("A=")) boardActivePlayer = (uint8_t)field.substring(2).toInt();
    else if(upper.startsWith("S=")) boardPhase = field.substring(2);
    else if(upper.startsWith("H=")) parseBoardHints(field.substring(2));
    else if(upper.startsWith("P=")) parseBoardPlayers(field.substring(2));
    if(semi < 0) break;
    start = semi+1;
  }
  if(boardPlayerCount == 0) return false;
  if(boardActivePlayer >= boardPlayerCount) boardActivePlayer = 0;
  boardStateActive = true;
  return true;
}

void handleCmd(String s){
  s.trim(); if (!s.length()) return;
  String u = s; u.toUpperCase();

  // Full browser-owned game snapshot:
  // BOARD:T=3;A=1;S=move-preview;H=9;P=1@4@S@2563EB,2@10@L@BE123C
  if(u == "BOARD:CLEAR") { boardStateActive=false; return; }
  if(u.startsWith("BOARD:")) { parseBoardState(s.substring(6)); return; }

  // Idle controls
  if (u == "IDLE:ON")  { idleEnabled=true;  return; }
  if (u == "IDLE:OFF") { idleEnabled=false; return; }
  if (u == "IDLE:RESET"){ idleResetToDefaults(); return; }

  if (u.startsWith("IDLE:GROUPS=")){
    String csv = s.substring(String("IDLE:GROUPS=").length());
    csv.trim();
    if (parseIdleGroupsCSV(csv)) { /* applied */ }
    return;
  }
  if (u.startsWith("IDLE:COLORS=")){
    String csv = s.substring(String("IDLE:COLORS=").length());
    csv.trim();
    if (parseIdleColorsCSV(csv)) { /* applied */ }
    return;
  }

  // Keyboard toggle
  if (u == "KEY:ON")  { keyboardEnabled = true;  return; }
  if (u == "KEY:OFF") { keyboardEnabled = false; return; }

  // Animations + Discrete LEDs TF state
  if (u == "HELLO") return;
  if (u == "GOODBYE") { boardStateActive=false; return; }
  if (u.startsWith("MODE:")) u = u.substring(5);
  if (u.startsWith("ANS:"))  u = u.substring(4);

  if (u == "MC")        { tfActive = false; discUpdate(); startAnim((uint8_t)ANIM_PULSE,   COL_BLUE,   DUR_MODE_MS);   return; }
  if (u == "TF")        { tfActive = true;  discUpdate(); startAnim((uint8_t)ANIM_TRAILS,  COL_GREEN,  DUR_MODE_MS);   return; }
  if (u == "FORTUNE")   { tfActive = false; discUpdate(); startAnim((uint8_t)ANIM_TWINKLE, COL_YELLOW, DUR_MODE_MS);   return; }
  if (u == "BAD")       { tfActive = false; discUpdate(); startAnim((uint8_t)ANIM_PULSE,   COL_RED,    DUR_MODE_MS);   return; }
  if (u == "CORRECT")   { tfActive = false; discUpdate(); startAnim((uint8_t)ANIM_PULSE,   COL_GREEN,  DUR_PULSE_MS);  return; }
  if (u == "WRONG")     { tfActive = false; discUpdate(); startAnim((uint8_t)ANIM_FLASH3,  COL_RED,    DUR_WRONG_MS);  return; }
}

void setup(){
  // Strip
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show();
  idleResetToDefaults();

  // Serial & randomness
  Serial.begin(115200);
  line.reserve(256);
  randomSeed(analogRead(0));

  // Keyboard buttons
  initButtons();

  // Discrete LEDs
  for (uint8_t i=0;i<4;i++){ pinMode(DISC_PINS[i], OUTPUT); }
  tfActive = false; // default state
  discUpdate();     // turn all on
}

void loop(){
  // serial parsing
  while(Serial.available()){
    char c = (char)Serial.read();
    if (c=='\r') continue;
    if (c=='\n'){ handleCmd(line); line=""; }
    else if (line.length() < 255){ line+=c; }
  }

  // poll buttons
  pollButtons();

  // animations
  renderAnim();
}
