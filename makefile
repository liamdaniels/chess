CC = clang

CFLAGS = -Wall -Werror 
CFLAGS2 = -ansi -c

SDLOBJ = ../../my_API/sdl/sdl_util.o

all: chess

chess: display.o chess.o chess_bot.o $(SDLOBJ)
	$(CC) $(CFLAGS) display.o chess.o chess_bot.o $(SDLOBJ) -o chess -lSDL2 -lSDL2_image

botbattle: bot_fighter.o chess.o chess_bot.o
	$(CC) $(CFLAGS) bot_fighter.o chess.o chess_bot.o -o botbattle

display.o: display.c 
	 $(CC) $(CFLAGS) $(CFLAGS2) display.c

chess.o: chess.c
	$(CC) $(CFLAGS) $(CFLAGS2) chess.c

chess_bot.o: chess_bot.c
	$(CC) $(CFLAGS) $(CFLAGS2) chess_bot.c

bot_fighter.o:
	$(CC) $(CFLAGS) $(CFLAGS2) bot_fighter.c

clean:
	rm *.o botbattle chess
