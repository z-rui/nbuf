#include "my_game.pb.h"

#include <cassert>
#include <cstdio>
#include <sstream>

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>

void
save(std::ostream *os)
{
	my_game::Hero x;
	my_game::Item *it;
	x.set_name("n37h4ck");
	x.set_gender(my_game::FEMALE);
	x.set_race(my_game::ELF);
	x.set_hp(1337);
	x.set_mana(42);
	it = x.add_inventory();
	it->set_name("quarterstaff");
	it->set_quantity(1);
	it->set_corroded(true);
	it = x.add_inventory();
	it->set_name("cloak of magic resistance");
	it->set_quantity(1);
	it->set_buc(my_game::BLESSED);
	it = x.add_inventory();
	it->set_name("wand of wishing");
	it = x.add_inventory();
	it->set_name("orchish helmet");
	it->set_quantity(1);
	it->set_buc(my_game::CURSED);
	it->set_enchantment(-4);

	google::protobuf::io::OstreamOutputStream s(os);
	x.SerializeToZeroCopyStream(&s);
}

void
dump(const std::string& s)
{
	FILE *f = popen("xxd", "w");
	fwrite(s.data(), 1, s.size(), f);
	pclose(f);
}

void
load(std::istream *is)
{
	my_game::Hero x;
	{
		google::protobuf::io::IstreamInputStream s(is);
		x.ParseFromZeroCopyStream(&s);
	}
	{
		google::protobuf::io::OstreamOutputStream s(&std::cout);
		google::protobuf::TextFormat::Print(x, &s);
	}
	assert(x.hp() == 1337);
}

int
main()
{
	std::stringstream ss;

	save(&ss); /* write something into the buffer */
	dump(ss.str()); /* show the buffer content */
	load(&ss); /* read back from the buffer */
}
