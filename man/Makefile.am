man1_MANS = sunifdef.1

.phony: all man clean distlean

all:	sunifdef.1

clean:
	rm -f sunifdef.1 ./html/* *.tmp

distclean:
	rm -f Makefile

man:	sunifdef.1
 
sunifdef.1: man_pod.pl makeman.sh
	./makeman.sh	

