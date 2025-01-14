/*
 * unbwt.c for libdivsufsort
 * Copyright (c) 2003-2008 Yuta Mori All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#if HAVE_CONFIG_H
# include "config.hpp"
#endif
#include <stdio.h>
#if HAVE_STRING_H
# include <string.h>
#endif
#if HAVE_STDLIB_H
# include <stdlib.h>
#endif
#if HAVE_MEMORY_H
# include <memory.h>
#endif
#if HAVE_STDDEF_H
# include <stddef.h>
#endif
#if HAVE_STRINGS_H
# include <strings.h>
#endif
#if HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#if HAVE_IO_H && HAVE_FCNTL_H
# include <io.h>
# include <fcntl.h>
#endif
#include <time.h>
#include <divsufsort.hpp>


static
size_t
read_int(FILE *fp, saidx_t *n) {
  unsigned char c[4];
  size_t m = fread(c, sizeof(unsigned char), 4, fp);
  if(m == 4) {
    *n = (c[0] <<  0) | (c[1] <<  8) |
         (c[2] << 16) | (c[3] << 24);
  }
  return m;
}

static
void
print_help(const char *progname, int status) {
  fprintf(stderr,
          "unbwt, an inverse burrows-wheeler transform program, version %s.\n",
          divsufsort_version());
  fprintf(stderr, "usage: %s INFILE OUTFILE\n\n", progname);
  exit(status);
}

int
main(int argc, const char *argv[]) {
  FILE *fp, *ofp;
  const char *fname, *ofname;
  sauchar_t *T;
  saidx_t *A;
  LFS_OFF_T n;
  size_t m;
  saidx_t pidx;
  clock_t start, finish;
  saint_t err, blocksize, needclose = 3;

  /* Check arguments. */
  if((argc == 1) ||
     (strcmp(argv[1], "-h") == 0) ||
     (strcmp(argv[1], "--help") == 0)) { print_help(argv[0], EXIT_SUCCESS); }
  if(argc != 3) { print_help(argv[0], EXIT_FAILURE); }

  /* Open a file for reading. */
  if(strcmp(argv[1], "-") != 0) {
#if HAVE_FOPEN_S
    if(fopen_s(&fp, fname = argv[1], "rb") != 0) {
#else
    if((fp = LFS_FOPEN(fname = argv[1], "rb")) == NULL) {
#endif
      fprintf(stderr, "%s: Cannot open file `%s': ", argv[0], fname);
      perror(NULL);
      exit(EXIT_FAILURE);
    }
  } else {
#if HAVE__SETMODE && HAVE__FILENO
    if(_setmode(_fileno(stdin), _O_BINARY) == -1) {
      fprintf(stderr, "%s: Cannot set mode: ", argv[0]);
      perror(NULL);
      exit(EXIT_FAILURE);
    }
#endif
    fp = stdin;
    fname = "stdin";
    needclose ^= 1;
  }

  /* Open a file for writing. */
  if(strcmp(argv[2], "-") != 0) {
#if HAVE_FOPEN_S
    if(fopen_s(&ofp, ofname = argv[2], "wb") != 0) {
#else
    if((ofp = LFS_FOPEN(ofname = argv[2], "wb")) == NULL) {
#endif
      fprintf(stderr, "%s: Cannot open file `%s': ", argv[0], ofname);
      perror(NULL);
      exit(EXIT_FAILURE);
    }
  } else {
#if HAVE__SETMODE && HAVE__FILENO
    if(_setmode(_fileno(stdout), _O_BINARY) == -1) {
      fprintf(stderr, "%s: Cannot set mode: ", argv[0]);
      perror(NULL);
      exit(EXIT_FAILURE);
    }
#endif
    ofp = stdout;
    ofname = "stdout";
    needclose ^= 2;
  }

  /* Read the blocksize. */
  if(read_int(fp, &blocksize) != 4) {
    fprintf(stderr, "%s: Cannot read from `%s': ", argv[0], fname);
    perror(NULL);
    exit(EXIT_FAILURE);
  }

  /* Allocate 5blocksize bytes of memory. */
  T = (sauchar_t *)malloc(blocksize * sizeof(sauchar_t));
  A = (saidx_t *)malloc(blocksize * sizeof(saidx_t));
  if((T == NULL) || (A == NULL)) {
    fprintf(stderr, "%s: Cannot allocate memory.\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  fprintf(stderr, "UnBWT (blocksize %" PRIdSAINT_T ") ... ", blocksize);
  start = clock();
  for(n = 0; (m = read_int(fp, &pidx)) != 0; n += m) {
    /* Read blocksize bytes of data. */
    if((m != 4) || ((m = fread(T, sizeof(sauchar_t), blocksize, fp)) == 0)) {
      fprintf(stderr, "%s: %s `%s': ",
        argv[0],
        (ferror(fp) || !feof(fp)) ? "Cannot read from" : "Unexpected EOF in",
        fname);
      perror(NULL);
      exit(EXIT_FAILURE);
    }

    /* Inverse Burrows-Wheeler Transform. */
    if((err = inverse_bw_transform(T, T, A, m, pidx)) != 0) {
      fprintf(stderr, "%s (reverseBWT): %s.\n",
        argv[0],
        (err == -1) ? "Invalid data" : "Cannot allocate memory");
      exit(EXIT_FAILURE);
    }

    /* Write m bytes of data. */
    if(fwrite(T, sizeof(sauchar_t), m, ofp) != m) {
      fprintf(stderr, "%s: Cannot write to `%s': ", argv[0], ofname);
      perror(NULL);
      exit(EXIT_FAILURE);
    }
  }
  if(ferror(fp)) {
    fprintf(stderr, "%s: Cannot read from `%s': ", argv[0], fname);
    perror(NULL);
    exit(EXIT_FAILURE);
  }
  finish = clock();
  fprintf(stderr, "%" PRIdOFF_T " bytes: %.4f sec\n",
    n, (double)(finish - start) / (double)CLOCKS_PER_SEC);

  /* Close files */
  if(needclose & 1) { fclose(fp); }
  if(needclose & 2) { fclose(ofp); }

  /* Deallocate memory. */
  free(A);
  free(T);

  return 0;
}
