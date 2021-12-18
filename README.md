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
