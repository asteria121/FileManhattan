#pragma once
#include <Windows.h>
#include <random>
#include "RandEngine.h"

#define MAX_OVERWRITE 35

enum RemoveAlgorithm
{
	ONEPASS,
	THREEPASS,
	SEVENPASS,
	THIRTYFIVEPASS,
	NORMALDELETE
};

enum ErasePassMethod
{
	RANDOM,
	ONEBYTE,
	THREEBYTE
};

typedef struct
{
	ErasePassMethod Method;
	BYTE OneByte;
	BYTE ThreeByte[3];
} ErasePass;

void GetPassArray(ErasePass* passes, int* passCounts, RemoveAlgorithm mode);
void OnePass(ErasePass* passes, int* count);
void DoDE(ErasePass* passes, int* count);
void DoDECE(ErasePass* passes, int* count);
void Gutmann(ErasePass* passes, int* count);