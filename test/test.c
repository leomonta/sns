#include "HttpMessage.h"
#include <stdio.h>
#ifndef DO_TEST
#	error "This source file should only be processed when doing tests"
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

bool test_trim(const char* to_be_trimmed, const char* trimmed) {

	StringRef to_be = CAST_STRINGREF(to_be_trimmed);
	StringRef is = CAST_STRINGREF(trimmed);

	auto res = trim(&to_be);

	bool b = equal_StringRef(&res, &is);
	llog(LOG_DEBUG, "%.*s == %s, %s\n", CLAMP_TO_POSITIVE_INT(res.len), res.str, trimmed, b ? "Success" : "Failure");

	return b;
}

void do_nothing(StringRef a, StringRef b, InboundHttpMessage *c) {
	llog(LOG_DEBUG, "%.*s = %.*s\n", CLAMP_TO_POSITIVE_INT(a.len), a.str, CLAMP_TO_POSITIVE_INT(b.len), b.str);
}

bool test_parse_options(const char * str) {

	const StringRef ref = CAST_STRINGREF(str);
	
	auto __ = parse_options(&ref, do_nothing, "&", '=', nullptr);
	return true;
}


bool test_urldecode( char * encoded, const char * ref) {

	StringRef res = CAST_STRINGREF(encoded);
	StringView encoded_v = {encoded, strlen(encoded)};

	auto err = url_decode(&res, &encoded_v, true);

	if (ref != nullptr) {
		StringView ref_v = {ref, strlen(ref)};
		return equal_StringView(&encoded_v, &ref_v);
	} else {
		return err != NO_ERROR;
	}
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
	TEST(test_num_to_string(STRING(1846744073709551615)));
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

	TEST(test_trim("asd", "asd"));
	TEST(test_trim("      asd", "asd"));
	TEST(test_trim("      asd        ", "asd"));
	TEST(test_trim("asd        ", "asd"));
	TEST(test_trim("	asd        ", "asd"));
	TEST(test_trim("	asd 	", "asd"));
	TEST(test_trim("	 	", ""));
	TEST(test_trim(" ", ""));

	TEST(test_urldecode("%20", " "));
	TEST(test_urldecode("%20v%26", " v&"));
	TEST(test_urldecode("%20", nullptr));

	TEST(test_parse_options("a=b&c=d&e=f"));
	TEST(test_parse_options("a=bf"));

	llog(LOG_INFO, "%zu tests passed out of %zu. Pass rate of %.3f%%\n", tests_passed, total_tests, ((double)tests_passed / (double)total_tests) * 100);
	return 0;
}

#endif
