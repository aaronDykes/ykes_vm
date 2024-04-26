.PHONY	:= all clean
CC		:= clang
CFLAGS	:= -g -Wall -Wextra -MP -MD
SRC		:= $(wildcard ./*.c)
OBJ		:= $(SRC:%.c=%.o)
YKES	:= ./

all: ykes

ykes:	$(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) -L$(YKES)lib -larena

%.o:	$(YKES)%.c
	$(CC) -I$(YKES)../arena -I$(YKES)includes -c $< $(CFLAGS)

clean:
	rm -rf *.dSYM *.o *.d ykes
