#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "BigFigure.h"

int main()
{
	struct BFDetail *a = CreateBF(13, 0);
	char str[15];
	//clock_t t1 = clock();
	//for (int b = -100000000; b < 100000000; b++)
		
	printf("%hd\n",toBF2_s(a, "-123.232300000"));
	toString(a, str);
	printf("'%s'\n", str);
	//printf("%d", clock() - t1);


	system("pause");
}