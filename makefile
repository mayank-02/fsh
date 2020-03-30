program: shell.o parse.o
		gcc shell.o parse.o -o program -g3
	
shell.o: shell.c shell.h
		gcc shell.c -c -g3 

parse.o: parse.c parse.h
		gcc parse.c -c -g3
		
run:
	./program

clean:
	rm *.o program 

A3_111703021.tar.gz:
		tar -zvcf A3_111703021.tar.gz shell.c parse.c shell.h parse.h makefile