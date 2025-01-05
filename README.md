# Blinky Badge "light"!
Hello!  Welcome to the github repo for Blinky Badge "light".

## What is this thing?
 - Blinky Badge "light" is a lighting badge with unique patterns and sound reactivity! 
 - It's meant to be worn at festivals or whenever you'd like to light your person in a fun way.  Safety!
 - Heart shaped!

## Why is it named Blinky Badge "light"?
### This project shares similar features with two projects:
   -  [bunnie's BM blinky badge of 2017 and 2020](https://github.com/bunnie/chibios-xz/tree/bm20)
   -  [the cubegarden art project](https://github.com/rowr111/cubegarden)

### So, about the name?

- Although this project was written from scratch, has some different features, and uses a different microcontroller etc from the original badge, it shares the fundamental shared goal of the original - to light up the night in a fun and personal way!
- I also re-use the pattern generation concepts from the original badge in some of the patterns, so visually there are also some similarities.
- This badge does not share the radio communication features of the original, nor does it have a screen/ui - thus it's called blinky badge "light".  (Also, it makes.. light. get it? :D)
   
## Features
### Patterns:
 - three unique patterns generated on initial startup for each badge
    - the first pattern will also be sound reactive
 - fourth sound reactive pattern whose color indicates dB level
 - one 'safety' pattern
    - not in the normal pattern list
    - forced when battery level is very low

### Capacitive Touch Buttons:
1. iterate through unique patterns (wraps around when gets to the end)
2. change brightness level (wraps around after brightest)
3. replace current pattern with new unique pattern
    - doesn't work on fourth pattern or safety pattern
    - small flash on pattern change for visual indicator of change

### Power Button (physical button):
 - press to turn badge on
 - long press to turn badge off

### Power Indicator LED (small, yellow):
 - off during normal operation with good battery charge
 - on (solid) when power is starting to get low
 - blinking during charging


## Physical Structure/Components

The badge is a heart shaped PCB with LEDs outlining the heart.  The LEDs are covered by the case for protection and some light diffusion, but the center front of the PCB is exposed for easy access of the buttons.

The form is similar to the original blinky badge, but there are differences:
 - the original has 36 LEDs, this badge has 24.
 - the original has a screen and antenna, this badge does not.
 - the original has 8 capacitive touch buttons, this badge has 3.
 - this badge uses user-supplied, removeable batteries.

### Structural Components (WIP!)
 - PCB with these main components:
   - ESP32 microcontroller
   - microphone
   - 24 WS2812B LEDs
   - USB-C port for:
     - charging batteries
     - programming the microcontroller
   - 3 capacitive touch buttons
     - change pattern
     - change brightness
     - replace current pattern with new unique pattern
   - power button for On/Off
   - small led for lower battery charge/currently charging indicator
   - battery holder attached to the back
      - requires two 18650 lithium-ion rechargeable batteries
 - Case:
   - heart shaped ring on the front, same as the original blinky badge
   - back fully covers the back of the PCB as well as protects the battery holder
   - will have various points for attaching/hanging the badge

