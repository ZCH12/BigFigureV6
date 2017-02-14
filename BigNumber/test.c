#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "BigFigure.h"

int main()
{
	struct BFDetail *a = CreateBF(13, 0), *b = CreateBF(13, 0),*c=CreateBF(100,100);
	char str[100];
	//clock_t t1 = clock();
	//for (int b = -100000000; b < 100000000; b++)

	toBF2_s(a, "234.12345678");
	toBF2_s(b, "1456.91111122");
	BFAdd(c, a, b);
	test(c, a, b);
	toString(c, str);
	printf("'%s'\n", str);
	//printf("%d", clock() - t1);


	system("pause");
}