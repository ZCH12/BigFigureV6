#pragma once
#ifndef BigFigure_H
#define BigFigure_H

#ifdef __cplusplus
extern "C"
{
#endif
#include <stdint.h>
	//********************错误值常量**********************
#define ERR_BADALLOC					1
#define ERR_ILLEGALNUMSTRING			2
#define ERR_SUCCESS						0

	//**********************编译开关**********************
#define BF_BUFF_USE						0//决定是否开启异常安全保证(开启后存在代价,内存使用量变多,但操作失败时对原数据没有影响)
#define BF_MAXSIZE_BIT					16
#define BF_IN_RESERVEZERO				1//建议为1,设为0后会自动去除传入的小数的尾部的'0',消耗更多性能,但可能减少不必要的内存分配


	//**********************初始化************************
	typedef int8_t ErrVal;									//定义返回值类型
#if BF_MAXSIZE_BIT==8
	typedef uint8_t usize;									//定义保存长度所用的类型,有利于提高可移植性
#elif BF_MAXSIZE_BIT==16
	typedef uint16_t usize;									//定义保存长度所用的类型,有利于提高可移植性
#elif BF_MAXSIZE_BIT==32
	typedef uint32_t usize;									//定义保存长度所用的类型,有利于提高可移植性
#elif BF_MAXSIZE_BIT==64
	typedef uint64_t usize;									//定义保存长度所用的类型,有利于提高可移植性
#else
	static_assert(BF_MAXSIZE_BIT != 8 || BF_MAXSIZE_BIT != 16 ||
		BF_MAXSIZE_BIT != 32 || BF_MAXSIZE_BIT != 64, "BF_MAXSIZE_BIT only be 8,16,32,64");
#endif


	//****************************************************



	//***************************************************************
	struct BFDetail;				//声明结构体

	//函数声明
	struct BFDetail* CreateBF(usize intLen, usize FloatLen);				//工厂函数
	void DestroyBF(struct BFDetail* OperateBF);								//回收函数
	usize GetBitCount(struct BFDetail *OperateBF);
	int BFCmp(struct BFDetail * OperandA, struct BFDetail * OperandB);		//比较函数

	ErrVal BFAdd(struct BFDetail * ResultBF, struct BFDetail * OperandA, struct BFDetail *OperandB);



	//不安全函数,仅用于参数必定正确的情况下
	_CRT_INSECURE_DEPRECATE(toBF1_s) 
		ErrVal toBF1(struct BFDetail * OperateBF, const char* String);		//接收整数字符串并转换后存进OperateBF
	_CRT_INSECURE_DEPRECATE(toBF2_s) 
		ErrVal toBF2(struct BFDetail * OperateBF, const char* String);		//接收小数字符串并转换后存进OperateBF
	_CRT_INSECURE_DEPRECATE(toString_s) 
		char* toString(struct BFDetail * OperateBF, char * Buffer);			//将OperateBF中的值转换为字符串

	//安全版本的函数
	ErrVal toBF1_s(struct BFDetail * OperateBF, const char * String);		//接收整数字符串并转换后存进OperateBF,非整数拒绝接受
	ErrVal toBF2_s(struct BFDetail * OperateBF, const char * String);		//接收小数或整数字符串并转换后存进OperateBF,非小数或整数拒绝接受
	char* toString_s(struct BFDetail * OperateBF, char *Buffer, size_t BufferSize);
																			//将OperateBF中的数值安全得写入到Buffer中,如果Buffer的容量太小,不足以存放所有的内容,则数据会被截断



	



	//****************************************************************
#ifdef __cplusplus
}
#endif
#endif