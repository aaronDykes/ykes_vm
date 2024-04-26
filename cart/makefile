.PHONY	:= all clean
CC		:= clang
CFLAGS	:= -O2#-g -Wall -Wextra -MP -MD -pedantic
SRC		:= $(wildcard ./*.c)
OBJ		:= $(SRC:%.c=%.o)
YKES	:= ./

all: ykes

ykes:	$(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

%.o:	$(YKES)%.c
	$(CC) -I$(YKES)includes -c $< $(CFLAGS)

clean:
	rm -rf *.dSYM *.o *.d ykes
