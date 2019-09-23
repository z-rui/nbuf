@0xcc774e5a34ebd700;

enum Enum {
	apples @0;
	pears @1;
	bananas @2;
}

struct Foo {
	id @0 : UInt64;
	count @1 : UInt16;
	prefix @2 : UInt8;
	length @3 : UInt32;
}

struct Bar {
	parent @0 : Foo;
	time @1 : Int32;
	ratio @2 : Float32;
	size @3 : UInt16;
}

struct FooBar {
	sibling @0 : Bar;
	name @1 : Text;
	rating @2 : Float64;
	postfix @3 : UInt8;
}

struct FooBarContainer {
	list @0 : List(FooBar);
	initialized @1 : Bool;
	fruit @2 : Enum;
	location @3 : Text;
}
