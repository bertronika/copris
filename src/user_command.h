/*
 * Take input text 'copris_text' and scan it for any invocations of feature
 * commands, prefixed by USER_CMD_SYMBOL. If found commands exist in
 * 'features' hash table, substitute them with commands from 'features'.
 *
 * Return values (user_action_t):
 * - NO_ACTION on success
 * - ERROR on partial/complete failure
 * - COMMENT when a comment was encountered and skipped ($#comment_text)
 * - DISABLE_MARKDOWN when "$DISABLE_MARKDOWN" was found
 * - DISABLE_COMMANDS when:
 *   + None of
 *     > $ENABLE_COMMANDS
 *     > $ENABLE_CMD
 *     > $CMD
 *     were found at the beginning of 'copris_text'
 *   + "$DISABLE_MARKDOWN" was found somewhere in 'copris_text'
 */

typedef enum user_action {
	NO_ACTION,
	ERROR,
	SKIP_CMD,
	DISABLE_MARKDOWN,
	DISABLE_COMMANDS
} user_action_t;

user_action_t parse_user_commands(UT_string *copris_text, struct Inifile **features);
