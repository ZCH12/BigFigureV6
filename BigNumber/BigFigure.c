#include <malloc.h>
#include <assert.h>
#include <string.h>
#include "BigFigure.h"

#define MIN(a,b) ((a<b)?(a):(b))
#define MAX(a,b) ((a>b)?(a):(b))


typedef uint32_t bit32;									//bit32Ϊ32λ���ڴ��,����struct��
typedef int8_t ErrVal;									//���巵��ֵ����

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
static char* _StrCpyB(const char * DestStart, char *DestEnd, const char * SourceStart, const char * SourceEnd);
static char* _StrCpy(char * DestStart, const char *DestEnd, const char *SourceStart, const char* SourceEnd);

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


//�����µ�Data
//��߱�дЧ��
static inline ErrVal __AllocData(pchar* Data, pchar* psRPt, usize IntLen, usize FltLen)
{
	*Data = (pchar)malloc(sizeof(char)*(IntLen + (FltLen ? FltLen + 2 : 1)));
	if (!*Data)
		return ERR_BADALLOC;
	*psRPt = *Data + IntLen;
	**psRPt = '\0';											//д�������
	return SUCCESS;
}

//�������� д��λ��(���ֽ��б���ʱ����ʼλ��)
static inline ErrVal __ResetDataPoint(pchar * psRPt, pchar * psInt, usize iLInt, usize iAInt, pchar * psFlt, usize iAFlt)
{
	*psInt = *psRPt - (iLInt < iAInt ? iLInt : iAInt);
	if (iAFlt)
		*psFlt = *psRPt + 1;
	return SUCCESS;
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
	return SUCCESS;
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
	{
		return retVal;
	}

	//�����µ�Data���ѷ��䳤��
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

	free(OperateBF->pData[0]);			//pData�е������Ѿ����Ƶ��µ�Data��(Tmp),�����ͷ��ڴ�,�����ڴ�й¶
	OperateBF->psRPt[0] = Tmp_RPt;
	OperateBF->pData[0] = Tmp;
	return SUCCESS;
}

//Ϊ����Data�����ڴ�(�뵱ǰ���еĻ������Ĵ�Сһ��,ֻ�Ƕ�����һ���ڴ�,����BF�ڵ����в�������ʼ��Data)
//������DataΪNULLʱ,�˺�����ִ��,������������
//δ����
static ErrVal _AllocNewData(struct BFDetail* OperateBF)
{
	uint8_t CopyIndex = !OperateBF->iCDI;
	ErrVal retVal;
	if (OperateBF->pData[CopyIndex])
		return SUCCESS;										//�Ѿ������ڴ�,����Ҫ���з���,�����ͷ��ǲ����ڴ�
	retVal = __AllocData(&OperateBF->pData[CopyIndex], &OperateBF->psRPt[CopyIndex], OperateBF->iAInt, OperateBF->iAFlt);
	if (retVal)
		return retVal;
	__ResetDataPoint(&OperateBF->psRPt[CopyIndex], &OperateBF->psInt[CopyIndex], OperateBF->iLInt, OperateBF->iAInt, &OperateBF->psFlt[CopyIndex], OperateBF->iAFlt);
	return SUCCESS;
}

//��Source�����һ��Ԫ����,�������ݵ�Dest(�Ӻ���ǰ����,���Dest�ڴ治��,��ض�Source���ڴ��λ)
//SourceEnd�������������
//�˺����������������ֵĸ���
static char* _StrCpyB(const char * DestStart, char *DestEnd, const char * SourceStart, const char * SourceEnd)
{
	ptrdiff_t DestSize = DestEnd - DestStart, SourceSize = SourceEnd - SourceStart;
	if (DestSize < SourceSize)
	{
		//Dest�Ŀռ䲻��
		while (DestStart <= DestEnd)
			*DestEnd-- = *SourceEnd--;
	}
	else
	{
		//Source�����ݽ���
		while (SourceStart <= SourceEnd)
			*DestEnd-- = *SourceEnd--;
	}
	return ++DestEnd;
}

//����ֵΪд������һ��Ԫ�ص�λ��
static char* _StrCpy(char * DestStart, const char *DestEnd, const char *SourceStart, const char* SourceEnd)
{
	ptrdiff_t DestSize = DestEnd - DestStart, SourceSize = SourceEnd - SourceStart;
	if (DestSize < SourceSize)
	{
		//Dest�Ŀռ䲻��
		while (DestStart <= DestEnd)
			*DestStart++ = *SourceStart++;
	}
	else
	{
		//Source�����ݽ���
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