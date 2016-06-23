build: audioserver.o audioclient.o audio.o
	gcc audioserver.o audio.o -o audioserver
	gcc audioclient.o audio.o -o audioclient
	rm -rf* .o
clean:
	rm -rf audioserver
	rm -rf audioclient
	
