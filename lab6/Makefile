build:
	gcc -O0 -g -pthread main.c -o lab6
run:
	./lab6 32 in.txt
clear:
	rm lab6 *.o *.s *.i test.txt
fill:
	python3 testdata.py fill in.txt
test:
	python3 testdata.py test output_paral.txt && python3 testdata.py test output_sync.txt