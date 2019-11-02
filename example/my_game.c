#include "libnbuf.h"
#include "my_game.nb.h"

#include <assert.h>
#include <stdio.h>

void
save(struct nbuf_buffer *buf)
{
	my_game_Hero x;
	my_game_Item item;

	x = my_game_new_Hero(buf);
	my_game_Hero_set_name(&x, "n37h4ck", -1);
	my_game_Hero_set_gender(&x, my_game_Gender_FEMALE);
	my_game_Hero_set_race(&x, my_game_Race_ELF);
	my_game_Hero_set_hp(&x, 1337);
	my_game_Hero_set_mana(&x, 42);
	my_game_Hero_init_inventory(&x, 4);
	item = my_game_Hero_inventory(&x, 0);
	my_game_Item_set_name(&item, "quarterstaff", -1);
	my_game_Item_set_quantity(&item, 1);
	my_game_Item_set_corroded(&item, true);
	item = my_game_Hero_inventory(&x, 1);
	my_game_Item_set_name(&item, "cloak of magic resistance", -1);
	my_game_Item_set_quantity(&item, 1);
	my_game_Item_set_buc(&item, my_game_BUC_BLESSED);
	item = my_game_Hero_inventory(&x, 2);
	my_game_Item_set_name(&item, "wand of wishing", -1);
	item = my_game_Hero_inventory(&x, 3);
	my_game_Item_set_name(&item, "orchish helmet", -1);
	my_game_Item_set_quantity(&item, 1);
	my_game_Item_set_buc(&item, my_game_BUC_CURSED);
	my_game_Item_set_enchantment(&item, -4);
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
	my_game_Hero x;
	x = my_game_get_Hero(buf);

	nbuf_print(&x.o, stdout, /*indent=*/2,
		&my_game_refl_schema,
		&my_game_refl_Hero);
	assert(my_game_Hero_hp(&x) == 1337);
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
