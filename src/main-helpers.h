/*
 * Append string number 'count' named 'filename' to pointer
 * list 'filenames'.
 */
void append_file_name(char *filename, char **filenames, int count);

/*
 * Free 'count' number of pointers in 'filenames' from memory.
 */
void free_filenames(char **filenames, int count);
