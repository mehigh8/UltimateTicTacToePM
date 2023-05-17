#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Arduino_ST7789_Fast.h>

#define SCR_SIZE   240
#define TFT_DC  7
#define TFT_RST 8

#define ALL 0
#define LEFTUP 1
#define UP 2
#define RIGHTUP 3
#define LEFT 4
#define CENTER 5
#define RIGHT 6
#define LEFTDOWN 7
#define DOWN 8
#define RIGHTDOWN 9

Arduino_ST7789 tft = Arduino_ST7789(TFT_DC, TFT_RST);
char board[9][9];
char bigBoard[3][3];

volatile int currentBox = -1;
volatile int placement = ALL;
volatile char player = 'X';
// uint16_t replacementPixels[20];

ISR(INT0_vect) {
  //Serial.println("int0");
  if (placeValue(player, currentBox))
    player = (player == 'X' ? 'O' : 'X');
}

// ISR(INT1_vect) {
//   //Serial.println("int1");
//   placeValue('O', currentBox);
// }

void drawBoard() {
  for (int p = 12; p < SCR_SIZE; p += 24) {
    uint16_t color = RGBto565(128, 128, 128);
    tft.drawLine(p, 12, p, 228, color);
    tft.drawLine(12, p, 228, p, color);
  }

  for (int p = 12; p < SCR_SIZE; p += 72) {
    tft.drawLine(p, 12, p, 228, WHITE);
    tft.drawLine(12, p, 228, p, WHITE);
  }
}

bool canPlace(int boxNumber) {
  int x = boxNumber % 9;
  int y = boxNumber / 9;
  switch(placement) {
    case ALL:
      return board[y][x] == '_';
    case LEFTUP:
      return y >= 0 && y < 3 && x >= 0 && x < 3 && board[y][x] == '_';
    case UP:
      return y >= 0 && y < 3 && x >= 3 && x < 6 && board[y][x] == '_';
    case RIGHTUP:
      return y >= 0 && y < 3 && x >= 6 && x < 9 && board[y][x] == '_';
    case LEFT:
      return y >= 3 && y < 6 && x >= 0 && x < 3 && board[y][x] == '_';
    case CENTER:
      return y >= 3 && y < 6 && x >= 3 && x < 6 && board[y][x] == '_';
    case RIGHT:
      return y >= 3 && y < 6 && x >= 6 && x < 9 && board[y][x] == '_';
    case LEFTDOWN:
      return y >= 6 && y < 9 && x >= 0 && x < 3 && board[y][x] == '_';
    case DOWN:
      return y >= 6 && y < 9 && x >= 3 && x < 6 && board[y][x] == '_';
    case RIGHTDOWN:
      return y >= 6 && y < 9 && x >= 6 && x < 9 && board[y][x] == '_';
  }
}

void updatePlacement(int boxNumber) {
  int x = boxNumber % 9;
  int y = boxNumber / 9;
  if (y % 3 == 0) {
    if (x % 3 == 0)
      placement = LEFTUP;
    else if (x % 3 == 1)
      placement = UP;
    else
      placement = RIGHTUP;
  } else if (y % 3 == 1) {
    if (x % 3 == 0)
      placement = LEFT;
    else if (x % 3 == 1)
      placement = CENTER;
    else
      placement = RIGHT;
  } else {
    if (x % 3 == 0)
      placement = LEFTDOWN;
    else if (x % 3 == 1)
      placement = DOWN;
    else
      placement = RIGHTDOWN;
  }
}

void drawWinnerSmall(int x, int y, char value) {
  int xPos = 12 + x * 24;
  int yPos = 12 + y * 24;
  tft.fillRect(xPos + 1, yPos + 1, 71, 71, BLACK);

  xPos += 8;
  yPos += 5;
  tft.setTextColor(GREEN);
  if (value == 'X') {
    tft.setCursor(xPos, yPos);
    tft.println(value);
    tft.setCursor(xPos + 48, yPos);
    tft.println(value);
    tft.setCursor(xPos + 24, yPos + 24);
    tft.println(value);
    tft.setCursor(xPos, yPos + 48);
    tft.println(value);
    tft.setCursor(xPos + 48, yPos + 48);
    tft.println(value);
  } else {
    tft.setCursor(xPos, yPos);
    tft.println(value);
    tft.setCursor(xPos + 24, yPos);
    tft.println(value);
    tft.setCursor(xPos + 48, yPos);
    tft.println(value);
    tft.setCursor(xPos, yPos + 24);
    tft.println(value);
    tft.setCursor(xPos + 48, yPos + 24);
    tft.println(value);
    tft.setCursor(xPos, yPos + 48);
    tft.println(value);
    tft.setCursor(xPos + 24, yPos + 48);
    tft.println(value);
    tft.setCursor(xPos + 48, yPos + 48);
    tft.println(value);
  }

  tft.setTextColor(WHITE);
}

void checkWinSmall(int boxNumber) {
  int x = boxNumber % 9;
  int y = boxNumber / 9;
  x -= x % 3;
  y -= y % 3;

  if (board[y][x] != '_' && board[y][x] != '#' && board[y][x] == board[y+1][x] && board[y][x] == board[y+2][x]) {
    bigBoard[y/3][x/3] = player;
    placement = ALL;
    for (int i = 0; i < 3; i++)
      for (int j = 0; j < 3; j++)
        board[y + i][x + j] = '#';
    drawWinnerSmall(x, y, player);
  }
}

bool placeValue(char value, int boxNumber) {
  if (canPlace(boxNumber)) {
    int x = 12 + (boxNumber % 9) * 24 + 8;
    int y = 12 + (boxNumber / 9) * 24 + 5;
    tft.setCursor(x, y);
    tft.println(value);
    board[boxNumber / 9][boxNumber % 9] = value;
    updatePlacement(boxNumber);
    checkWinSmall(boxNumber);
    return true;
  }

  return false;
}

void selectBox(uint16_t potentiometerValue) {
  int box = potentiometerValue / 12 - 2;
  if (box < 0)
    box = 0;
  if (box > 80)
    box = 80;
  
  if (currentBox == box)
    return;

  int16_t xPos = 12 + (currentBox % 9) * 24 + 2;
  int16_t yPos = 12 + (currentBox / 9) * 24 + 22;
  if (currentBox != -1) {
    for (int i = 0; i < 20; i++) {
      tft.drawPixel(xPos, yPos, BLACK);
      xPos++;
    }
  }

  currentBox = box;
  xPos = 12 + (currentBox % 9) * 24 + 2;
  yPos = 12 + (currentBox / 9) * 24 + 22;
  tft.drawLine(xPos, yPos, xPos + 19, yPos, RGBto565(255, 217, 0));
}

void initADC() {
  ADMUX = 0;
  ADMUX |= (1 << REFS0);

  ADCSRA = 0;
  // ADCSRA |= (1 << ADPS0);
  ADCSRA |= (1 << ADPS1);
  ADCSRA |= (1 << ADPS2);
  ADCSRA |= (1 << ADEN);
}

uint16_t readADC() {
  uint32_t results = 0;
  int readCount = 100;
  for (int i = 0; i < readCount; i++) {
    ADCSRA |= (1 << ADSC);
    while (!(ADCSRA & (1 << ADIF)));
    results += ADC;
  }
  return results / readCount;
}

void initInterrupts() {
  cli();
  EICRA = 0;
  EIMSK = 0;

  // EICRA |= (1 << ISC11);
  //EICRA |= (1 << ISC10);
  EICRA |= (1 << ISC01);
  //EICRA |= (1 << ISC00);

  EIMSK |= (1 << INT0);
  // EIMSK |= (1 << INT1);

  sei();
}

void setup() {
  for (int i = 0; i < 9; i++)
    for (int j = 0; j < 9; j++)
      board[i][j] = '_';

  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
      bigBoard[i][j] = '_';

  Serial.begin(9600);

  initADC();
  initInterrupts();

  tft.begin();
  tft.fillScreen(BLACK);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  drawBoard();
}

void loop() {
  selectBox(readADC());
}
