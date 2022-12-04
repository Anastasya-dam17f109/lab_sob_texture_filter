#pragma once
#include "MU16Data.h"
typedef void(*tdTextureFiltr_Mean_V8)(MU16Data& ar_cmmIn, MFData& ar_cmmOut, double pseudo_min, double kfct);
extern tdTextureFiltr_Mean_V8 g_afunTextureFiltr_Mean_V8[10];
