#include <stdio.h>
#ifndef DO_TEST
#	error "This source file should only be processed when doing tests, "
#else

#	include "utils.h"

#	include <logger.h>
#	define STRING(a) a, #a
#	define TEST(x)        \
		total_tests++; \
		tests_passed += x

bool test_num_to_string(size_t t, const char *s) {
	StringRef res = num_to_string(t);
	bool      b   = strcmp(s, res.str) == 0;
	llog(LOG_DEBUG, "%s == %s, %s\n", res.str, s, b ? "Success" : "Failure");
	return b;
}

int main() {
	size_t tests_passed = 0;
	size_t total_tests  = 0;

	llog(LOG_DEBUG, "---- num_to_string ----\n");
	TEST(test_num_to_string(STRING(19)));
	TEST(test_num_to_string(STRING(651)));
	TEST(test_num_to_string(STRING(6541841658)));
	TEST(test_num_to_string(STRING(16843155)));
	TEST(test_num_to_string(STRING(2664581)));
	TEST(test_num_to_string(STRING(12312319)));
	TEST(test_num_to_string(STRING(0)));
	TEST(test_num_to_string(STRING(18446744073709551615)));
	TEST(test_num_to_string(STRING(1698)));
	TEST(test_num_to_string(STRING(56516)));
	TEST(test_num_to_string(STRING(41698874651)));
	TEST(test_num_to_string(STRING(6541351)));
	TEST(test_num_to_string(STRING(7984613648)));
	TEST(test_num_to_string(STRING(84651321)));
	TEST(test_num_to_string(STRING(6541515651654685)));
	TEST(test_num_to_string(STRING(591648519815)));
	TEST(test_num_to_string(STRING(6534154849)));
	TEST(test_num_to_string(STRING(66513186)));

	llog(LOG_INFO, "%zu tests passed out of %zu. Pass rate of %.3f%%\n", tests_passed, total_tests, ((double)tests_passed / (double)total_tests) * 100);
	return 0;
}

#endif
