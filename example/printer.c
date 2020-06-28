#include "libnbuf.h"
#include "my_game.nb.h"

#include <assert.h>
#include <stdio.h>

void
load(struct nbuf_buffer *buf)
{
	my_game_Hero x;
	FILE *f;

	f = fopen("my_game.bin", "rb");
	nbuf_load_file(buf, f);
	fclose(f);
	x = my_game_get_Hero(buf);
	assert(my_game_Hero_hp(&x) == 1337);
	nbuf_print(&x.o, stdout, /*indent=*/2,
		my_game_refl_schema,
		&my_game_refl_Hero);
}

int
main()
{
	struct nbuf_buffer buf = {NULL};

	load(&buf); /* read back from the buffer */
	nbuf_free(&buf);
}
