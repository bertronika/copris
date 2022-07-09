// Tests expect the following buffer size:
#if BUFSIZE != 10
#	error "Buffer size set improperly for unit tests."
#endif

#define TEST_HANDLE_SOCKET(input_1, input_2, result)             \
    do {                                                         \
        int parentfd = 0;                                        \
                                                                 \
        struct Attribs attrib;                                   \
        attrib.daemon = false;                                   \
        attrib.limitnum = 0;                                     \
        attrib.copris_flags = 0x00;                              \
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
        utstring_clear(copris_text);                             \
    } while (0);

#define TEST_HANDLE_SOCKET_LIMITED(input, result, limit, flags)  \
    do {                                                         \
        int parentfd = 0;                                        \
                                                                 \
        struct Attribs attrib;                                   \
        attrib.daemon = false;                                   \
        attrib.limitnum = limit;                                 \
        attrib.copris_flags = flags;                             \
                                                                 \
        will_return(__wrap_read, input);                         \
                                                                 \
        int error = copris_handle_socket(copris_text, &parentfd, &attrib); \
                                                                 \
        assert_false(error);                                     \
        assert_string_equal(utstring_body(copris_text), result); \
                                                                 \
        utstring_clear(copris_text);                             \
    } while (0);

#define TEST_HANDLE_STDIN(input_1, input_2, result)              \
    do {                                                         \
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
        utstring_clear(copris_text);                             \
    } while (0);
