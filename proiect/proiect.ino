#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Arduino_ST7789_Fast.h>
#include "macros.h"

#define SCR_SIZE   240
#define TFT_DC  7
#define TFT_RST 8

Arduino_ST7789 tft = Arduino_ST7789(TFT_DC, TFT_RST);
char board[9][9];
char bigBoard[3][3];

volatile int currentBox = -1;
volatile int placement = ALL;
volatile char player = 'X';
volatile int state = MENU;
volatile int gameMode = MULTI;
volatile int difficulty = EASY;
volatile int moveCount = 0;
volatile bool hasWon = false;

volatile long lastDebounceTime1 = 0;
volatile long lastDebounceTime0 = 0;
volatile long debounceDelay = 50;

volatile int currentTimerCount = 0;
volatile int stopTimerCount = 0;

/*    INTERRUPTS    */
ISR(INT0_vect) {
  if (millis() - lastDebounceTime0 > debounceDelay) {
    if (state == MENU) {
      drawSingleplayerMenu();
    } else if (state == GAME) {
      if (placeValue(player, currentBox)) {
        player = (player == 'X' ? 'O' : 'X');
        if (gameMode == SINGLE && !hasWon) {
          makeMoveAI();
        }
      }
    } else if (state == SP_MENU) {
      difficulty = EASY;
      startGame();
    } else {
      drawMenu();
    }
  }

  lastDebounceTime0 = millis();
}

ISR(INT1_vect) {
  if (millis() - lastDebounceTime1 > debounceDelay) {
    if (state == MENU) {
      gameMode = MULTI;
      startGame();
    } else if (state == SP_MENU) {
      difficulty = HARD;
      startGame();
    } else {
      drawMenu();
    }
  }

  lastDebounceTime1 = millis();
}

ISR(TIMER1_OVF_vect) {
  if (TCCR1A != 0 || TCCR1B != 0) {
    currentTimerCount++;
    if (currentTimerCount == stopTimerCount) {
      TCCR1A = 0;
      TCCR1B = 0;
    }
  }
}

/*    GAME LOGIC    */
void startGame() {
  state = GAME;
  currentBox = -1;
  placement = ALL;
  player = 'X';
  moveCount = 0;
  hasWon = false;

  for (int i = 0; i < 9; i++)
    for (int j = 0; j < 9; j++)
      board[i][j] = '_';

  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
      bigBoard[i][j] = '_';
  
  drawBoard();
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
  if (bigBoard[y%3][x%3] != '_')
    placement = ALL;
  else {
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
}

bool canWinBig(char value) {
  int x = 0;
  int y = 0;
  return (bigBoard[y][x] == value && bigBoard[y][x] == bigBoard[y+1][x] && bigBoard[y][x] == bigBoard[y+2][x])
      || (bigBoard[y][x+1] == value && bigBoard[y][x+1] == bigBoard[y+1][x+1] && bigBoard[y][x+1] == bigBoard[y+2][x+1])
      || (bigBoard[y][x+2] == value && bigBoard[y][x+2] == bigBoard[y+1][x+2] && bigBoard[y][x+2] == bigBoard[y+2][x+2])
      || (bigBoard[y][x] == value && bigBoard[y][x] == bigBoard[y][x+1] && bigBoard[y][x] == bigBoard[y][x+2])
      || (bigBoard[y+1][x] == value && bigBoard[y+1][x] == bigBoard[y+1][x+1] && bigBoard[y+1][x] == bigBoard[y+1][x+2])
      || (bigBoard[y+2][x] == value && bigBoard[y+2][x] == bigBoard[y+2][x+1] && bigBoard[y+2][x] == bigBoard[y+2][x+2])
      || (bigBoard[y][x] == value && bigBoard[y][x] == bigBoard[y+1][x+1] && bigBoard[y][x] == bigBoard[y+2][x+2])
      || (bigBoard[y][x+2] == value && bigBoard[y][x+2] == bigBoard[y+1][x+1] && bigBoard[y][x+2] == bigBoard[y+2][x]);
}

bool canWinSmall(int x, int y, char value) {
  return (board[y][x] == value && board[y][x] == board[y+1][x] && board[y][x] == board[y+2][x])
      || (board[y][x+1] == value && board[y][x+1] == board[y+1][x+1] && board[y][x+1] == board[y+2][x+1])
      || (board[y][x+2] == value && board[y][x+2] == board[y+1][x+2] && board[y][x+2] == board[y+2][x+2])
      || (board[y][x] == value && board[y][x] == board[y][x+1] && board[y][x] == board[y][x+2])
      || (board[y+1][x] == value && board[y+1][x] == board[y+1][x+1] && board[y+1][x] == board[y+1][x+2])
      || (board[y+2][x] == value && board[y+2][x] == board[y+2][x+1] && board[y+2][x] == board[y+2][x+2])
      || (board[y][x] == value && board[y][x] == board[y+1][x+1] && board[y][x] == board[y+2][x+2])
      || (board[y][x+2] == value && board[y][x+2] == board[y+1][x+1] && board[y][x+2] == board[y+2][x]);
}

void checkWin(int boxNumber, char value) {
  int x = boxNumber % 9;
  int y = boxNumber / 9;
  x -= x % 3;
  y -= y % 3;

  if (canWinSmall(x, y, value)) {
    bigBoard[y/3][x/3] = value;
    if (canWinBig(value)) {
      finishGame(false, value);
      return;
    }
    for (int i = 0; i < 3; i++)
      for (int j = 0; j < 3; j++)
        board[y + i][x + j] = '#';
    drawWinnerSmall(x, y, value);
    currentBox = -1;
  } else {
    bool ok = false;
    for (int i = 0; i < 3; i++)
      for (int j = 0; j < 3; j++)
        if (board[y+i][x+j] == '_')
          ok = true;
    
    if (ok == false)
      bigBoard[y/3][x/3] = '#';
  }
  if (moveCount == 81) {
    finishGame(true, value);
    return;
  }

  updatePlacement(boxNumber);
}

bool placeValue(char value, int boxNumber) {
  if (canPlace(boxNumber)) {
    moveCount++;
    int x = 12 + (boxNumber % 9) * 24 + 8;
    int y = 12 + (boxNumber / 9) * 24 + 5;
    tft.setCursor(x, y);
    tft.println(value);
    board[boxNumber / 9][boxNumber % 9] = value;
    checkWin(boxNumber, value);
    return true;
  }
  playSound(29, 100);
  return false;
}

/*    SINGLEPLAYER AI     */
int getStartSmallBoard(int place) {
  place -= 1;
  int x = (place % 3) * 3;
  int y = (place / 3) * 9;
  return y * 3 + x;
}

bool emptyBoard(char localBoard[3][3]) {
  for (int i = 0; i < 3; i++)
      for (int j = 0; j < 3; j++)
        if (localBoard[i][j] != '_')
          return false;
  
  return true;
}

int countInBoard(char localBoard[3][3], char value) {
  int cnt = 0;
  for (int i = 0; i < 3; i++)
      for (int j = 0; j < 3; j++)
        if (localBoard[i][j] == value)
          cnt++;
  
  return cnt;
}

bool canWinAI(char localBoard[3][3], char value) {
  int x = 0;
  int y = 0;
  return (localBoard[y][x] == value && localBoard[y][x] == localBoard[y+1][x] && localBoard[y][x] == localBoard[y+2][x])
      || (localBoard[y][x+1] == value && localBoard[y][x+1] == localBoard[y+1][x+1] && localBoard[y][x+1] == localBoard[y+2][x+1])
      || (localBoard[y][x+2] == value && localBoard[y][x+2] == localBoard[y+1][x+2] && localBoard[y][x+2] == localBoard[y+2][x+2])
      || (localBoard[y][x] == value && localBoard[y][x] == localBoard[y][x+1] && localBoard[y][x] == localBoard[y][x+2])
      || (localBoard[y+1][x] == value && localBoard[y+1][x] == localBoard[y+1][x+1] && localBoard[y+1][x] == localBoard[y+1][x+2])
      || (localBoard[y+2][x] == value && localBoard[y+2][x] == localBoard[y+2][x+1] && localBoard[y+2][x] == localBoard[y+2][x+2])
      || (localBoard[y][x] == value && localBoard[y][x] == localBoard[y+1][x+1] && localBoard[y][x] == localBoard[y+2][x+2])
      || (localBoard[y][x+2] == value && localBoard[y][x+2] == localBoard[y+1][x+1] && localBoard[y][x+2] == localBoard[y+2][x]);
}

int movePriorityEasy(char localBoard[3][3], int i, int j) {
  if (localBoard[i][j] != '_')
    return 99;
  
  char oppositePlayer = (player == 'X' ? 'O' : 'X');
  char secondLocalBoard[3][3];
  for (int i = 0; i < 3; i++)
      for (int j = 0; j < 3; j++)
        secondLocalBoard[i][j] = localBoard[i][j];
  
  secondLocalBoard[i][j] = player;
  if (canWinAI(secondLocalBoard, player))
    return 0;
  
  secondLocalBoard[i][j] = oppositePlayer;
  if (canWinAI(secondLocalBoard, oppositePlayer))
    return 1;
  
  if (emptyBoard(localBoard) && ((i == 0 || i == 2) && (j == 0 || j == 2)))
    return 2;
  
  if (countInBoard(localBoard, player) == 0) {
    if (localBoard[1][1] == oppositePlayer) {
      if ((i == 0 || i == 2) && (j == 0 || j == 2))
        return 3;
    } else {
      if (i == 1 && j == 1)
        return 3;
    }
  }

  secondLocalBoard[i][j] = player;
  int box = findBestEasy(secondLocalBoard);
  if (movePriorityEasy(secondLocalBoard, box / 3, box % 3) == 0)
    return 4;
  
  if ((i == 0 || i == 2) && (j == 0 || j == 2))
    return 5;
  
  return 6;
}

int findBestEasy(char localBoard[3][3]) {
  int bestMove = 99, bestI = 0, bestJ = 0;
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) {
      int currentMove = movePriorityEasy(localBoard, i, j);
      if (currentMove < bestMove) {
        bestMove = currentMove;
        bestI = i;
        bestJ = j;
      }
    }
  
  return bestI * 3 + bestJ;
}

int searchMoveEasy() {
  int localPlacement = placement;

  if (placement == ALL) {
    char localBigBoard[3][3];
    for (int i = 0; i < 3; i++)
      for (int j = 0; j < 3; j++)
        localBigBoard[i][j] = bigBoard[i][j];
    
    localPlacement = findBestEasy(localBigBoard) + 1;
  }

  int box = getStartSmallBoard(localPlacement);
  char localBoard[3][3];
  for (int i = 0; i < 3; i++)
      for (int j = 0; j < 3; j++)
        localBoard[i][j] = board[box / 9 + i][box % 9 + j];

  int localBox = findBestEasy(localBoard);
  placeValue(player, box + ((localBox / 3) * 9 + localBox % 3));
}

void makeMoveAI() {
  if (difficulty == EASY)
    searchMoveEasy();

  player = (player == 'X' ? 'O' : 'X');
}

/*    SCREEN MANIPULATION     */
void finishGame(bool draw, char winner) {
  state = FIN;
  hasWon = true;
  playSound(200, 200);
  tft.fillScreen(BLACK);
  tft.setTextSize(3);

  if (draw == false) {
    tft.setCursor(110, 95);
    tft.println(winner);
    tft.setCursor(80, 125);
    tft.println("WINS");
  } else {
    tft.setCursor(80, 95);
    tft.println("DRAW");
  }

  tft.setTextSize(2);
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
    tft.drawLine(xPos, yPos, xPos + 19, yPos, BLACK);
  }

  currentBox = box;
  xPos = 12 + (currentBox % 9) * 24 + 2;
  yPos = 12 + (currentBox / 9) * 24 + 22;
  tft.drawLine(xPos, yPos, xPos + 19, yPos, RED);
}

void drawBoard() {
  tft.fillScreen(BLACK);
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

void drawMenu() {
  state = MENU;
  tft.setTextSize(3);
  tft.fillScreen(BLACK);

  tft.setCursor(10, 95);
  tft.println("Singleplayer");
  tft.setCursor(10, 125);
  tft.println("Multiplayer");

  tft.setTextSize(2);
}

void drawSingleplayerMenu() {
  state = SP_MENU;
  gameMode = SINGLE;
  tft.setTextSize(3);
  tft.fillScreen(BLACK);

  tft.setCursor(10, 95);
  tft.println("Easy");
  tft.setCursor(10, 125);
  tft.println("Hard");

  tft.setTextSize(2);
}

/*    ARDUINO     */
void initADC() {
  ADMUX = 0;
  ADMUX |= (1 << REFS0);

  ADCSRA = 0;
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

  EICRA |= (1 << ISC11);
  EICRA |= (1 << ISC01);

  EIMSK |= (1 << INT0);
  EIMSK |= (1 << INT1);

  TCCR1A = 0;
  TCCR1B = 0;
  
  TCNT1 = 0;
  TIMSK1 = 0;
  TIMSK1 |= (1 << TOIE1);

  sei();
}

void playSound(uint8_t compareValue, int stopCount) {
  cli();
  currentTimerCount = 0;
  stopTimerCount = stopCount;
  TCNT1 = 0;
  TCCR1A |= (1 << COM1A1);
  TCCR1A |= (1 << WGM10);
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << CS11);
  TCCR1B |= (1 << CS10);
  OCR1A = compareValue;
  sei();
}

void setup() {
  Serial.begin(9600);
  DDRB |= (1 << PB1);
  initADC();
  initInterrupts();
  tft.begin();

  tft.setTextColor(WHITE);
  drawMenu();
}

void loop() {
  if (state == GAME)
    selectBox(readADC());
}
