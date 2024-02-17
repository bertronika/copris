/*
 * Write text from 'copris_text' to output file 'output_file'.
 * Return zero on success, nonzero on failure.
 */
int copris_write_file(const char *output_file, UT_string *copris_text);

/*
 * Write text from 'copris_text' to the standard output.
 * Return zero on success, nonzero on failure.
 */
int copris_write_stdout(UT_string *copris_text);
