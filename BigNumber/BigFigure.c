#include <malloc.h>
#include <assert.h>
#include <string.h>
#include "BigFigure.h"

#define MIN(a,b) ((a<b)?(a):(b))
#define MAX(a,b) ((a>b)?(a):(b))


typedef uint32_t bit32;									//bit32为32位的内存块,用于struct中
typedef int8_t ErrVal;									//定义返回值类型

#if BF_BUFF_USE==1				//当开启异常安全保证时
#define BF_BUFF_SIZE 2			//开启双缓冲,能够确保异常安全,但不节省内存
#else 
#define BF_BUFF_SIZE 1			//开启单缓冲,不能确保异常安全,但节省内存
#endif

typedef char* pchar;
struct BFDetail
{
	bit32 bTemp : 1;									//记录此数字是否是一个临时对象(只用于C++类中)
	bit32 bMinus : 1;									//记录此数字是否有负号(有负号为true)
	bit32 iCDI : 1;										//CurrentDataIndex(当前正在使用的缓冲区的数组下标)
	bit32 iReferCount : 16;								//此指针的引用计数(只用于C++类中)
	usize iAInt;										//为整数部分分配的内存的大小
	usize iAFlt;										//为小数部分分配的内存的大小
	usize iLInt;										//整数部分的实际长度
	usize iLFlt;										//整数部分的实际长度
	pchar psInt[BF_BUFF_SIZE];							//指向整数部分最高位的首指针
	pchar psRPt[BF_BUFF_SIZE];							//pData的中表示整数的字符串的结束符(处于小数点的位置)
	pchar psFlt[BF_BUFF_SIZE];							//指向小数部分最高位的首指针
	pchar pData[BF_BUFF_SIZE];							//记录两个正在使用的数据的缓冲区的首地址
};

//本地函数声明
static inline ErrVal __AllocData(pchar* Data, pchar* psRPt, usize IntLen, usize FltLen);
static inline ErrVal __ResetDataPoint(pchar * psRPt, pchar * psInt, usize iLInt, usize iAInt, pchar * psFlt, usize iAFlt);
static char* _StrCpyB(const char * DestStart, char *DestEnd, const char * SourceStart, const char * SourceEnd);
static char* _StrCpy(char * DestStart, const char *DestEnd, const char *SourceStart, const char* SourceEnd);

//工厂函数(用于生产BF)
struct BFDetail* CreateBF(usize intLen, usize FloatLen)
{
	struct BFDetail* BF;
	ErrVal retVal;
	if (!intLen)										//阻止整数部分长度为0,整数部分的长度至少应为1
		return NULL;
	BF = (struct BFDetail*)malloc(sizeof(struct BFDetail));
	if (!BF)
		return NULL;

	retVal = __AllocData(&BF->pData[0], &BF->psRPt[0], intLen, FloatLen);
	if (retVal)
	{
		free(BF);
		return NULL;
	}
	BF->iAFlt = FloatLen;
	BF->iAInt = intLen;
	BF->iLFlt = 0;
	BF->iLInt = 1;

	BF->bMinus = 0;
	BF->bTemp = 0;
	BF->iCDI = 0;
	BF->iReferCount = 1;
#if BF_BUFF_USE 
	BF->pData[1] = (pchar)0;
#endif

	__ResetDataPoint(&BF->psRPt[0], &BF->psInt[0], BF->iLInt, BF->iAInt, &BF->psFlt[0], BF->iAFlt);	//设置写入位置
	if (FloatLen)
		*BF->psFlt[0] = 0;									//初始化小数部分数字为空
	*BF->psInt[0] = '0';									//初始化整数部分数字为0
	return BF;
}

//BF回收器,用于回收不需要用到的BF的内存
void DestroyBF(struct BFDetail* OperateBF)
{
	if (OperateBF)
	{
		if (OperateBF->pData[0])
		{
			free(OperateBF->pData[0]);
		}
#if BF_BUFF_USE
		if (OperateBF->pData[1])
		{
			free(OperateBF->pData[1]);
		}
#endif
		free(OperateBF);
	}
	return;
}


//分配新的Data
//提高编写效率
static inline ErrVal __AllocData(pchar* Data, pchar* psRPt, usize IntLen, usize FltLen)
{
	*Data = (pchar)malloc(sizeof(char)*(IntLen + (FltLen ? FltLen + 2 : 1)));
	if (!*Data)
		return ERR_BADALLOC;
	*psRPt = *Data + IntLen;
	**psRPt = '\0';											//写入结束符
	return SUCCESS;
}

//重新设置 写入位置(数字进行保存时的起始位置)
static inline ErrVal __ResetDataPoint(pchar * psRPt, pchar * psInt, usize iLInt, usize iAInt, pchar * psFlt, usize iAFlt)
{
	*psInt = *psRPt - (iLInt < iAInt ? iLInt : iAInt);
	if (iAFlt)
		*psFlt = *psRPt + 1;
	return SUCCESS;
}

//为副本Data重新分配内存,此,需要传入新的大小
//分配后新的Data和旧的Data的参数不一致,请务必在副本Data升级为主Data之时释放原主Data
//此函数只有开启异常安全时才可以使用
//未测试
static ErrVal _ReSizeBF_s(struct BFDetail* OperateBF, usize newIntLen, usize newFltLen)
{
	uint8_t CopyIndex = !OperateBF->iCDI;
	ErrVal retVal;
	if (OperateBF->pData[CopyIndex])
		free(OperateBF->pData[CopyIndex]);			//如果已经有数据,则进行内存释放
	retVal = __AllocData(&OperateBF->pData[CopyIndex], &OperateBF->psRPt[CopyIndex], newIntLen, newFltLen);
	if (retVal)
		return retVal;
	return SUCCESS;
}

//为BF重新更改大小,只有一块内存,必须重新分配内存后进行复制
//该函数只对一个Data进行修改
//未仔细测试
static ErrVal _ReSizeBF(struct BFDetail* OperateBF, usize newIntLen, usize newFltLen)
{
	pchar Tmp;
	pchar Tmp_RPt;
	ErrVal retVal = __AllocData(&Tmp, &Tmp_RPt, newIntLen, newFltLen);
	if (retVal)
	{
		return retVal;
	}

	//保存新的Data的已分配长度
	OperateBF->iAFlt = newFltLen;
	OperateBF->iAInt = newIntLen;


	OperateBF->psInt[0] = _StrCpyB(Tmp, Tmp_RPt, OperateBF->psInt[0], OperateBF->psRPt[0]);
	OperateBF->iLInt = Tmp_RPt - OperateBF->psInt[0];

	if (newFltLen)
	{
		OperateBF->psFlt[0] = Tmp_RPt + 1;
		if (OperateBF->iLFlt)
			OperateBF->iLFlt = _StrCpy(OperateBF->psFlt[0], Tmp_RPt + newFltLen, OperateBF->psFlt[0], OperateBF->psFlt[0] + OperateBF->iLFlt) - OperateBF->psFlt[0];
		else
			OperateBF->psFlt[0][0] = '\0';
	}

	free(OperateBF->pData[0]);			//pData中的数据已经复制到新的Data中(Tmp),进行释放内存,以免内存泄露
	OperateBF->psRPt[0] = Tmp_RPt;
	OperateBF->pData[0] = Tmp;
	return SUCCESS;
}

//为副本Data分配内存(与当前已有的缓冲区的大小一致,只是多申请一块内存,采用BF内的已有参数来初始化Data)
//当副本Data为NULL时,此函数才执行,否则将跳过函数
//未测试
static ErrVal _AllocNewData(struct BFDetail* OperateBF)
{
	uint8_t CopyIndex = !OperateBF->iCDI;
	ErrVal retVal;
	if (OperateBF->pData[CopyIndex])
		return SUCCESS;										//已经分配内存,不需要进行分配,请先释放那部分内存
	retVal = __AllocData(&OperateBF->pData[CopyIndex], &OperateBF->psRPt[CopyIndex], OperateBF->iAInt, OperateBF->iAFlt);
	if (retVal)
		return retVal;
	__ResetDataPoint(&OperateBF->psRPt[CopyIndex], &OperateBF->psInt[CopyIndex], OperateBF->iLInt, OperateBF->iAInt, &OperateBF->psFlt[CopyIndex], OperateBF->iAFlt);
	return SUCCESS;
}

//从Source的最后一个元素起,复制数据到Dest(从后往前复制,如果Dest内存不足,则截断Source的内存低位)
//SourceEnd必须包含结束符
//此函数适用于整数部分的复制
static char* _StrCpyB(const char * DestStart, char *DestEnd, const char * SourceStart, const char * SourceEnd)
{
	ptrdiff_t DestSize = DestEnd - DestStart, SourceSize = SourceEnd - SourceStart;
	if (DestSize < SourceSize)
	{
		//Dest的空间不足
		while (DestStart <= DestEnd)
			*DestEnd-- = *SourceEnd--;
	}
	else
	{
		//Source的内容较少
		while (SourceStart <= SourceEnd)
			*DestEnd-- = *SourceEnd--;
	}
	return ++DestEnd;
}

//返回值为写入的最后一个元素的位置
static char* _StrCpy(char * DestStart, const char *DestEnd, const char *SourceStart, const char* SourceEnd)
{
	ptrdiff_t DestSize = DestEnd - DestStart, SourceSize = SourceEnd - SourceStart;
	if (DestSize < SourceSize)
	{
		//Dest的空间不足
		while (DestStart <= DestEnd)
			*DestStart++ = *SourceStart++;
	}
	else
	{
		//Source的内容较少
		while (SourceStart <= SourceEnd)
			*DestStart++ = *SourceStart++;
	}
	return --DestStart;
}


void test(struct BFDetail* BF)
{
	_ReSizeBF(BF, 100, 100);
	_ReSizeBF(BF, 1000, 1);
}