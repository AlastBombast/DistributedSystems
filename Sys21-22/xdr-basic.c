#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <rpc/types.h>
#include <rpc/rpc.h>

int main()
{
	int a;
	char b;

	printf("Insert an int value and a char value:\n");
	scanf("%d %c",&a,&b);

	XDR operation;
	FILE *file;
	file = fopen("basics.dat", "w");
	if (file == NULL){
		perror("Error on file open");
		exit(1);
	}

	xdrstdio_create(&operation, file, XDR_ENCODE);
	xdr_int(&operation, &a);
	xdr_char(&operation, &b);
	xdr_destroy(&operation);
	fclose(file);

	return 0;

}
