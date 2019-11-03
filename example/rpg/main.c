#include "rpg.nb.h"
#include "common.h"

#include <stdlib.h>

#ifndef _WIN32
# include <termios.h>
static struct termios term;
#endif

void restore_term(void)
{
#ifndef _WIN32
	tcsetattr(0, 0, &term);
#endif
}

int main()
{
#ifndef _WIN32
	struct termios term1;
	CHECK(tcgetattr(0, &term) == 0);
#endif
	load_config();
	load_state();
#ifndef _WIN32
	term1 = term;
	term1.c_lflag &= ~ICANON & ~ECHO;
	CHECK(tcsetattr(0, 0, &term1) == 0);
#endif
	atexit(restore_term);
	while (resume())
		;
	return 0;
}
