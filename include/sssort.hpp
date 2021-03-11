/*
 * sssort.c for libdivsufsort
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
#include "static_stack.hpp"

/*- Private Functions -*/

#if (SS_BLOCKSIZE == 0) || (SS_INSERTIONSORT_THRESHOLD < SS_BLOCKSIZE)

static inline int32_t ss_ilg(int n) {
#if SS_BLOCKSIZE == 0
# if defined(BUILD_DIVSUFSORT64)
  return (n >> 32) ?
          ((n >> 48) ?
            ((n >> 56) ?
              56 + lg_table[(n >> 56) & 0xff] :
              48 + lg_table[(n >> 48) & 0xff]) :
            ((n >> 40) ?
              40 + lg_table[(n >> 40) & 0xff] :
              32 + lg_table[(n >> 32) & 0xff])) :
          ((n & 0xffff0000) ?
            ((n & 0xff000000) ?
              24 + lg_table[(n >> 24) & 0xff] :
              16 + lg_table[(n >> 16) & 0xff]) :
            ((n & 0x0000ff00) ?
               8 + lg_table[(n >>  8) & 0xff] :
               0 + lg_table[(n >>  0) & 0xff]));
# else
  return (n & 0xffff0000) ?
          ((n & 0xff000000) ?
            24 + lg_table[(n >> 24) & 0xff] :
            16 + lg_table[(n >> 16) & 0xff]) :
          ((n & 0x0000ff00) ?
             8 + lg_table[(n >>  8) & 0xff] :
             0 + lg_table[(n >>  0) & 0xff]);
# endif
#elif SS_BLOCKSIZE < 256
  return lg_table[n];
#else
  return (n & 0xff00) ?
          8 + lg_table[(n >> 8) & 0xff] :
          0 + lg_table[(n >> 0) & 0xff];
#endif
}

#endif /* (SS_BLOCKSIZE == 0) || (SS_INSERTIONSORT_THRESHOLD < SS_BLOCKSIZE) */

#if SS_BLOCKSIZE != 0

static inline int ss_isqrt(int x) {
  int y, e;

  if(x >= (SS_BLOCKSIZE * SS_BLOCKSIZE)) { return SS_BLOCKSIZE; }
  e = (x & 0xffff0000) ?
        ((x & 0xff000000) ?
          24 + lg_table[(x >> 24) & 0xff] :
          16 + lg_table[(x >> 16) & 0xff]) :
        ((x & 0x0000ff00) ?
           8 + lg_table[(x >>  8) & 0xff] :
           0 + lg_table[(x >>  0) & 0xff]);

  if(e >= 16) {
    y = sqq_table[x >> ((e - 6) - (e & 1))] << ((e >> 1) - 7);
    if(e >= 24) { y = (y + 1 + x / y) >> 1; }
    y = (y + 1 + x / y) >> 1;
  } else if(e >= 8) {
    y = (sqq_table[x >> ((e - 6) - (e & 1))] >> (7 - (e >> 1))) + 1;
  } else {
    return sqq_table[x] >> 4;
  }

  return (x < (y * y)) ? y - 1 : y;
}

#endif /* SS_BLOCKSIZE != 0 */


/*---------------------------------------------------------------------------*/

/* Compares two suffixes. */
template <typename CharT = unsigned char, typename ResultT = int> static inline int32_t ss_compare(const CharT *T, const ResultT *p1, const ResultT *p2, int depth) {
  const CharT *U1, *U2, *U1n, *U2n;

  for(U1 = T + depth + *p1,
      U2 = T + depth + *p2,
      U1n = T + *(p1 + 1) + 2,
      U2n = T + *(p2 + 1) + 2;
      (U1 < U1n) && (U2 < U2n) && (*U1 == *U2);
      ++U1, ++U2) {
  }

  return U1 < U1n ?
        (U2 < U2n ? *U1 - *U2 : 1) :
        (U2 < U2n ? -1 : 0);
}


/*---------------------------------------------------------------------------*/

#if (SS_BLOCKSIZE != 1) && (SS_INSERTIONSORT_THRESHOLD != 1)

/* Insertionsort for small size groups */
template <typename CharT = unsigned char, typename ResultT = int> static void ss_insertionsort(const CharT *T, const ResultT *PA, ResultT *first, ResultT *last, int depth) {
  ResultT *i, *j;
  ResultT t;
  int32_t r;

  for(i = last - 2; first <= i; --i) {
    for(t = *i, j = i + 1; 0 < (r = ss_compare(T, PA + t, PA + *j, depth));) {
      do { *(j - 1) = *j; } while((++j < last) && (*j < 0));
      if(last <= j) { break; }
    }
    if(r == 0) { *j = ~*j; }
    *(j - 1) = t;
  }
}

#endif /* (SS_BLOCKSIZE != 1) && (SS_INSERTIONSORT_THRESHOLD != 1) */


/*---------------------------------------------------------------------------*/

#if (SS_BLOCKSIZE == 0) || (SS_INSERTIONSORT_THRESHOLD < SS_BLOCKSIZE)

template <typename CharT = unsigned char, typename ResultT = int> static inline void ss_fixdown(const CharT *Td, const ResultT *PA, ResultT *SA, int i, int size) {
  ResultT j, k;
  ResultT v;
  int32_t c, d, e;

  for(v = SA[i], c = Td[PA[v]]; (j = 2 * i + 1) < size; SA[i] = SA[k], i = k) {
    d = Td[PA[SA[k = j++]]];
    if(d < (e = Td[PA[SA[j]]])) { k = j; d = e; }
    if(d <= c) { break; }
  }
  SA[i] = v;
}

/* Simple top-down heapsort. */
template <typename CharT = unsigned char, typename ResultT = int> static void ss_heapsort(const CharT *Td, const ResultT *PA, ResultT *SA, int size) {
  ResultT i, m;
  ResultT t;

  m = size;
  if((size % 2) == 0) {
    m--;
    if(Td[PA[SA[m / 2]]] < Td[PA[SA[m]]]) { std::swap(SA[m], SA[m / 2]); }
  }

  for(i = m / 2 - 1; 0 <= i; --i) { ss_fixdown(Td, PA, SA, i, m); }
  if((size % 2) == 0) { std::swap(SA[0], SA[m]); ss_fixdown(Td, PA, SA, 0, m); }
  for(i = m - 1; 0 < i; --i) {
    t = SA[0], SA[0] = SA[i];
    ss_fixdown(Td, PA, SA, 0, i);
    SA[i] = t;
  }
}


/*---------------------------------------------------------------------------*/

/* Returns the median of three elements. */
// TODO use reference
template <typename CharT = unsigned char, typename ResultT = int> static inline int * ss_median3(const CharT *Td, const ResultT *PA, ResultT *v1, ResultT *v2, ResultT *v3) {
  ResultT *t;
  if(Td[PA[*v1]] > Td[PA[*v2]]) { std::swap(v1, v2); }
  if(Td[PA[*v2]] > Td[PA[*v3]]) {
    if(Td[PA[*v1]] > Td[PA[*v3]]) { return v1; }
    else { return v3; }
  }
  return v2;
}

/* Returns the median of five elements. */
template <typename CharT = unsigned char, typename ResultT = int> static inline int * ss_median5(const CharT *Td, const ResultT *PA, ResultT *v1, ResultT *v2, ResultT *v3, ResultT *v4, ResultT *v5) {
  ResultT *t;
  if(Td[PA[*v2]] > Td[PA[*v3]]) { std::swap(v2, v3); }
  if(Td[PA[*v4]] > Td[PA[*v5]]) { std::swap(v4, v5); }
  if(Td[PA[*v2]] > Td[PA[*v4]]) { std::swap(v2, v4); std::swap(v3, v5); }
  if(Td[PA[*v1]] > Td[PA[*v3]]) { std::swap(v1, v3); }
  if(Td[PA[*v1]] > Td[PA[*v4]]) { std::swap(v1, v4); std::swap(v3, v5); }
  if(Td[PA[*v3]] > Td[PA[*v4]]) { return v4; }
  return v3;
}

/* Returns the pivot element. */
template <typename CharT = unsigned char, typename ResultT = int> static inline int * ss_pivot(const CharT *Td, const ResultT *PA, ResultT *first, ResultT *last) {
  ResultT *middle;
  ResultT t;

  t = last - first;
  middle = first + t / 2;

  if(t <= 512) {
    if(t <= 32) {
      return ss_median3(Td, PA, first, middle, last - 1);
    } else {
      t >>= 2;
      return ss_median5(Td, PA, first, first + t, middle, last - 1 - t, last - 1);
    }
  }
  t >>= 3;
  first  = ss_median3(Td, PA, first, first + t, first + (t << 1));
  middle = ss_median3(Td, PA, middle - t, middle, middle + t);
  last   = ss_median3(Td, PA, last - 1 - (t << 1), last - 1 - t, last - 1);
  return ss_median3(Td, PA, first, middle, last);
}


/*---------------------------------------------------------------------------*/

/* Binary partition for substrings. */
template <typename ResultT = int> static inline int * ss_partition(const ResultT *PA, int *first, int *last, int depth) {
  ResultT *a, *b;
  ResultT t;
  for(a = first - 1, b = last;;) {
    for(; (++a < b) && ((PA[*a] + depth) >= (PA[*a + 1] + 1));) { *a = ~*a; }
    for(; (a < --b) && ((PA[*b] + depth) <  (PA[*b + 1] + 1));) { }
    if(b <= a) { break; }
    t = ~*b;
    *b = *a;
    *a = t;
  }
  if(first < a) { *first = ~*first; }
  return a;
}

/* Multikey introsort for medium size groups. */
template <typename CharT = unsigned char, typename ResultT = int> static void ss_mintrosort(const CharT *T, const ResultT *PA, ResultT *first, ResultT *last, int depth) {
  struct stack_type {
    ResultT *a;
    ResultT *b;
    ResultT c;
    int32_t d;
    constexpr operator auto() noexcept {
      return std::tie(a,b,c,d);
    }
  };
  
  static_stack<stack_type, SS_MISORT_STACKSIZE> stack;
  const CharT *Td;
  ResultT *a, *b, *c, *d, *e, *f;
  ResultT s, t;
  int32_t limit;
  int32_t v, x = 0;

  for(limit = ss_ilg(last - first);;) {

    if((last - first) <= SS_INSERTIONSORT_THRESHOLD) {
      if constexpr (1 < SS_INSERTIONSORT_THRESHOLD) {
        if(1 < (last - first)) {
          ss_insertionsort(T, PA, first, last, depth);
        }
      }
      if (stack.size() == 0) return;
      stack.pop_into(first, last, depth, limit);
      continue;
    }

    Td = T + depth;
    if(limit-- == 0) { ss_heapsort(Td, PA, first, last - first); }
    if(limit < 0) {
      for(a = first + 1, v = Td[PA[*first]]; a < last; ++a) {
        if((x = Td[PA[*a]]) != v) {
          if(1 < (a - first)) { break; }
          v = x;
          first = a;
        }
      }
      if(Td[PA[*first] - 1] < v) {
        first = ss_partition(PA, first, a, depth);
      }
      if((a - first) <= (last - a)) {
        if(1 < (a - first)) {
          stack.push(a, last, depth, -1);
          last = a, depth += 1, limit = ss_ilg(a - first);
        } else {
          first = a, limit = -1;
        }
      } else {
        if(1 < (last - a)) {
          stack.push(first, a, depth + 1, ss_ilg(a - first));
          first = a, limit = -1;
        } else {
          last = a, depth += 1, limit = ss_ilg(a - first);
        }
      }
      continue;
    }

    /* choose pivot */
    a = ss_pivot(Td, PA, first, last);
    v = Td[PA[*a]];
    std::swap(*first, *a);

    /* partition */
    for(b = first; (++b < last) && ((x = Td[PA[*b]]) == v);) { }
    if(((a = b) < last) && (x < v)) {
      for(; (++b < last) && ((x = Td[PA[*b]]) <= v);) {
        if(x == v) { std::swap(*b, *a); ++a; }
      }
    }
    for(c = last; (b < --c) && ((x = Td[PA[*c]]) == v);) { }
    if((b < (d = c)) && (x > v)) {
      for(; (b < --c) && ((x = Td[PA[*c]]) >= v);) {
        if(x == v) { std::swap(*c, *d); --d; }
      }
    }
    for(; b < c;) {
      std::swap(*b, *c);
      for(; (++b < c) && ((x = Td[PA[*b]]) <= v);) {
        if(x == v) { std::swap(*b, *a); ++a; }
      }
      for(; (b < --c) && ((x = Td[PA[*c]]) >= v);) {
        if(x == v) { std::swap(*c, *d); --d; }
      }
    }

    if(a <= d) {
      c = b - 1;

      if((s = a - first) > (t = b - a)) { s = t; }
      for(e = first, f = b - s; 0 < s; --s, ++e, ++f) { std::swap(*e, *f); }
      if((s = d - c) > (t = last - d - 1)) { s = t; }
      for(e = b, f = last - s; 0 < s; --s, ++e, ++f) { std::swap(*e, *f); }

      a = first + (b - a), c = last - (d - c);
      b = (v <= Td[PA[*a] - 1]) ? a : ss_partition(PA, a, c, depth);

      if((a - first) <= (last - c)) {
        if((last - c) <= (c - b)) {
          stack.push(b, c, depth + 1, ss_ilg(c - b));
          stack.push(c, last, depth, limit);
          last = a;
        } else if((a - first) <= (c - b)) {
          stack.push(c, last, depth, limit);
          stack.push(b, c, depth + 1, ss_ilg(c - b));
          last = a;
        } else {
          stack.push(c, last, depth, limit);
          stack.push(first, a, depth, limit);
          first = b, last = c, depth += 1, limit = ss_ilg(c - b);
        }
      } else {
        if((a - first) <= (c - b)) {
          stack.push(b, c, depth + 1, ss_ilg(c - b));
          stack.push(first, a, depth, limit);
          first = c;
        } else if((last - c) <= (c - b)) {
          stack.push(first, a, depth, limit);
          stack.push(b, c, depth + 1, ss_ilg(c - b));
          first = c;
        } else {
          stack.push(first, a, depth, limit);
          stack.push(c, last, depth, limit);
          first = b, last = c, depth += 1, limit = ss_ilg(c - b);
        }
      }
    } else {
      limit += 1;
      if(Td[PA[*first] - 1] < v) {
        first = ss_partition(PA, first, last, depth);
        limit = ss_ilg(last - first);
      }
      depth += 1;
    }
  }
}

#endif /* (SS_BLOCKSIZE == 0) || (SS_INSERTIONSORT_THRESHOLD < SS_BLOCKSIZE) */


/*---------------------------------------------------------------------------*/

#if SS_BLOCKSIZE != 0

template <typename ResultT = int> static inline void ss_blockswap(ResultT *a, ResultT *b, int n) {
  for(; 0 < n; --n, ++a, ++b) {
		std::swap(*a, *b);
  }
}

template <typename ResultT = int> static inline void ss_rotate(ResultT *first, ResultT *middle, ResultT *last) {
  ResultT *a, *b, t;
  ResultT l, r;
  l = middle - first, r = last - middle;
  for(; (0 < l) && (0 < r);) {
    if(l == r) { ss_blockswap(first, middle, l); break; }
    if(l < r) {
      a = last - 1, b = middle - 1;
      t = *a;
      for (;;) {
        *a-- = *b, *b-- = *a;
        if(b < first) {
          *a = t;
          last = a;
          if((r -= l + 1) <= l) { break; }
          a -= 1, b = middle - 1;
          t = *a;
        }
      }
    } else {
      a = first, b = middle;
      t = *a;
      for (;;) {
        *a++ = *b, *b++ = *a;
        if(last <= b) {
          *a = t;
          first = a + 1;
          if((l -= r + 1) <= r) { break; }
          a += 1, b = middle;
          t = *a;
        }
      }
    }
  }
}


/*---------------------------------------------------------------------------*/

template <typename CharT = unsigned char, typename ResultT = int> static void ss_inplacemerge(const CharT *T, const ResultT *PA, ResultT *first, ResultT *middle, ResultT *last, int depth) {
  const ResultT *p;
  ResultT *a, *b;
  ResultT len, half;
  int32_t q, r;
  int32_t x;

  for(;;) {
    if(*(last - 1) < 0) { x = 1; p = PA + ~*(last - 1); }
    else                { x = 0; p = PA +  *(last - 1); }
    for(a = first, len = middle - first, half = len >> 1, r = -1;
        0 < len;
        len = half, half >>= 1) {
      b = a + half;
      q = ss_compare(T, PA + ((0 <= *b) ? *b : ~*b), p, depth);
      if(q < 0) {
        a = b + 1;
        half -= (len & 1) ^ 1;
      } else {
        r = q;
      }
    }
    if(a < middle) {
      if(r == 0) { *a = ~*a; }
      ss_rotate(a, middle, last);
      last -= middle - a;
      middle = a;
      if(first == middle) { break; }
    }
    --last;
    if(x != 0) { while(*--last < 0) { } }
    if(middle == last) { break; }
  }
}


/*---------------------------------------------------------------------------*/

/* Merge-forward with internal buffer. */
template <typename CharT = unsigned char, typename ResultT = int> static void ss_mergeforward(const CharT *T, const ResultT *PA, ResultT *first, ResultT *middle, ResultT *last, ResultT *buf, int depth) {
  ResultT *a, *b, *c, *bufend;
  ResultT t;
  int32_t r;

  bufend = buf + (middle - first) - 1;
  ss_blockswap(buf, first, middle - first);

  for(t = *(a = first), b = buf, c = middle;;) {
    r = ss_compare(T, PA + *b, PA + *c, depth);
    if(r < 0) {
      do {
        *a++ = *b;
        if(bufend <= b) { *bufend = t; return; }
        *b++ = *a;
      } while(*b < 0);
    } else if(r > 0) {
      do {
        *a++ = *c, *c++ = *a;
        if(last <= c) {
          while(b < bufend) { *a++ = *b, *b++ = *a; }
          *a = *b, *b = t;
          return;
        }
      } while(*c < 0);
    } else {
      *c = ~*c;
      do {
        *a++ = *b;
        if(bufend <= b) { *bufend = t; return; }
        *b++ = *a;
      } while(*b < 0);

      do {
        *a++ = *c, *c++ = *a;
        if(last <= c) {
          while(b < bufend) { *a++ = *b, *b++ = *a; }
          *a = *b, *b = t;
          return;
        }
      } while(*c < 0);
    }
  }
}

/* Merge-backward with internal buffer. */
template <typename CharT = unsigned char, typename ResultT = int> static void ss_mergebackward(const CharT *T, const ResultT *PA, ResultT *first, ResultT *middle, ResultT *last, ResultT *buf, int depth) {
  const ResultT *p1, *p2;
  ResultT *a, *b, *c, *bufend;
  ResultT t;
  int32_t r;
  int32_t x;

  bufend = buf + (last - middle) - 1;
  ss_blockswap(buf, middle, last - middle);

  x = 0;
  if(*bufend < 0)       { p1 = PA + ~*bufend; x |= 1; }
  else                  { p1 = PA +  *bufend; }
  if(*(middle - 1) < 0) { p2 = PA + ~*(middle - 1); x |= 2; }
  else                  { p2 = PA +  *(middle - 1); }
  for(t = *(a = last - 1), b = bufend, c = middle - 1;;) {
    r = ss_compare(T, p1, p2, depth);
    if(0 < r) {
      if(x & 1) { do { *a-- = *b, *b-- = *a; } while(*b < 0); x ^= 1; }
      *a-- = *b;
      if(b <= buf) { *buf = t; break; }
      *b-- = *a;
      if(*b < 0) { p1 = PA + ~*b; x |= 1; }
      else       { p1 = PA +  *b; }
    } else if(r < 0) {
      if(x & 2) { do { *a-- = *c, *c-- = *a; } while(*c < 0); x ^= 2; }
      *a-- = *c, *c-- = *a;
      if(c < first) {
        while(buf < b) { *a-- = *b, *b-- = *a; }
        *a = *b, *b = t;
        break;
      }
      if(*c < 0) { p2 = PA + ~*c; x |= 2; }
      else       { p2 = PA +  *c; }
    } else {
      if(x & 1) { do { *a-- = *b, *b-- = *a; } while(*b < 0); x ^= 1; }
      *a-- = ~*b;
      if(b <= buf) { *buf = t; break; }
      *b-- = *a;
      if(x & 2) { do { *a-- = *c, *c-- = *a; } while(*c < 0); x ^= 2; }
      *a-- = *c, *c-- = *a;
      if(c < first) {
        while(buf < b) { *a-- = *b, *b-- = *a; }
        *a = *b, *b = t;
        break;
      }
      if(*b < 0) { p1 = PA + ~*b; x |= 1; }
      else       { p1 = PA +  *b; }
      if(*c < 0) { p2 = PA + ~*c; x |= 2; }
      else       { p2 = PA +  *c; }
    }
  }
}

/* D&C based merge. */
template <typename CharT = unsigned char, typename ResultT = int> static void ss_swapmerge(const CharT *T, const ResultT *PA, ResultT *first, ResultT *middle, ResultT *last, ResultT *buf, int bufsize, int depth) {
  struct stack_type {
    ResultT *a;
    ResultT *b;
    ResultT *c;
    int32_t d;
    constexpr operator auto() noexcept {
      return std::tie(a,b,c,d);
    }
  };
  
  auto get_idx = [](auto a) {
    return (0 <= a) ? a : ~a;
  };
  
  auto merge_check = [&](auto a, auto b, auto c) {
    if ((c & 1) || ((c & 2) && (ss_compare(T, PA + get_idx(*(a - 1)), PA + *a, depth) == 0))) {
      *a = ~*a;
    }
    if ((c & 4) && ((ss_compare(T, PA + get_idx(*(b - 1)), PA + *b, depth) == 0))) {
      *b = ~*b;
    }
  };
  
  static_stack<stack_type, SS_SMERGE_STACKSIZE> stack;
  ResultT *l, *r, *lm, *rm;
  ResultT m, len, half;
  int32_t check, next;

  for(check = 0;;) {
    if((last - middle) <= bufsize) {
      if((first < middle) && (middle < last)) {
        ss_mergebackward(T, PA, first, middle, last, buf, depth);
      }
      merge_check(first, last, check);
      if (stack.size() == 0) return;
      stack.pop_into(first, middle, last, check);
      continue;
    }

    if((middle - first) <= bufsize) {
      if(first < middle) {
        ss_mergeforward(T, PA, first, middle, last, buf, depth);
      }
      merge_check(first, last, check);
      if (stack.size() == 0) return;
      stack.pop_into(first, middle, last, check);
      continue;
    }

    for(m = 0, len = std::min(middle - first, last - middle), half = len >> 1;
        0 < len;
        len = half, half >>= 1) {
      if(ss_compare(T, PA + get_idx(*(middle + m + half)),
                       PA + get_idx(*(middle - m - half - 1)), depth) < 0) {
        m += half + 1;
        half -= (len & 1) ^ 1;
      }
    }

    if(0 < m) {
      lm = middle - m, rm = middle + m;
      ss_blockswap(lm, middle, m);
      l = r = middle, next = 0;
      if(rm < last) {
        if(*rm < 0) {
          *rm = ~*rm;
          if(first < lm) { for(; *--l < 0;) { } next |= 4; }
          next |= 1;
        } else if(first < lm) {
          for(; *r < 0; ++r) { }
          next |= 2;
        }
      }

      if((l - first) <= (last - r)) {
        stack.push(r, rm, last, (next & 3) | (check & 4));
        middle = lm, last = l, check = (check & 3) | (next & 4);
      } else {
        if((next & 2) && (r == middle)) { next ^= 6; }
        stack.push(first, lm, l, (check & 3) | (next & 4));
        first = r, middle = rm, check = (next & 3) | (check & 4);
      }
    } else {
      if(ss_compare(T, PA + get_idx(*(middle - 1)), PA + *middle, depth) == 0) {
        *middle = ~*middle;
      }
      merge_check(first, last, check);
      if (stack.size() == 0) return;
      stack.pop_into(first, middle, last, check);
    }
  }
}

#endif /* SS_BLOCKSIZE != 0 */


/*---------------------------------------------------------------------------*/

/*- Function -*/

/* Substring sort */
template <typename CharT = unsigned char, typename ResultT = int> void sssort(const CharT *T, const ResultT *PA, ResultT *first, ResultT *last, ResultT *buf, int bufsize, int depth, int n, int32_t lastsuffix) {
  
  ResultT *a;
#if SS_BLOCKSIZE != 0
  ResultT *b, *middle, *curbuf;
  int j, k, curbufsize, limit;
#endif
  int i;

  if(lastsuffix != 0) {
    ++first;
  }

  if constexpr (SS_BLOCKSIZE == 0) {
    ss_mintrosort(T, PA, first, last, depth);
  } else {
    if((bufsize < SS_BLOCKSIZE) && (bufsize < (last - first)) && (bufsize < (limit = ss_isqrt(last - first)))) {
      if(SS_BLOCKSIZE < limit) {
        limit = SS_BLOCKSIZE;
      }
      buf = middle = last - limit, bufsize = limit;
    } else {
      middle = last, limit = 0;
    }
    
    for(a = first, i = 0; SS_BLOCKSIZE < (middle - a); a += SS_BLOCKSIZE, ++i) {
      if constexpr (SS_INSERTIONSORT_THRESHOLD < SS_BLOCKSIZE) {
        ss_mintrosort(T, PA, a, a + SS_BLOCKSIZE, depth);
      } else if constexpr (1 < SS_BLOCKSIZE) {
        ss_insertionsort(T, PA, a, a + SS_BLOCKSIZE, depth);
      }
    
      curbufsize = last - (a + SS_BLOCKSIZE);
      curbuf = a + SS_BLOCKSIZE;
      if(curbufsize <= bufsize) {
        curbufsize = bufsize, curbuf = buf;
      }
      for(b = a, k = SS_BLOCKSIZE, j = i; j & 1; b -= k, k <<= 1, j >>= 1) {
        ss_swapmerge(T, PA, b - k, b, b + k, curbuf, curbufsize, depth);
      }
    }
  
    if constexpr (SS_INSERTIONSORT_THRESHOLD < SS_BLOCKSIZE) {
      ss_mintrosort(T, PA, a, middle, depth);
    } else if (1 < SS_BLOCKSIZE) {
      ss_insertionsort(T, PA, a, middle, depth);
    }
    
    for(k = SS_BLOCKSIZE; i != 0; k <<= 1, i >>= 1) {
      if(i & 1) {
        ss_swapmerge(T, PA, a - k, a, middle, buf, bufsize, depth);
        a -= k;
      }
    }
    if(limit != 0) {
      if constexpr (SS_INSERTIONSORT_THRESHOLD < SS_BLOCKSIZE) {
        ss_mintrosort(T, PA, middle, last, depth);
      } else if (1 < SS_BLOCKSIZE) {
        ss_insertionsort(T, PA, middle, last, depth);
      }
      ss_inplacemerge(T, PA, first, middle, last, depth);
    }
  }

  if(lastsuffix != 0) {
    /* Insert last type B* suffix. */
    ResultT PAi[2] = { PA[*(first - 1)], n - 2};
    for(a = first, i = *(first - 1); (a < last) && ((*a < 0) || (0 < ss_compare(T, &(PAi[0]), PA + *a, depth))); ++a) {
      *(a - 1) = *a;
    }
    *(a - 1) = i;
  }
}
