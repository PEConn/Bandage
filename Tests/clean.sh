for i in *.c
do
	rm ${i%.*}.bc			> /dev/null
	rm ${i%.*}_ban.bc		> /dev/null
	rm ${i%.*}_ban.s		> /dev/null
	rm ${i%.*} 				> /dev/null
done
