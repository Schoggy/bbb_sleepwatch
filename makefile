src_folder = src/

cfn = sleepwatch

compiler = gcc
compiler_args = -std=gnu99 -ggdb -pthread -I $(src_folder)
linker_args = -lsqlite3 -pthread -lm

formatter = clang-format
formatter_args = -style="LLVM" -i

all: help info

std2: format run

std1: format compile

test_all: format compile cppcheck run memcheck flawfinder clean

help:
	@echo
	@echo Targets are: help, compile, clean, run, cppcheck,
	@echo memcheck, flawfinder, test_all, format, clean_all
	@echo

info: 
	@echo
	@echo This makefile assumes that all source files are located in $(src_folder)
	@echo This Program requires sqlite3
	@echo

compile: _dot_o _compile_final

_dot_o:
	$(compiler) $(compiler_args) src/dht/*.c src/dht/BBB/*.c $(src_folder)*.c -c 

_compile_final:
	$(compiler) $(compiler_args) *.o -o $(cfn) $(linker_args)

debug: compile
	gdb -directory=src/ -tui -se=./$(cnf)

run: compile
	./$(cfn)

cppcheck:
	cppcheck --enable=all -v $(src_folder)*.c

memcheck: compile
	valgrind -v --leak-check=full --show-leak-kinds=all ./$(cfn)

flawfinder:
	flawfinder $(src_folder)*.c

format: 
	$(formatter) $(formatter_args) $(src_folder)*.c
	$(formatter) $(formatter_args) $(src_folder)*.h

clean_all: clean clean_wiggly remove_exec
	
clean: 
	rm *.o

clean_wiggly: 
	rm -r *~

remove_exec: 
	rm $(cfn)
