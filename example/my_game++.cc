#include "libnbuf.h"
#include "my_game.nb.hpp"

#include <assert.h>
#include <stdio.h>

void
save(struct nbuf_buffer *buf)
{
	using namespace my_game;

	Hero x = new_Hero(buf);
	x.set_name("n37h4ck");
	x.set_gender(Gender::FEMALE);
	x.set_race(Race::ELF);
	x.set_hp(1337);
	x.set_mana(42);
	x.init_inventory(4);
	Item it = x.inventory(0);
	it.set_name("quarterstaff");
	it.set_quantity(1);
	it.set_corroded(true);
	it = x.inventory(1);
	it.set_name("cloak of magic resistance");
	it.set_quantity(1);
	it.set_buc(BUC::BLESSED);
	it = x.inventory(2);
	it.set_name("wand of wishing");
	it = x.inventory(3);
	it.set_name("orchish helmet");
	it.set_quantity(1);
	it.set_buc(BUC::CURSED);
	it.set_enchantment(-4);
}

void
dump(struct nbuf_buffer *buf)
{
	FILE *f = popen("xxd", "w");
	fwrite(buf->base, buf->len, 1, f);
	pclose(f);
}

void
load(struct nbuf_buffer *buf)
{
	using namespace my_game;

	Hero x = get_Hero(buf);
	nbuf_print(&x, stdout, /*indent=*/2,
		&my_game_refl_schema,
		&my_game_refl_Hero);
	assert(x.hp() == 1337);
}

int
main()
{
	struct nbuf_buffer buf = {NULL};

	save(&buf); /* write something into the buffer */
	dump(&buf); /* show the buffer content */
	load(&buf); /* read back from the buffer */
	nbuf_free(&buf);
}
