# M1-Projet

Projet M1 2021-2022

## Goal

The goal is to build an aggregate of small libraries to quickly use when working on different embedded cards.  
These libraries need to be easy to use but flexible enough to be able to do advanced stuff with them.  
 
## Usage

Add to your include path the `include` folder of the card you are working on, for instance `rpi/include`.  

```c
#include <sense-hat/joystick.h>

int main(void) {

        /** Use any function you need from the library **/
        return 0;
}
```

```sh
cc main.c -I./rpi/include -std=c11 -o test
``` 

See the `utils` folder for more examples and helpful utilities.

## Cards

### Rasbperry Pi 

#### Sense-Hat by element14

- *Add what has been done*

#### ARPI600 by xBee

- *Add what has been done*

### Arduino Due

*Add what has been done*

### Atmetl xPlain Mini

*Add what has been done*

#### Arduino Sensor Kit 

- *Add what has been done*
  
## Projet Guidelines

- Only use lowercase name with dash for naming files, all filesystem are not case-insensitive like Windows...  
- Use `.h` for C headers and `.hpp` for C++ headers
- There should not be any `.c` (or `.cpp`) file in any library
- Keep all your `.c` files for testing in the `example` folder, this will always be useful for people

### Attribution

Put your name at the top of the file, if you are adding something to someone else file add your name under it like so: 

```c
/*
 * A phrase to tell what the file does
 * 
 * (c) John Smith 
 * (c) John Doe   
 *  
 * More details about what the code does
 * this is usually more than one line
 */
``` 

If you are using code from previous projects **do not forget** to add contributors' names!

### Clang-Format

Please use the .clang-format at the root of this git to ***properly*** format the code, it does the job automatically for you, you have no excuse for not using it.  
Currently the clang-format used is from the Linux Kernel project.

#### From VSCode

`Alt+Shift+F` should work by default on Windows or Linux.  
`Cmd+Shift+F` for macOS.

#### From command line

```sh
clang-format -i <file>
# Or
clang-format -i *
```

