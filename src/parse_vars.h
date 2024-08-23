typedef enum modeline {
	NO_MODELINE   = (1 << 0), // No modeline found at the beginning of text
	ML_EMPTY      = (1 << 1), // Modeline was found, but contains no command
	ML_UNKNOWN    = (1 << 2), // Modeline was found, but contains unknown command(s)
	ML_ENABLE_VAR = (1 << 3), // Modeline instructs us to enable variable parsing
	ML_DISABLE_MD = (1 << 4)  // Modeline instructs us to disable parsing Markdown
} modeline_t;
/*
 * Check 'copris_text' if there's a "modeline" at the beginning of text:
 *   COPRIS [ENABLE-VARIABLES|ENABLE-VARS] [DISABLE-MARKDOWN|DISABLE-MD]
 *
 * Letters are case-insensitive, order of commands is not important.
 * At least one command must be specified to make a modeline valid.
 *
 * Return modeline_t according to the parsed result.
 */
modeline_t parse_modeline(UT_string *copris_text);

/*
 * Validate 'modeline' commands in 'copris_text', display possible
 * error messages and remove it from 'copris_text'.
 */
void apply_modeline(UT_string *copris_text, modeline_t modeline);

/*
 * Parse comment, number and command variables in 'copris_text'. Get
 * command variables from 'features'.
 */
void parse_variables(UT_string *copris_text, struct Inifile **features);
