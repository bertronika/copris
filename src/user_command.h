/*
 * Take input text 'copris_text' and scan it for any invocations of feature
 * commands, prefixed by USER_CMD_SYMBOL. If found commands exist in
 * 'features' hash table, substitute them with commands from 'features'.
 */
int parse_user_commands(UT_string *copris_text, struct Inifile **features);
