/*
 * suftest.c for libdivsufsort
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

#include <cstring>
#include <time.h>
#include <divsufsort.hpp>
#include <lfs.hpp>
#include <iostream>
#include <chrono>
#include <utils.hpp>

static void print_help(const char *progname, int status) {
	std::cerr << "suftest, a suffixsort tester, version " << divsufsort_version() << "\n";
	std::cerr << "usage " << progname << " FILE\n\n";
  exit(status);
}

int main(int argc, const char *argv[]) {
  FILE *fp;
  const char *fname;

  off_t n;
  int32_t needclose = 1;

  /* Check arguments. */
  if((argc == 1) ||
     (strcmp(argv[1], "-h") == 0) ||
     (strcmp(argv[1], "--help") == 0)) { print_help(argv[0], EXIT_SUCCESS); }
  if(argc != 2) { print_help(argv[0], EXIT_FAILURE); }

  /* Open a file for reading. */
  if(strcmp(argv[1], "-") != 0) {
    if((fp = fopen(fname = argv[1], "rb")) == nullptr) {
			std::cout << argv[0] << ": Cannot open file '" << fname << "': ";
      fprintf(stderr, "%s: Cannot open file `%s': ", argv[0], fname);
      perror(NULL);
      exit(EXIT_FAILURE);
    }
  } else {
    fp = stdin;
    fname = "stdin";
    needclose = 0;
  }

  /* Get the file size. */
  if(fseek(fp, 0, SEEK_END) == 0) {
    n = ftell(fp);
    rewind(fp);
    if(n < 0) {
      fprintf(stderr, "%s: Cannot ftell `%s': ", argv[0], fname);
      perror(NULL);
      exit(EXIT_FAILURE);
    }
    if(0x7fffffff <= n) {
      fprintf(stderr, "%s: Input file `%s' is too big.\n", argv[0], fname);
      exit(EXIT_FAILURE);
    }
  } else {
    fprintf(stderr, "%s: Cannot fseek `%s': ", argv[0], fname);
    perror(NULL);
    exit(EXIT_FAILURE);
  }

  /* Allocate 5n bytes of memory. */
	auto * T = new unsigned char[n];

  /* Read n bytes of data. */
  if(fread(T, sizeof(unsigned char), (size_t)n, fp) != (size_t)n) {
    fprintf(stderr, "%s: %s `%s': ",
      argv[0],
      (ferror(fp) || !feof(fp)) ? "Cannot read from" : "Unexpected EOF in",
      argv[1]);
    perror(NULL);
    exit(EXIT_FAILURE);
  }
  if(needclose & 1) { fclose(fp); }

  /* Construct the suffix array. */
	std::cerr << fname << ": " << n << " bytes ... \n";
  auto start = std::chrono::high_resolution_clock::now();
	
	auto result = divsufsort<int32_t>(std::span<const unsigned char>(reinterpret_cast<const unsigned char *>(T), n));
	
  auto finish = std::chrono::high_resolution_clock::now();
	std::cerr << std::chrono::duration_cast<std::chrono::milliseconds>(finish-start).count() << " ms\n";
  
  /* Check the suffix array. */
  if(sufcheck(T, result.data(), (size_t)n, 1) != 0) { exit(EXIT_FAILURE); }

  /* Deallocate memory. */
  delete[] T;

  return 0;
}
