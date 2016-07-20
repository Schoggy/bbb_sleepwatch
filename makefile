src_folder = src/
additional_sources = $(src_folder)dht/*.c $(src_folder)dht/BBB/*.c

cfn = sleepwatch

compiler = gcc
compiler_args = -std=gnu99 -ggdb -pthread -I $(src_folder) $(additionsal_sources)
compiler_args_final = -std=gnu99 -pthread -I $(src_folder) $(additionsal_sources)
linker_args = -lsqlite3 -pthread -lm

formatter = clang-format
formatter_args = -style="LLVM" -i

all: help info

std2: format run

std1: format compile

test_all: format compile cppcheck run memcheck flawfinder clean

install: compile_final
	rm -rf /opt/sleepwatch
	mkdir /opt/sleepwatch
	cp -f sleepwatch /opt/sleepwatch

install_autostart: install
	cp -f resources/sleepwatch.local /usr/local/sbin/
	chmod u+x /usr/local/sbin/sleepwatch.local
	cp -f resources/sleepwatch.service /etc/systemd/system
	systemctl enable sleepwatch.service
	
help:
	@echo
	@echo Targets are: help, compile, clean, run, cppcheck,
	@echo memcheck, flawfinder, test_all, format, clean_all
	@echo install, install_autostart
	@echo

info: 
	@echo
	@echo This makefile assumes that all source files are located in $(src_folder)
	@echo This Program requires sqlite3
	@echo install_autostart will only work with systemd
	@echo

_dot_o:
	$(compiler) $(compiler_args) $(src_folder)*.c -c 

_dot_o_final:
	$(compiler) $(compiler_args_final) $(src_folder)*.c -c 

compile: _dot_o
	$(compiler) $(compiler_args) *.o -o $(cfn) $(linker_args)

compile_final: _dot_o_final
	$(compiler) $(compiler_args_final) *.o -o $(cfn) $(linker_args)

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
