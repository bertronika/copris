#!/bin/sh
# Print listing in the following format to standard output, picking all
# printable characters:
#
#   hex_value (dec_value): 'character'  hex_value (dec_value): 'character' ...
#
# Run as ./printchar.sh [OPTIONAL_START_NUM] [OPTIONAL_END_NUM] > /dev/ttyUSB0,
# according to your output device. Confirmation text is sent to standard error
# and will thus not appear in the redirected output.

START_VALUE=${1:-32}
END_VALUE=${2:-255}
BREAK_AFTER=4

if [ "${1}x" = "-hx" ] || [ "${1}x" = "--helpx" ]; then
	sed -n '2,10p' "$0"
	exit
fi

# If script is running through a pipe, omit confirmation message
if [ $# -le 1 ] && [ -t 1 ]; then
	printf 'Printing values between %d and %d.
Override with %s [START_NUM] [END_NUM], pass optional values in decimal.\n
Press ENTER to continue or Ctrl-C to abort.\n' \
	"$START_VALUE" "$END_VALUE" "$0" >&2
	read CONFIRMATION
fi

COUNT=1

for i in $(seq "$START_VALUE" "$END_VALUE"); do
	if [ "$i" = 127 ]; then
		# Replace DEL with a space
		CHAR=" "
	else
		# Convert character to octal
		CHAR="\\$(printf '%o' "$i")"
	fi

	printf "0x%X (%3d): ${CHAR}   " "$i" "$i"

	if [ $((COUNT % BREAK_AFTER)) = 0 ]; then
		printf '\n'
	fi

	COUNT=$((COUNT + 1))
done

printf '\n'
