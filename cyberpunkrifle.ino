// Cyberpunk rifle code for Popeye
// By phil Montgomery - phil@ majura.com
// 8th Jan 2025
// Freely distributable

#include <WS2812FX.h>
#include <FlashStorage.h>
#include <Bounce2.h>  // Include Bounce2 library

constexpr int powerOnSpeed = 30000;      //Changes the speed of the PowerOn Mode - bigger number is SLOWER, smaller is faster
constexpr int triggerSpeed = 1000;       //Changes the speed of the Trigger Mode - bigger number is SLOWER, smaller is faster
constexpr int triggerColor = 0xD50000;   //The HEX RGB color for trigger strip animations (RGB: (255, 53, 94))
constexpr int powerOnColor = 0x00D1FF;   //The HEX RGB color for trigger strip animations (RGB: (255, 53, 94))
constexpr uint8_t MAX_BRIGHTNESS = 255;  //Brightness (255 is brightest) - the same for all animations
constexpr uint8_t powerOnDelay = 50;     //number of ms to pause between trigger effect and powerOn.

constexpr uint8_t LEDCount = 26;   //change this if the strip length changes
constexpr uint8_t LED1Pin = 0;     //where the LED strip is connected
constexpr uint8_t LED2Pin = 1;     //where the LED strip is connected
constexpr uint8_t ModePin = 4;     //Mode switch to change both power on and trigger patterns
constexpr uint8_t TriggerPin = 3;  //Trigger switch

constexpr int patternModes[] = { 13, 14, 15, 18, 40, 43, 44, 56 };         //Array of the modes Michael wanted to select through - if you want more modes just add them.
constexpr int NUM_MODES = sizeof(patternModes) / sizeof(patternModes[0]);  //dynamically size

struct Mode {  //Using a STRUCT for all the settings for each Mode.  May be overkill here but a fun activitywrite
  uint8_t pattern;
  uint8_t patternIndex;
  int speed;
  int color;
};
Mode powerOn, trigger;

FlashStorage(powerOnMode, int);  //Store and read the current pattern modes in Flash
FlashStorage(triggerMode, int);  //Store and read the current pattern modes in Flash

WS2812FX strip1 = WS2812FX(LEDCount, LED1Pin, NEO_GRB + NEO_KHZ800);  //initialize the strip
WS2812FX strip2 = WS2812FX(LEDCount, LED2Pin, NEO_GRB + NEO_KHZ800);  //initialize the strip

Bounce triggerButton = Bounce();  // Create a Bounce object for trigger button
Bounce modeButton = Bounce();     // Create a Bounce object for mode button

bool triggerState = false;

void setup() {
  //Serial.begin(115200);

  pinMode(ModePin, INPUT_PULLUP);
  pinMode(TriggerPin, INPUT_PULLUP);

  // Initialize the Bounce object for trigger button
  triggerButton.attach(TriggerPin);
  triggerButton.interval(100);  // Set debounce interval to 50ms

  // Initialize the Bounce object for mode button
  modeButton.attach(ModePin);  // Attach the mode button pin to Bounce object
  modeButton.interval(50);     // Set debounce interval to 50ms

  //Read from FlashStorage and Validate

  powerOn.pattern = powerOnMode.read();
  validateMode(powerOn);
  trigger.pattern = triggerMode.read();
  validateMode(trigger);

  //set variables in each struct value)
  trigger.color = triggerColor;
  powerOn.color = powerOnColor;
  trigger.speed = triggerSpeed;
  powerOn.speed = powerOnSpeed;

  strip1.init();
  strip2.init();

  strip1.setBrightness(MAX_BRIGHTNESS);
  strip2.setBrightness(MAX_BRIGHTNESS);

  setEffect(powerOn);

  //Serial.println("Setup complete. Power-on mode activated.");
}

void loop() {

  // Update the debounce state for the mode button
  modeButton.update();
  triggerButton.update();  // Update the debounce state of the trigger button

  // Handle mode  button press
  if (modeButton.fell()) {
    //Serial.println("Mode button pressed.");
    checkAndChangePatternMode();
  }

  // Handle trigger button press
  if (triggerButton.fell()) {
    //Serial.println("Trigger button pressed.");
    activateTriggerMode();
  }

  // If the trigger button is released (debounced)
  if (triggerButton.rose()) {
    setEffect(powerOn);  // Power-on effect activated
    triggerState = false;
    //Serial.println("Power-on effect activated.");
    delay(powerOnDelay);
  }

  strip1.service();  // this gives the strip time to service
  strip2.service();  // this gives the strip time to service
}

void activateTriggerMode() {

  setEffect(trigger);  // Trigger effect activated
  //Serial.println("Trigger effect activated.");
  triggerState = true;
}

void checkAndChangePatternMode() {

  // Change mode based on the state of the mode button
  if (triggerState) {  // Check if trigger is pressed
    changeMode(trigger);

    if (triggerMode.read() != trigger.pattern) {
      triggerMode.write(trigger.pattern);
    }
    //Serial.println("Changing Trigger Pattern");
  } else {
    changeMode(powerOn);

    if (powerOnMode.read() != powerOn.pattern) {
      powerOnMode.write(powerOn.pattern);
    }

    //Serial.println("Changing PowerOn Pattern");
  }
}

void validateMode(Mode &mode) {
  for (int i = 0; i < NUM_MODES; i++) {
    if (mode.pattern == patternModes[i]) {
      mode.patternIndex = i;
      return;
    }
  }

  // FlashStorage error detected
  //Serial.println("FlashStorage error: Values are corrupt.");
  flashErrorIndicator();
  mode.pattern = patternModes[0];
  powerOnMode.write(powerOn.pattern);
  triggerMode.write(trigger.pattern);
}

void setEffect(Mode &mode) {
  strip1.clear();
  strip2.clear();
  strip1.show();
  strip2.show();
  applyEffectToStrip(strip1, mode);
  applyEffectToStrip(strip2, mode);
}

void changeMode(Mode &mode) {
  mode.patternIndex = (mode.patternIndex + 1) % NUM_MODES;
  mode.pattern = patternModes[mode.patternIndex];
  setEffect(mode);
  //Serial.print(" Pattern mode changed to: ");
  //Serial.println(mode.pattern);
}

void applyEffectToStrip(WS2812FX &strip, Mode &mode) {
  strip.setMode(mode.pattern);
  strip.setSpeed(mode.speed);
  strip.setColor(mode.color);
  strip.start();
}

void flashErrorIndicator() {
  const int flashDelay = 250;  // Delay between flashes (milliseconds)
  const int flashCount = 5;    // Number of flashes

  for (int i = 0; i < flashCount; i++) {
    strip1.setColor(0xFF0000);  // Set all LEDs on strip1 to red
    strip2.setColor(0xFF0000);  // Set all LEDs on strip2 to red

    strip1.show();
    strip2.show();
    delay(flashDelay);

    strip1.clear();  // Turn off all LEDs
    strip2.clear();

    strip1.show();
    strip2.show();
    delay(flashDelay);
  }

  // Reset to ensure strips are cleared
  strip1.clear();
  strip2.clear();
  strip1.show();
  strip2.show();
}