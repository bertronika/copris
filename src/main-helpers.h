/*
 * Append string number 'count' named 'filename' to pointer
 * list 'filenames'.
 */
void append_file_name(char *filename, char **filenames, int count);

/*
 * Free 'count' number of pointers in 'filenames' from memory.
 */
void free_filenames(char **filenames, int count);

/*
 * Write 'copris_text' to the appropriate output, specified in 'attrib'.
 * Return zero on success, nonzero on failure.
 */
int write_to_output(UT_string *copris_text, struct Attribs *attrib);
