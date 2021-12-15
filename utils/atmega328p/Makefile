####################################################################
# This makefile helps compiling and uploading to the xPlain Mini   #
#                                                                  #
# (c) Pierre Boisselier                                            # 
####################################################################

####################
# Project Settings #
####################

# Project name 
PROJECTNAME=test

# Microcontroller Settings
UC=ATmega328PB
DFP=$(XC8_DFP)
COM_PORT=/dev/ttyACM0
AVRDUDE_CONF=/etc/avrdude.conf

# Compilation Flags
CC=xc8-cc
CFLAGS=-I../xplain/include -mcpu=$(UC) -c -x c -funsigned-char -funsigned-bitfields -mext=cci -Og -ffunction-sections -fdata-sections -fpack-struct -fshort-enums -g2 -Wall 
CPPFLAGS=-D__$(UC)__ -DDEBUG
LDFLAGS=-mcpu=$(UC) -mdfp=$(DFP) -funsigned-char -funsigned-char -funsigned-bitfields -Og -ffunction-sections -fdata-sections -fpack-struct -fshort-enums 

############
# Makefile #
############

# Files
SRC=$(wildcard *.c) 
OBJ=$(SRC:.c=.o)

all: $(PROJECTNAME)

# Generate binary files
$(PROJECTNAME): $(PROJECTNAME).elf
	avr-objcopy -O ihex -R .eeprom -R .fuse -R .lock -R .signature -R .user_signatures $< $(PROJECTNAME).hex
	avr-objcopy -j .eeprom --set-section-flags=.eeprom=alloc,load --change-section-lma .eeprom=0 --no-change-warnings -O ihex $< $(PROJECTNAME).eep 
	avr-objdump -h -S $< > $(PROJECTNAME).lss

$(PROJECTNAME).elf: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)


include depends.d

.PHONY: clean upload

# Upload binary file to controller
upload:
	    avrdude -C $(AVRDUDE_CONF) -c arduino -F -v -p $(UC) -P $(COM_PORT) -D -U flash:w:"$(PROJECTNAME).hex":i

# Clean build artifacts 
clean: 
	rm -f depends.d $(OBJ) $(PROJECTNAME).elf $(PROJECTNAME).hex $(PROJECTNAME).lss $(PROJECTNAME).eep

# Generate dependencies
depends.d: $(SRC)
	$(CC) $(CFLAGS) $(CPPFLAGS) -M $(SRC) > depends.d