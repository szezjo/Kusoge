target: client botapp server

client: player.c client_functions.c
	@gcc -g -Wall -pedantic player.c client_functions.c -lncurses -pthread -lrt -o player.out
	@echo "Compiled: client"

botapp: bot.c bot_functions.c
	@gcc -g -Wall -pedantic bot.c bot_functions.c -lncurses -pthread -lrt -o bot.out
	@echo "Compiled: bot"

server: main.c server_functions.c
	@gcc -g -Wall -pedantic main.c server_functions.c -lncurses -pthread -lrt -o srv.out
	@echo "Compiled: server"
	@-rm /dev/shm/* || true
	@echo "Semaphores cleaned"

clean:
	@-rm *.out || true
	@echo "Binary files removed"

sem:
	@-rm /dev/shm/* || true
	@echo "Semaphores cleaned"