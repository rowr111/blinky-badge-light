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
 - three unique patterns generated on initial startup for each badge (not sound reactive)
 - two sound reactive patterns with a base same as ^
    - flashes the whole badge
    - fills up the badge from the bottom
 - one 'safety' pattern
    - not in the normal pattern list
    - forced when battery level is very low
- future pattern ideas:
    - sound reactive pattern whose color indicates dB level?  tbd..


### Capacitive Touch Buttons:
1. iterate through unique patterns (wraps around when gets to the end)
2. change brightness level (wraps around after brightest)
3. replace current pattern with new unique pattern
    - for sound reactive patterns changes the base pattern, not sound reactivity
    - small flash on pattern change for visual indicator of change
4. off (hold)
5. check battery level
6. ? (mystery spot. does nothing for now. use it to add a feature!)

### Power Button (physical button):
 - press to turn badge on

### Power Indicator LED (small, green):
 - on during charging


## Physical Structure/Components

The badge is a heart shaped PCB with LEDs outlining the heart.  The LEDs are covered by the case for protection and some light diffusion, but the center front of the PCB is exposed for easy access of the buttons.

The form is similar to the original blinky badge, but there are differences:
 - the original has 36 LEDs, this badge has 24.
 - the original has a screen and antenna, this badge does not.
 - the original has 8 capacitive touch buttons, this badge has 3 big and 3 small ones.
 - this badge uses a user-supplied, removeable battery.

### Structural Components (WIP!)
 - PCB with these main components:
   - ESP32 microcontroller
   - microphone
   - 24 WS2812B LEDs
   - USB-C port for:
     - charging batteries
     - programming the microcontroller
   - 6 capacitive touch buttons
     - change pattern
     - change brightness
     - replace current pattern with new unique pattern
     - off
     - battery check
     - ? mystery spot
   - power button for On
   - small led for lower battery charge/currently charging indicator
   - battery holder attached to the back
      - requires one 18650 lithium-ion rechargeable battery
 - Case:
   - heart shaped ring on the front, same as the original blinky badge
   - back fully covers the back of the PCB as well as protects the battery holder
   - will have various points for attaching/hanging the badge

## How to write code for the badge!
 - Visual Studio Code
 - add the Espressif IDF extension
 - set up the extension
   - configure ESP-IDF extension, I recommend Express mode:  
     - install idf release version 5.5 (can prob use later versions too, but use at least this version)
     - no other configuration needed!
 - install CP210x Universal Windows Driver (or appropriate driver for your OS) from [CP2102N USB to UART Bridge Controller](https://www.silabs.com/software-and-tools/usb-to-uart-bridge-vcp-drivers?tab=downloads)
 - plug in badge to computer using USB-C cable that *supports data transmission*
 - open the blinky-badge-light/blinky-badge-light folder in VSCode
 - update code on badge using ESP-IDF:
   - FIRST TIME: 
     - go to ESP-IDF: EXPLORER (list of commands) from  icon in VS Code's plugin icon menu
     - select current ESP-IDF version (5.5 that you just installed)
     - select flash method - UART
     - 'Select Port to Use (COM, tty, usbserial)'
       - hold down 'ON' button on badge (or just make sure the badge is on)
       - click 'Select Port to Use (COM, tty, usbserial)'
       - pick the port that matches your badge (check in device manager if you're not sure)
     - 'Select Monitor Port to Use (COM, tty, usbserial)'
       - repeat ^ process and pick the same port
     - Full Clean
     - Build Project
     - Flash Device
       - ON button must be held down while flash happens
     - after flash is complete, release ON, then press and hold ON to start the badge
     - once badge has started, you can use 'Monitor Device' to see the serial monitor.
   - BUILDING AFTER THIS FIRST SETUP BUILD:
     - just do build/flash steps!
     - if you have problems:
       - check if the com port changed for some reason and reset them
       - full clean before rebuilding
 - Now that you know how to update the code, add your own fun stuff!
 - Code Ideas:
   - something using the antenna? 
   - cool new lighting patterns?
   - use the ? mystery spot capacitive touch pad to do something cool?

