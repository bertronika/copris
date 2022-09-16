/*
 * Parse input string `value', consisting of whitespace-separated numbers and variables, with
 * length `value_len'. Resolve variables from `prset' hash table. Append it to the string,
 * passed on by `parsed_value'.
 *
 * Return number of parsed elements, or -1 on error.
 */
int parse_all_to_commands(const char *value, size_t value_len,
                          UT_string *parsed_value, struct Inifile **prset);

/*
 * Parse input string `value', consisting of decimal, hexadecimal or octal numbers, separated by
 * whitespace, to an array of char's `parsed_value' with length `parsed_value_length'.
 *
 * Return number of parsed elements, or -1 on error.
 */
int parse_numbers_to_commands(const char *value, char *parsed_value, int parsed_value_length);
