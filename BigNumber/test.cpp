#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "cBigFigure.h"

int main()
{
	struct ZCH::BFDetail *a = ZCH::CreateBF(12, 0);
	ZCH::test(a);
	
	system("pause"); 
}