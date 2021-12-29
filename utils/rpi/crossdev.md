# Crosscompilation on the Raspberry Pi 

Cross compilation allows to compile from a computer with a different architecture from the target, in this case we compile from a x86_64 host to an arm device(embedded abi with hard floats).  

*These steps were produced on Ubuntu 21.10, with GLIBC 2.34 and GCC 11*

## sysroot

The sysroot folder will contain a copy of the libraries and includes that are on the Raspberry Pi, those libraries are binaries already in the "arm" format. 

```sh
# Where we will store our sysroot, you can change it
SYSPATH=/tools/pi4

mkdir $SYSPATH/sysroot
mkdir $SYSPATH/sysroot/usr

# The option -L transforms symlink into actual files, this prevent weird bug with symlinks
rsync -azL pi@raspberrypi:/lib $SYSPATH/sysroot
rsync -azL pi@raspberrypi:/usr/include $SYSPATH/sysroot/usr
rsync -azL pi@raspberrypi:/usr/lib $SYSPATH/sysroot/usr
```

## Compilation

```sh
# Install gcc with embedded ABI with Hard Float support 
sudo apt install gcc-arm-linux-gnueabihf
# NOTE: If the distribution does not come with a packaged cross compiler like this one, you need to build it yourself :)

# Compile
arm-linux-gnueabihf-gcc --sysroot=/tools/pi4/sysroot -L/tools/pi4/sysroot/lib/arm-linux-gnueabihf -std=c99 -Wall test.c -o test
# NOTE: the -L argument forces to use the GLIBC of the sysroot, yeah spent 2 hours on this...
```

## Remote debugging with gdbserver

https://developers.redhat.com/blog/2015/04/28/remote-debugging-with-gdb

```sh
# Remote
sudo apt install gdbserver

# Local
sudo apt install gdb-multiarch
```

### Start the gdbserver

```sh
gdbserver :9999 ./test
```

### Connect

```sh
gdb-multiarch -q

(gdb) target remote raspberrypi:9999
```

Or better, as it does both from the local computer

```sh
gdb-multiarch -q

(gdb) target remote | ssh pi@raspberrypi gdbserver - ./test
```

## vscode 

### With SSH -L 

https://medium.com/@spe_/debugging-c-c-programs-remotely-using-visual-studio-code-and-gdbserver-559d3434fb78
