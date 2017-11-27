#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <math.h>

/*
Compile command on Intel CPU with POPCNT flag (check the flags at `/proc/cpuinfo`):
  `gcc -o search_binary search_binary.c -Wall -mpopcnt -O3 -march=native -mtune=native -DINTEL_POPCNT -lm -fopenmp`
Or compile without POPCNT (the runtime will be about 2-3 times longer...):
  `gcc -o search_binary search_binary.c -Wall -O3 -march=native -mtune=native`
*/
/* For `./search_binary 32 3 0 -1`
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

// enable this flag for explicit Intel POPCNT support
// #define INTEL_POPCNT

// enable this flag for explicit 32bit support
// #define WIDTH_32BIT

#ifdef WIDTH_32BIT
  typedef uint32_t num_type;
  #define NUM_TYPE_FORMAT "%#0*x"
  #define BIT_MAX_LENGTH 32
  #define BIT_SATURATED (0xffffffffU)
  #define POPCNT_FUNC(x) (_mm_popcnt_u32(x))
  static const unsigned char BitsSetTable256[256] = {
    #define B2(n) n,     n+1,     n+1,     n+2
    #define B4(n) B2(n), B2(n+1), B2(n+1), B2(n+2)
    #define B6(n) B4(n), B4(n+1), B4(n+1), B4(n+2)
    B6(0), B6(1), B6(1), B6(2)
  };
  static inline int bit_set_count(num_type v) {
    return  BitsSetTable256[v & 0xff] + 
            BitsSetTable256[(v >> 8) & 0xff] + 
            BitsSetTable256[(v >> 16) & 0xff] + 
            BitsSetTable256[v >> 24]; 
  }
#else
  // or use uint64_t with casts at printf %llx
  typedef long long unsigned int num_type;
  #define NUM_TYPE_FORMAT "%#0*llx"
  #define BIT_MAX_LENGTH 64
  #define BIT_SATURATED (0xffffffffffffffffULL)
  #define POPCNT_FUNC(x) (_mm_popcnt_u64(x))
  static inline int bit_set_count(num_type x) {
    x = x - ((x >> 1) & 0x5555555555555555UL);
    x = (x & 0x3333333333333333UL) + ((x >> 2) & 0x3333333333333333UL);
    return (int)((((x + (x >> 4)) & 0xF0F0F0F0F0F0F0FUL) * 0x101010101010101UL) >> 56);
  }
#endif

#define STRINGIZE_NX(A) #A
#define STRINGIZE(A) STRINGIZE_NX(A)

#ifdef INTEL_POPCNT
  #pragma message ("Compiling with Intel POPCNT support and " STRINGIZE(BIT_MAX_LENGTH) " bit length")
  #include <nmmintrin.h>
  #define BIT_SET_COUNT(x) POPCNT_FUNC(x)
#else
  #pragma message ("Compiling without Intel POPCNT support and " STRINGIZE(BIT_MAX_LENGTH) " bit length")
  /* Trick from: http://graphics.stanford.edu/~seander/bithacks.html#ParityLookupTable */
  #define BIT_SET_COUNT(x) (bit_set_count(x))
#endif

/* set the length of the preables; must be <= 32 (4 bytes) */
int bitlen;
num_type bitmask; // = (0xffffffff >> (32 - bitlen));
num_type stop_limit; // = 1 << (bitlen - 1);
int num_type_length;

/*
  Peak Sidelobe Level:
  psl(x) = Max[Table[Sum[x[i] * x[i - lag], {i, lag, bitlen}], {lag, 1, bitlen}]]
*/
int psl(num_type val) {
  int i, curr, max = -bitlen;
  num_type pon = val >> 1;
  num_type neg = ((~val) & bitmask) >> 1;

  for (i = 1; i < bitlen; ++i) {
    curr = BIT_SET_COUNT(neg ^ val) - BIT_SET_COUNT(pon ^ val);
    if (curr < 0) {
      curr = -curr;
    }
    if (curr > max) {
      max = curr;
    }
    pon >>= 1;
    neg >>= 1;
  }
  return max;
}

/*
  Merit Factor / Integrated Sidelobe Level (ISL) / score
    mf(x) = Total[#^2 & /@ Table[Sum[x[i] * x[i - lag], {i, lag, bitlen}], {lag, 1, bitlen}]]
    defines the "waviness" of the sidelobe
    the smaller the better
    only comparable for the same bitlen (? - not sure...)
  Sort the output with `sort -t $'\t' -k 4,4 -n PSL_3_mf  > PSL_3_mf_sorted_by_mf`
*/
void print_autocorr_and_score(num_type val) {
  int i, curr, sum = 0;
  num_type pon = val >> 1;
  num_type neg = ((~val) & bitmask) >> 1;

  printf("[%2d", bitlen);
  for (i = 1; i < bitlen; ++i) {
    curr = (int) (BIT_SET_COUNT(neg ^ val)) - (int) BIT_SET_COUNT(pon ^ val);
    sum += curr * curr;
    printf(",%2d", curr);
    pon >>= 1;
    neg >>= 1;
  }
  printf("]  ");
  // The square sum and its normalized logaritm of the sidelobe
  printf("%4d %4.4f", sum, 10.0 * log10((double) sum / (double) (bitlen * bitlen)));
}

int main(int argc, char *argv[]) {
  num_type i, start, stop;
  int threshold;
  char *p;

  if (argc != 5) {
    fprintf(stderr, "Usage: `%s <bitlen> <THRESHOLD> <START> <STOP>`\n", argv[0]);
    exit(1);
  }

  bitlen = (int) strtol(argv[1], &p, 10);
  if (errno != 0 || *p != '\0' || bitlen < 2 || bitlen > BIT_MAX_LENGTH) {
    fprintf(stderr, "BITLEN error\n");
    exit(1);
  }

  /* Only test the half of the codespace, because the negated pairs have the
     same autocorrelation, so fix the first bit to zero!
     Unfortunately still reversed sequences remains.
     (e.g. 0b0101001100001001111110 and 0b0111111001000011001010 have the same
     autocorrelation) */
  stop_limit = 1UL << (bitlen - 1);
  bitmask = (BIT_SATURATED >> (BIT_MAX_LENGTH - bitlen));

  // Round to hex characters and `+ 2` for the "0x" prefix
  num_type_length = ((bitlen + 3) >> 2) + 2; 

  threshold = (int) strtol(argv[2], &p, 10);
  if (errno != 0 || *p != '\0') {
    fprintf(stderr, "THRESHOLD error\n");
    exit(1);
  }

  start = (num_type) strtoll(argv[3], &p, 10);
  if (errno != 0 || *p != '\0') {
    fprintf(stderr, "START error\n");
    exit(1);
  }

  stop = (num_type) strtoll(argv[4], &p, 10);
  if (errno != 0 || *p != '\0') {
    fprintf(stderr, "STOP error\n");
    exit(1);
  }

  if (stop > stop_limit) {
    stop = stop_limit;
  }

  fprintf(stderr, "Search settings:\n");
  fprintf(stderr, "  bit length = %d\n", bitlen);
  fprintf(stderr, "  num type length = %d\n", num_type_length - 2);
  fprintf(stderr, 
          "   -> (so omit the first (MSB) %d bits from the number representation)\n",
         ((num_type_length - 2) << 2) - bitlen);
  fprintf(stderr, "  threshold = %d\n", threshold);
  fprintf(stderr, "  start = " NUM_TYPE_FORMAT "\n", num_type_length, start);
  fprintf(stderr, "  stop = " NUM_TYPE_FORMAT "\n", num_type_length, stop);
  fprintf(stderr, "  stop limit = " NUM_TYPE_FORMAT "\n", num_type_length, stop_limit);

  #pragma omp parallel for 
  for (i = start; i < stop; ++i) {
    int curr;
//    if (!(i & 0x3fffff)) { // this makes to print status at rate of ~1/sec
//      fprintf(stderr, "Currently searching at i = " NUM_TYPE_FORMAT "\n", num_type_length, i);
//    }
    curr = psl(i);
    
    if (curr <= threshold) {
      printf(NUM_TYPE_FORMAT " %2d ", num_type_length, i, curr);
      print_autocorr_and_score(i);
      printf("\n");
    }
  }

  return 0;
}
