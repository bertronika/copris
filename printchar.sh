#!/bin/bash
# Print characters from the following interval.
# Run as ./printchar.sh > /dev/ttyUSB0, according to your printer.

for i in {121..250} # Values are octal
do
	octal=$(printf "%o" $i)
	printf "$i: \\$octal  "
	[[ $(($i % 10)) == 0 ]] && printf "\n"
done

printf "\n"
