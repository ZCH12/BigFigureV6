#pragma once
#ifndef BigFigure_H
#define BigFigure_H

#ifdef __cplusplus
extern "C"
{
#endif
#include <stdint.h>
	//********************����ֵ����**********************
#define ERR_BADALLOC					1
#define ERR_ILLEGALNUMSTRING			2
#define ERR_MAYLOSSACCURACY				3
#define ERR_INTSIZECANBEZERO			4
#define ERR_SUCCESS						0

	//**********************���뿪��**********************
#define BF_BUFF_USE						0//�����Ƿ����쳣��ȫ��֤(��������ڴ���,�ڴ�ʹ�������,������ʧ��ʱ��ԭ����û��Ӱ��)
#define BF_MAXSIZE_BIT					16
#define BF_IN_RESERVEZERO				1//����Ϊ1,��Ϊ0����Զ�ȥ�������С����β����'0',���ĸ�������,�����ܼ��ٲ���Ҫ���ڴ����


	//**********************��ʼ��************************
	typedef int8_t ErrVal;									//���巵��ֵ����
#if BF_MAXSIZE_BIT==8
	typedef uint8_t usize;									//���屣�泤�����õ�����,��������߿���ֲ��
#pragma message("������󳤶�Ϊ:2^8")
#elif BF_MAXSIZE_BIT==16
	typedef uint16_t usize;									//���屣�泤�����õ�����,��������߿���ֲ��
#pragma message("������󳤶�Ϊ:2^16")
#elif BF_MAXSIZE_BIT==32
	typedef uint32_t usize;									//���屣�泤�����õ�����,��������߿���ֲ��
#pragma message("������󳤶�Ϊ:2^32")
#elif BF_MAXSIZE_BIT==64
	typedef uint64_t usize;									//���屣�泤�����õ�����,��������߿���ֲ��
#pragma message("������󳤶�Ϊ:2^64")
#else
	static_assert(BF_MAXSIZE_BIT != 8 || BF_MAXSIZE_BIT != 16 ||
		BF_MAXSIZE_BIT != 32 || BF_MAXSIZE_BIT != 64, "BF_MAXSIZE_BIT only be 8,16,32,64");
#endif
 
#if BF_IN_RESERVEZERO
#pragma message("ת������ʱ,�Զ�ȥ��С������'0'")
#else
#pragma message("ת������ʱ,��ȥ��С������'0'")
#endif

	//****************************************************



	//***************************************************************
	struct BFDetail;				//�����ṹ��

	//��������
	struct BFDetail* CreateBF(usize intLen, usize FloatLen);				//��������
	void DestroyBF(struct BFDetail* OperateBF);								//���պ���
	usize GetBitCount(const struct BFDetail *OperateBF);
	int BFCmp(const struct BFDetail * OperandA, const struct BFDetail * OperandB);		//�ȽϺ���
	int CheckBF(const struct BFDetail * OperateBF);

	ErrVal ReSizeBF(struct BFDetail* OperateBF, usize newIntLen, usize newFltLen);
	ErrVal ReSizeBF_s(struct BFDetail* OperateBF, usize newIntLen, usize newFltLen);

	ErrVal BFAdd(struct BFDetail * ResultBF, const struct BFDetail * OperandA, const struct BFDetail *OperandB);



	//����ȫ����,�����ڲ����ض���ȷ�������
	_CRT_INSECURE_DEPRECATE(toBF1_s) 
		ErrVal toBF1(struct BFDetail * OperateBF, const char* String);		//���������ַ�����ת������OperateBF
	_CRT_INSECURE_DEPRECATE(toBF2_s) 
		ErrVal toBF2(struct BFDetail * OperateBF, const char* String);		//����С���ַ�����ת������OperateBF
	_CRT_INSECURE_DEPRECATE(toString_s) 
		char* toString(const struct BFDetail * OperateBF, char * Buffer);			//��OperateBF�е�ֵת��Ϊ�ַ���

	//��ȫ�汾�ĺ���
	ErrVal toBF1_s(struct BFDetail * OperateBF, const char * String);		//���������ַ�����ת������OperateBF,�������ܾ�����
	ErrVal toBF2_s(struct BFDetail * OperateBF, const char * String);		//����С���������ַ�����ת������OperateBF,��С���������ܾ�����
	char* toString_s(const struct BFDetail * OperateBF, char *Buffer, size_t BufferSize);
																			//��OperateBF�е���ֵ��ȫ��д�뵽Buffer��,���Buffer������̫С,�����Դ�����е�����,�����ݻᱻ�ض�



	



	//****************************************************************
#ifdef __cplusplus
}
#endif
#endif