// Code of client.c
#include "calculator.h"
// Missing more includes ...

int main(int argc, char *argv[])
{
    CLIENT *clnt; // will contain information about the remote service we are looking for
    Operands op;  // This variable will be the parameter to the service
    Result* result;  // And here we will save the response of the service

    // Initialize the structure with the information of the searched service
    // You have to pass:
    // 1. IP or name of the machine where the server is (we get it
    // in this case from the command line)
    // 2. Program number (or the constant that represents it)
    // 3. Version number (or the constant that represents it)
    // 4. "tcp" or "udp" protocol
    clnt = clnt_create(argv[1], Calculator, Basic, "udp");
    if (clnt == NULL)
    {
        clnt_pcreateerror("Can't initialize client");
        exit(1);
    }

    // TODO
    // Initialization of the op variable
    // asking the user for a couple of numbers to add
    
    int operator;
    printf("Choose command 1 - add, 2 - subtract, 3 - multiply, 4 - divide\n");
    scanf("%d", &operator);

    op.a;
    op.b;

    printf("Give first number: ");
    scanf("%d", &op.a);
    printf("\nGive second number: ");
    scanf("%d", &op.b);
    
    switch(operator) {
        case 1: result = add_1(&op, clnt);
                break;
        case 2: result = subtract_1(&op, clnt);
                break;
        case 3: result = multiply_1(&op, clnt);
                break;
        case 4: result = divide_1(&op, clnt);
                break;
    }

   



    // Perform remote invocation

    // Show result, remember that it returns a pointer
    if (result == NULL)
    {
        clnt_perror(clnt, "Remote invocation failed");
        exit(1);
    }

    if (result->type == 1)
        printf("Result: %d\n", result->Result_u.x);
    else if (result->type == 2)
        printf("Result: %f\n", result->Result_u.f);
    else
        printf("Result: %s\n", result->Result_u.err);

    // TODO
    // Print the received value
    clnt_destroy(clnt);
    return 0;
}