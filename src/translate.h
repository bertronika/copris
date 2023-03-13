/*
 * Load translation file 'filename' into a hash table, passed by 'prset'.
 * Return 0 on success.
 */
int load_translation_file(const char *filename, struct Inifile **trfile);

/*
 * Unload translation hash table, passed on by 'trfile'.
 */
void unload_translation_file(struct Inifile **trfile);

/*
 * Take input text 'copris_text' and translate it according to definitions, passed by
 * 'trfile' hash table. Put translated text into 'copris_text', overwriting previous content.
 */
void translate_text(UT_string *copris_text, struct Inifile **trfile);

