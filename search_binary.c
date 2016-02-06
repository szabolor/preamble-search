#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

/*
Compile command on Intel CPU with POPCNT flag (check the flags at `/proc/cpuinfo`):
  `gcc -o search_binary search_binary.c -Wall -mpopcnt -O3 -march=native -mtune=native -DINTEL_POPCNT`
Or compile without POPCNT (the runtime will be about 20 times longer...):
  `gcc -o search_binary search_binary.c -Wall -O3 -march=native -mtune=native`
*/
/*
 Performance counter stats for './search_binary':

     142411,731942      task-clock (msec)         #    0,999 CPUs utilized          
            14.331      context-switches          #    0,101 K/sec                  
                 1      cpu-migrations            #    0,000 K/sec                  
                48      page-faults               #    0,000 K/sec                  
   415.023.475.658      cycles                    #    2,914 GHz                    
       436.976.800      stalled-cycles-frontend   #    0,11% frontend cycles idle   
    58.352.558.834      stalled-cycles-backend    #   14,06% backend  cycles idle   
   910.885.432.229      instructions              #    2,19  insns per cycle        
                                                  #    0,06  stalled cycles per insn
    75.236.749.248      branches                  #  528,304 M/sec                  
         1.106.548      branch-misses             #    0,00% of all branches        

     142,491557005 seconds time elapsed
*/

/* set the length of the preables; must be <= 32 (4 bytes) */
int bitlen;
unsigned int bitmask; // = (0xffffffff >> (32 - bitlen));
/* Only test the half of the codespace, because the negated pairs have the
   same autocorrelation, so fix the first bit to zero!
   Unfortunately still the reversed sequences remains.
   (e.g. 0b0101001100001001111110 and 0b0111111001000011001010 have the same
   autocorrelation) */
int stop_limit; // = 1 << (bitlen - 1);

/* enable this flag for explicit Intel POPCNT support */
/* #define INTEL_POPCNT */

#ifndef INTEL_POPCNT
  /* Trick from: http://graphics.stanford.edu/~seander/bithacks.html#ParityLookupTable */
  #define BIT_SET_COUNT(x) (bit_set_count(x))
  static const unsigned char BitsSetTable256[256] = {
    #define B2(n) n,     n+1,     n+1,     n+2
    #define B4(n) B2(n), B2(n+1), B2(n+1), B2(n+2)
    #define B6(n) B4(n), B4(n+1), B4(n+1), B4(n+2)
    B6(0), B6(1), B6(1), B6(2)
  };
  static inline int bit_set_count(uint32_t v) {
    return  BitsSetTable256[v & 0xff] + 
            BitsSetTable256[(v >> 8) & 0xff] + 
            BitsSetTable256[(v >> 16) & 0xff] + 
            BitsSetTable256[v >> 24]; 
  }
#else
  #include <nmmintrin.h>
  #define BIT_SET_COUNT(x) (_mm_popcnt_u32(x))
#endif


int psl(unsigned int val) {
  int i, curr, max = -bitlen;
  unsigned int pon = val >> 1;
  unsigned int neg = ((~val) & bitmask) >> 1;

  for (i = 1; i < bitlen; ++i) {
    curr = BIT_SET_COUNT(neg ^ val) - BIT_SET_COUNT(pon ^ val);
    if (curr < 0) {
      curr *= -1;
    }
    if (curr > max) {
      max = curr;
    }
    pon >>= 1;
    neg >>= 1;
  }
  return max;
}

void print_autocorr(unsigned int val) {
  int i;
  unsigned int pon = val >> 1;
  unsigned int neg = ((~val) & bitmask) >> 1;

  printf("[%d", bitlen);
  for (i = 1; i < bitlen; ++i) {
    //printf("\npos: %08x; neg: %08x; mask: %d", pon, neg, BITMASK);
    printf(", %d", BIT_SET_COUNT(neg ^ val) - BIT_SET_COUNT(pon ^ val));
    pon >>= 1;
    neg >>= 1;
  }
  printf("]");
}

int main(int argc, char *argv[]) {
  /*
    Thresholds: 20*np.log10(threshold/32)
      6: -14.5 dB
      5: -16.1 dB
      4: -18.1 dB
      3: -20.6 dB
      2: -24.1 dB
      1: -30.1 dB
  */
  unsigned int i, start, stop;
  int curr, threshold;
  char *p;

  if (argc != 5) {
    printf("Usage: `%s <bitlen> <THRESHOLD> <START> <STOP>`\n", argv[0]);
    exit(1);
  }

  bitlen = (int) strtol(argv[1], &p, 10);
  if (errno != 0 || *p != '\0' || bitlen < 2 || bitlen > 32) {
    printf("bitlen error`\n");
    exit(1);
  }

  stop_limit = 1 << (bitlen - 1);
  bitmask = (0xffffffff >> (32 - bitlen));

  threshold = (int) strtol(argv[2], &p, 10);
  if (errno != 0 || *p != '\0') {
    printf("THRESHOLD error`\n");
    exit(1);
  }

  start = (unsigned int) strtoul(argv[3], &p, 10);
  if (errno != 0 || *p != '\0') {
    printf("START error`\n");
    exit(1);
  }

  stop = (unsigned int) strtoul(argv[4], &p, 10);
  if (errno != 0 || *p != '\0') {
    printf("STOP error`\n");
    exit(1);
  }

  if (stop > stop_limit) {
    stop = stop_limit;
  }

  for (i = start; i < stop; ++i) {
    if (!(i & 0x003fffff)) {
      fprintf(stderr, "Currently searching at i = %08x\n", i);
    }
    curr = psl(i);
    
    if (curr <= threshold) {
      printf("%08x\t%d\t", i, curr);
      print_autocorr(i);
      printf("\n");
    }
  }

  return 0;
}