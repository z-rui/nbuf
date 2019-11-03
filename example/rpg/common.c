#include "common.h"
#include "libnbuf.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef _WIN32
# include <conio.h>
#else
# define getch getchar
#endif

#define CONFIG_FILE "config.bin"
#define SAVE_FILE "rpg.sav"

rpg_GameConfig config_;
rpg_GameState state_;
rpg_Hero hero_;
rpg_Prop hero_prop_;
rpg_Dungeon dungeon_;

static int
rn(int k, int n)
{
	int x = 0;
	while (k--)
		x += (rand() / (RAND_MAX+1.0)) * n;
	return x;
}

static int
add_inventory(rpg_ItemClass what)
{
	int current = rpg_Hero_inventory(&hero_, what);
	int quantity = 1;

	switch (what) {
	case rpg_ItemClass_SWORD:
	case rpg_ItemClass_SHIELD:
	case rpg_ItemClass_BOW:
		if (current)
			quantity = 0;
		break;
	case rpg_ItemClass_ARROW:
		quantity += rn(4, 4);
		break;
	case rpg_ItemClass_GOLD:
		quantity += rn(3, 5);
		break;
	case rpg_ItemClass_POTION:
		break;
	default:
		LOG_FATAL("unknown ItemClass %d", (int) what);
		break;
	}
	if (quantity == 0) {
		printf("You tried to reach out to it, but it disappeared.\n");
		return 0;
	}
	rpg_Hero_set_inventory(&hero_, what, current + quantity);
	switch (what) {
	case rpg_ItemClass_SWORD:
		printf("You got a sword.\n");
		printf("In battle, hit 'x' key to equip/unequip it.\n");
		break;
	case rpg_ItemClass_SHIELD:
		printf("You got a shield.\n");
		printf("In battle, hit 'y' key to equip/unequip it.\n");
		break;
	case rpg_ItemClass_BOW:
		printf("You got a bow.\n");
		printf("In battle, hit 'z' key to equip/unequip it.\n");
		printf("Once equipped, you have hit 'b' key to shoot\n");
		printf("an arrow (if you have any).\n");
		break;
	case rpg_ItemClass_ARROW:
		if (quantity == 1)
			printf("You got an arrow.\n");
		else
			printf("You got %d arrows.\n", quantity);
		break;
	case rpg_ItemClass_GOLD:
		printf("You found %d piece%s of gold!  "
			"How useful could it be?\n",
			quantity, (quantity == 1) ? "" : "s");
		break;
	case rpg_ItemClass_POTION:
		printf("You got a magic potion... Is it poisonous?\n");
		break;
	default:
		LOG_FATAL("unknown ItemClass %d", (int) what);
		break;
	}
	return quantity;
}

#define HAVE(what) (rpg_Hero_inventory(&hero_, rpg_ItemClass_##what))
#define EQUIPPED(what) (HAVE(what) == 2)

static void
toggle_equip(rpg_ItemClass what, const char *name)
{
	int n = rpg_Hero_inventory(&hero_, what);
	rpg_Prop prop;

	prop = rpg_GameConfig_item(&config_, what);
	switch (n) {
	case 1:
		printf("You are now equipping a %s.\n", name);
		rpg_Hero_set_inventory(&hero_, what, 2);
		rpg_Prop_set_attack(&hero_prop_,
			rpg_Prop_attack(&hero_prop_) +
			rpg_Prop_attack(&prop));
		rpg_Prop_set_defend(&hero_prop_,
			rpg_Prop_defend(&hero_prop_) +
			rpg_Prop_defend(&prop));
		rpg_Prop_set_crit(&hero_prop_,
			rpg_Prop_crit(&hero_prop_) +
			rpg_Prop_crit(&prop));
		break;
	case 2:
		printf("You are no longer equipping the %s.\n", name);
		rpg_Hero_set_inventory(&hero_, what, 1);
		rpg_Prop_set_attack(&hero_prop_,
			rpg_Prop_attack(&hero_prop_) -
			rpg_Prop_attack(&prop));
		rpg_Prop_set_defend(&hero_prop_,
			rpg_Prop_defend(&hero_prop_) -
			rpg_Prop_defend(&prop));
		rpg_Prop_set_crit(&hero_prop_,
			rpg_Prop_crit(&hero_prop_) -
			rpg_Prop_crit(&prop));
		break;
	default:
		LOG_FATAL("bad inventory for \"%s\": %d\n", name, n);
		break;
	}
}

static int
fight_input()
{
	int key;
	char buf[10];
	int i = 0;

	buf[i++] = 'a';
	if (EQUIPPED(BOW) && HAVE(ARROW))
		buf[i++] = 'b';
	if (HAVE(GOLD))
		buf[i++] = 'g';
	if (HAVE(POTION))
		buf[i++] = 'p';
	if (HAVE(SWORD) && !(EQUIPPED(SHIELD) && EQUIPPED(BOW)))
		buf[i++] = 'x';
	if (HAVE(SHIELD) && !(EQUIPPED(BOW) && EQUIPPED(SWORD)))
		buf[i++] = 'y';
	if (HAVE(BOW) && !(EQUIPPED(SWORD) && EQUIPPED(SHIELD)))
		buf[i++] = 'z';
	buf[i++] = '\0';

reinput:
	printf("What do you want to do? [%s?]\n", buf);
	key = getch();
	if (strchr(buf, key) == NULL) {
		for (i = 0; buf[i] != '\0'; i++) {
			switch (buf[i]) {
			case 'a':
				printf("a - attack\n");
				break;
			case 'b':
				printf("b - shoot\n");
				break;
			case 'g':
				printf("g - use gold\n");
				break;
			case 'p':
				printf("p - use potion\n");
				break;
			case 'x':
				printf("x - toogle sword\n");
				break;
			case 'y':
				printf("y - toggle shield\n");
				break;
			case 'z':
				printf("z - toggle bow\n");
				break;
			}
		}
		goto reinput;
	}
	return key;
}

static int
calc_damage(rpg_Prop *attacker, rpg_Prop *defender, bool ignore_defend)
{
	int attack = rpg_Prop_attack(attacker);
	int crit = rpg_Prop_crit(attacker);
	int defend = (ignore_defend) ? 0 : rpg_Prop_defend(defender);
	if (rn(1, 100) <= crit) {
		printf("Critical hit!\n");
		attack *= 3;
	}
	return (attack > defend) ? attack - defend : 0;
}

static void
print_status(rpg_Prop *prop, int hp)
{
	printf("%-30s hp:%d attack:%d defend:%d\n",
		rpg_Prop_name(prop, NULL),
		hp,
		rpg_Prop_attack(prop),
		rpg_Prop_defend(prop));
}

static void
print_inventory(void)
{
	int n;

	printf("Inventory:\n");
	if ((n = HAVE(SWORD)))
		printf("  sword%s\n", (n > 1) ? " (equipped)" : "");
	if ((n = HAVE(SHIELD)))
		printf("  shield%s\n", (n > 1) ? " (equipped)" : "");
	if ((n = HAVE(BOW)))
		printf("  bow%s\n", (n > 1) ? " (equipped)" : "");
	if ((n = HAVE(ARROW)))
		printf("  arrow x%d\n", n);
	if ((n = HAVE(GOLD)))
		printf("  gold x%d\n", n);
	if ((n = HAVE(POTION)))
		printf("  potion x%d\n", n);
}

static void
loot(struct nbuf_obj *o, bool monster)
{
	size_t n, i;
	int p = 80;
	rpg_ItemClass cls;
	bool first;

	n = (monster) ?
		rpg_Monster_item_size((rpg_Monster *) o) :
		rpg_Dungeon_item_size((rpg_Dungeon *) o);
	first = true;
	for (i = 0; i < n; i++) {
		if (rn(1, 100) > p)
			break;
		if (first) {
			printf("You found something %s.\n",
				(monster) ? "from the corpse"
					: "in this room");
			first = false;
		}
		cls = (monster) ?
			rpg_Monster_item((rpg_Monster *) o, i) :
			rpg_Dungeon_item((rpg_Dungeon *) o, i);
		if (add_inventory(cls) > 0)
			p /= 2;
	}
}

static void
do_fight(rpg_Monster *monster)
{
	rpg_Prop monster_prop, item_prop;
	int hero_hp, monster_hp;
	int damage;
	const char *monster_name;
	char art[] = " the";
	rpg_DungeonClass cls;

	cls = rpg_Dungeon_cls(&dungeon_);
	hero_hp = rpg_Hero_hp(&hero_);
	monster_prop = rpg_Monster_prop(monster);
	monster_hp = rpg_Prop_hp(&monster_prop);
	monster_name = rpg_Prop_name(&monster_prop, NULL);

	if (monster_name[0] == ' ') {
		*art = '\0';
		monster_name++;
		printf("It must be %s.  ", monster_name);
		printf("Oh my, fighting is probably a bad idea...\n");
	} else {
		printf("You just woke up a %s!\n", monster_name);
		printf("The %s is angry!\n", monster_name);
	}

	if (rn(2, 2) == 1) {
		printf("Unfortunately,%s %s decides to attack you first.\n",
			art, monster_name);
		print_status(&hero_prop_, hero_hp);
		print_status(&monster_prop, monster_hp);
		goto mhitu;
	}
uhitm:
	print_status(&hero_prop_, hero_hp);
	print_inventory();
	print_status(&monster_prop, monster_hp);
	switch (fight_input()) {
	case 'a':
		damage = calc_damage(&hero_prop_, &monster_prop, false);
		monster_hp -= damage;
		printf("Your attack dealt %d damage to %s %s.\n",
			damage, art, monster_name);
		break;
	case 'b': {
		rpg_Prop arrowProp = rpg_GameConfig_item(&config_, rpg_ItemClass_ARROW);
		int p = 80;
		printf("You shot an arrow.\n");
		for (;;) {
			rpg_Hero_set_inventory(&hero_, rpg_ItemClass_ARROW, HAVE(ARROW)-1);
			damage = calc_damage(&arrowProp, &monster_prop, true);
			monster_hp -= damage;
			printf("Your shot dealt %d damage to%s %s.\n",
				damage, art, monster_name);
			if (monster_hp > 0 && HAVE(ARROW) && rn(1, 100) <= p) {
				printf("And you are fast enough to shoot another one.\n");
				p -= 10;
			} else {
				break;
			}
		}
		break;
	}
	case 'g':
		rpg_Hero_set_inventory(&hero_, rpg_ItemClass_GOLD, 0);
		printf("You dropped all your gold to the ground.\n");
		if (cls == rpg_DungeonClass_PREBOSS) {
			printf("\"Sure, I'll show you the road, Master.\"\n");
			goto out;
		} else {
			printf("You find it a stupid idea...\n");
		}
		break;
	case 'p': {
		int max_hp = rpg_Prop_hp(&hero_prop_);

		item_prop = rpg_GameConfig_item(&config_, rpg_ItemClass_POTION);
		hero_hp += rpg_Prop_hp(&item_prop);
		if (hero_hp > max_hp) {
			hero_hp = max_hp;
			printf("You feel fully healed!\n");
		} else if (hero_hp * 2 < max_hp) {
			hero_hp = max_hp / 2;
			printf("You feel much better.\n");
		} else {
			printf("You feel better.\n");
		}
		rpg_Hero_set_inventory(&hero_, rpg_ItemClass_POTION, HAVE(POTION)-1);
		break;
	}
	case 'x':
		toggle_equip(rpg_ItemClass_SWORD, "sword");
		goto uhitm;
	case 'y':
		toggle_equip(rpg_ItemClass_SHIELD, "shield");
		goto uhitm;
	case 'z':
		toggle_equip(rpg_ItemClass_BOW, "bow");
		goto uhitm;
	}
	if (monster_hp <= 0)
		goto mdie;
mhitu:
	damage = calc_damage(&monster_prop, &hero_prop_, false);
	hero_hp -= damage;
	printf("You are hit by%s %s, ", art, monster_name);
	if (EQUIPPED(SHIELD) && rn(1, 100) <= 10)
		printf("but your shield absorbed all the damage.\n");
	else
		switch (damage) {
		case 0:
			printf("but you are not hurt at all.\n");
			break;
		case 1:
			printf("you lose 1 health point.\n");
			break;
		default:
			printf("you lose %d health points.\n", damage);
			break;
		}
	if (hero_hp <= 0) {
		printf("You die!\n");
		printf("May your poor soul rest in peace.\n");
		hero_hp = 0;
		goto out;
	}
	if (EQUIPPED(SWORD) && rn(1, 100) <= 50) {
		damage = calc_damage(&hero_prop_, &monster_prop, false);
		damage /= 2;
		printf("You used your sword to fight back!\n");
		monster_hp -= damage;
		if (monster_hp <= 0)
			goto mdie;
	}
	goto uhitm;
mdie:
	printf("You killed%s %s!\n", art, monster_name);
	loot(&monster->o, true);
	goto out;
out:
	rpg_Hero_set_hp(&hero_, hero_hp);
	CHECK(rpg_Hero_hp(&hero_) == hero_hp);
}

bool
resume(void)
{
	int id;
	rpg_DungeonClass cls;
	const char *hero_name;
	size_t n, i;

	hero_name = rpg_Prop_name(&hero_prop_, NULL);
	id = rpg_GameState_dungeonId(&state_);
	CHECK(id >= 0);
	CHECK(id < rpg_GameConfig_dungeon_size(&config_));
	dungeon_ = rpg_GameConfig_dungeon(&config_, id);
	cls = rpg_Dungeon_cls(&dungeon_);

	// message
	switch (cls) {
	case rpg_DungeonClass_ENTRY:
		printf("You, %s, have just found the entry to the dungeon "
			"of mystery.\n", hero_name);
		printf("\"It's dangerous to go alone.  Take this!\" "
			"Said an old man.\n");
		add_inventory(rpg_ItemClass_SWORD);
		printf("Now, your adventure starts...\n");
		break;
	case rpg_DungeonClass_BASIC: 
		printf("You walked down a narrow corridor.\n");
		printf("You stepped into another room.\n");
		break;
	case rpg_DungeonClass_PREBOSS:
		printf("You are now in front of a secret door.\n");
		printf("The door looks kind of out of place...\n");
		if (HAVE(SWORD) && HAVE(SHIELD) && HAVE(BOW) && HAVE(ARROW)) {
			printf("You figured there's a hidden badge-scanner.\n");
			printf("But what is waiting for you?\n");
			printf("This is a dark room and you cannot feel anything...\n");
			printf("\"Who are you?\" Suddenly, a voice came from the darkness.\n");
			printf("\"%s, you are not supposed to be here...\"\n", hero_name);
		} else {
			printf("You feel confused, but proceed anyways.\n");
			printf("\"I'll catch you, sooner or later!\"\n");
			printf("That sounds familiar...\n");
			dungeon_ = rpg_GameConfig_dungeon(&config_, rn(1, id));
		}
		break;
	case rpg_DungeonClass_BOSS:
		printf("You survived.  But now you have a bigger trouble...\n");
		printf("You arrived at the heart of the dungeon, where the king of \n");
		printf("the darkness lurks...\n");
		break;
	default:
		LOG_FATAL("unknown DungeonClass %d", (int) cls);
	}

	// fight monsters
	n = rpg_Dungeon_monster_size(&dungeon_);
	for (i = 0; i < n; i++) {
		rpg_Monster monster = rpg_Dungeon_monster(&dungeon_, i);
		do_fight(&monster);
		if (rpg_Hero_hp(&hero_) <= 0)
			return false;
	}

	// loot
	loot(&dungeon_.o, false);

	// leave
	if (cls == rpg_DungeonClass_BOSS) {
		printf("It seems to be the end of the story.\n");
		if (HAVE(GOLD) >= 100) {
			printf("Darkness is dispersed.\n");
			printf("Brightness returns.\n");
			printf("And your story spreads through the ages.\n");
		} else {
			printf("Now you have endless power,\n");
			printf("And wealth...\n");
			printf("But somehow you are constrained in the dungeon,\n");
			printf("and there's no way out...\n");
		}
		return false;
	}

	n = rpg_Dungeon_next_size(&dungeon_);
	CHECK(n > 0);
	if (n > 1) {
		printf("You see a fork along the way.  ");
		do {
			printf("Which one do you want to go? [1-%d]\n", (int) n);
			i = getch();
		} while (!('1' <= i && i <= '9' && i-'1' < n));
		i -= '1';
	} else {
		i = 0;
	}

	id = rpg_Dungeon_next(&dungeon_, i);
	rpg_GameState_set_dungeonId(&state_, id);
	dungeon_ = rpg_GameConfig_dungeon(&config_, id);
	cls = rpg_Dungeon_cls(&dungeon_);

	// save
	save_state();
	printf("Progress saved.\n");
	do {
		printf("Continue playing? [yn]\n");
		i = getch();
	} while (i != 'y' && i != 'n');
	return i == 'y';

	return true;
}

void
save_state(void)
{
	const char *path = SAVE_FILE;
	FILE *f;

	f = fopen(path, "wb");
	if (f == NULL)
		LOG_FATAL("cannot open save file %s: %s",
			path, strerror(errno));
	if (!nbuf_save_file(state_.o.buf, f))
		LOG_FATAL("failed to write to file %s: %s",
			path, strerror(errno));
	fclose(f);
}

static
void new_state(struct nbuf_buffer *buf)
{
	char name[120];
	size_t name_len;
	rpg_Hero hero;
	rpg_Prop prop, prop_def;

	state_ = rpg_new_GameState(buf);
	hero = rpg_GameState_init_hero(&state_);
	prop = rpg_Hero_init_prop(&hero);
	prop_def = rpg_GameConfig_hero(&config_);
	printf("What's your name? ");
	if (fgets(name, sizeof name, stdin) == NULL)
		LOG_FATAL("fgets failed");
	name_len = strlen(name);
	if (name[name_len-1] == '\n')
		name_len--;
	rpg_Prop_set_name(&prop, name, name_len);
	rpg_Prop_set_hp(&prop, rpg_Prop_hp(&prop_def));
	rpg_Prop_set_attack(&prop, rpg_Prop_attack(&prop_def));
	rpg_Prop_set_defend(&prop, rpg_Prop_defend(&prop_def));
	rpg_Prop_set_crit(&prop, rpg_Prop_crit(&prop_def));
	rpg_Hero_set_hp(&hero, rpg_Prop_hp(&prop_def) / 2);
	rpg_Hero_init_inventory(&hero, 6);
}

void
load_state(void)
{
	static struct nbuf_buffer buf;
	const char *path = SAVE_FILE;
	FILE *f;

	nbuf_init_write(&buf, NULL, 0);
	f = fopen(path, "rb");
	if (f == NULL) {
		new_state(&buf);
	} else {
		if (!nbuf_load_file(&buf, f))
			LOG_FATAL("failed to read from file %s: %s",
				path, strerror(errno));
		fclose(f);
	}
	state_ = rpg_get_GameState(&buf);
	hero_ = rpg_GameState_hero(&state_);
	hero_prop_ = rpg_Hero_prop(&hero_);
}

void
load_config(void)
{
	static struct nbuf_buffer buf;
	const char *path = CONFIG_FILE;
	FILE *f;

	f = fopen(path, "rb");
	if (f == NULL)
		LOG_FATAL("cannot open save file %s: %s",
			path, strerror(errno));
	if (!nbuf_load_file(&buf, f))
		LOG_FATAL("failed to read from file %s: %s",
			path, strerror(errno));
	config_ = rpg_get_GameConfig(&buf);
	fclose(f);
}
