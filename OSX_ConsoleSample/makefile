.PHONY : clean all

all : MiP

MiP : *.m *.c *.h
	clang $(filter-out %.h,$^) -g -Wall -Werror -framework Foundation -framework AppKit -framework CoreBluetooth -o $@

clean :
	rm -rf MiP.dSYM/ 2>/dev/null ; exit 0
	rm MiP 2>/dev/null ; exit 0
