#define _CRT_SECURE_NO_WARNINGS 
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include "BigFigure.h"

#define MIN(a,b) ((a<b)?(a):(b))
#define MAX(a,b) ((a>b)?(a):(b))

#define _IntFLtAdd(result,OperandA,OperandB) _IntPartAdd(result,OperandA,OperandB,_FltPartAdd(result,OperandA,OperandB))
//#define _IntFLtAdd(result,OperandA,OperandB) _IntPartAdd(result,OperandA,OperandB,0)
typedef uint32_t bit32;									//bit32Ϊ32λ���ڴ��,����struct��

#if BF_BUFF_USE					//�������쳣��ȫ��֤ʱ
#define BF_BUFF_SIZE 2			//����˫����,�ܹ�ȷ���쳣��ȫ,������ʡ�ڴ�
#pragma message("�쳣��ȫ(˫����)����")
#else 
#define BF_BUFF_SIZE 1			//����������,����ȷ���쳣��ȫ,����ʡ�ڴ�
#pragma message("�쳣��ȫ(˫����)�ر�")
#endif

typedef char* pchar;
struct BFDetail
{
	bit32 bTemp : 1;									//��¼�������Ƿ���һ����ʱ����(ֻ����C++����)
	bit32 bMinus : 1;									//��¼�������Ƿ��и���(�и���Ϊtrue)
	bit32 iCDI : 1;										//CurrentDataIndex(��ǰ����ʹ�õĻ������������±�)
	bit32 iReferCount : 16;								//��ָ������ü���(ֻ����C++����)
	usize iAInt;										//Ϊ�������ַ�����ڴ�Ĵ�С
	usize iAFlt;										//ΪС�����ַ�����ڴ�Ĵ�С
	usize iLInt;										//�������ֵ�ʵ�ʳ���
	usize iLFlt;										//�������ֵ�ʵ�ʳ���
	pchar psInt[BF_BUFF_SIZE];							//ָ�������������λ����ָ��
	pchar psRPt[BF_BUFF_SIZE];							//pData���б�ʾ�������ַ����Ľ�����(����С�����λ��)
	pchar psFlt[BF_BUFF_SIZE];							//ָ��С���������λ����ָ��
	pchar pData[BF_BUFF_SIZE];							//��¼��������ʹ�õ����ݵĻ��������׵�ַ
};

//���غ�������
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


//��������(��������BF)
struct BFDetail* CreateBF(usize intLen, usize FloatLen)
{
	struct BFDetail* BF;
	ErrVal retVal;
	if (!intLen)										//��ֹ�������ֳ���Ϊ0,�������ֵĳ�������ӦΪ1
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

	__ResetDataPoint(&BF->psRPt[0], &BF->psInt[0], BF->iLInt, BF->iAInt, &BF->psFlt[0], BF->iAFlt);	//����д��λ��
	if (FloatLen)
		*BF->psFlt[0] = 0;									//��ʼ��С����������Ϊ��
	*BF->psInt[0] = '0';									//��ʼ��������������Ϊ0
	return BF;
}

//BF������,���ڻ��ղ���Ҫ�õ���BF���ڴ�
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

//���BF�Ƿ�Ϸ�
//����ֵ����:
//1		OperateBFΪNULL
//2		pDataΪNULL;
//3		�ڴ�������(��ǰ����ʹ�õ�Data)
//4		�ڴ�������(���õ�Data)
//5		�������ֵĳ��ȴ��ڷ���ĳ���
//6		С�����ֵĳ��ȴ��ڷ���ĳ���
//7		С����λ�ô���(��ǰ����ʹ�õ�Data)
//8		С����λ�ô���(���õ�Data)
//9		С����λ�ô���(��ǰ����ʹ�õ�Data)
//10	С����λ�ô���(���õ�Data)
//0		��BF��ȫ�Ϸ�
int CheckBF(const struct BFDetail * OperateBF)
{
	size_t MemS;
	if (!OperateBF)
		return 1;

	if (!OperateBF->pData[OperateBF->iCDI])
		return 2;
	MemS = (size_t)(OperateBF->iAFlt ? OperateBF->iAFlt + 2 : 1) + OperateBF->iAInt;
#if _WIN32	//Win32����
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

//���뺯��

//�˺�����������������ת��
//�������б�֤������׼ȷ��,��׼ȷ�Ĳ����ᵼ�²���Ԥ�ϵĺ��
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
	//�������Ϊ��,����з����ڴ�

	return _toBF1(OperateBF, StringHead, String, HasMinus);
}

//�˺���������������ת��,�а�ȫ��У��,�������û���������ݵ�ת��
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

//�˺���ֵ�����ں�С�����С����ת��,���Ҫת������,��ʹ��toBF1
//�˺��������а�ȫ��,������벻��ȷ����ֵ,�����������Ԥ�ϵĽ��
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
	StringFltHead = String;		//�ҵ���һ��С��λ���ڵ�λ��
	String += strlen(String);		//�ҵ��ַ����Ľ�β
	return _toBF2(OperateBF, StringIntHead, StringFltHead, String, HasMinus);
}

//������������������С����ת��,
//���������а�ȫ��,������ת���û��������ֵ�ַ���
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
		StringFltHead = NULL;				//��¼����Ϊ��������
	}
	if (*String)
		return ERR_ILLEGALNUMSTRING;		//���ǺϷ�����ֵ�ַ���

#if !BF_IN_RESERVEZERO
	//ȥ��ĩβ��0
	String--;
	while (*String == '0')
		String--;
	String++;
#endif
	if (!StringFltHead)
	{
		//���С����û������,���϶�Ϊ����(�����һλΪ������ʱ)
		return _toBF1(OperateBF, StringIntHead, String, HasMinus);
	}
	else if (StringFltHead == String)
	{
		//���С����û������,���϶�Ϊ����(�����һλΪС����ʱ)
		return _toBF1(OperateBF, StringIntHead, String - 1, HasMinus);
	}
	else
	{
		//�����С����,��С�����������,���϶�ΪС��
		return _toBF2(OperateBF, StringIntHead, StringFltHead, String, HasMinus);
	}
}

ErrVal BFAdd(struct BFDetail * ResultBF, const struct BFDetail * OperandA, const struct BFDetail *OperandB)
{
	int Compare;
	//ErrVal retVal;

	if (!OperandA->bMinus && !OperandB->bMinus)
	{
		//����������
		return _BFAdd(ResultBF, 0, OperandA, OperandB);
	}
	else if (!OperandA->bMinus && OperandB->bMinus)
	{
		Compare = _BFCmp(OperandA, OperandB);
		//�����Ӹ���
		if (Compare)
		{

		}
	}
	else if (OperandA->bMinus && !OperandB->bMinus)
	{
		Compare = _BFCmp(OperandA, OperandB);
		//����������
	}
	else
	{
		//�����Ӹ���
		return _BFAdd(ResultBF, 1, OperandA, OperandB);

	}
	return ERR_SUCCESS;
}

//����ص���BF�Ĵ�С(�ɱ����С,������ֹ���ݶ�ʧ)
ErrVal ReSizeBF(struct BFDetail* OperateBF, usize newIntLen, usize newFltLen)
{
	return (_ReSizeBF(OperateBF, newIntLen, newFltLen));
}

//���԰�ȫ����СOperateBF,����µĴ�С���װ��ԭ����,�����ʾ����
ErrVal ReSizeBF_s(struct BFDetail* OperateBF, usize newIntLen, usize newFltLen)
{
	if (OperateBF->iLFlt > newFltLen || OperateBF->iLInt > newIntLen)
		return ERR_MAYLOSSACCURACY;
	return (_ReSizeBF(OperateBF, newIntLen, newFltLen));
}

//�ú�����BF�е���ֵ�����ָ���Ļ�������
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

//��ȡ��Ŵ�BF�Ļ���������С�ߴ�Ҫ��
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


/**********************************************************���غ���**************************************************************************/
//�ӷ�����(ͬ���������)
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
//�ӷ�����(��������)
//����ֵΪ�������ֵ��׵�ַ
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
//��������(ͬ������)
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
//��������(B��A���Ĳ���)
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

//�����µ�Data
static inline ErrVal __AllocData(pchar * Data, pchar * psRPt, usize IntLen, usize FltLen)
{
	*Data = (pchar)malloc(sizeof(char)*(IntLen + (FltLen ? FltLen + 2 : 1)));
	if (!*Data)
		return ERR_BADALLOC;
	*psRPt = *Data + IntLen;
	**psRPt = '\0';											//д�������
	return ERR_SUCCESS;
}

//�������� д��λ��(���ֽ��б���ʱ����ʼλ��)
static inline ErrVal __ResetDataPoint(pchar * psRPt, pchar * psInt, usize iLInt, usize iAInt, pchar * psFlt, usize iAFlt)
{
	*psInt = *psRPt - (iLInt < iAInt ? iLInt : iAInt);
	if (iAFlt)
		*psFlt = *psRPt + 1;
	return ERR_SUCCESS;
}

//��Source�����һ��Ԫ����,�������ݵ�Dest(�Ӻ���ǰ����,���Dest�ڴ治��,��ض�Source���ڴ��λ)
//�˺����������������ֵĸ���
static inline char* _StrCpyB(char * DestStart, char *DestEnd, const char * SourceStart, const char * SourceEnd)
{
	ptrdiff_t SourceSize = SourceEnd - SourceStart, DestSize = DestEnd - DestStart, offset = SourceSize - DestSize;
	char * retVal;
	if (offset >= 0)
	{
		//Dest�Ŀռ䲻��
		retVal = DestStart;
		strncpy(retVal, SourceStart + offset, DestSize);
	}
	else
	{
		//Dest�Ŀռ����,ƫ��д��λ��
		retVal = DestStart - offset;
		strncpy(retVal, SourceStart, SourceSize);
	}

	//*DestEnd = 0;
	return retVal;
}
//����ֵΪд������һ��Ԫ�ص�λ��
//�˺���������С�����ֵĸ���
static inline char* _StrCpy(char * DestStart, usize DestSize, const char * SourceStart, usize SourceSize)
{
	usize RealSize = MIN(DestSize, SourceSize);
	strncpy(DestStart, SourceStart, RealSize);
	DestStart[RealSize] = 0;
	return DestStart + RealSize - 1;
}

//����������ֵĴ�С,�������,���Զ����з���(������ԭ����)
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
		//Data�ڴ治��
		if (OperateBF->pData[OperateIndex])
			free(OperateBF->pData[OperateIndex]);
		//Data�ڴ治��,�����·���һ���㹻����ڴ�
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
		//Data�ڴ��㹻,�����ڴ�δ����,���Խ��з����ڴ�(ֻ���쳣��ȫʱ)
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
//���������С�����ֵĴ�С,�������,���Զ����з���(������ԭ����)
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
		//Data�ڴ治��
		if (OperateBF->pData[OperateIndex])
			free(OperateBF->pData[OperateIndex]);
		//Data�ڴ治��,�����·���һ���㹻����ڴ�
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
		//Data�ڴ��㹻,�����ڴ�δ����,���Խ��з����ڴ�(ֻ���쳣��ȫʱ)
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

//�������ַ���д��BF��
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

//��С���ַ���д��BF��
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

//�޷��żӷ�����
static inline ErrVal _BFAdd(struct BFDetail * ResultBF, int iMinus, const struct BFDetail * OperandA, const struct BFDetail * OperandB)
{
	ErrVal retVal = 0;
	usize newIntSize, newFltSize;
	newIntSize = MAX(OperandA->iLInt, OperandB->iLInt) + 1;
	newFltSize = MAX(OperandA->iLFlt, OperandB->iLFlt);
	if (newIntSize > ResultBF->iAInt)
	{
		//Int��Ҫ�ı�
		if (newFltSize < ResultBF->iAFlt)
			newFltSize = ResultBF->iAFlt;						//Flt����Ҫ�ı�
		retVal = _ReSizeBF(ResultBF, newIntSize, newFltSize);	//�����ܿ��ٴ�Ŀռ�
	}
	else
	{
		//Int����Ҫ�ı�
		if (newFltSize > ResultBF->iAFlt)
		{
			//Flt��Ҫ�ı�
			newIntSize = ResultBF->iAInt;
			retVal = _ReSizeBF(ResultBF, newIntSize, newFltSize);	//�����ܿ��ٴ�Ŀռ�
		}
		else {
			//Int,Flt������Ҫ�ı�
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

//�޷��ż�������
static inline ErrVal _BFSub(struct BFDetail * ResultBF, int iMinus, const struct BFDetail * OperandA, const struct BFDetail * OperandB)
{

}

//�Ƚ�����BF�ľ���ֵ�Ĵ�С
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
				return 0;//���A,B��Ϊ'\0'
			if (!*StringA&&*StringB)
			{
				//���AΪ'\0',BΪ'0'
				while (*StringB == '0')
					StringB++;
				if (*StringB)
					return -1;			//A<B
				else
					return 0;			//A==B
			}
			if (!*StringB&&*StringA)
			{
				//���BΪ'\0',AΪ'0'
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

//С�����ֵļӷ����㺯��(�޷���)
//�����а�ȫ��
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

//С�����ֵļ������㺯��(�޷���)
//�����а�ȫ��
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

//�������ֵļӷ����㺯��(�޷���)
//���ô˺���֮ǰ,Ӧȷ���ڴ����
//�����а�ȫ��
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

//�������ֵļ������㺯��(�޷���)
//���ô˺���֮ǰ,Ӧȷ���ڴ����
//�����а�ȫ��
static ErrVal _IntPartSub(struct BFDetail * ResultBF, const struct BFDetail* OperateBFA, const struct BFDetail* OperateBFB, int carry)
{

}

//ΪBF���·����ڴ�
//�������쳣��ȫʱ,�·�����Data���ܺ;ɵ�Data������һ��,���������Data����Ϊ��Data֮ǰ�ͷ�ԭ��Data,
//���ر��쳣��ȫʱ,�˺��������������չData�ý��еĲ���,��ʱֻ��һ��Data�����޸�
//δ��ϸ����
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
	//��������
	StringDataHead = OperateBF->pData[0];
	StringDataRPt = OperateBF->psRPt[0];
#endif

	retVal = __AllocData(&OperateBF->pData[OperateIndex], &OperateBF->psRPt[OperateIndex], IntSizeRequest, FltSizeRequest);
	if (retVal)
	{
#if !BF_BUFF_USE
		OperateBF->pData[0] = StringDataHead;		//�ڴ������ִ���,�ָ�֮ǰ������
#endif
		return retVal;
	}
	OperateBF->iAInt = IntSizeRequest;
	OperateBF->iAFlt = FltSizeRequest;
	//�Ѿ�������ڴ�,�����ڴ濽��,���µĲ������׳��쳣

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

//Ϊ����Data�����ڴ�(�뵱ǰ���еĻ������Ĵ�Сһ��,ֻ�Ƕ�����һ���ڴ�,����BF�ڵ����в�������ʼ��Data)
//������DataΪNULLʱ,�˺�����ִ��,������������
//����Ϊδ�����ڴ�ĸ���Data�����ڴ�
static ErrVal _AllocNewData(struct BFDetail* OperateBF)
{
	uint8_t CopyIndex = !OperateBF->iCDI;
	ErrVal retVal;
	if (OperateBF->pData[CopyIndex])
		return ERR_SUCCESS;										//�Ѿ������ڴ�,����Ҫ���з���,�����ͷ��ǲ����ڴ�
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