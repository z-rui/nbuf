#!/usr/bin/env python3

import sys
sys.path.append('../python')

from my_game_nb import *

def print_hero(x):
    print("name:", repr(x.name()))
    print("gender:", x.gender())
    print("race:", x.race())
    print("hp:", x.hp())
    print("mana:", x.mana())
    for item in x.inventory():
        print("item: <")
        print(" name:", repr(item.name()))
        print(" quantity:", item.quantity())
        print(" buc:", item.buc())
        print(" corroded:", item.corroded())
        print(" enchantment:", item.enchantment())
        print(">")

def load(buf):
    x = Hero.from_buf(buf)
    assert x.hp() == 1337
    print_hero(x)

def main():
    with open('my_game.bin', 'rb') as f:
        buf = f.read()
    load(buf) # read back from the buffer

if __name__ == '__main__':
    main()
