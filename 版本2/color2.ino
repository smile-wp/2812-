/*
  一维灯带反应力游戏 (连续同色块消除)
  - 按下按键消除最左边第一个匹配颜色及其右侧连续的同色灯珠
  - 例如：蓝蓝红绿 -> 按蓝，两个蓝色一起灭
*/

#include <FastLED.h>

// ========== 用户参数 ==========
#define LED_COUNT     30          // 灯珠数量
#define BRIGHTNESS    150         // 亮度
#define MOVE_INTERVAL 500         // 移动间隔(ms)
#define DEBOUNCE_MS   180         // 按键防抖

// ========== 硬件引脚 ==========
#define LED_PIN       6
#define SPEAKER_PIN   9
#define BUTTON_R_PIN  3
#define BUTTON_G_PIN  4
#define BUTTON_B_PIN  2

// ========== 颜色定义 ==========
const CRGB colorMap[3] = {
    CRGB::Red,    // 0
    CRGB::Green,  // 1
    CRGB::Blue    // 2
};

// ========== 全局变量 ==========
CRGB leds[LED_COUNT];
int beads[LED_COUNT];               // -1 表示空

unsigned long lastMoveTime = 0;
unsigned long lastButtonTime = 0;
bool gameOver = false;

// ========== 函数声明 ==========
void initGame();
void moveBeads();
void addNewBead();
void eliminateFirstGroupOfColor(int colorIndex);
void playButtonSound(int colorIndex);
void playGameOverSound();
void updateLEDs();
void startupTest();
void gameOverSequence();

// ========== 初始化 ==========
void setup() {
    Serial.begin(115200);
    Serial.println(F("\n===== 反应力游戏 (连续同色块消除) ====="));
    
    FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, LED_COUNT);
    FastLED.setBrightness(BRIGHTNESS);
    FastLED.clear();
    FastLED.show();
    
    pinMode(BUTTON_R_PIN, INPUT_PULLUP);
    pinMode(BUTTON_G_PIN, INPUT_PULLUP);
    pinMode(BUTTON_B_PIN, INPUT_PULLUP);
    pinMode(SPEAKER_PIN, OUTPUT);
    
    startupTest();
    randomSeed(analogRead(A0));
    
    initGame();
    updateLEDs();
    FastLED.show();
    
    Serial.println(F("游戏开始！按下颜色键消除最左侧对应的连续同色块。"));
}

void startupTest() {
    tone(SPEAKER_PIN, 1000, 150);
    delay(200);
    noTone(SPEAKER_PIN);
    for (int i = 0; i < LED_COUNT; i++) {
        leds[i] = CRGB::White;
        FastLED.show();
        delay(20);
    }
    delay(150);
    for (int i = 0; i < LED_COUNT; i++) {
        leds[i] = CRGB::Black;
        FastLED.show();
        delay(10);
    }
}

void initGame() {
    for (int i = 0; i < LED_COUNT; i++) {
        beads[i] = -1;
    }
    gameOver = false;
    lastMoveTime = millis();
}

// ========== 主循环 ==========
void loop() {
    unsigned long now = millis();
    
    // 流水灯移动
    if (!gameOver && (now - lastMoveTime >= MOVE_INTERVAL)) {
        lastMoveTime = now;
        moveBeads();
        addNewBead();
        updateLEDs();
        FastLED.show();
        
        // 失败检测
        if (beads[LED_COUNT - 1] != -1) {
            gameOverSequence();
        }
    }
    
    // 按键处理
    if (now - lastButtonTime >= DEBOUNCE_MS) {
        int pressedColor = -1;
        if (digitalRead(BUTTON_R_PIN) == LOW) {
            pressedColor = 0;
        } else if (digitalRead(BUTTON_G_PIN) == LOW) {
            pressedColor = 1;
        } else if (digitalRead(BUTTON_B_PIN) == LOW) {
            pressedColor = 2;
        }
        
        if (pressedColor != -1) {
            lastButtonTime = now;
            
            if (gameOver) {
                initGame();
                updateLEDs();
                FastLED.show();
                Serial.println(F("重新开始！"));
                return;
            }
            
            // 消除最左侧连续同色块
            eliminateFirstGroupOfColor(pressedColor);
            playButtonSound(pressedColor);
            updateLEDs();
            FastLED.show();
        }
    }
}

// ========== 流水右移 ==========
void moveBeads() {
    for (int i = LED_COUNT - 1; i > 0; i--) {
        beads[i] = beads[i - 1];
    }
    beads[0] = -1;
}

void addNewBead() {
    beads[0] = random(0, 3);
}

// ========== 消除最左边第一个匹配颜色及其右侧连续的同色灯珠 ==========
void eliminateFirstGroupOfColor(int colorIndex) {
    // 找到第一个匹配的颜色
    int start = -1;
    for (int i = 0; i < LED_COUNT; i++) {
        if (beads[i] == colorIndex) {
            start = i;
            break;
        }
    }
    if (start == -1) return;  // 没有该颜色
    
    // 从 start 开始向后消除所有连续同色
    int end = start;
    while (end < LED_COUNT && beads[end] == colorIndex) {
        beads[end] = -1;
        end++;
    }
    
    Serial.print(F("消除连续"));
    if (colorIndex == 0) Serial.print(F("红"));
    else if (colorIndex == 1) Serial.print(F("绿"));
    else Serial.print(F("蓝"));
    Serial.print(F("色块，位置 "));
    Serial.print(start);
    Serial.print(F("~"));
    Serial.println(end - 1);
}

// ========== 按键音效 ==========
void playButtonSound(int colorIndex) {
    int freq = 800 + colorIndex * 200;
    tone(SPEAKER_PIN, freq, 120);
    delay(100);
    noTone(SPEAKER_PIN);
}

// ========== 失败音效与灯效 ==========
void playGameOverSound() {
    tone(SPEAKER_PIN, 800, 200);
    delay(200);
    tone(SPEAKER_PIN, 600, 200);
    delay(200);
    tone(SPEAKER_PIN, 400, 300);
    delay(300);
    noTone(SPEAKER_PIN);
}

void gameOverSequence() {
    gameOver = true;
    Serial.println(F("游戏失败！"));
    
    for (int t = 0; t < 3; t++) {
        for (int i = 0; i < LED_COUNT; i++) {
            leds[i] = CRGB::Red;
        }
        FastLED.show();
        delay(200);
        for (int i = 0; i < LED_COUNT; i++) {
            leds[i] = CRGB::Black;
        }
        FastLED.show();
        delay(200);
    }
    playGameOverSound();
    Serial.println(F("按任意颜色键重新开始。"));
}

// ========== 更新灯带显示 ==========
void updateLEDs() {
    for (int i = 0; i < LED_COUNT; i++) {
        if (beads[i] >= 0 && beads[i] <= 2) {
            leds[i] = colorMap[beads[i]];
        } else {
            leds[i] = CRGB::Black;
        }
    }
}