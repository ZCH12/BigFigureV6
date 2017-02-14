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

	printf("%hd\n", toBF2_s(a, "-234.232300000"));
	printf("%hd\n", toBF2_s(b, "-1456.232300000"));
	test(c, a, b);
	toString(c, str);
	printf("'%s'\n", str);
	//printf("%d", clock() - t1);


	system("pause");
}