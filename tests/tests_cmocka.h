// Tests expect the following buffer size:
#if BUFSIZE != 10
#	error "Buffer size set improperly for unit tests."
#endif

#define CALL_PARSE(value) \
        parse_value_to_binary(value, parsed_value, (sizeof parsed_value) - 1)

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
        will_return(__wrap_read, input_2);                       \
        will_return(__wrap_read, NULL); /* Signal an EOF */      \
                                                                 \
        int error = copris_handle_socket(copris_text, &parentfd, &attrib); \
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
                                                                 \
        int error = copris_handle_socket(copris_text, &parentfd, &attrib); \
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
        int error = copris_handle_stdin(copris_text);            \
        expected_stats(sizeof result, 2);                        \
                                                                 \
        assert_false(error);                                     \
        assert_string_equal(utstring_body(copris_text), result); \
                                                                 \
        utstring_free(copris_text);                              \
    } while (0);

#define MUST_DISCARD 0x00

#define EXIT_ON_ERROR \
	    if (error) { return error; }
