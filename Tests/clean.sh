for i in *.c
do
	rm ${i%.*}.bc
	rm ${i%.*}_ban.bc
	rm ${i%.*}_ban.s
	rm ${i%.*}
done
