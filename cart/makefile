.PHONY	:= all clean
CC		:= clang
# CFLAGS	:= -O2
CFLAGS	:= -g -Wall -Wextra -MP -MD -pedantic
# CFLAGS	:= -g -MP -MD -pedantic
SRC		:= $(wildcard ./*.c)
OBJ		:= $(SRC:%.c=%.o)
VMYKES	:= ./

all: ykes

ykes:	$(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

%.o:	$(VMYKES)%.c
	$(CC) -I$(VMYKES)includes -c $< $(CFLAGS)

clean:
	rm -rf *.dSYM *.o *.d ykes
