#include <stdio.h>
#include <malloc.h>
#include <time.h>
#include "BigFigure.h"

int main()
{
	struct BFDetail *a = CreateBF(13, 0), *b = CreateBF(13, 0), *c = CreateBF(100, 100);
	char str[3000];

	clock_t t1 = clock();
	for (int DD = 0; DD < 100; DD++)
	{
		toBF1_s(a, "97891612313212351531564123156121231535132151313231326556456145151551598");
		toBF1_s(b, "10000000000000000000000000000000000000000000000000000000000000000000000");
		BFAdd(c, a, b);
		
	}
		
	printf("%d\n",CheckBF(b));
	toString(c, str);
	printf("'%s'\n", str);
	printf("ÔËÐÐºÄÊ±:%dms\n", clock() - t1);


	system("pause");
}