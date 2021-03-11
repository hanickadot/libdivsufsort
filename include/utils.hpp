/*
 * utils.c for libdivsufsort
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

#include "common.hpp"


/*- Private Function -*/

/* Binary search for inverse bwt. */
template <typename CharT, typename ResultT> static ResultT binarysearch_lower(const ResultT *A, ResultT size, ResultT value) {
  ResultT half, i;
  for(i = 0, half = size >> 1;
      0 < size;
      size = half, half >>= 1) {
    if(A[i + half] < value) {
      i += half + 1;
      half -= (size & 1) ^ 1;
    }
  }
  return i;
}


/*- Functions -*/

/* Burrows-Wheeler transform. */
template <typename CharT, typename ResultT> int32_t bw_transform(const CharT *T, CharT *U, ResultT *SA, ResultT n, ResultT *idx) {
  ResultT *A, i, j, p, t;
  int32_t c;

  /* Check arguments. */
  if((T == nullptr) || (U == nullptr) || (n < 0) || (idx == nullptr)) { return -1; }
  if(n <= 1) {
    if(n == 1) { U[0] = T[0]; }
    *idx = n;
    return 0;
  }

  if((A = SA) == nullptr) {
    i = divbwt(T, U, nullptr, n);
    if(0 <= i) { *idx = i; i = 0; }
    return static_cast<int32_t>(i);
  }

  /* BW transform. */
  if(T == U) {
    t = n;
    for(i = 0, j = 0; i < n; ++i) {
      p = t - 1;
      t = A[i];
      if(0 <= p) {
        c = T[j];
        U[j] = (j <= p) ? T[p] : static_cast<CharT>(A[p]);
        A[j] = c;
        j++;
      } else {
        *idx = i;
      }
    }
    p = t - 1;
    if(0 <= p) {
      c = T[j];
      U[j] = (j <= p) ? T[p] : static_cast<CharT>(A[p]);
      A[j] = c;
    } else {
      *idx = i;
    }
  } else {
    U[0] = T[n - 1];
    for(i = 0; A[i] != 0; ++i) { U[i + 1] = T[A[i] - 1]; }
    *idx = i + 1;
    for(++i; i < n; ++i) { U[i] = T[A[i] - 1]; }
  }

  if(SA == nullptr) {
    /* Deallocate memory. */
    free(A);
  }

  return 0;
}

/* Inverse Burrows-Wheeler transform. */
template <typename CharT, typename ResultT> int32_t inverse_bw_transform(const CharT *T, CharT *U, ResultT *A, ResultT n, ResultT idx) {
  ResultT C[alphabet_size<CharT>];
  CharT D[alphabet_size<CharT>];
  ResultT *B;
  ResultT i, p;
  int32_t c, d;

  /* Check arguments. */
  if((T == nullptr) || (U == nullptr) || (n < 0) || (idx < 0) ||
     (n < idx) || ((0 < n) && (idx == 0))) {
    return -1;
  }
  if(n <= 1) { return 0; }

  if((B = A) == nullptr) {
    /* Allocate n*sizeof(ResultT) bytes of memory. */
    B = new ResultT[n];
    if (B == nullptr) {
      return -2;
    }
  }

  /* Inverse BW transform. */
  for(c = 0; c < alphabet_size<CharT>; ++c) { C[c] = 0; }
  for(i = 0; i < n; ++i) { ++C[T[i]]; }
  for(c = 0, d = 0, i = 0; c < alphabet_size<CharT>; ++c) {
    p = C[c];
    if(0 < p) {
      C[c] = i;
      D[d++] = static_cast<CharT>(c);
      i += p;
    }
  }
  for(i = 0; i < idx; ++i) { B[C[T[i]]++] = i; }
  for( ; i < n; ++i)       { B[C[T[i]]++] = i + 1; }
  for(c = 0; c < d; ++c) { C[c] = C[D[c]]; }
  for(i = 0, p = idx; i < n; ++i) {
    U[i] = D[binarysearch_lower(C, d, p)];
    p = B[p - 1];
  }

  if(A == nullptr) {
    /* Deallocate memory. */
    delete[] B;
  }

  return 0;
}

/* Checks the suffix array SA of the string T. */
template <typename CharT, typename ResultT> int32_t sufcheck(const CharT *T, const ResultT *SA, no_deduce<ResultT> n, int32_t verbose) {
  ResultT C[alphabet_size<CharT>];
  ResultT i, p, q, t;
  int32_t c;

  if(verbose) { fprintf(stderr, "sufcheck: "); }

  /* Check arguments. */
  if((T == nullptr) || (SA == nullptr) || (n < 0)) {
    if(verbose) { fprintf(stderr, "Invalid arguments.\n"); }
    return -1;
  }
  if(n == 0) {
    if(verbose) { fprintf(stderr, "Done.\n"); }
    return 0;
  }

  /* check range: [0..n-1] */
  for(i = 0; i < n; ++i) {
    if((SA[i] < 0) || (n <= SA[i])) {
      if(verbose) {
        fprintf(stderr, "Out of the range [0,%zu].\n"
                        "  SA[%zu]=%zu\n",
                        n - 1, i, SA[i]);
      }
      return -2;
    }
  }

  /* check first characters. */
  for(i = 1; i < n; ++i) {
    if(T[SA[i - 1]] > T[SA[i]]) {
      if(verbose) {
        fprintf(stderr, "Suffixes in wrong order.\n"
                        "  T[SA[%zu]=%zu]=%d"
                        " > T[SA[%zu]=%zu]=%d\n",
                        i - 1, SA[i - 1], T[SA[i - 1]], i, SA[i], T[SA[i]]);
      }
      return -3;
    }
  }

  /* check suffixes. */
  for(i = 0; i < alphabet_size<CharT>; ++i) { C[i] = 0; }
  for(i = 0; i < n; ++i) { ++C[T[i]]; }
  for(i = 0, p = 0; i < alphabet_size<CharT>; ++i) {
    t = C[i];
    C[i] = p;
    p += t;
  }

  q = C[T[n - 1]];
  C[T[n - 1]] += 1;
  for(i = 0; i < n; ++i) {
    p = SA[i];
    if(0 < p) {
      c = T[--p];
      t = C[c];
    } else {
      c = T[p = n - 1];
      t = q;
    }
    if((t < 0) || (p != SA[t])) {
      if(verbose) {
        fprintf(stderr, "Suffix in wrong position.\n"
                        "  SA[%zu]=%zu or\n"
                        "  SA[%zu]=%zu\n",
                        t, (0 <= t) ? SA[t] : -1, i, SA[i]);
      }
      return -4;
    }
    if(t != q) {
      ++C[c];
      if((n <= C[c]) || (T[SA[C[c]]] != c)) { C[c] = -1; }
    }
  }

  if(1 <= verbose) { fprintf(stderr, "Done.\n"); }
  return 0;
}


template <typename CharT, typename ResultT> static int32_t _compare(const CharT *T, ResultT Tsize, const CharT *P, ResultT Psize, ResultT suf, ResultT *match) {
  ResultT i, j;
  int32_t r;
  for(i = suf + *match, j = *match, r = 0; (i < Tsize) && (j < Psize) && ((r = T[i] - P[j]) == 0); ++i, ++j) { }
  *match = j;
  return (r == 0) ? -(j != Psize) : r;
}

/* Search for the pattern P in the string T. */
template <typename CharT, typename ResultT> ResultT sa_search(const CharT *T, ResultT Tsize, const CharT *P, ResultT Psize, const ResultT *SA, ResultT SAsize, ResultT *idx) {
  ResultT size, lsize, rsize, half;
  ResultT match, lmatch, rmatch;
  ResultT llmatch, lrmatch, rlmatch, rrmatch;
  ResultT i, j, k;
  int32_t r;

  if(idx != nullptr) { *idx = -1; }
  if((T == nullptr) || (P == nullptr) || (SA == nullptr) || (Tsize < 0) || (Psize < 0) || (SAsize < 0)) { return -1; }
  if((Tsize == 0) || (SAsize == 0)) { return 0; }
  if(Psize == 0) { if(idx != nullptr) { *idx = 0; } return SAsize; }

  for(i = j = k = 0, lmatch = rmatch = 0, size = SAsize, half = size >> 1;
      0 < size;
      size = half, half >>= 1) {
    match = std::min(lmatch, rmatch);
    r = _compare(T, Tsize, P, Psize, SA[i + half], &match);
    if(r < 0) {
      i += half + 1;
      half -= (size & 1) ^ 1;
      lmatch = match;
    } else if(r > 0) {
      rmatch = match;
    } else {
      lsize = half, j = i, rsize = size - half - 1, k = i + half + 1;

      /* left part */
      for(llmatch = lmatch, lrmatch = match, half = lsize >> 1;
          0 < lsize;
          lsize = half, half >>= 1) {
        lmatch = std::min(llmatch, lrmatch);
        r = _compare(T, Tsize, P, Psize, SA[j + half], &lmatch);
        if(r < 0) {
          j += half + 1;
          half -= (lsize & 1) ^ 1;
          llmatch = lmatch;
        } else {
          lrmatch = lmatch;
        }
      }

      /* right part */
      for(rlmatch = match, rrmatch = rmatch, half = rsize >> 1;
          0 < rsize;
          rsize = half, half >>= 1) {
        rmatch = std::min(rlmatch, rrmatch);
        r = _compare(T, Tsize, P, Psize, SA[k + half], &rmatch);
        if(r <= 0) {
          k += half + 1;
          half -= (rsize & 1) ^ 1;
          rlmatch = rmatch;
        } else {
          rrmatch = rmatch;
        }
      }

      break;
    }
  }

  if(idx != nullptr) { *idx = (0 < (k - j)) ? j : i; }
  return k - j;
}

/* Search for the character c in the string T. */
template <typename CharT, typename ResultT> ResultT sa_simplesearch(const CharT *T, ResultT Tsize, const ResultT *SA, ResultT SAsize, int32_t c, ResultT *idx) {
  ResultT size, lsize, rsize, half;
  ResultT i, j, k, p;
  int32_t r;

  if(idx != nullptr) { *idx = -1; }
  if((T == nullptr) || (SA == nullptr) || (Tsize < 0) || (SAsize < 0)) { return -1; }
  if((Tsize == 0) || (SAsize == 0)) { return 0; }

  for(i = j = k = 0, size = SAsize, half = size >> 1;
      0 < size;
      size = half, half >>= 1) {
    p = SA[i + half];
    r = (p < Tsize) ? T[p] - c : -1;
    if(r < 0) {
      i += half + 1;
      half -= (size & 1) ^ 1;
    } else if(r == 0) {
      lsize = half, j = i, rsize = size - half - 1, k = i + half + 1;

      /* left part */
      for(half = lsize >> 1;
          0 < lsize;
          lsize = half, half >>= 1) {
        p = SA[j + half];
        r = (p < Tsize) ? T[p] - c : -1;
        if(r < 0) {
          j += half + 1;
          half -= (lsize & 1) ^ 1;
        }
      }

      /* right part */
      for(half = rsize >> 1;
          0 < rsize;
          rsize = half, half >>= 1) {
        p = SA[k + half];
        r = (p < Tsize) ? T[p] - c : -1;
        if(r <= 0) {
          k += half + 1;
          half -= (rsize & 1) ^ 1;
        }
      }

      break;
    }
  }

  if(idx != nullptr) { *idx = (0 < (k - j)) ? j : i; }
  return k - j;
}
