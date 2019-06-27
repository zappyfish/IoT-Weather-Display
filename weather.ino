#include <neopixel.h>

#define PIXEL_PIN D4
#define PIXEL_COUNT 12
#define PIXEL_TYPE WS2812B

#define CYAN 10,150,70
#define PURPLE 180,3,180
#define BLUE 20,20,255
#define WHITE 255,255,255
#define GREEN 10,180,10
#define YELLOW 200,150,0
#define LIGHTBLUE 135,206,250
#define INDIGO 75,0,130
#define OFF 0,0,0

#define TIME_BETWEEN_REQUESTS 120000 // time between API requests in seconds
#define MAX_CONDITION_SIZE 35 // The max character size of a condition string

unsigned long last_request_time;

int NUM_CONDITIONS = 10;
char *conditions[] = {
    "clear-day", "clear-night", "rain", "snow", "sleet", "wind", "fog", "cloudy", "partly-cloudy-day", "partly-cloudy-night"
};

// Place your color mappings in here. There is a 1-1 ordered correspondence between the condition array and the color array. For example, the second color in the color
// array should correspond to "clear-night"
int colors[] = {LIGHTBLUE, LIGHTBLUE, BLUE, WHITE, WHITE, YELLOW, INDIGO, INDIGO, INDIGO, INDIGO};

char *CURRENTLY_KEY = "currently";
char *CONDITION_KEY = "icon";

Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

int knop1 = D2;
int knop2 = D3;

int var_knop1 = 0;
int var_knop2 = 0;

int weather = 1;

bool publish_debug = false;

void get_current_weather() {
  Particle.publish("weather", NULL, PRIVATE);
}

int handle_condition(const char *data, int pos_start) {
    char condition[MAX_CONDITION_SIZE + 1];
    int i = 0;
    while (data[pos_start] != '\"') {
        condition[i++] = data[pos_start++];
    }
    condition[i] = '\0';
    for (int i = 0; i < NUM_CONDITIONS; i++) {
        if (!strcmp(conditions[i], condition)) return i;
    }
    return -1;
}

int parse_weather_response(const char *data, const char *target, int parse_index, int max_len) {
  int char_matches = 0;
  char c;
  while (parse_index < max_len) { // this breaks if the api response changes, so be aware of that
      c = data[parse_index];
      if (c == target[char_matches]) {
            char_matches++;
      } else {
            char_matches = 0;
      }
      if (char_matches == strlen(target)) {
            if (target == CURRENTLY_KEY) {
                return parse_weather_response(data, CONDITION_KEY, parse_index + 4, max_len); // Skip past the ":{  tokens
          } else {
                return handle_condition(data, parse_index + 4);
          }
      }
      parse_index++;
  }
}

void drive_leds() {
    for(int i = 0; i < PIXEL_COUNT; i++) {
      strip.setPixelColor(i, colors[weather]);
    }
    strip.show();
}

void weather_response_handler(const char *event, const char *data) {
  int api_weather = parse_weather_response(data, CURRENTLY_KEY, 1, strlen(data));

  if (api_weather == -1) {
      if (publish_debug) {
        Spark.publish("weather", "error parsing weather");
      }
  } else {
      weather = api_weather;
      if (publish_debug) {
        Spark.publish("weather", conditions[weather]);
      }
  }
}

void handle_buttons() {
  var_knop1 = digitalRead(knop1);
  var_knop2 = digitalRead(knop2);
  if(var_knop1 == HIGH) {
    // Knop AAN / CHANGE
    weather++;
    if(weather == NUM_CONDITIONS) {
        weather = 1;
    }
    delay(500);
  }

  if(var_knop2 == HIGH) {
    // Knop UIT
    weather = 0;
    delay(500);
  }
}

// The setup
void setup() {
  Particle.subscribe("hook-response/weather", weather_response_handler, MY_DEVICES);
  pinMode(knop1, INPUT_PULLDOWN);
  pinMode(knop2, INPUT_PULLDOWN);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  get_current_weather(); // Make a starting API call
  last_request_time = millis();
}


// The main loop
void loop() {
  handle_buttons();

  if (millis() - last_request_time >= TIME_BETWEEN_REQUESTS) {
      last_request_time = millis();
      get_current_weather();
  }

  drive_leds();
}
