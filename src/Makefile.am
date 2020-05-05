bin_PROGRAMS = sunifdef


# set the include path found by configure
INCLUDES = $(all_includes)

# the library search path.
sunifdef_LDFLAGS = $(all_libraries) 
sunifdef_SOURCES = args.c args.h bool.h categorical.c categorical.h chew.c \
	chew.h dataset.c dataset.h doxygen.h evaluator.c evaluator.h exception.c \
	exception.h file_tree.c file_tree.h filesys.c filesys.h fs_nix.c fs_win.c \
	if_control.c if_control.h io.c io.h line_despatch.c line_despatch.h line_edit.c \
	line_edit.h main.c memory.c memory.h opts.h platform.h ptr_vector.c ptr_vector.h \
	report.c report.h state_utils.c state_utils.h symbol_table.c symbol_table.h \
	string_util.c string_util.h
noinst_HEADERS = args.h bool.h categorical.h chew.h doxygen.h evaluator.h \
	if_control.h io.h line_despatch.h line_edit.h memory.h platform.h ptr_vector.h \
	state_utils.h symbol_table.h opts.h file_tree.h filesys.h exception.h dataset.h \
	report.h string_util.h

