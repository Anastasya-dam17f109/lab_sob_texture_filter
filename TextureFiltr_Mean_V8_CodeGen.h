#pragma once
#include "MU16Data.h"
#include <xmmintrin.h>
#include <immintrin.h>
#include <emmintrin.h>
#include <pmmintrin.h>
#include <smmintrin.h>
typedef void(*tdTextureFiltr_Mean_V8)(MU16Data& ar_cmmIn, MFData& ar_cmmOut, double pseudo_min, double kfct);
typedef void(*tdTextureFiltr_Mean_V8_omp)(MU16Data& ar_cmmIn, MFData& ar_cmmOut, double pseudo_min, double kfct, int thr_amount);
extern tdTextureFiltr_Mean_V8 g_afunTextureFiltr_Mean_V8[10];
extern tdTextureFiltr_Mean_V8 g_afunTextureFiltr_Mean_V8_sse4[10];
extern tdTextureFiltr_Mean_V8_omp g_afunTextureFiltr_Mean_V8_sse4_omp[10];

