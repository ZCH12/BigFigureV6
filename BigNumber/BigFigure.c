#include <malloc.h>
#include <assert.h>
#define _CRT_SECURE_NO_WARNINGS 
#include <string.h>
#include "BigFigure.h"

#define MIN(a,b) ((a<b)?(a):(b))
#define MAX(a,b) ((a>b)?(a):(b))

#define _IntFLtAdd(result,OperandA,OperandB) _IntPartAdd(result,OperandA,OperandB,_FltPartAdd(result,OperandA,OperandB))

typedef uint32_t bit32;									//bit32Ϊ32λ���ڴ��,����struct��

#if BF_BUFF_USE==1				//�������쳣��ȫ��֤ʱ
#define BF_BUFF_SIZE 2			//����˫����,�ܹ�ȷ���쳣��ȫ,������ʡ�ڴ�
#else 
#define BF_BUFF_SIZE 1			//����������,����ȷ���쳣��ȫ,����ʡ�ڴ�
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
static inline ErrVal __AllocData(pchar* Data, pchar* psRPt, usize IntLen, usize FltLen);
static inline ErrVal __ResetDataPoint(pchar * psRPt, pchar * psInt, usize iLInt, usize iAInt, pchar * psFlt, usize iAFlt);
static inline int __DoAdd(char * StringTailR, const char * StringHeadA, const char * StringTailA, const char * StringTailB, int carry);
static char* _StrCpyB(char * DestStart, char *DestEnd, const char * SourceStart, const char * SourceEnd);
static inline char* _StrCpy(char * DestStart, usize DestSize, const char *SourceStart, usize SourceSize);
static inline ErrVal _CheckIntSize(struct BFDetail* OperateBF, usize IntSizeRequest);
static inline ErrVal _CheckIntFltSize(struct BFDetail* OperateBF, usize IntSizeRequest, usize FltSizeRequest);
static inline ErrVal _toBF1(struct BFDetail * OperateBF, const char * StringHead, const char* StringTail, short HasMinus);
static inline ErrVal _toBF2(struct BFDetail * OperateBF, const char * StringIntHead, const char * StringFltHead, const char* StringTail, short HasMinus);
static ErrVal _IntPartAdd(struct BFDetail * ResultBF, struct BFDetail* OperateBFA, struct BFDetail* OperateBFB, int carry);
static int _FltPartAdd(struct BFDetail * ResultBF, struct BFDetail* OperateBFA, struct BFDetail* OperateBFB);
static int _BFCmp(struct BFDetail * OperandA, struct BFDetail * OperandB);




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

//�ú�����BF�е���ֵ�����ָ���Ļ�������
char* toString(struct BFDetail * OperateBF, char * Buffer)
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

char* toString_s(struct BFDetail * OperateBF, char *Buffer, size_t BufferSize)
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

//��ȡ��Ŵ�BF�Ļ���������С��СҪ��
usize GetBitCount(struct BFDetail *OperateBF)
{
	return OperateBF->iLInt + (OperateBF->iLFlt ? OperateBF->iLFlt + 2 : 1) + (OperateBF->bMinus ? 1 : 0);
}

ErrVal BFAdd(struct BFDetail * ResultBF, struct BFDetail * OperandA, struct BFDetail *OperandB)
{
	int Compare = _BFCmp(OperandA, OperandB);

	if (!OperandA->bMinus && !OperandB->bMinus)
	{
		//����������
		ResultBF->bMinus = 0;
		return _IntFLtAdd(ResultBF, OperandA, OperandB);

	}
	else if (!OperandA->bMinus && OperandB->bMinus)
	{
		//�����Ӹ���
		if (Compare)
		{

		}
	}
	else if (OperandA->bMinus && !OperandB->bMinus)
	{
		//����������
	}
	else
	{
		//�����Ӹ���
		ResultBF->bMinus = 1;
		return _IntFLtAdd(ResultBF, OperandA, OperandB);

	}
}

int BFCmp(struct BFDetail * OperandA, struct BFDetail * OperandB)
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

//�Ƚ�����BF�ľ���ֵ�Ĵ�С
static int _BFCmp(struct BFDetail * OperandA, struct BFDetail * OperandB)
{
	char *StringA, *StringB;
	int retVal;
	if (OperandA->iLInt > OperandB->iLInt)
		return 2;			//A>>B
	else if (OperandA->iLInt > OperandB->iLInt)
		return -2;			//A<<B
	else
	{
		retVal = strcmp(OperandA->psInt, OperandB->psInt);
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
	//û��Ĭ�Ϸ���·��
}



//�������ֵļӷ����㺯��(���Ը���)
//�����а�ȫ��
static ErrVal _IntPartAdd(struct BFDetail * ResultBF, struct BFDetail* OperateBFA, struct BFDetail* OperateBFB, int carry)
{
	short CopyIndexA;
	short CopyIndexB;
	short CopyIndexR;

	CopyIndexA = OperateBFA->iCDI;
	CopyIndexB = OperateBFB->iCDI;
#if BF_BUFF_USE
	CopyIndexR = !ResultBF->iCDI;
#else
	CopyIndexR = ResultBF->iCDI;
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
		LenA = StringTailA - StringHeadA,
		LenB = StringTailB - StringHeadB;

	if (LenA > LenB)
	{
		//����Ҫ����Result���ڴ�,�ڵ��ô˺���֮ǰ,Ӧ�ȷ�����ڴ�
		carry = __DoAdd(StringTailR, StringHeadA + LenA - LenB, StringTailA, StringTailB, carry);
		StringTailA -= LenB;
		StringTailR -= LenB;

		while (carry&&*StringHeadA < *StringTailA)
		{
			*StringTailR = *StringHeadA-- + 1;
			if (*StringTailR > '9')
				*StringTailR -= 10;
			StringTailR--;
		}

		if (StringHeadA != StringTailA)
			StringTailR = _StrCpyB(StringHeadR, StringTailR, StringHeadA, StringTailA);
		else
			StringTailR++;
	}
	else if (LenA < LenB)
	{
		carry = __DoAdd(StringTailR, StringHeadA, StringTailA, StringTailB, carry);
		StringTailB -= LenA;
		StringTailR -= LenA;

		while (carry&&*StringHeadB <= *StringTailB)
		{
			*StringTailR = *StringHeadB-- + 1;
			if (*StringTailR > '9')
				*StringTailR -= 10;
			StringTailR--;
		}
		if (StringHeadB != StringTailB)
			StringTailR = _StrCpyB(StringHeadR, StringTailR, StringHeadB, StringTailB);
		else
			StringTailR++;
	}
	else
	{
		carry = __DoAdd(StringTailR, StringHeadA, StringTailA, StringHeadB, StringTailB, carry);
		StringTailR -= LenA;
	}
	if (carry)
		*(--StringTailR) = '1';

	ResultBF->psInt[CopyIndexR] = StringTailR;
	ResultBF->iLInt = ResultBF->psRPt[CopyIndexR] - StringTailR;
	return ERR_SUCCESS;
}

//С�����ֵļӷ����㺯��(���Ը���)
static int _FltPartAdd(struct BFDetail * ResultBF, struct BFDetail* OperateBFA, struct BFDetail* OperateBFB)
{
	short CopyIndexA;
	short CopyIndexB;
	short CopyIndexR;

	CopyIndexA = OperateBFA->iCDI;
	CopyIndexB = OperateBFB->iCDI;
#if BF_BUFF_USE
	CopyIndexR = !ResultBF->iCDI;
#else
	CopyIndexR = ResultBF->iCDI;
#endif
	const char
		*StringHeadA = OperateBFA->psFlt[CopyIndexA],
		*StringHeadB = OperateBFB->psFlt[CopyIndexB];
	char
		*StringHeadR = ResultBF->psFlt[CopyIndexR];

	usize
		LenA = OperateBFA->iLFlt - 1,
		LenB = OperateBFB->iLFlt - 1;
	int carry = 0;
	if (LenA > LenB)
	{
		carry = __DoAdd(StringHeadR + LenB, StringHeadA, StringHeadA + LenB, StringHeadB += LenB, 0);
		StringHeadA += LenB;
		ResultBF->iLFlt = (usize)(_StrCpy(StringHeadR + LenB + 1, ResultBF->iAFlt - LenB, StringHeadA, LenB - LenA) - ResultBF->psRPt[CopyIndexR]);
	}
	else if (LenA < LenB)
	{
		carry = __DoAdd(StringHeadR + LenA, StringHeadA, StringHeadA + LenA, StringHeadB += LenA + 1, 0);
		StringHeadA += LenA;
		ResultBF->iLFlt = (usize)(_StrCpy(StringHeadR + LenA + 1, ResultBF->iAFlt - LenA, StringHeadB, LenB - LenA) - ResultBF->psRPt[CopyIndexR]);
	}
	else
	{
		carry = __DoAdd(StringHeadR + LenB, StringHeadA, StringHeadA + LenB, StringHeadB += LenB, 0);
		ResultBF->iLFlt = LenB + 1;
		StringHeadR[ResultBF->iLFlt] = 0;
	}
	return carry;
}




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

//�����µ�Data
//��߱�дЧ��
static inline ErrVal __AllocData(pchar* Data, pchar* psRPt, usize IntLen, usize FltLen)
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

//�������ַ���д��BF��
static inline ErrVal _toBF1(struct BFDetail * OperateBF, const char * StringHead, const char* StringTail, short HasMinus)
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
static inline ErrVal _toBF2(struct BFDetail * OperateBF, const char * StringIntHead, const char * StringFltHead, const char* StringTail, short HasMinus)
{
	ErrVal retVal;
	short CopyIndex;
	retVal = _CheckIntFltSize(OperateBF, StringFltHead - StringIntHead, StringTail - StringFltHead);
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

	OperateBF->iLFlt = (usize)(_StrCpy(OperateBF->psFlt[CopyIndex], OperateBF->iAFlt, StringFltHead, StringTail - StringFltHead) - OperateBF->psRPt[CopyIndex]);

#if BF_BUFF_USE
	OperateBF->iCDI = CopyIndex;
#endif
	return ERR_SUCCESS;
}


//����������ֵĴ�С,�������,���Զ����з���
static inline ErrVal _CheckIntSize(struct BFDetail* OperateBF, usize IntSizeRequest)
{
	ErrVal retVal;
	short OperateIndex = !OperateBF->iCDI;
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

//���������С�����ֵĴ�С,�������,���Զ����з���
static inline ErrVal _CheckIntFltSize(struct BFDetail* OperateBF, usize IntSizeRequest, usize FltSizeRequest)
{
	ErrVal retVal;
	short OperateIndex;
#if BF_BUFF_USE
	OperateIndex = !OperateBF->iCDI;
#else 
	OperateIndex = OperateBF->iCDI;
#endif
	if (IntSizeRequest > OperateBF->iAInt || FltSizeRequest > OperateBF->iAFlt)
	{
		//Data�ڴ治��
		if (OperateBF->pData[OperateIndex])
			free(OperateBF->pData[OperateIndex]);
		//Data�ڴ治��,�����·���һ���㹻����ڴ�
		retVal = __AllocData(&OperateBF->pData[OperateIndex], &OperateBF->psRPt[OperateIndex], (IntSizeRequest = MAX(IntSizeRequest, OperateBF->iAInt)), (FltSizeRequest = MAX(FltSizeRequest, OperateBF->iAFlt)));
		if (retVal)
			return retVal;
		OperateBF->iAInt = IntSizeRequest;
		OperateBF->iAFlt = FltSizeRequest;
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
	}
#endif
	return ERR_SUCCESS;
}

//Ϊ����Data���·����ڴ�,��,��Ҫ�����µĴ�С
//������µ�Data�;ɵ�Data�Ĳ�����һ��,������ڸ���Data����Ϊ��Data֮ʱ�ͷ�ԭ��Data
//�˺���ֻ�п����쳣��ȫʱ�ſ���ʹ��
//δ����
static ErrVal _ReSizeBF_s(struct BFDetail* OperateBF, usize newIntLen, usize newFltLen)
{
	uint8_t CopyIndex = !OperateBF->iCDI;
	ErrVal retVal;
	if (OperateBF->pData[CopyIndex])
		free(OperateBF->pData[CopyIndex]);			//����Ѿ�������,������ڴ��ͷ�
	retVal = __AllocData(&OperateBF->pData[CopyIndex], &OperateBF->psRPt[CopyIndex], newIntLen, newFltLen);
	if (retVal)
		return retVal;
	return ERR_SUCCESS;
}

//ΪBF���¸��Ĵ�С,ֻ��һ���ڴ�,�������·����ڴ����и���
//�ú���ֻ��һ��Data�����޸�
//δ��ϸ����
static ErrVal _ReSizeBF(struct BFDetail* OperateBF, usize newIntLen, usize newFltLen)
{
	pchar Tmp;
	pchar Tmp_RPt;
	ErrVal retVal = __AllocData(&Tmp, &Tmp_RPt, newIntLen, newFltLen);
	if (retVal)
		return retVal;

	//�����µ�Data���ѷ��䳤��
	OperateBF->iAFlt = newFltLen;
	OperateBF->iAInt = newIntLen;

	OperateBF->psInt[0] = _StrCpyB(Tmp, Tmp_RPt, OperateBF->psInt[0], OperateBF->psRPt[0]);
	OperateBF->iLInt = (usize)(Tmp_RPt - OperateBF->psInt[0]);

	if (newFltLen)
	{
		if (OperateBF->iLFlt)
		{
			OperateBF->iLFlt = (usize)(_StrCpy(Tmp_RPt + 1, newFltLen, OperateBF->psFlt[0], OperateBF->iLFlt));
			OperateBF->psFlt[0] = Tmp_RPt + 1;
		}
		else
		{
			//OperateBF->iLFlt����Ҫ�ı�
			OperateBF->psFlt[0] = Tmp_RPt + 1;
			OperateBF->psFlt[0][0] = '\0';
		}
	}

	free(OperateBF->pData[0]);			//pData�е������Ѿ����Ƶ��µ�Data��(Tmp),�����ͷ��ڴ�,�����ڴ�й¶
	OperateBF->psRPt[0] = Tmp_RPt;
	OperateBF->pData[0] = Tmp;
	return ERR_SUCCESS;
}

//Ϊ����Data�����ڴ�(�뵱ǰ���еĻ������Ĵ�Сһ��,ֻ�Ƕ�����һ���ڴ�,����BF�ڵ����в�������ʼ��Data)
//������DataΪNULLʱ,�˺�����ִ��,������������
//δ����
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

//��Source�����һ��Ԫ����,�������ݵ�Dest(�Ӻ���ǰ����,���Dest�ڴ治��,��ض�Source���ڴ��λ)
//�˺����������������ֵĸ���
static char* _StrCpyB(char * DestStart, char *DestEnd, const char * SourceStart, const char * SourceEnd)
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
static inline char* _StrCpy(char * DestStart, usize DestSize, const char *SourceStart, usize SourceSize)
{
	usize RealSize = MIN(DestSize, SourceSize);
	strncpy(DestStart, SourceStart, RealSize);
	DestStart[RealSize] = 0;
	return DestStart + RealSize - 1;
}


void test(struct BFDetail* c, struct BFDetail* a, struct BFDetail* b)
{
	_IntPartAdd(c, a, b, _FltPartAdd(c, a, b));
	//system("pause");
}