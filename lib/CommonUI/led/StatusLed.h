#pragma once

#include <Arduino.h>
#include <FastLED.h>

// =========================
// LED patterns
// =========================
enum class LedPattern {
    OFF,
    SOLID,
    BLINK,
    PULSE,
    ALTERNATE
};

struct LedStyle {
    LedPattern pattern;
    CRGB color1;
    CRGB color2;
    uint16_t onMs;
    uint16_t offMs;
};

// =========================
// StatusLed (hardware + logic)
// =========================
template<uint8_t DATA_PIN>
class StatusLed {
public:
    void begin() {
        FastLED.addLeds<WS2811, DATA_PIN, GRB>(leds, NUM_LEDS);
        FastLED.clear();
        FastLED.show();
        lastUpdate = millis();
    }

    void setStyle(const LedStyle& style) {
        current = style;
        lastUpdate = millis();
        ledOn = false;
    }

    void update() {
        uint32_t now = millis();

        switch (current.pattern) {

            case LedPattern::OFF:
                setColor(CRGB::Black);
                break;

            case LedPattern::SOLID:
                setColor(current.color1);
                break;

            case LedPattern::BLINK:
                if (now - lastUpdate >= (ledOn ? current.onMs : current.offMs)) {
                    ledOn = !ledOn;
                    lastUpdate = now;
                    setColor(ledOn ? current.color1 : CRGB::Black);
                }
                break;

            case LedPattern::ALTERNATE:
                if (now - lastUpdate >= current.onMs) {
                    ledOn = !ledOn;
                    lastUpdate = now;
                    setColor(ledOn ? current.color1 : current.color2);
                }
                break;

            case LedPattern::PULSE: {
                uint8_t brightness = beatsin8(20, 30, 255);
                CRGB c = current.color1;
                c.nscale8(brightness);
                setColor(c);
                break;
            }
        }
    }

private:
    static constexpr uint8_t NUM_LEDS = 1;
    CRGB leds[NUM_LEDS];

    LedStyle current {LedPattern::OFF, CRGB::Black, CRGB::Black, 0, 0};
    uint32_t lastUpdate {0};
    bool ledOn {false};

    void setColor(const CRGB& color) {
        leds[0] = color;
        FastLED.show();
    }
};
