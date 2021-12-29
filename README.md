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

See the *utils* folder for more examples and helpful utilities.

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
  
## Project Guidelines

- Only use lowercase name with dash for naming files, all filesystem are not case-insensitive like Windows...  
- Use `.h` for C headers and `.hpp` for C++ headers
- There should not be any `.c` (or `.cpp`) file in any library
- Keep all your `.c` files for testing in the `example` folder, this will always be useful for people

### Attribution

Put your name at the top of the file, if you are adding something to someone else file add your name under it like so: 

```c
/*\*
 * @brief A phrase to tell what the file does
 * 
 * @file file.h
 * @copyright (c) John Smith 
 * @copyright (c) John Doe   
 *  
 * @details
 * More details about what the code does
 * this is usually more than one line
 * 
 * @warning A warning if there are important things the user needs to know
 */
``` 

*Note: please remove the `\` in-between the `/*\*`, this is here to prevent Doxygen from interpreting this comment block.*

If you are using code from previous projects **do not forget** to add contributors' names!

### Clang-Format

Please use the `.clang-format` at the root of this git to ***properly*** format the code, it does the job automatically for you, you have no excuse for not using it.  
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

## Documentation

You can quickly generate the doxygen documentation using the Doxyfile configuration present at the root of the project.
The generated documentation will be found in the [doc](doc/) folder, open the [index.html](doc/html/index.html) file to navigate it.  
You need to have `doxygen` and `dot` installed, on Debian systems you can use the code below:

```sh
# Install required packages
sudo apt install doxygen graphviz

# Generate documentation
doxygen Doxyfile
```

You can also use the `doxywizard` utility to easily modify the configuration, this utility is found in the package `doxygen-gui` in Debian based distros.
