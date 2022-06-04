/*
 * Parse input string `value', consisting of decimal, hexadecimal or octal numbers, separated by
 * whitespace, to an array of char's `parsed_value' with 'length' elements.
 *
 * Return number of parsed elements, or -1 on error.
 */
int parse_value_to_binary(const char *value, char *parsed_value, int length);
