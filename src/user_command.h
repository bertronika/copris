/*
 * Take input text 'copris_text' and scan it for any invocations of feature
 * commands, prefixed by USER_CMD_SYMBOL. If found commands exist in
 * 'features' hash table, substitute them with commands from 'features'.
 *
 * If none of
 *   > $ENABLE_COMMANDS
 *   > $ENABLE_CMD
 *   > $CMD
 * are found at the beginning of 'copris_text', command parsing
 * doesn't commence.
 *
 * Comments in text ($#comment_text) are skipped.
 *
 * Return values (user_action_t):
 * - NO_ACTION on success
 * - DISABLE_MARKDOWN when "$DISABLE_MARKDOWN" was found in 'copris_text'
 */

typedef enum parse_action {
	NO_ACTION,
	SKIP_CMD,
	DISABLE_MARKDOWN
} parse_action_t;

parse_action_t parse_user_commands(UT_string *copris_text, struct Inifile **features);
