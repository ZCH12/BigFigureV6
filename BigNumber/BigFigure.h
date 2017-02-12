#pragma once
#ifndef BigFigure_H
#define BigFigure_H

#ifdef __cplusplus
extern "C"
{
#endif
#include <stdint.h>
	//********************错误值常量**********************
#define ERR_BADALLOC 1
#define SUCCESS 0

	//**********************编译开关**********************
	//决定是否开启异常安全保证
#define BF_BUFF_USE 0			//决定是否开启异常安全保证(开启后存在代价,内存使用量递增,但若操作失败对数据没有影响)
#define BF_MAXSIZE_BIT 16
	//**********************初始化************************

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



	void test(struct BFDetail* BF);

	//****************************************************************
#ifdef __cplusplus
}
#endif
#endif