typedef string Text<100>;

struct Person {
	Text name;
	int age;
};

union Result switch(int type) {
	case 1: int n;
	case 2: float x;
	case 3: Text error;
};

struct List {
	int data;
	List *next;
};
