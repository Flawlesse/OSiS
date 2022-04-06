build:
	gcc -g -pthread main.c -o lab6
run:
	./lab6 7 in.txt
clear:
	rm lab6 *.o *.s *.i test.txt
fill:
	python3 testdata.py fill in.txt
test:
	python3 testdata.py test output_paral.txt && python3 testdata.py test output_sync.txt