intel64: search_binary.c
	gcc -o search_binary search_binary.c -Wall -mpopcnt -O3 -march=native -mtune=native -DINTEL_POPCNT

intel32: search_binary.c
	gcc -o search_binary search_binary.c -Wall -mpopcnt -O3 -march=native -mtune=native -DINTEL_POPCNT -DWIDTH_32BIT

nonintel64: search_binary.c
	gcc -o search_binary search_binary.c -Wall -O3 -march=native -mtune=native

nonintel32: search_binary.c
	gcc -o search_binary search_binary.c -Wall -O3 -march=native -mtune=native -DWIDTH_32BIT
