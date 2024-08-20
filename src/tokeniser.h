typedef struct {
	char *data;    // Token data
	size_t length; // Token length (until the next separator, not \0)
	bool last;     // Is the token the last one in the input text?
} binary_token_t;

/*
 * Split 'text' with 'sep' separators into tokens. Store internal state in 'storage'.
 * If invoked the first time, set 'first_time' to true, else false (similar to
 * strtok_r(3)).
 *
 * Each token is returned as a binary_token_t struct.
 */
binary_token_t binary_tokeniser(UT_string *text, const char sep,
                                binary_token_t *storage, bool first_time);
