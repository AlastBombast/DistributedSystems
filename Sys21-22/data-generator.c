#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <rpc/rpc.h>
#include "types.h"

int main()
{
	XDR operation;
	FILE *file;
	int type;

	Text a = "Testing";
	
	file = fopen("Text.dat", "w");
	if (file == NULL){
		perror("Error on file open");
		exit(1);
	}

	xdrstdio_create(&operation, file, XDR_ENCODE);
	xdr_Text(&operation, &a);
	xdr_destroy(&operation);
	fclose(file);

	struct Person* b = (Person*) malloc(sizeof(Person));
	b->name = "Ana";
	b->age = 22;

	file = fopen("Person.dat", "w");
	if (file == NULL){
		perror("Error on file open");
		exit(1);
	}
	xdrstdio_create(&operation, file, XDR_ENCODE);
	xdr_Person(&operation, b);
	xdr_destroy(&operation);
	fclose(file);
	
	Result r1;

	printf("Choose an option:\n	1.Integer\n	2.Real\n	3.Text\nYour choice:");
	scanf("%d",&r1.type);
	
	switch(r1.type){
		case 1:
			printf("\nGive an integer: ");
			scanf("%d", &r1.Result_u.n);
			break;
		case 2:
			printf("\nGive a float: ");
			scanf("%f", &r1.Result_u.x);
			break;
		case 3:
			printf("\nGive a Text: ");
			scanf("%s", r1.Result_u.error);
			break;	
		}
	
	file = fopen("Result.dat", "w");
	if (file == NULL){
		perror("Error on file open");
		exit(1);
	}

	xdrstdio_create(&operation, file, XDR_ENCODE);
	xdr_Result(&operation, &r1);
	xdr_destroy(&operation);
	fclose(file);

return 0;

}
