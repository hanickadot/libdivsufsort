#ifndef STATIC_STACK_HPP
#define STATIC_STACK_HPP

#include <array>
#include <cassert>
#include <cstddef>
#include <tuple>

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
