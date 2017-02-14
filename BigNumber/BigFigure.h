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
#define BF_BUFF_USE						1//决定是否开启异常安全保证(开启后存在代价,内存使用量变多,但操作失败时对原数据没有影响)
#define BF_MAXSIZE_BIT					16
#define BF_IN_RESERVEZERO				0//建议为1,设为0后会自动去除传入的小数的尾部的'0',消耗更多性能,但可能减少不必要的内存分配


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

#define WARNING_TEXT(_replace) #_replace"函数只能用于格式完全正确的字符串,如果你要转换格式不确定的字符串,请改用"\
#_replace"_s函数"


	//****************************************************



	//***************************************************************
	struct BFDetail;				//声明结构体

	//函数声明
	struct BFDetail* CreateBF(usize intLen, usize FloatLen);				//工厂函数
	void DestroyBF(struct BFDetail* OperateBF);								//回收函数

	__declspec(deprecated(WARNING_TEXT(toBF1)))
		ErrVal toBF1(struct BFDetail * OperateBF, const char* String);

	ErrVal toBF2_s(struct BFDetail * OperateBF, const char * String);

	void test(struct BFDetail* BF);

	//****************************************************************
#ifdef __cplusplus
}
#endif
#endif