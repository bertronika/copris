/*
 * Take input text `copris_text' and replace Markdown elements with appropriate command
 * values. If there are missing ones, close them automatically and print warnings. Put
 * parsed text into `copris_text', overwriting previous content.
 *
 * Note: a missing bold+italic combination (`***') doesn't produce its own error. It is too
 * ambiguous to figure it out.
 */
void parse_markdown(UT_string *copris_text, struct Inifile **prset);
