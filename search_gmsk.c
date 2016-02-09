#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>

static const int sample_per_bit = 8;
static const double weight_len = 23;

// Gauss shaped pulse for BT=0.5 and multiplied by Pi/2
static const float weight[] = {
  5.26267033460816e-05, 2.86280251398993e-04, 1.28276378930330e-03,
  4.76287011252231e-03, 1.47655382034551e-02, 3.85876747231160e-02,
  8.60336720446424e-02, 1.66062686377383e-01, 2.82331562430371e-01,
  4.31000574017815e-01, 6.02531404900838e-01, 7.85398163397448e-01,
  9.68264921894059e-01, 1.13979575277708e+00, 1.28846476436453e+00,
  1.40473364041751e+00, 1.48476265475025e+00, 1.53220865207178e+00,
  1.55603078859144e+00, 1.56603345668237e+00, 1.56951356300559e+00,
  1.57051004654350e+00, 1.57074370009155e+00
};


void GMSK_autocorr_with_print(double *phase, const int phase_len) {
  int lag, i;
  double current_autocorr_phase;
  double autocorr_sum_re, autocorr_sum_im;
  double max = -phase_len, current_max = -phase_len;
  double prev, curr = phase_len;
  int is_going_up = 0;

  printf("[");
  for (lag = 1; lag < phase_len; ++lag) {
    autocorr_sum_re = 0.0;
    autocorr_sum_im = 0.0;
    for (i = lag; i < phase_len; ++i) {
      current_autocorr_phase = phase[i] - phase[i - lag];
      autocorr_sum_re += cos(current_autocorr_phase);
      autocorr_sum_im += sin(current_autocorr_phase);
    }
    prev = curr;
    curr = sqrt(autocorr_sum_im*autocorr_sum_im + autocorr_sum_re*autocorr_sum_re);
    // For unbiased measures: `curr_unbiased = curr / (1.0 - (double) lag / (double) phase_len)`
    printf("%f, ", curr);

    if (curr > prev) {
      // A new up-going range is about to begin
      is_going_up = 1;
      current_max = curr;
    } 
    else if (curr < prev && is_going_up == 1) {
      // The up-going range is ended, refresh the max
      is_going_up = 0;
      if (current_max > max) {
        max = current_max;
      }
    }
  }

  if (current_max > max) {
    max = current_max;
  }

  printf("]\t%f\n", max);
}


double GMSK_autocorr(double *phase, const int phase_len) {
  int lag, i;
  double current_autocorr_phase;
  double autocorr_sum_re, autocorr_sum_im;
  double max = -phase_len, current_max = -phase_len;
  double prev, curr = phase_len;
  int is_going_up = 0;

  for (lag = 1; lag < phase_len; ++lag) {
    autocorr_sum_re = 0.0;
    autocorr_sum_im = 0.0;
    for (i = lag; i < phase_len; ++i) {
      current_autocorr_phase = phase[i] - phase[i - lag];
      autocorr_sum_re += cos(current_autocorr_phase);
      autocorr_sum_im += sin(current_autocorr_phase);
    }
    prev = curr;
    curr = sqrt(autocorr_sum_im*autocorr_sum_im + autocorr_sum_re*autocorr_sum_re);

    if (curr > prev) {
      // A new up-going range is about to begin
      is_going_up = 1;
      current_max = curr;
    } 
    else if (curr < prev && is_going_up == 1) {
      // The up-going range is ended, refresh the max
      is_going_up = 0;
      if (current_max > max) {
        max = current_max;
      }
    }
  }

  if (current_max > max) {
    max = current_max;
  }
  return max;
}

void GMSK_phase(const int *bits, const int bits_len, double *phase) {
  int i;
  double bit_sum = 0.0;

  // shift in the first two bits with a zero prepended bit
    *phase = bits[0] * weight[ 7];
  *++phase = bits[0] * weight[ 8] + bits[1] * weight[0];
  *++phase = bits[0] * weight[ 9] + bits[1] * weight[1];
  *++phase = bits[0] * weight[10] + bits[1] * weight[2];
  *++phase = bits[0] * weight[11] + bits[1] * weight[3];
  *++phase = bits[0] * weight[12] + bits[1] * weight[4];
  *++phase = bits[0] * weight[13] + bits[1] * weight[5];
  *++phase = bits[0] * weight[14] + bits[1] * weight[6];

  for (i = 0; i < bits_len - 2; ++i) {
    *++phase = bit_sum + bits[i] * weight[15] + bits[i+1] * weight[ 7];
    *++phase = bit_sum + bits[i] * weight[16] + bits[i+1] * weight[ 8] + bits[i+2] * weight[0];
    *++phase = bit_sum + bits[i] * weight[17] + bits[i+1] * weight[ 9] + bits[i+2] * weight[1];
    *++phase = bit_sum + bits[i] * weight[18] + bits[i+1] * weight[10] + bits[i+2] * weight[2];
    *++phase = bit_sum + bits[i] * weight[19] + bits[i+1] * weight[11] + bits[i+2] * weight[3];
    *++phase = bit_sum + bits[i] * weight[20] + bits[i+1] * weight[12] + bits[i+2] * weight[4];
    *++phase = bit_sum + bits[i] * weight[21] + bits[i+1] * weight[13] + bits[i+2] * weight[5];
    *++phase = bit_sum + bits[i] * weight[22] + bits[i+1] * weight[14] + bits[i+2] * weight[6];
    
    bit_sum += M_PI_2 * bits[i]; // = Pi * 1/2, so c = 1/2 (!)
  }


  *++phase = bit_sum + bits[i] * weight[15] + bits[i+1] * weight[ 7];
  *++phase = bit_sum + bits[i] * weight[16] + bits[i+1] * weight[ 8];
  *++phase = bit_sum + bits[i] * weight[17] + bits[i+1] * weight[ 9];
  *++phase = bit_sum + bits[i] * weight[18] + bits[i+1] * weight[10];
  *++phase = bit_sum + bits[i] * weight[19] + bits[i+1] * weight[11];
  *++phase = bit_sum + bits[i] * weight[20] + bits[i+1] * weight[12];
  *++phase = bit_sum + bits[i] * weight[21] + bits[i+1] * weight[13];
  *++phase = bit_sum + bits[i] * weight[22] + bits[i+1] * weight[14];
    
  bit_sum += M_PI_2 * bits[i];
  *++phase = bit_sum + bits[i+1] * weight[15];
  *++phase = bit_sum + bits[i+1] * weight[16];
  *++phase = bit_sum + bits[i+1] * weight[17];
  *++phase = bit_sum + bits[i+1] * weight[18];
  *++phase = bit_sum + bits[i+1] * weight[19];
  *++phase = bit_sum + bits[i+1] * weight[20];
  *++phase = bit_sum + bits[i+1] * weight[21];
  *++phase = bit_sum + bits[i+1] * weight[22];
}

void GMSK_binary(int data, int bit_len, int *bits) {
  int i;

  bits = bits + bit_len;
  for (i = 0; i < bit_len; ++i) {
    *--bits = (data & 1) * 2 - 1;
    data >>= 1;
  }
}

void test_form_file(const char *bit_len_str, const char *filename) {
  unsigned long long num;
  FILE *f;
  unsigned long long line_count = 0, i;
  int bit_len, phase_len, num_type_length;
  int *bits;
  double *phase;
  
  if (sscanf(bit_len_str, "%d", &bit_len) != 1) {
    fprintf(stderr, "Error with bit_len!\n");
    exit(1);
  }

  if ((f = fopen(filename, "r")) == NULL) {
    fprintf(stderr, "Error open file!\n");
    exit(1);
  }

  bits = (int *) malloc(bit_len * sizeof(int));
  phase_len = (bit_len + 1) * sample_per_bit;
  phase = (double *) malloc(phase_len * sizeof(double));

  num_type_length = ((bit_len + 3) >> 2) + 2; 

  while (feof(f) == 0) {
    if (!(line_count & 0xff)) {
      fprintf(stderr, "Currently searching at line %lld\n", line_count);
    }
    ++line_count;
    if (fscanf(f, "%llx", &num) == 1) {

      GMSK_binary(num, bit_len, bits);
      
      GMSK_phase(bits, bit_len, phase);

      printf("%#0*llx\t", num_type_length, num);
      GMSK_autocorr_with_print(phase, phase_len);
    }
    /* else {
      fprintf(stderr, "Error at line %d\n", line_count);
    } */

    while (feof(f) == 0 && fgetc(f) != '\n');
  }

  free(bits);
  free(phase);
  fclose(f);
}

int main(int argc, char const *argv[]) {

#if 1 

  const int data = 0x3bf5c961;
  const int bit_len = 32;
  const int phase_len = (bit_len + 1) * sample_per_bit;

  int *bits;
  double *phase;
  int i;

  bits = (int *) malloc(bit_len * sizeof(int));
  phase = (double *) malloc(phase_len * sizeof(double));
 
  GMSK_binary(data, bit_len, bits);

  printf("number: %#08x\n", data);
  printf("Bits:\n[");
  for (i = 0; i < bit_len; ++i) {
    printf("%d, ", bits[i]);
  }
  printf("]\n");

  GMSK_phase(bits, bit_len, phase);

  printf("%d samples written:\n", phase_len);
  printf("[");
  for (i = 0; i < phase_len; ++i) {
    printf("%f, ", phase[i]);
  }
  printf("]\n");

  printf("Autocorrelate:\n");
  GMSK_autocorr_with_print(phase, phase_len);
  printf("\n");

#else
  
  if (argc == 3) {
    test_form_file(argv[1], argv[2]);
  } else {
    fprintf(stderr, "Usage: %s <BIT_LEN> <FILE_PATH>\n", argv[0]);
  }

#endif

  return 0;
}
