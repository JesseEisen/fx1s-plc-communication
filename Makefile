all:
	gcc -g main.c debug.c serial.c util.c priqueue/local_queue.c plc.c -Ipriqueue -o serial -lpthread

clean:
	rm -rf serial a.out
