# SPDX-License-Identifier: GPL-2.0-or-later
head := w80xhw.h w80xprog.h
obj := w80xprog.o main.o termios.o crc16.o

%.o:%.c $(head)
	@ echo -e "  \e[32mCC\e[0m	" $@
	@ gcc -o $@ -c $< -g -Wall -Wextra -Werror

w80xprog: $(obj)
	@ echo -e "  \e[34mMKELF\e[0m	" $@
	@ gcc -o $@ $^ -g

clean:
	@ rm -f $(obj) w80xprog
