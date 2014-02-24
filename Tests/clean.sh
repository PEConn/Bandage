for i in *.c
do
	rm ${i%.c}.bc			> /dev/null
	rm ${i%.c}_ban.bc		> /dev/null
	rm ${i%.c}_ban.s		> /dev/null
	rm ${i%.c} 				> /dev/null
done
