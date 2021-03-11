#include "include/divsufsort.hpp"
#include <string_view>
#include <cstdio>

int main() {
	using namespace std::string_view_literals;
	auto in = "banana"sv;
	auto a = divsufsort(std::span<const char>(in.data(), in.size()));
		
	for (size_t i = 0; i != a.size(); ++i) {
		printf("%c: %u\n",in[i], unsigned(a[i]));
		//std::cout << in[i] << ": " << a[i] << "\n";
	}
	//
	//for (unsigned v: a) {
	//	std::cout << v << "\n";
	//}
}