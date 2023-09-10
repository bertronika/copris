/*
 * Load encoding file 'filename' into a hash table, passed on by 'encoding'.
 * Return 0 on success.
 */
int load_encoding_file(const char *filename, struct Inifile **encoding);

/*
 * Unload encoding hash table, passed on by 'encoding'.
 */
void unload_encoding_definitions(struct Inifile **encoding);

/*
 * Take input text 'copris_text' and recode it according to definitions, passed on by
 * 'encoding' hash table. Put recoded text into 'copris_text', overwriting previous content.
 */
void recode_text(UT_string *copris_text, struct Inifile **encoding);
