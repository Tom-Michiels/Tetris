# Tetris

I made this Tetris game console as a secret santa present for organized for the Facebook group [Make in Belgium](https://www.facebook.com/groups/makeinbelgium/)
![image of the tetris game](images/tetris_working.jpeg)

The game console was made out of:
- lasercut 
  - 3.6mm plywood for the [enclosure](hardware/lasercut%20parts/enclosure.png)
  - 3mm tinted plexiglass for the [screen](hardware/lasercut%20parts/screen.png)
- 3D printed PLA:
  - a [grid for holding the leds](hardware/3d%20printed%20parts/grid.png) and [mounting plates](hardware/3d%20printed%20parts/mount.png) 
  - [USB charger](hardware/3d%20printed%20parts/usb%20charger.png) 
  - [the buttons](hardware/3d%20printed%20parts/button.png) 
- Some electronic components
  - an arduino nano (or any other arduino)
  - TP4056 Lithium Battery Charger Module
  - a DC/DC Lithium Battery Boost Module Step Up Board
  - two WS2812 Led 5050 Rgb 8X8 64 Led Matrices
  - a 4cm x 6cm prototyping PCB
  - tactile switches (6x6x7mm)
  - a passive electronic buzzer
  - 4 M3 x 5 mm spacers
  - 1 MiMoo On/Off rocker switch
  - a (recycled from an old laptop) 18650 Li-Ion battery
- some threaded inserts (ruthex RX-M3x5.7 Brass) and M3x6mm bolts with button head socket

![image of the tetris game](images/internals.jpeg)

## Seeed XIAO ESP32-C3 version

`src/xiao_esp32c3/main.cpp` runs the same console on a Seeed XIAO ESP32-C3, with a game menu and wifi.
Build and flash it with PlatformIO: `pio run -e seeed_xiao_esp32c3 -t upload` (the Nano build is unchanged: `pio run -e nanoatmega328`).

| | Nano | XIAO ESP32-C3 |
|---|---|---|
| LED matrix data | D6 | D10 |
| buzzer | D11 | D1 |
| left / right / up / down | D7 / D8 / D10 / D9 | D2 / D3 / D4 / D5 |

**Games:** tetris, snake, dodge (steer through falling walls, hold down to go faster), bricks (paddle and ball, up launches) and memory (repeat the lights; the zones sit like the buttons).
In the menu left/right picks a game and up or down starts it. Hold up + down for about a second in any game to go back to the menu.
Every game keeps its best score in flash.

**Wifi:** after 20 s without a known network the console opens an access point `tetris-setup`; join it with a phone to pick the network.
Hold up + down while switching on to forget the network. On the network:
- `http://tetris.local/play` shows the screen in the browser and can be played from there (buttons or arrow keys)
- `http://tetris.local/` shows the state of every button pin, handy for checking the wiring
- new firmware can be flashed over the air (network port `tetris`); set your own `OTA_PASS` first

**Power:** after 10 minutes without a button press it switches the LEDs off and goes to deep sleep, left or right wakes it.
With a 2x 220k divider from the battery to D0, set `BATTERY_SENSE` to 1 to see the battery level and to sleep before the cell runs empty.

**Tests:** `test/host_esp32c3/run.sh` runs the game logic on your computer with stubbed Arduino libraries.
