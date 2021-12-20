# emulator-8080
A simple emulator for the Intel 8080 microprocessor written in C, based on the fantastic emulator101.com tutorial.

## Emulator
emulator.c contains the source code for the emulator itself. It takes a binary file as an input and spits out some debugging information as it steps through and emulates the instructions. It's very early and probably filled with bugs at the moment, and most 8080 instructions are not implemented yet.

### Usage
1. Compile using your favorite compiler. I use `gcc`:

```
gcc <path_to_emulator.c> -o <path_to_output>
```

2. Obtain an 8080-compatible ROM file. In the future, I may include some in this repo. For now, it is up to you to obtain one.
3. Run the following:

```
./<path_to_output> <path_to_rom>
```

## Disassembler
disassembler.c contains source code for a very basic disassembler, which takes a binary file as an input and prints it out as valid 8080 assembly code.

### Usage
1. Compile using your favorite compiler. I use `gcc`:

```
gcc <path_to_disassembler.c> -o <path_to_output>
```

2. Obtain an 8080-compatible ROM file. In the future, I may include some in this repo. For now, it is up to you to obtain one.
3. Run the following:

```
./<path_to_output> <path_to_rom>
```


## Latest Progress
I implemented enough operations to get through the first 50,000 or so instructions of the Space Invaders ROM. Comparing with an existing 8080 emulator, the states seem to match up until it gets into an infinite loop that's waiting for an interrupt, which hasn't been implemented.

The next steps would be:
- Set up proper way of testing the emulator using a test ROM
- Finish implementing the rest of the operations
- Implement rest of the Space Invaders arcade machine (graphics, sound, interrupts, buttons, etc.)
