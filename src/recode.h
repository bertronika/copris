/*
 * Load encoding file 'filename' into a hash table, passed by 'encoding'.
 * Return 0 on success.
 */
int load_translation_file(const char *filename, struct Inifile **encoding);

/*
 * Unload encoding hash table, passed on by 'encoding'.
 */
void unload_translation_file(struct Inifile **encoding);

/*
 * Take input text 'copris_text' and translate it according to definitions, passed on by
 * 'encoding' hash table. Put translated text into 'copris_text', overwriting previous content.
 */
void translate_text(UT_string *copris_text, struct Inifile **encoding);

