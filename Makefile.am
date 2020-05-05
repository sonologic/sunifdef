# not a GNU package. You can remove this line, if
# have all needed files, that a GNU package needs
AUTOMAKE_OPTIONS = foreign 1.4

EXTRA_DIST := man/sunifdef.1 man/html/sunifdef_man_1.html man/makeman.sh man/man_pod.pl\
test_sunifdef/scrap_dir_tree.pl test_sunifdef/sunifdef_case_tester.pl \
test_sunifdef/sunifdef_bulk_tester.pl test_sunifdef/sunifdef_softlink_tester.pl \
perl/SunifdefLib.pm \
$(wildcard test_sunifdef/test_cases/*.c) \
$(wildcard test_sunifdef/test_cases/*.expect) \
test_sunifdef/test_cases/altfiles \
Doxyfile \
LICENSE.BSD


SUBDIRS = src man test_sunifdef

.phony:  clean-recursive doxy-clean doxygen

clean-recursive: doxy-clean

doxy-clean:
	rm -f ./html/*

doxygen: ./html/index.html

./html/index.html:	./src/sunifdef
	if test -d ./html;	then rm -f ./html/*; fi
	doxygen Doxyfile

man/sunifdef.1:
	cd man && make

man/html/sunifdef_man_1.html: man/sunifdef.1

./src/sunifdef:
	cd src && make
