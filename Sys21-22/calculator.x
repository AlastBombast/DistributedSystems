typedef string Text<100>;

struct Operands {
    int a;
    int b;
};

union Result switch(int type){
    case 1: int x;
    case 2: float f;
    case 3: Text err;
};

program Calculator {

    version Basic {

        Result add(Operands ab) = 1;

        Result subtract(Operands ab) = 2;

        Result multiply(Operands ab) = 3;

        Result divide(Operands ab) = 4;
        
    } = 1;

} = 0x40001234;