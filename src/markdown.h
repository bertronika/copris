/*
 * Take input text 'copris_text' and replace Markdown elements with appropriate command
 * values - printer escape codes, passed by 'features' hash table. Put parsed text into
 * 'copris_text', overwriting previous content.
 *
 * If there are element pairs, close them automatically and print warnings.
 * Note: a missing bold+italic combination ('***') doesn't produce its own error. That one
 * is too ambiguous to be figured out.
 */
void parse_markdown(UT_string *copris_text, struct Inifile **features);
