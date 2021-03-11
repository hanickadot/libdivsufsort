#ifndef LIBDIVSUFSORT_COMMON_HPP
#define LIBDIVSUFSORT_COMMON_HPP

#include <cstdint>
#include <cstddef>
#include <limits>
#include <array>
#include <cassert>
#include <cstddef>
#include <tuple>

#if defined(SS_INSERTIONSORT_THRESHOLD)
# if SS_INSERTIONSORT_THRESHOLD < 1
#  undef SS_INSERTIONSORT_THRESHOLD
#  define SS_INSERTIONSORT_THRESHOLD (1)
# endif
#else
# define SS_INSERTIONSORT_THRESHOLD (8)
#endif
#if defined(SS_BLOCKSIZE)
# if SS_BLOCKSIZE < 0
#  undef SS_BLOCKSIZE
#  define SS_BLOCKSIZE (0)
# elif 32768 <= SS_BLOCKSIZE
#  undef SS_BLOCKSIZE
#  define SS_BLOCKSIZE (32767)
# endif
#else
# define SS_BLOCKSIZE (1024)
#endif
/* minstacksize = log(SS_BLOCKSIZE) / log(3) * 2 */
#if SS_BLOCKSIZE == 0
# if defined(BUILD_DIVSUFSORT64)
#  define SS_MISORT_STACKSIZE (96)
# else
#  define SS_MISORT_STACKSIZE (64)
# endif
#elif SS_BLOCKSIZE <= 4096
# define SS_MISORT_STACKSIZE (16)
#else
# define SS_MISORT_STACKSIZE (24)
#endif
#if defined(BUILD_DIVSUFSORT64)
# define SS_SMERGE_STACKSIZE (64)
#else
# define SS_SMERGE_STACKSIZE (32)
#endif
/* for trsort.c */
#define TR_INSERTIONSORT_THRESHOLD (8)
#if defined(BUILD_DIVSUFSORT64)
# define TR_STACKSIZE (96)
#else
# define TR_STACKSIZE (64)
#endif

static constexpr inline int32_t lg_table[256]= {
 -1,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
  6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
};

static constexpr inline int32_t sqq_table[256] = {
  0,  16,  22,  27,  32,  35,  39,  42,  45,  48,  50,  53,  55,  57,  59,  61,
 64,  65,  67,  69,  71,  73,  75,  76,  78,  80,  81,  83,  84,  86,  87,  89,
 90,  91,  93,  94,  96,  97,  98,  99, 101, 102, 103, 104, 106, 107, 108, 109,
110, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126,
128, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142,
143, 144, 144, 145, 146, 147, 148, 149, 150, 150, 151, 152, 153, 154, 155, 155,
156, 157, 158, 159, 160, 160, 161, 162, 163, 163, 164, 165, 166, 167, 167, 168,
169, 170, 170, 171, 172, 173, 173, 174, 175, 176, 176, 177, 178, 178, 179, 180,
181, 181, 182, 183, 183, 184, 185, 185, 186, 187, 187, 188, 189, 189, 190, 191,
192, 192, 193, 193, 194, 195, 195, 196, 197, 197, 198, 199, 199, 200, 201, 201,
202, 203, 203, 204, 204, 205, 206, 206, 207, 208, 208, 209, 209, 210, 211, 211,
212, 212, 213, 214, 214, 215, 215, 216, 217, 217, 218, 218, 219, 219, 220, 221,
221, 222, 222, 223, 224, 224, 225, 225, 226, 226, 227, 227, 228, 229, 229, 230,
230, 231, 231, 232, 232, 233, 234, 234, 235, 235, 236, 236, 237, 237, 238, 238,
239, 240, 240, 241, 241, 242, 242, 243, 243, 244, 244, 245, 245, 246, 246, 247,
247, 248, 248, 249, 249, 250, 250, 251, 251, 252, 252, 253, 253, 254, 254, 255
};

template <typename T> struct identity_t {
	using type = T;
};

template <typename T> using no_deduce = identity_t<T>::type;

template <typename CharT> constexpr std::size_t alphabet_size = std::numeric_limits<CharT>::max() + 1;
template <> constexpr std::size_t alphabet_size<std::byte> = std::numeric_limits<uint8_t>::max() + 1;
	
template <typename CharT> constexpr std::size_t bucket_A_size = alphabet_size<CharT>;
template <typename CharT> constexpr std::size_t bucket_B_size = alphabet_size<CharT> * alphabet_size<CharT>;

template <typename T, std::size_t N> struct static_stack {
	std::array<T, N> data{};
	std::size_t usage = 0;
	
	constexpr std::size_t size() const noexcept {
		return usage;
	}
	
	constexpr T & operator[](std::size_t idx) noexcept {
		assert(idx < usage);
		return data[idx];
	}
	
	constexpr const T & operator[](std::size_t idx) const noexcept {
		assert(idx < usage);
		return data[idx];
	}
	
	template <typename... Ts> constexpr void push(Ts && ... args) noexcept {
		assert(usage < data.size());
		data[usage++] = T{std::forward<Ts>(args)...};
	}
	
	constexpr T & top() noexcept {
		assert(usage > 0);
		return data[usage-1];
	}
	
	constexpr const T & top() const noexcept {
		assert(usage > 0);
		return data[usage-1];
	}
	
	constexpr T pop() noexcept {
		assert(usage > 0);
		return data[--usage];
	}
	
	constexpr void pop_into(T & obj) noexcept {
		obj = data[--usage];
	}
	
	template <typename... Ts> constexpr void pop_into(Ts & ... args) noexcept {
		std::tie(args...) = data[--usage];
	}
};


#endif
