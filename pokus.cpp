#include <tuple>

struct test {
	int a;
	int b;
	operator auto() {
		return std::tie(a,b);
	}
};

int main() {
	test x{1,2};
	auto & [a,b] = x;
}