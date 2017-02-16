#define _CRT_SECURE_NO_WARNINGS 
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include "BigFigure.h"

#define MIN(a,b) ((a<b)?(a):(b))
#define MAX(a,b) ((a>b)?(a):(b))

#define _IntFLtAdd(result,OperandA,OperandB) _IntPartAdd(result,OperandA,OperandB,_FltPartAdd(result,OperandA,OperandB))
//#define _IntFLtAdd(result,OperandA,OperandB) _IntPartAdd(result,OperandA,OperandB,0)
typedef uint32_t bit32;									//bit32为32位的内存块,用于struct中

#if BF_BUFF_USE					//当开启异常安全保证时
#define BF_BUFF_SIZE 2			//开启双缓冲,能够确保异常安全,但不节省内存
#pragma message("异常安全(双缓冲)开启")
#else 
#define BF_BUFF_SIZE 1			//开启单缓冲,不能确保异常安全,但节省内存
#pragma message("异常安全(双缓冲)关闭")
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
static inline int __DoAdd(char * StringTailR, const char * StringHeadA, const char * StringTailA, const char * StringTailB, int carry);
static inline char * __DoAdd2(char* StringHeadR, char * StringTailR, const char * StringHead, const char * StringTail, int carry);
static inline ErrVal __AllocData(pchar* Data, pchar* psRPt, usize IntLen, usize FltLen);
static inline ErrVal __ResetDataPoint(pchar * psRPt, pchar * psInt, usize iLInt, usize iAInt, pchar * psFlt, usize iAFlt);


static inline char* _StrCpyB(char * DestStart, char *DestEnd, const char * SourceStart, const char * SourceEnd);
static inline char* _StrCpy(char * DestStart, usize DestSize, const char *SourceStart, usize SourceSize);
static inline ErrVal _CheckIntSize(struct BFDetail* OperateBF, usize IntSizeRequest);
static inline ErrVal _CheckIntFltSize(struct BFDetail* OperateBF, usize IntSizeRequest, usize FltSizeRequest);
static inline ErrVal _toBF1(struct BFDetail * OperateBF, const char * StringHead, const char* StringTail, short HasMinus);
static inline ErrVal _toBF2(struct BFDetail * OperateBF, const char * StringIntHead, const char * StringFltHead, const char* StringTail, short HasMinus);
static inline ErrVal _BFAdd(struct BFDetail * ResultBF, int iMinus, const struct BFDetail * OperandA, const struct BFDetail *OperandB);


static int _BFCmp(const struct BFDetail * OperandA, const struct BFDetail * OperandB);
static int _FltPartAdd(struct BFDetail * ResultBF, const struct BFDetail* OperateBFA, const struct BFDetail* OperateBFB);
static ErrVal _IntPartAdd(struct BFDetail * ResultBF, const struct BFDetail* OperateBFA, const struct BFDetail* OperateBFB, int carry);
static ErrVal _ReSizeBF(struct BFDetail* OperateBF, usize IntSizeRequest, usize FltSizeRequest);
static ErrVal _AllocNewData(struct BFDetail* OperateBF);


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

//检查BF是否合法
//返回值含义:
//1		OperateBF为NULL
//2		pData为NULL;
//3		内存分配错误(当前正在使用的Data)
//4		内存分配错误(备用的Data)
//5		整数部分的长度大于分配的长度
//6		小数部分的长度大于分配的长度
//7		小数点位置错误(当前正在使用的Data)
//8		小数点位置错误(备用的Data)
//9		小数首位置错误(当前正在使用的Data)
//10	小数首位置错误(备用的Data)
//0		此BF完全合法
int CheckBF(const struct BFDetail * OperateBF)
{
	size_t MemS;
	if (!OperateBF)
		return 1;

	if (!OperateBF->pData[OperateBF->iCDI])
		return 2;
	MemS = (size_t)(OperateBF->iAFlt ? OperateBF->iAFlt + 2 : 1) + OperateBF->iAInt;
#if _WIN32	//Win32独有
	if (_msize(OperateBF->pData[OperateBF->iCDI]) != MemS)
		return 3;
#if BF_BUFF_USE
	if (OperateBF->pData[!OperateBF->iCDI] && _msize(OperateBF->pData[OperateBF->iCDI]) != MemS)
		return 4;
#endif
#endif
	if (OperateBF->iAInt < OperateBF->iLInt)
		return 5;
	if (OperateBF->iAFlt < OperateBF->iLFlt)
		return 6;

	if (OperateBF->psRPt[OperateBF->iCDI] != OperateBF->pData[OperateBF->iCDI] + OperateBF->iAInt)
		return 7;
	if (OperateBF->iAFlt&&OperateBF->psFlt[OperateBF->iCDI] != OperateBF->psRPt[OperateBF->iCDI] + 1)
		return 9;

#if BF_BUFF_USE
	if (OperateBF->psRPt[!OperateBF->iCDI] != OperateBF->pData[!OperateBF->iCDI] + OperateBF->iAInt)
		return 8;
	if (OperateBF->iAFlt&&OperateBF->psFlt[!OperateBF->iCDI] != OperateBF->psRPt[!OperateBF->iCDI] + 1)
		return 10;
#endif
	return 0;
}

//输入函数

//此函数仅适用于整数的转换
//必须自行保证参数的准确性,不准确的参数会导致不可预料的后果
ErrVal toBF1(struct BFDetail * OperateBF, const char* String)
{
	int HasMinus = 0;
	const char *StringHead;

	if (String[0] == '-')
	{
		HasMinus = 1;
		StringHead = ++String;
	}
	else
		StringHead = String;
	String += strlen(String);
	//如果发现为空,则进行分配内存

	return _toBF1(OperateBF, StringHead, String, HasMinus);
}

//此函数仅用于整数的转换,有安全性校验,可用于用户输入的数据的转换
ErrVal toBF1_s(struct BFDetail * OperateBF, const char * String)
{
	short HasMinus = 0;
	const char * StringHead;

	if (*String == '-')
	{
		HasMinus = 1;
		while (*(++String) == '0');
	}
	else
		while (*String == '0')
			String++;
	StringHead = String;

	while (*String)
		if (*String > '9' || *String < '0')
			break;
		else
			String++;
	if (*String != 0 || String == StringHead)
		return ERR_ILLEGALNUMSTRING;

	return _toBF1(OperateBF, StringHead, String, HasMinus);
}

//此函数值适用于含小数点的小数的转换,如果要转换整数,请使用toBF1
//此函数不具有安全性,如果传入不正确的数值,则会引发不可预料的结果
ErrVal toBF2(struct BFDetail * OperateBF, const char* String)
{
	int HasMinus = 0;
	const char *StringIntHead, *StringFltHead;

	if (String[0] == '-')
	{
		HasMinus = 1;
		StringIntHead = ++String;
	}
	else
		StringIntHead = String;
	while (*String++ != '.');
	StringFltHead = String;		//找到第一个小数位所在的位置
	String += strlen(String);		//找到字符串的结尾
	return _toBF2(OperateBF, StringIntHead, StringFltHead, String, HasMinus);
}

//本函数适用于整数和小数的转换,
//本函数具有安全性,可用于转换用户输入的数值字符串
ErrVal toBF2_s(struct BFDetail * OperateBF, const char * String)
{
	int HasMinus = 0;
	const char *StringIntHead, *StringFltHead;

	if (*String == '-')
	{
		HasMinus = 1;
		while (*(++String) == '0');
	}
	else
		while (*String == '0') String++;
	StringIntHead = String;

	while (*String != '.')
	{
		if (*String < '0' || *String>'9')
			break;
		else
			String++;
	}
	if (*String)
	{
		StringFltHead = ++String;
		while (*String != 0)
		{
			if (*String < '0' || *String>'9')
				break;
			else
				String++;
		}
	}
	else
	{
		StringFltHead = NULL;				//记录该数为整数类型
	}
	if (*String)
		return ERR_ILLEGALNUMSTRING;		//不是合法的数值字符串

#if !BF_IN_RESERVEZERO
	//去除末尾的0
	String--;
	while (*String == '0')
		String--;
	String++;
#endif
	if (!StringFltHead)
	{
		//如果小数点没有数字,则认定为整数(当最后一位为结束符时)
		return _toBF1(OperateBF, StringIntHead, String, HasMinus);
	}
	else if (StringFltHead == String)
	{
		//如果小数点没有数字,则认定为整数(当最后一位为小数点时)
		return _toBF1(OperateBF, StringIntHead, String - 1, HasMinus);
	}
	else
	{
		//如果有小数点,且小数点后有数字,则认定为小数
		return _toBF2(OperateBF, StringIntHead, StringFltHead, String, HasMinus);
	}
}

ErrVal BFAdd(struct BFDetail * ResultBF, const struct BFDetail * OperandA, const struct BFDetail *OperandB)
{
	int Compare;
	//ErrVal retVal;

	if (!OperandA->bMinus && !OperandB->bMinus)
	{
		//正数加正数
		return _BFAdd(ResultBF, 0, OperandA, OperandB);
	}
	else if (!OperandA->bMinus && OperandB->bMinus)
	{
		Compare = _BFCmp(OperandA, OperandB);
		//正数加负数
		if (Compare)
		{

		}
	}
	else if (OperandA->bMinus && !OperandB->bMinus)
	{
		Compare = _BFCmp(OperandA, OperandB);
		//负数加正数
	}
	else
	{
		//负数加负数
		return _BFAdd(ResultBF, 1, OperandA, OperandB);

	}
	return ERR_SUCCESS;
}

//任意地调整BF的大小(可变大或变小,不会阻止数据丢失)
ErrVal ReSizeBF(struct BFDetail* OperateBF, usize newIntLen, usize newFltLen)
{
	return (_ReSizeBF(OperateBF, newIntLen, newFltLen));
}

//可以安全得缩小OperateBF,如果新的大小如果装入原数据,则会提示错误
ErrVal ReSizeBF_s(struct BFDetail* OperateBF, usize newIntLen, usize newFltLen)
{
	if (OperateBF->iLFlt > newFltLen || OperateBF->iLInt > newIntLen)
		return ERR_MAYLOSSACCURACY;
	return (_ReSizeBF(OperateBF, newIntLen, newFltLen));
}

//该函数将BF中的数值输出到指定的缓冲区中
char* toString(const struct BFDetail * OperateBF, char * Buffer)
{
	char *Head = Buffer;
	if (OperateBF->bMinus)
		*Buffer++ = '-';
	strncpy(Buffer, OperateBF->psInt[OperateBF->iCDI], OperateBF->iLInt);
	Buffer += OperateBF->iLInt;
	if (OperateBF->iLFlt)
	{
		*Buffer++ = '.';
		strncpy(Buffer, OperateBF->psFlt[OperateBF->iCDI], OperateBF->iLFlt);
		Buffer[OperateBF->iLFlt] = 0;
	}
	else
		*Buffer = 0;
	return Head;
}

char* toString_s(const struct BFDetail * OperateBF, char *Buffer, size_t BufferSize)
{
	char *Head = Buffer;
	if (OperateBF->bMinus && BufferSize > 1U)
	{
		*Buffer++ = '-';
		BufferSize--;
	}
	if (BufferSize > (size_t)OperateBF->iLInt)
	{
		strncpy(Buffer, OperateBF->psInt[OperateBF->iCDI], OperateBF->iLInt);
		Buffer += OperateBF->iLInt;
		BufferSize -= OperateBF->iLInt;
	}
	else
	{
		strncpy(Buffer, OperateBF->psInt[OperateBF->iCDI], BufferSize - 1);
		Buffer[BufferSize - 1] = 0;
		return Head;
	}
	if (OperateBF->iLFlt)
	{
		if (BufferSize > (size_t)(OperateBF->iLFlt + 1))
		{
			*Buffer++ = '.';
			strncpy(Buffer, OperateBF->psFlt[OperateBF->iCDI], OperateBF->iLFlt);
			Buffer[OperateBF->iLFlt] = 0;
		}
		else {
			*Buffer++ = '.';
			BufferSize -= 2;
			strncpy(Buffer, OperateBF->psFlt[OperateBF->iCDI], BufferSize);
			Buffer[BufferSize] = 0;
			return Head;
		}
	}
	else
		*Buffer = 0;
	return Head;
}

//获取存放此BF的缓冲区的最小尺寸要求
size_t GetBitCount(const struct BFDetail *OperateBF)
{
	return (size_t)OperateBF->iLInt + (OperateBF->iLFlt ? OperateBF->iLFlt + 2U : 1U) + (OperateBF->bMinus ? 1U : 0U);
}


int BFCmp(const struct BFDetail * OperandA, const struct BFDetail * OperandB)
{
	if (!OperandA->bMinus && !OperandB->bMinus)
		return _BFCmp(OperandA, OperandB);
	else if (!OperandA->bMinus && OperandB->bMinus)
		return 3;
	else if (OperandA->bMinus && !OperandB->bMinus)
		return -3;
	else
		return -_BFCmp(OperandA, OperandB);
}


/**********************************************************本地函数**************************************************************************/
//加法运算(同长部分相加)
static inline int __DoAdd(char * StringTailR, const char * StringHeadA, const char * StringTailA, const char * StringTailB, int carry)
{
	register int Val;
	while (StringHeadA <= StringTailA)
	{
		Val = (int)*StringTailA-- + (int)*StringTailB-- + carry;
		if (Val > 105)			//'0'+'9'=105
		{
			carry = 1;
			Val -= 58;			//'9'+1=58
		}
		else
		{
			carry = 0;
			Val -= '0';
		}
		*StringTailR-- = (char)Val;
	}
	return carry;
}
//加法运算(超出部分)
//返回值为整数部分的首地址
static inline char * __DoAdd2(char* StringHeadR, char * StringTailR, const char * StringHead, const char * StringTail, int carry)
{
	while (carry&&*StringHead <= *StringTail)
	{
		*StringTailR = *StringHead-- + 1;
		if (*StringTailR > '9')
			*StringTailR -= 10;
		else
			carry = 0;
		StringTailR--;
	}
	if (carry)
	{
		if (StringHead != StringTail)
			StringTailR = _StrCpyB(StringHeadR, StringTailR, StringHead, StringTail);
		else
			StringTailR++;
	}
	else {
		*StringTailR = '1';
	}
	return StringTailR;
}
//减法运算(同长部分)
static inline int __DoSub(char * StringTailR, const char * StringHeadA, const char * StringTailA, const char * StringTailB, int borrow)
{
	register int Val;

	while (StringHeadA <= StringTailA)
	{
		Val = (int)(*StringTailA--) - (int)(*StringTailB--) - borrow;
		if (Val >= 0)
		{
			borrow = 0;
			Val += '0';
		}
		else
		{
			borrow = 1;
			Val += 58;		//58='9'+1
		}
		*StringTailR-- = (char)Val;
	}
	return borrow;
}
//减法运算(B比A长的部分)
static inline int __DoSub2(char * StringTailR, const char *StringHeadB, const char * StringTailB, int borrow)
{
	register int Val;

	while (StringHeadB <= StringTailB)
	{
		Val = 106 - *StringTailB-- - borrow;		//106='9'+1+'0'
		if (Val >= 58)
		{
			borrow = 0;
			Val -= 10;
		}
		else
			borrow = 1;
		*StringTailR-- = (char)Val;
	}
	return borrow;
}

//分配新的Data
static inline ErrVal __AllocData(pchar * Data, pchar * psRPt, usize IntLen, usize FltLen)
{
	*Data = (pchar)malloc(sizeof(char)*(IntLen + (FltLen ? FltLen + 2 : 1)));
	if (!*Data)
		return ERR_BADALLOC;
	*psRPt = *Data + IntLen;
	**psRPt = '\0';											//写入结束符
	return ERR_SUCCESS;
}

//重新设置 写入位置(数字进行保存时的起始位置)
static inline ErrVal __ResetDataPoint(pchar * psRPt, pchar * psInt, usize iLInt, usize iAInt, pchar * psFlt, usize iAFlt)
{
	*psInt = *psRPt - (iLInt < iAInt ? iLInt : iAInt);
	if (iAFlt)
		*psFlt = *psRPt + 1;
	return ERR_SUCCESS;
}

//从Source的最后一个元素起,复制数据到Dest(从后往前复制,如果Dest内存不足,则截断Source的内存低位)
//此函数适用于整数部分的复制
static inline char* _StrCpyB(char * DestStart, char *DestEnd, const char * SourceStart, const char * SourceEnd)
{
	ptrdiff_t SourceSize = SourceEnd - SourceStart, DestSize = DestEnd - DestStart, offset = SourceSize - DestSize;
	char * retVal;
	if (offset >= 0)
	{
		//Dest的空间不足
		retVal = DestStart;
		strncpy(retVal, SourceStart + offset, DestSize);
	}
	else
	{
		//Dest的空间充足,偏移写入位置
		retVal = DestStart - offset;
		strncpy(retVal, SourceStart, SourceSize);
	}

	//*DestEnd = 0;
	return retVal;
}
//返回值为写入的最后一个元素的位置
//此函数适用于小数部分的复制
static inline char* _StrCpy(char * DestStart, usize DestSize, const char * SourceStart, usize SourceSize)
{
	usize RealSize = MIN(DestSize, SourceSize);
	strncpy(DestStart, SourceStart, RealSize);
	DestStart[RealSize] = 0;
	return DestStart + RealSize - 1;
}

//检查整数部分的大小,如果不足,则自动进行分配(不考虑原数据)
static inline ErrVal _CheckIntSize(struct BFDetail * OperateBF, usize IntSizeRequest)
{
	ErrVal retVal;
#if BF_BUFF_USE
	uint8_t OperateIndex = !OperateBF->iCDI;
#else
	uint8_t OperateIndex = 0;
#endif
	if (IntSizeRequest > OperateBF->iAInt)
	{
		//Data内存不足
		if (OperateBF->pData[OperateIndex])
			free(OperateBF->pData[OperateIndex]);
		//Data内存不足,将重新分配一块足够大的内存
		retVal = __AllocData(&OperateBF->pData[OperateIndex], &OperateBF->psRPt[OperateIndex], IntSizeRequest, OperateBF->iAFlt);
		if (retVal)
			return retVal;
		OperateBF->iAInt = IntSizeRequest;
#if BF_BUFF_USE
		OperateIndex = OperateBF->iCDI;
		if (OperateBF->pData[OperateIndex])
		{
			free(OperateBF->pData[OperateIndex]);
			OperateBF->pData[OperateIndex] = (pchar)0;
		}
#endif
	}
#if BF_BUFF_USE
	else
	{
		//Data内存足够,但是内存未分配,所以进行分配内存(只在异常安全时)
		if (!OperateBF->pData[OperateIndex])
		{
			retVal = __AllocData(&OperateBF->pData[OperateIndex], &OperateBF->psRPt[OperateIndex], OperateBF->iAInt, OperateBF->iAFlt);
			if (retVal)
				return retVal;
		}
	}
#endif
	return ERR_SUCCESS;
}
//检查整数和小数部分的大小,如果不足,则自动进行分配(不考虑原数据)
static inline ErrVal _CheckIntFltSize(struct BFDetail * OperateBF, usize IntSizeRequest, usize FltSizeRequest)
{
	ErrVal retVal;
#if BF_BUFF_USE
	uint8_t OperateIndex = !OperateBF->iCDI;
#else 
	uint8_t OperateIndex = 0;
	pchar StringSource = OperateBF->pData[0];
#endif
	if (IntSizeRequest > OperateBF->iAInt || FltSizeRequest > OperateBF->iAFlt)
	{
		//Data内存不足
		if (OperateBF->pData[OperateIndex])
			free(OperateBF->pData[OperateIndex]);
		//Data内存不足,将重新分配一块足够大的内存
		retVal = __AllocData(&OperateBF->pData[OperateIndex], &OperateBF->psRPt[OperateIndex], (IntSizeRequest = MAX(IntSizeRequest, OperateBF->iAInt)), (FltSizeRequest = MAX(FltSizeRequest, OperateBF->iAFlt)));
		if (retVal)
		{
#if !BF_BUFF_USE
			OperateBF->pData[OperateIndex] = StringSource;
#endif
			return retVal;
		}
		OperateBF->iAInt = IntSizeRequest;
		OperateBF->iAFlt = FltSizeRequest;
		if (FltSizeRequest)
			OperateBF->psFlt[OperateIndex] = OperateBF->psRPt[OperateIndex] + 1;
#if BF_BUFF_USE
		OperateIndex = OperateBF->iCDI;
		if (OperateBF->pData[OperateIndex])
		{
			free(OperateBF->pData[OperateIndex]);
			OperateBF->pData[OperateIndex] = (pchar)0;
		}
#endif
	}
#if BF_BUFF_USE
	else
	{
		//Data内存足够,但是内存未分配,所以进行分配内存(只在异常安全时)
		if (!OperateBF->pData[OperateIndex])
		{
			retVal = __AllocData(&OperateBF->pData[OperateIndex], &OperateBF->psRPt[OperateIndex], OperateBF->iAInt, OperateBF->iAFlt);
			if (retVal)
				return retVal;
		}
		if (OperateBF->iAFlt)
			OperateBF->psFlt[OperateIndex] = OperateBF->psRPt[OperateIndex] + 1;
	}
#endif
	return ERR_SUCCESS;
}

//将整数字符串写入BF中
static inline ErrVal _toBF1(struct BFDetail * OperateBF, const char * StringHead, const char * StringTail, short HasMinus)
{
	ErrVal retVal;
	short CopyIndex;

	retVal = _CheckIntSize(OperateBF, (usize)(StringTail - StringHead));
	if (retVal)
		return retVal;


#if BF_BUFF_USE
	CopyIndex = !OperateBF->iCDI;
#else
	CopyIndex = OperateBF->iCDI;
#endif
	OperateBF->bMinus = HasMinus;
	OperateBF->psInt[CopyIndex] = _StrCpyB(OperateBF->pData[CopyIndex], OperateBF->psRPt[CopyIndex], StringHead, StringTail);
	OperateBF->iLInt = (usize)(OperateBF->psRPt[CopyIndex] - OperateBF->psInt[CopyIndex]);
#if BF_BUFF_USE
	OperateBF->iCDI = CopyIndex;
#endif
	return ERR_SUCCESS;
}

//将小数字符串写入BF中
static inline ErrVal _toBF2(struct BFDetail * OperateBF, const char * StringIntHead, const char * StringFltHead, const char * StringTail, short HasMinus)
{
	ErrVal retVal;
	short CopyIndex;
	retVal = _CheckIntFltSize(OperateBF, (usize)(StringFltHead - StringIntHead), (usize)(StringTail - StringFltHead));
	if (retVal)
		return retVal;

#if BF_BUFF_USE
	CopyIndex = !OperateBF->iCDI;
#else
	CopyIndex = OperateBF->iCDI;
#endif

	OperateBF->bMinus = HasMinus;

	OperateBF->psInt[CopyIndex] = _StrCpyB(OperateBF->pData[CopyIndex], OperateBF->psRPt[CopyIndex], StringIntHead, StringFltHead - 1);
	OperateBF->iLInt = (usize)(OperateBF->psRPt[CopyIndex] - OperateBF->psInt[CopyIndex]);

	OperateBF->iLFlt = (usize)(_StrCpy(OperateBF->psFlt[CopyIndex], OperateBF->iAFlt, StringFltHead, (usize)(StringTail - StringFltHead)) - OperateBF->psRPt[CopyIndex]);

#if BF_BUFF_USE
	OperateBF->iCDI = CopyIndex;
#endif
	return ERR_SUCCESS;
}

//无符号加法核心
static inline ErrVal _BFAdd(struct BFDetail * ResultBF, int iMinus, const struct BFDetail * OperandA, const struct BFDetail * OperandB)
{
	ErrVal retVal = 0;
	usize newIntSize, newFltSize;
	newIntSize = MAX(OperandA->iLInt, OperandB->iLInt) + 1;
	newFltSize = MAX(OperandA->iLFlt, OperandB->iLFlt);
	if (newIntSize > ResultBF->iAInt)
	{
		//Int需要改变
		if (newFltSize < ResultBF->iAFlt)
			newFltSize = ResultBF->iAFlt;						//Flt不需要改变
		retVal = _ReSizeBF(ResultBF, newIntSize, newFltSize);	//尽可能开辟大的空间
	}
	else
	{
		//Int不需要改变
		if (newFltSize > ResultBF->iAFlt)
		{
			//Flt需要改变
			newIntSize = ResultBF->iAInt;
			retVal = _ReSizeBF(ResultBF, newIntSize, newFltSize);	//尽可能开辟大的空间
		}
		else {
			//Int,Flt都不需要改变
			newIntSize = ResultBF->iAInt;
			newFltSize = ResultBF->iAFlt;
#if BF_BUFF_USE
			if (!ResultBF->pData[!ResultBF->iCDI])
			{
				retVal = _AllocNewData(ResultBF);
			}
#endif
		}

	}

	if (retVal)
		return retVal;
	retVal = _IntFLtAdd(ResultBF, OperandA, OperandB);
	if (retVal)
	{
#if BF_BUFF_USE
		if (ResultBF->iAInt > newIntSize || ResultBF->iAFlt > newFltSize)
		{
			free(ResultBF->pData[!ResultBF->iCDI]);
			ResultBF->pData[!ResultBF->iCDI] = NULL;
			return retVal;
		}
#endif
	}
	else
	{
		ResultBF->iAFlt = newFltSize;
		ResultBF->iAInt = newIntSize;
		ResultBF->bMinus = 0;
#if BF_BUFF_USE
		ResultBF->iCDI = !ResultBF->iCDI;
#endif
	}
	return ERR_SUCCESS;
}

//无符号减法核心
static inline ErrVal _BFSub(struct BFDetail * ResultBF, int iMinus, const struct BFDetail * OperandA, const struct BFDetail * OperandB)
{

}

//比较两个BF的绝对值的大小
static int _BFCmp(const struct BFDetail * OperandA, const struct BFDetail * OperandB)
{
	char *StringA, *StringB;
	int retVal;
	if (OperandA->iLInt > OperandB->iLInt)
		return 2;			//A>>B
	else if (OperandA->iLInt > OperandB->iLInt)
		return -2;			//A<<B
	else
	{
		retVal = strcmp(OperandA->psInt[OperandA->iCDI], OperandB->psInt[OperandB->iCDI]);
		if (retVal)
			return retVal > 0 ? 1 : -1;
		StringA = OperandA->psFlt[OperandA->iCDI];
		StringB = OperandB->psFlt[OperandB->iCDI];
		while (*StringA == *StringB && !*StringA)
			StringA++, StringB++;
		if (*StringA > *StringB && !*StringB)
			return 1;		//A>B
		else if (*StringA < *StringB && !*StringA)
			return -1;		//A<B
		else
		{
			if (!*StringA && !*StringB)
				return 0;//如果A,B都为'\0'
			if (!*StringA&&*StringB)
			{
				//如果A为'\0',B为'0'
				while (*StringB == '0')
					StringB++;
				if (*StringB)
					return -1;			//A<B
				else
					return 0;			//A==B
			}
			if (!*StringB&&*StringA)
			{
				//如果B为'\0',A为'0'
				while (*StringA == '0')
					StringA++;
				if (*StringA)
					return 1;			//A>B
				else
					return 0;			//A==B
			}
		}
	}
	return 0;
}

//小数部分的加法运算函数(无符号)
//不具有安全性
static int _FltPartAdd(struct BFDetail * ResultBF, const struct BFDetail* OperateBFA, const struct BFDetail* OperateBFB)
{
	uint8_t CopyIndexA;
	uint8_t CopyIndexB;
	uint8_t CopyIndexR;

	CopyIndexA = OperateBFA->iCDI;
	CopyIndexB = OperateBFB->iCDI;
#if BF_BUFF_USE
	CopyIndexR = !ResultBF->iCDI;
#else
	CopyIndexR = 0;
#endif

	usize LenA, LenB;
	register int carry = 0;

	LenA = OperateBFA->iLFlt;
	LenB = OperateBFB->iLFlt;

	if (LenA == LenB)
	{
		ResultBF->psFlt[CopyIndexR][LenA] = 0;
		ResultBF->iLFlt = LenA;
		if (LenA)
		{
			LenA--;
			carry = __DoAdd(ResultBF->psFlt[CopyIndexR] + LenA, OperateBFA->psFlt[CopyIndexA], OperateBFA->psFlt[CopyIndexA] + LenA, OperateBFB->psFlt[CopyIndexB] + LenA, 0);
		}
	}
	else if (LenA > LenB)
	{
		OperateBFA->psFlt[CopyIndexR][LenA] = 0;
		ResultBF->iLFlt = LenA;

		strncpy(ResultBF->psFlt[CopyIndexR] + LenB, OperateBFA->psFlt[CopyIndexR] + LenB, LenA - LenB);
		if (LenB)
		{
			LenB--;
			carry = __DoAdd(ResultBF->psFlt[CopyIndexR] + LenB, OperateBFA->psFlt[CopyIndexR], OperateBFA->psFlt[CopyIndexR] + LenB, OperateBFB->psFlt[CopyIndexR] + LenB, 0);
		}
	}
	else
	{
		//LenA<LenB
		ResultBF->psFlt[CopyIndexR][LenB] = 0;
		ResultBF->iLFlt = LenB;

		strncpy(ResultBF->psFlt[CopyIndexR] + LenA, OperateBFB->psFlt[CopyIndexB] + LenA, LenB - LenA);
		if (LenA)
		{
			LenA--;
			carry = __DoAdd(ResultBF->psFlt[CopyIndexR] + LenA, OperateBFA->psFlt[CopyIndexB], OperateBFA->psFlt[CopyIndexB] + LenA, OperateBFB->psFlt[CopyIndexB] + LenA, 0);
		}

	}
	return carry;
}

//小数部分的减法运算函数(无符号)
//不具有安全性
static int _FltPartSub(struct BFDetail * ResultBF, const struct BFDetail* OperateBFA, const struct BFDetail* OperateBFB)
{
	uint8_t CopyIndexA;
	uint8_t CopyIndexB;
	uint8_t CopyIndexR;

	CopyIndexA = OperateBFA->iCDI;
	CopyIndexB = OperateBFB->iCDI;
#if BF_BUFF_USE
	CopyIndexR = !ResultBF->iCDI;
#else
	CopyIndexR = 0;
#endif

	usize LenA, LenB;
	int borrow = 0;

	LenA = OperateBFA->iLFlt;
	LenB = OperateBFB->iLFlt;

	if (LenA == LenB)
	{
		ResultBF->psFlt[CopyIndexR][LenA] = 0;
		ResultBF->iLFlt = LenA;
		if (LenA)
		{
			LenA--;
			borrow = __DoSub(ResultBF->psFlt[CopyIndexR] + LenA, OperateBFA->psFlt[CopyIndexA], OperateBFA->psFlt[CopyIndexA] + LenA, OperateBFB->psFlt[CopyIndexB] + LenA, 0);
		}

	}
	else if (LenA > LenB)
	{
		ResultBF->psFlt[CopyIndexR][LenA] = 0;
		ResultBF->iLFlt = LenA;

		strncpy(ResultBF->psFlt[CopyIndexR] + LenB, OperateBFA->psFlt[CopyIndexA] + LenB, LenA - LenB);
		if (LenB)
		{
			LenB--;
			borrow = __DoSub(ResultBF->psFlt[CopyIndexR] + LenB, OperateBFA->psFlt[CopyIndexA], OperateBFA->psFlt[CopyIndexA] + LenB, OperateBFB->psFlt[CopyIndexB] + LenB, 0);
		}
	}
	else
	{
		//LenA<LenB
		ResultBF->psFlt[CopyIndexR][LenB] = 0;
		ResultBF->iLFlt = LenB;
		if (LenA)
		{
			LenA--, LenB--;
			borrow = __DoSub2(ResultBF->psFlt[CopyIndexR] + LenB, OperateBFB->psFlt[CopyIndexB] + LenA + 1, OperateBFB->psFlt[CopyIndexB] + LenB, 0);
			borrow = __DoSub(ResultBF->psFlt[CopyIndexR] + LenA, OperateBFA->psFlt[CopyIndexA], OperateBFA->psFlt[CopyIndexA] + LenA, OperateBFB->psFlt[CopyIndexB] + LenA, borrow);
		}
	}
	return borrow;
}

//整数部分的加法运算函数(无符号)
//调用此函数之前,应确保内存充足
//不具有安全性
static ErrVal _IntPartAdd(struct BFDetail * ResultBF, const struct BFDetail* OperateBFA, const struct BFDetail* OperateBFB, int carry)
{
	uint8_t CopyIndexA = OperateBFA->iCDI;
	uint8_t CopyIndexB = OperateBFB->iCDI;
#if BF_BUFF_USE
	uint8_t CopyIndexR = !ResultBF->iCDI;
#else
	uint8_t CopyIndexR = 0;
#endif
	const char
		*StringHeadA = OperateBFA->psInt[CopyIndexA],
		*StringHeadB = OperateBFB->psInt[CopyIndexB],
		*StringTailA = OperateBFA->psRPt[CopyIndexA] - 1,
		*StringTailB = OperateBFB->psRPt[CopyIndexB] - 1;
	char
		*StringHeadR = ResultBF->pData[CopyIndexR],
		*StringTailR = ResultBF->psRPt[CopyIndexR] - 1;

	usize
		LenA = OperateBFA->iLInt,
		LenB = OperateBFB->iLInt;

	if (LenA > LenB)
	{
		carry = __DoAdd(StringTailR, StringHeadA + (LenA - LenB), StringTailA, StringTailB, carry);
		ResultBF->psInt[CopyIndexR] = __DoAdd2(StringHeadR, StringTailR - LenB, StringHeadA, StringTailA - LenB, carry);
	}
	else if (LenA < LenB)
	{
		carry = __DoAdd(StringTailR, StringHeadA, StringTailA, StringTailB, carry);
		ResultBF->psInt[CopyIndexR] = __DoAdd2(StringHeadR, StringTailR - LenA, StringHeadB, StringTailB - LenA, carry);
	}
	else
	{
		//LenA==LenB
		carry = __DoAdd(StringTailR, StringHeadA, StringTailA, StringTailB, carry);
		StringTailR -= LenA;
		if (carry)
			*StringTailR = '1';
		else
			StringTailR++;
		ResultBF->psInt[CopyIndexR] = StringTailR;
	}

	ResultBF->iLInt =(usize)(ResultBF->psRPt[CopyIndexR] - ResultBF->psInt[CopyIndexR]);
	return ERR_SUCCESS;
}

//整数部分的减法运算函数(无符号)
//调用此函数之前,应确保内存充足
//不具有安全性
static ErrVal _IntPartSub(struct BFDetail * ResultBF, const struct BFDetail* OperateBFA, const struct BFDetail* OperateBFB, int carry)
{

}

//为BF重新分配内存
//当开启异常安全时,新分配后的Data可能和旧的Data参数不一致,请务必在主Data升级为主Data之前释放原主Data,
//当关闭异常安全时,此函数会完成所有拓展Data该进行的操作,此时只对一个Data进行修改
//未仔细测试
static inline ErrVal _ReSizeBF(struct BFDetail* OperateBF, usize IntSizeRequest, usize FltSizeRequest)
{
#if BF_BUFF_USE 
	uint8_t OperateIndex = !OperateBF->iCDI, SourceIndex = OperateBF->iCDI;

#else
	uint8_t OperateIndex = 0;
	pchar StringDataHead, StringDataRPt;
#endif
	ErrVal retVal;
	if (!IntSizeRequest)
		return ERR_INTSIZECANBEZERO;

#if !BF_BUFF_USE
	//备份数据
	StringDataHead = OperateBF->pData[0];
	StringDataRPt = OperateBF->psRPt[0];
#endif

	retVal = __AllocData(&OperateBF->pData[OperateIndex], &OperateBF->psRPt[OperateIndex], IntSizeRequest, FltSizeRequest);
	if (retVal)
	{
#if !BF_BUFF_USE
		OperateBF->pData[0] = StringDataHead;		//内存分配出现错误,恢复之前的样子
#endif
		return retVal;
	}
	OperateBF->iAInt = IntSizeRequest;
	OperateBF->iAFlt = FltSizeRequest;
	//已经分配好内存,进行内存拷贝,以下的操作不抛出异常

#if BF_BUFF_USE
	OperateBF->psInt[OperateIndex] = _StrCpyB(OperateBF->pData[OperateIndex], OperateBF->psRPt[OperateIndex], OperateBF->psInt[SourceIndex], OperateBF->psRPt[SourceIndex]);
	OperateBF->iLInt = (usize)(OperateBF->psRPt[OperateIndex] - OperateBF->psInt[OperateIndex]);

	if (FltSizeRequest)
	{
		OperateBF->psFlt[OperateIndex] = OperateBF->psRPt[OperateIndex] + 1;
		if (OperateBF->iLFlt)
			OperateBF->iLFlt = (usize)(_StrCpy(OperateBF->psFlt[OperateIndex], FltSizeRequest, OperateBF->psFlt[SourceIndex], OperateBF->iLFlt) - OperateBF->psFlt[OperateIndex]);
		else
			OperateBF->psFlt[OperateIndex][0] = 0;
	}

	OperateBF->iCDI = !OperateIndex;
	free(OperateBF->pData[OperateIndex]);
	OperateBF->pData[OperateIndex] = (pchar)0;
#else
	OperateBF->psInt[0] = _StrCpyB(OperateBF->pData[0], OperateBF->psRPt[0], StringDataRPt - OperateBF->iLInt, StringDataRPt);
	OperateBF->iLInt = (usize)(OperateBF->psRPt[0] - OperateBF->psInt[0]);
	if (OperateBF->iAFlt)
	{
		OperateBF->psFlt[0] = OperateBF->psRPt[0] + 1;
		if (OperateBF->iLFlt)
			OperateBF->iLFlt = (usize)(_StrCpy(OperateBF->psFlt[0], FltSizeRequest, StringDataRPt + 1, OperateBF->iLFlt));
		else
			OperateBF->psFlt[0][0] = 0;
	}
	if (StringDataHead)
		free(StringDataHead);
#endif
	return ERR_SUCCESS;
}

//为副本Data分配内存(与当前已有的缓冲区的大小一致,只是多申请一块内存,采用BF内的已有参数来初始化Data)
//当副本Data为NULL时,此函数才执行,否则将跳过函数
//用于为未分配内存的副本Data分配内存
static ErrVal _AllocNewData(struct BFDetail* OperateBF)
{
	uint8_t CopyIndex = !OperateBF->iCDI;
	ErrVal retVal;
	if (OperateBF->pData[CopyIndex])
		return ERR_SUCCESS;										//已经分配内存,不需要进行分配,请先释放那部分内存
	retVal = __AllocData(&OperateBF->pData[CopyIndex], &OperateBF->psRPt[CopyIndex], OperateBF->iAInt, OperateBF->iAFlt);
	if (retVal)
		return retVal;
	__ResetDataPoint(&OperateBF->psRPt[CopyIndex], &OperateBF->psInt[CopyIndex], OperateBF->iLInt, OperateBF->iAInt, &OperateBF->psFlt[CopyIndex], OperateBF->iAFlt);
	return ERR_SUCCESS;
}



void test(struct BFDetail* c, struct BFDetail* a, struct BFDetail* b)
{
	_FltPartSub(c, a, b);
	//system("pause");
}