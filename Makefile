all: intel64 gmsk

intel64: search_binary.c
	gcc -o search_binary search_binary.c -Wall -lm -mpopcnt -O3 -march=native -mtune=native -DINTEL_POPCNT

intel32: search_binary.c
	gcc -o search_binary search_binary.c -Wall -lm -mpopcnt -O3 -march=native -mtune=native -DINTEL_POPCNT -DWIDTH_32BIT

nonintel64: search_binary.c
	gcc -o search_binary search_binary.c -Wall -lm -O3 -march=native -mtune=native

nonintel32: search_binary.c
	gcc -o search_binary search_binary.c -Wall -lm -O3 -march=native -mtune=native -DWIDTH_32BIT

gmsk: search_gmsk.c
	gcc -o search_gmsk search_gmsk.c -Ofast -march=native -mtune=native -lm -Wall