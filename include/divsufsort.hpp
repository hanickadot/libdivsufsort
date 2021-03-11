/*
 * divsufsort.c for libdivsufsort
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

#include "sssort.hpp"
#include "trsort.hpp"
#include <span>
#include <vector>
#include <cstddef>
#ifdef _OPENMP
# include <omp.h>
#endif


/*- Private Functions -*/

#define BUCKET_B(_c0, _c1) (bucket_B[(_c1) * alphabet_size<CharT> + (_c0)])
#define BUCKET_BSTAR(_c0, _c1) (bucket_B[(_c0) * alphabet_size<CharT> + (_c1)])

/* Sorts suffixes of type B*. */
template <typename CharT = unsigned char, typename ResultT = uint32_t> static ResultT sort_typeBstar(const CharT *T, ResultT *SA, ResultT *bucket_A, ResultT *bucket_B, ResultT n) {
  ResultT *PAb, *ISAb, *buf;
#ifdef _OPENMP
  ResultT *curbuf;
  ResultT l;
#endif
  ResultT i, j, k, t, m, bufsize;
  int32_t c0, c1;
#ifdef _OPENMP
  int32_t d0, d1;
  int tmp;
#endif
  // expects both buckets to be zero-initialized

  /* Count the number of occurrences of the first one or two characters of each
     type A, B and B* suffix. Moreover, store the beginning position of all
     type B* suffixes into the array SA. */
  for(i = n - 1, m = n, c0 = T[n - 1]; 0 <= i;) {
    /* type A suffix. */
    do { ++bucket_A[c1 = c0]; } while((0 <= --i) && ((c0 = T[i]) >= c1));
    if(0 <= i) {
      /* type B* suffix. */
      ++BUCKET_BSTAR(c0, c1);
      SA[--m] = i;
      /* type B suffix. */
      for(--i, c1 = c0; (0 <= i) && ((c0 = T[i]) <= c1); --i, c1 = c0) {
        ++BUCKET_B(c0, c1);
      }
    }
  }
  m = n - m;
	/*
	note:
	  A type B* suffix is lexicographically smaller than a type B suffix that
	  begins with the same first two characters.
	*/

  /* Calculate the index of start/end point of each bucket. */
  for(c0 = 0, i = 0, j = 0; c0 < static_cast<int32_t>(alphabet_size<CharT>); ++c0) {
    t = i + bucket_A[c0];
    bucket_A[c0] = i + j; /* start point */
    i = t + BUCKET_B(c0, c0);
    for(c1 = c0 + 1; c1 < static_cast<int32_t>(alphabet_size<CharT>); ++c1) {
      j += BUCKET_BSTAR(c0, c1);
      BUCKET_BSTAR(c0, c1) = j; /* end point */
      i += BUCKET_B(c0, c1);
    }
  }

  if(0 < m) {
    /* Sort the type B* suffixes by their first two characters. */
    PAb = SA + n - m; ISAb = SA + m;
    for(i = m - 2; 0 <= i; --i) {
      t = PAb[i], c0 = T[t], c1 = T[t + 1];
      SA[--BUCKET_BSTAR(c0, c1)] = i;
    }
    t = PAb[m - 1], c0 = T[t], c1 = T[t + 1];
    SA[--BUCKET_BSTAR(c0, c1)] = m - 1;

    /* Sort the type B* substrings using sssort. */
#ifdef _OPENMP
    tmp = omp_get_max_threads();
    buf = SA + m, bufsize = (n - (2 * m)) / tmp;
    c0 = alphabet_size<CharT> - 2, c1 = alphabet_size<CharT> - 1, j = m;
#pragma omp parallel default(shared) private(curbuf, k, l, d0, d1, tmp)
    {
      tmp = omp_get_thread_num();
      curbuf = buf + tmp * bufsize;
      k = 0;
      for(;;) {
        #pragma omp critical(sssort_lock)
        {
          if(0 < (l = j)) {
            d0 = c0, d1 = c1;
            do {
              k = BUCKET_BSTAR(d0, d1);
              if(--d1 <= d0) {
                d1 = alphabet_size<CharT> - 1;
                if(--d0 < 0) { break; }
              }
            } while(((l - k) <= 1) && (0 < (l = k)));
            c0 = d0, c1 = d1, j = k;
          }
        }
        if(l == 0) { break; }
        sssort(T, PAb, SA + k, SA + l,
               curbuf, bufsize, 2, n, *(SA + k) == (m - 1));
      }
    }
#else
    buf = SA + m, bufsize = n - (2 * m);
    for(c0 = alphabet_size<CharT> - 2, j = m; 0 < j; --c0) {
      for(c1 = alphabet_size<CharT> - 1; c0 < c1; j = i, --c1) {
        i = BUCKET_BSTAR(c0, c1);
        if(1 < (j - i)) {
          sssort<CharT, ResultT>(T, PAb, SA + i, SA + j, buf, bufsize, 2, n, *(SA + i) == (m - 1));
        }
      }
    }
#endif

    /* Compute ranks of type B* substrings. */
    for(i = m - 1; 0 <= i; --i) {
      if(0 <= SA[i]) {
        j = i;
        do { ISAb[SA[i]] = i; } while((0 <= --i) && (0 <= SA[i]));
        SA[i + 1] = i - j;
        if(i <= 0) { break; }
      }
      j = i;
      do { ISAb[SA[i] = ~SA[i]] = j; } while(SA[--i] < 0);
      ISAb[SA[i]] = j;
    }

    /* Construct the inverse suffix array of type B* suffixes using trsort. */
    trsort<ResultT>(ISAb, SA, m, 1);

    /* Set the sorted order of tyoe B* suffixes. */
    for(i = n - 1, j = m, c0 = T[n - 1]; 0 <= i;) {
      for(--i, c1 = c0; (0 <= i) && ((c0 = T[i]) >= c1); --i, c1 = c0) { }
      if(0 <= i) {
        t = i;
        for(--i, c1 = c0; (0 <= i) && ((c0 = T[i]) <= c1); --i, c1 = c0) { }
        SA[ISAb[--j]] = ((t == 0) || (1 < (t - i))) ? t : ~t;
      }
    }

    /* Calculate the index of start/end point of each bucket. */
    BUCKET_B(alphabet_size<CharT> - 1, alphabet_size<CharT> - 1) = n; /* end point */
    for(c0 = alphabet_size<CharT> - 2, k = m - 1; 0 <= c0; --c0) {
      i = bucket_A[c0 + 1] - 1;
      for(c1 = alphabet_size<CharT> - 1; c0 < c1; --c1) {
        t = i - BUCKET_B(c0, c1);
        BUCKET_B(c0, c1) = i; /* end point */

        /* Move all type B* suffixes to the correct position. */
        for(i = t, j = BUCKET_BSTAR(c0, c1);
            j <= k;
            --i, --k) { SA[i] = SA[k]; }
      }
      BUCKET_BSTAR(c0, c0 + 1) = i - BUCKET_B(c0, c0) + 1; /* start point */
      BUCKET_B(c0, c0) = i; /* end point */
    }
  }

  return m;
}

/* Constructs the suffix array by using the sorted order of type B* suffixes. */
template <typename CharT = unsigned char, typename ResultT = uint32_t> static void construct_SA(const CharT *T, ResultT *SA, ResultT *bucket_A, ResultT *bucket_B, ResultT n, ResultT m) {
  ResultT *i, *j, *k;
  ResultT s;
  int32_t c0, c1, c2;

  if(0 < m) {
    /* Construct the sorted order of type B suffixes by using
       the sorted order of type B* suffixes. */
    for(c1 = alphabet_size<CharT> - 2; 0 <= c1; --c1) {
      /* Scan the suffix array from right to left. */
      // hana: k = j is difference against upstream, so it won't dereference nullptr
      for(i = SA + BUCKET_BSTAR(c1, c1 + 1),
          j = SA + bucket_A[c1 + 1] - 1, k = j, c2 = -1;
          i <= j;
          --j) {
        if(0 < (s = *j)) {
          assert(T[s] == c1);
          assert(((s + 1) < n) && (T[s] <= T[s + 1]));
          assert(T[s - 1] <= T[s]);
          *j = ~s;
          c0 = T[--s];
          if((0 < s) && (T[s - 1] > c0)) { s = ~s; }
          if(c0 != c2) {
            if(0 <= c2) { BUCKET_B(c2, c1) = k - SA; }
            k = SA + BUCKET_B(c2 = c0, c1);
          }
          assert(k < j);
          *k-- = s;
        } else {
          assert(((s == 0) && (T[s] == c1)) || (s < 0));
          *j = ~s;
        }
      }
    }
  }

  /* Construct the suffix array by using
     the sorted order of type B suffixes. */
  k = SA + bucket_A[c2 = T[n - 1]];
  *k++ = (T[n - 2] < c2) ? ~(n - 1) : (n - 1);
  /* Scan the suffix array from left to right. */
  for(i = SA, j = SA + n; i < j; ++i) {
    if(0 < (s = *i)) {
      assert(T[s - 1] >= T[s]);
      c0 = T[--s];
      if((s == 0) || (T[s - 1] < c0)) { s = ~s; }
      if(c0 != c2) {
        bucket_A[c2] = k - SA;
        k = SA + bucket_A[c2 = c0];
      }
      assert(i < k);
      *k++ = s;
    } else {
      assert(s < 0);
      *i = ~s;
    }
  }
}

/* Constructs the burrows-wheeler transformed string directly
   by using the sorted order of type B* suffixes. */
template <typename CharT = unsigned char, typename ResultT = uint32_t> static ResultT construct_BWT(const CharT *T, ResultT *SA, ResultT *bucket_A, ResultT *bucket_B, ResultT n, ResultT m) {
  ResultT *i, *j, *k, *orig;
  ResultT s;
  int32_t c0, c1, c2;

  if(0 < m) {
    /* Construct the sorted order of type B suffixes by using
       the sorted order of type B* suffixes. */
    for(c1 = alphabet_size<CharT> - 2; 0 <= c1; --c1) {
      /* Scan the suffix array from right to left. */
      for(i = SA + BUCKET_BSTAR(c1, c1 + 1),
          j = SA + bucket_A[c1 + 1] - 1, k = nullptr, c2 = -1;
          i <= j;
          --j) {
        if(0 < (s = *j)) {
          assert(T[s] == c1);
          assert(((s + 1) < n) && (T[s] <= T[s + 1]));
          assert(T[s - 1] <= T[s]);
          c0 = T[--s];
          *j = ~(static_cast<ResultT>(c0));
          if((0 < s) && (T[s - 1] > c0)) { s = ~s; }
          if(c0 != c2) {
            if(0 <= c2) { BUCKET_B(c2, c1) = k - SA; }
            k = SA + BUCKET_B(c2 = c0, c1);
          }
          assert(k < j);
          *k-- = s;
        } else if(s != 0) {
          *j = ~s;
        } else {
          assert(T[s] == c1);
        }
      }
    }
  }

  /* Construct the BWTed string by using
     the sorted order of type B suffixes. */
  k = SA + bucket_A[c2 = T[n - 1]];
  *k++ = (T[n - 2] < c2) ? ~(static_cast<ResultT>(T[n - 2])) : (n - 1);
  /* Scan the suffix array from left to right. */
  for(i = SA, j = SA + n, orig = SA; i < j; ++i) {
    if(0 < (s = *i)) {
      assert(T[s - 1] >= T[s]);
      c0 = T[--s];
      *i = c0;
      if((0 < s) && (T[s - 1] < c0)) { s = ~(static_cast<ResultT>(T[s - 1])); }
      if(c0 != c2) {
        bucket_A[c2] = k - SA;
        k = SA + bucket_A[c2 = c0];
      }
      assert(i < k);
      *k++ = s;
    } else if(s != 0) {
      *i = ~s;
    } else {
      orig = i;
    }
  }

  return orig - SA;
}


/*---------------------------------------------------------------------------*/

/*- Function -*/

template <typename CharT = unsigned char, typename ResultT = uint32_t> void divsufsort(const CharT *T, ResultT *SA, no_deduce<ResultT> n) {
  /* Check arguments. */
	assert(T != nullptr);
	assert(SA != nullptr);
	assert(n >= 0);

  if(n == 0) { return; }
  else if(n == 1) { SA[0] = 0; return; }
  else if(n == 2) { bool ordered = (T[0] < T[1]); SA[ordered ^ 1] = 0, SA[ordered] = 1; return; }

  std::array<ResultT, bucket_A_size<CharT>> bucket_A{};
  std::array<ResultT, bucket_B_size<CharT>> bucket_B{};

  ResultT m = sort_typeBstar(T, SA, bucket_A.data(), bucket_B.data(), n);
  construct_SA(T, SA, bucket_A.data(), bucket_B.data(), n, m);
}

template <typename ResultT = uint32_t, typename CharT = unsigned char> auto divsufsort(std::span<const CharT> T) -> std::vector<ResultT> {
	auto result = std::vector<ResultT>(T.size());
	
	divsufsort(T.data(), result.data(), T.size());
	
	return result;
}

template <typename CharT = unsigned char, typename ResultT = uint32_t> ResultT divbwt(const CharT *T, CharT *U, ResultT *A, no_deduce<ResultT> n) {
  ResultT *B;

  /* Check arguments. */
  if((T == nullptr) || (U == nullptr) || (n < 0)) { return -1; }
  else if(n <= 1) { if(n == 1) { U[0] = T[0]; } return n; }

  if((B = A) == nullptr) { 
    B = new ResultT[n + 1]{};
  }
	
	std::array<ResultT, bucket_A_size<CharT>> bucket_A{};
	std::array<ResultT, bucket_B_size<CharT>> bucket_B{};

  /* Burrows-Wheeler Transform. */
  ResultT m = sort_typeBstar(T, B, bucket_A.data(), bucket_B.data(), n);
  ResultT pidx = construct_BWT(T, B, bucket_A.data(), bucket_B.data(), n, m);

  /* Copy to output string. */
  U[0] = T[n - 1];
  ResultT i = 0;
  for(; i < pidx; ++i) { U[i + 1] = static_cast<CharT>(B[i]); }
  for(i += 1; i < n; ++i) { U[i] = static_cast<CharT>(B[i]); }
  pidx += 1;

  if(A == nullptr) { delete[] B; }

  return pidx;
}

const char * divsufsort_version(void) {
  return "1.0";
}
