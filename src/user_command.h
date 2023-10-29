/*
 * Take input text 'copris_text' and scan it for any invocations of feature
 * commands, prefixed by USER_CMD_SYMBOL. If found commands exist in
 * 'features' hash table, substitute them with commands from 'features'.
 *
 * Return values (user_action_t):
 * - NO_ACTION on success
 * - DISABLE_MARKDOWN when "$NO_MARKDOWN" was found
 * - DISABLE_COMMANDS when "$NO_COMMANDS" was found
 */

typedef enum user_action {
	NO_ACTION,
	ERROR,
	DISABLE_MARKDOWN,
	DISABLE_COMMANDS
} user_action_t;

user_action_t parse_user_commands(UT_string *copris_text, struct Inifile **features);
