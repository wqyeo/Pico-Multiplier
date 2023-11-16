# Pico Multiplier
A simple multiplication calculator, on Raspberry Pi Pico W with Maker Pi Pico

## What it does

![maker-pi-pico-board-reference](https://github.com/wqyeo/Pico-Multiplier/assets/25131995/ad550458-ada2-4cba-9176-e9a1d94d6576)

With the Pico W attached on top of the Maker Board _(see image for reference of maker Board)_, the Pico W will:
- Count the number of presses on `GP20` and `GP21` respectively.
- The counted number on `GP20`, stored as variable `A`, will be displayed in 4-bit binary form. Ranging from `GP2` to `GP5`, with `GP2` being the MSB _(most significant bit)_.
- The counted number on `GP21`, stored as variable `B`, will be displayed in 4-bit binary form. Ranging from `GP6` to `GP9`, with `GP6` being the MSB.
- Pressing `GP22` will perform a multiplication on `A` and `B`. The value will be displayed in 6-bit binary form, ranging from `GP10` to `GP15`, with `GP15` being the MSB.
  - In the event that the result is unable to fit into 6 bits (overflow), all LEDs from `GP10` to `GP15` will light up, and `GP0` will become a 'breathing' light. _(Basically, slowly dim up/down in brightness)_
- After `GP22` is pressed, system will wait for 5 seconds, then reset all led/button/variable value states.
  - During 5 second wait time, buttons are unresponsive.
 
> **NOTE_1**: `GP0` uses PWM to drive the signal, while `GP2` to `GP15` are signalled using standard `GPIO`.<br>
> **NOTE_2**: `GP20` and `GP21` isn't expected to handle more than 16 presses. _(aka, more tan 4-bits of presses)_

## How to Run

> **NOTE:** Install [Pico C SDK](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf) if you haven't done so yet, with enviorment variable setup.

### Build with CMake

- In the `src` directory, make a new folder called `build`.
- Open a terminal, access the `build` folder.
- Run `cmake ..`
- Run `build`

A `main.uf2` file should appear in the `build` directory.

### Running the file

With the Raspberry Pi Pico W:

- Press and hold the button on the Pico W, then connect via USB to your PC.
- After a 3 seconds or so, let go of the button.
 - you should see a new device media _(Should be somthing like 'Pi2')_ appear on your respective file explorer program.
- Drop and drop the `main.uf2` into the new device media.

Enjoy ㊗️
