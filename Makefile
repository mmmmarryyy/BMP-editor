all: hw-01_bmp
  
test: hw-01_bmp
	./hw-01_bmp crop-rotate samples/small-one.bmp OUTPUT_FILE 0 0 3 2

obj: 
	if [ ! -d "obj" ]; then mkdir "obj"; fi

obj/bmp.o: src/bmp.c 
	gcc -c src/bmp.c -Iinclude -o obj/bmp.o

obj/stego.o: src/stego.c
	gcc -c src/stego.c -Iinclude -o obj/stego.o

obj/main.o: src/main.c 
	gcc -c src/main.c -Iinclude -o obj/main.o

hw-01_bmp: obj obj/bmp.o obj/main.o obj/stego.o
	gcc obj/main.o obj/bmp.o obj/stego.o -o hw-01_bmp

clean:
	rm -R obj hw-01_bmp