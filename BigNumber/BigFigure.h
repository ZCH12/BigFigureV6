#pragma once
#ifndef BigFigure_H
#define BigFigure_H

#ifdef __cplusplus
extern "C"
{
#endif
#include <stdint.h>
	//********************����ֵ����**********************
#define ERR_BADALLOC 1
#define SUCCESS 0

	//**********************���뿪��**********************
	//�����Ƿ����쳣��ȫ��֤
#define BF_BUFF_USE 0			//�����Ƿ����쳣��ȫ��֤(��������ڴ���,�ڴ�ʹ��������,��������ʧ�ܶ�����û��Ӱ��)
#define BF_MAXSIZE_BIT 16
	//**********************��ʼ��************************

#if BF_MAXSIZE_BIT==8
	typedef uint8_t usize;									//���屣�泤�����õ�����,��������߿���ֲ��
#elif BF_MAXSIZE_BIT==16
	typedef uint16_t usize;									//���屣�泤�����õ�����,��������߿���ֲ��
#elif BF_MAXSIZE_BIT==32
	typedef uint32_t usize;									//���屣�泤�����õ�����,��������߿���ֲ��
#elif BF_MAXSIZE_BIT==64
	typedef uint64_t usize;									//���屣�泤�����õ�����,��������߿���ֲ��
#else
	static_assert(BF_MAXSIZE_BIT != 8 || BF_MAXSIZE_BIT != 16 ||
		BF_MAXSIZE_BIT != 32 || BF_MAXSIZE_BIT != 64, "BF_MAXSIZE_BIT only be 8,16,32,64");
#endif
	//****************************************************



	//***************************************************************
	struct BFDetail;				//�����ṹ��

	//��������
	struct BFDetail* CreateBF(usize intLen, usize FloatLen);				//��������



	void test(struct BFDetail* BF);

	//****************************************************************
#ifdef __cplusplus
}
#endif
#endif