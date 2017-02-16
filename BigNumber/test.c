#include <stdio.h>
#include <malloc.h>
#include <time.h>
#include "BigFigure.h"


test1()
{
	struct BFDetail *a = CreateBF(13, 0), *b = CreateBF(13, 0), *c = CreateBF(100, 100);
	char *str;
	clock_t t1 = clock();
	toBF2_s(a, "978916123132123515315641231561212315351321513132313265564561451515515000.7");
	toBF2_s(b, "10000000000000000000000000000000000000000000000000000000000000000000000.333");
	printf("%d\n", CheckBF(a));

#if 1

	for (int DD = 0; DD < 10000000; DD++)
	{
		BFAdd(c, a, b);
	}
	
#else
	test(c, a, b);

#endif

	str = alloca(GetBitCount(c));

	toString(c, str);
	printf("'%s'\n", str);
	printf("ÔËÐÐºÄÊ±:%dms\n", clock() - t1);

}


test2()
{
	int *a = alloca(sizeof(int)*56000);
	a[0]=123;
	_freea(a);
	printf("%d",a[0]);
}


int main()
{


	//for (int b=0;b<1000000;b++)
test1();
	system("pause");
}
