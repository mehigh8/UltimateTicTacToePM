#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Arduino_ST7789_Fast.h>

#define SCR_SIZE   240
#define TFT_DC  7
#define TFT_RST 8

Arduino_ST7789 tft = Arduino_ST7789(TFT_DC, TFT_RST);
char board[9][9];

int currentBox = -1;
// uint16_t replacementPixels[20];

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

void placeValue(char value, int boxNumber) {
  int x = 12 + (boxNumber % 9) * 24 + 8;
  int y = 12 + (boxNumber / 9) * 24 + 5;
  tft.setCursor(x, y);
  tft.println(value);
  board[boxNumber / 9][boxNumber % 9] = value;
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

void setup() {
  for (int i = 0; i < 9; i++)
    for (int j = 0; j < 9; j++)
      board[i][j] = '_';

  Serial.begin(9600);

  initADC();

  tft.begin();
  tft.fillScreen(BLACK);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  drawBoard();
  placeValue('X', 40);
  placeValue('O', 48);
}

void loop() {
  selectBox(readADC());
}
