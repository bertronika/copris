#include "cmocka-boilerplate.h"
#include "../src/read_stdin.h"
#include "../src/read_socket.h"

// Tests expect the following buffer size:
#if BUFSIZE != 10
#	error "Buffer size set improperly for unit tests."
#endif

int verbosity = 0;

static void expected_stats(size_t sizeof_bytes, int chunks)
{
	if (verbosity)
		printf("Expected %zu byte(s) in %d chunk(s) from stdin\n", sizeof_bytes - 1, chunks);
}

#define TEST_WRAPPED_READ(input_1, input_2, result)              \
    do {                                                         \
        int parentfd = 0;                                        \
                                                                 \
        struct Attribs attrib;                                   \
        attrib.daemon = false;                                   \
        attrib.limitnum = 0;                                     \
        attrib.copris_flags = 0x00;                              \
                                                                 \
        UT_string *copris_text;                                  \
        utstring_new(copris_text);                               \
                                                                 \
        will_return(__wrap_read, input_1);                       \
        will_return(__wrap_read, (sizeof input_1) - 1);          \
        will_return(__wrap_read, input_2);                       \
        will_return(__wrap_read, (sizeof input_2) - 1);          \
        will_return(__wrap_read, NULL); /* Signal an EOF */      \
                                                                 \
        bool error = copris_handle_socket(copris_text, &parentfd, &attrib); \
                                                                 \
        assert_false(error);                                     \
        assert_string_equal(utstring_body(copris_text), result); \
                                                                 \
        utstring_free(copris_text);                              \
    } while (0);

#define TEST_WRAPPED_READ_LIMITED(input, result, limit, flags)   \
    do {                                                         \
        int parentfd = 0;                                        \
                                                                 \
        struct Attribs attrib;                                   \
        attrib.daemon = false;                                   \
        attrib.limitnum = limit;                                 \
        attrib.copris_flags = flags;                             \
                                                                 \
        UT_string *copris_text;                                  \
        utstring_new(copris_text);                               \
                                                                 \
        will_return(__wrap_read, input);                         \
        will_return(__wrap_read, (sizeof input) - 1);            \
                                                                 \
        bool error = copris_handle_socket(copris_text, &parentfd, &attrib); \
                                                                 \
        assert_false(error);                                     \
        assert_string_equal(utstring_body(copris_text), result); \
                                                                 \
        utstring_free(copris_text);                              \
    } while (0);

#define TEST_WRAPPED_FGETS(input_1, input_2, result)             \
    do {                                                         \
        UT_string *copris_text;                                  \
        utstring_new(copris_text);                               \
                                                                 \
        will_return(__wrap_fgets, input_1);                      \
        will_return(__wrap_fgets, input_2);                      \
        will_return(__wrap_fgets, NULL); /* Signal an EOF */     \
                                                                 \
        bool no_text_read = copris_handle_stdin(copris_text);    \
        expected_stats(sizeof result, 2);                        \
                                                                 \
        assert_false(no_text_read);                              \
        assert_string_equal(utstring_body(copris_text), result); \
                                                                 \
        utstring_free(copris_text);                              \
    } while (0);

// Read no text from stdin
static void stdin_read_no_text(void **state)
{
	(void)state;

	UT_string *copris_text;
	utstring_new(copris_text);

	will_return(__wrap_fgets, NULL); /* Signal an EOF */

	bool no_text_read = copris_handle_stdin(copris_text);
	expected_stats(1, 0);

	assert_true(no_text_read);
	assert_string_equal(utstring_body(copris_text), "");

	utstring_free(copris_text);
}

// Read two chunks of ASCII text
static void read_two_chunks(void **state)
{
	(void)state;
	const char input_1[] = "aaaBBBccc";
	const char input_2[] = "DDD";
	const char result[]  = "aaaBBBcccDDD";

	TEST_WRAPPED_READ(input_1, input_2, result);
	TEST_WRAPPED_FGETS(input_1, input_2, result);
}

// Read a string with a 2-byte multibyte character split between two reads
static void read_multibyte_char2(void **state)
{
	(void)state;
	const char input_1[] = {'a', 'a', 'a', 'B', 'B', 'B', 'c' ,'c', '\xC4', '\0'};
	const char input_2[] = {'\x8D', '\0'};
	const char result[]  = "aaaBBBccč";

	TEST_WRAPPED_READ(input_1, input_2, result);
	TEST_WRAPPED_FGETS(input_1, input_2, result);
}

// Read a string with a 3-byte multibyte character split between two reads, variant 1
static void read_multibyte_char3a(void **state)
{
	(void)state;
	const char input_1[] = {'a', 'a', 'a', 'B', 'B', 'B', 'c' ,'c', '\xE2', '\0'};
	const char input_2[] = {'\x82', '\xAC', '\0'};
	const char result[]  = "aaaBBBcc€";

	TEST_WRAPPED_READ(input_1, input_2, result);
	TEST_WRAPPED_FGETS(input_1, input_2, result);
}

// Read a string with a 3-byte multibyte character split between two reads, variant 2
static void read_multibyte_char3b(void **state)
{
	(void)state;
	const char input_1[] = {'a', 'a', 'a', 'B', 'B', 'B', 'c', '\xE2', '\x82', '\0'};
	const char input_2[] = {'\xAC', '\0'};
	const char result[]  = "aaaBBBc€";

	TEST_WRAPPED_READ(input_1, input_2, result);
	TEST_WRAPPED_FGETS(input_1, input_2, result);
}

// Read a string with a 4-byte multibyte character split between two reads
static void read_multibyte_char4(void **state)
{
	(void)state;
	const char input_1[] = {'a', 'a', 'a', 'B', 'B', 'B', 'c' ,'c', '\xF0', '\0'};
	const char input_2[] = {'\x9F', '\x84', '\x8C', '\0'};
	const char result[]  = "aaaBBBcc🄌";

	TEST_WRAPPED_READ(input_1, input_2, result);
	TEST_WRAPPED_FGETS(input_1, input_2, result);
}

// Check if text limit gets applied properly
static void byte_limit_discard_and_cutoff(void **state)
{
	(void)state;
	const char input[]  = "aaaBBBccc";
	const char result[] = "aaaBB";

	TEST_WRAPPED_READ_LIMITED(input, "", 1, 0x00);
	TEST_WRAPPED_READ_LIMITED(input, result, 5, MUST_CUTOFF);
}

// Check if multibyte characters are stripped instead of being partially send through
static void byte_limit_discard_and_cutoff2(void **state)
{
	(void)state;
	const char input1[] = "abcč"; // strlen("abcč") == 5
	const char result1[] = "abc";

	const char input2[] = "aaaBBBccc€";
	const char result2[] = "aaaBBBccc";

	TEST_WRAPPED_READ_LIMITED(input1, "", 1, 0x00);
	TEST_WRAPPED_READ_LIMITED(input1, result1, 4, MUST_CUTOFF);
	TEST_WRAPPED_READ_LIMITED(input2, result2, 11, MUST_CUTOFF);
}

int main(int argc, char **argv)
{
	// Number of (arbitrary) arguments sets the verbosity level
	verbosity = argc - 1;
	(void)argv;

	const struct CMUnitTest tests[] = {
		cmocka_unit_test(stdin_read_no_text),
		cmocka_unit_test(read_two_chunks),
		cmocka_unit_test(read_multibyte_char2),
		cmocka_unit_test(read_multibyte_char3a),
		cmocka_unit_test(read_multibyte_char3b),
		cmocka_unit_test(read_multibyte_char4),
		cmocka_unit_test(byte_limit_discard_and_cutoff),
		cmocka_unit_test(byte_limit_discard_and_cutoff2),
	};

	puts("Test read_stdin.c and read_socket.c");
	return cmocka_run_group_tests(tests, NULL, NULL);
}
