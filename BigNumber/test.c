#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "BigFigure.h"
//#include <string.h>

int main()
{
	struct BFDetail *a = CreateBF(13, 0);
	clock_t t1 = clock();
	for (int b = 0; b < 100000000; b++)
	toBF1(a, "123456789123456789");
	test(a);
	printf("%d", clock() - t1);

	toBF1(a, "1234561234567");
	toBF1(a, "1234");
	toBF1(a, "1");
	DestroyBF(a);

	system("pause");
}