#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <cstdlib>
#include <conio.h>
#include <string>
#include <omp.h>

#include "MU16Data.h"

#include "TextureFiltr_Mean_V8_CodeGen.h"
using namespace std;


void TextureFiltr_Mean_V8_3(MU16Data& ar_cmmIn, MFData& ar_cmmOut, double pseudo_min, double kfct)
{
	double Size_obratn = 1.0 / 9;

	// Кэш для гистограмм
	uint16_t* pHistAll = new uint16_t[256 * ar_cmmIn.m_i64W]; // [256] [ar_cmmIn.m_i64W]
	memset(pHistAll, 0, sizeof(uint16_t) * 256 * ar_cmmIn.m_i64W);
	uint16_t* pHist, * pHistAdd, * pHistSub;

	// Кэш для сумм
	uint32_t* puiSum = new uint32_t[ar_cmmIn.m_i64W], * puiSumCurr;
	memset(puiSum, 0, sizeof(uint32_t) * ar_cmmIn.m_i64W);

	// Кэш для яркостей
	uint8_t* pbBrightness = new uint8_t[ar_cmmIn.m_i64W * 3];
	uint8_t* pbBrightnessRow;

	int64_t iRow, iCol, iColMax = ar_cmmIn.m_i64W - 3;
	ar_cmmOut.iCreate(ar_cmmIn.m_i64W, ar_cmmIn.m_i64H, 6);

	// Обнуляем края. Первые строки.
	for (iRow = 0; iRow < 1; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

	int64_t i, iCmin, iCmax, iPos = 0;
	uint32_t u32ValueAdd, u32ValueSub;
	double d;

	// Первые [0, 3 - 1] строки
	pbBrightnessRow = pbBrightness;
	float* pfRowOut = ar_cmmOut.pfGetRow(1) + 1;
	for (iRow = 0; iRow < 3; iRow++, pbBrightnessRow += ar_cmmIn.m_i64W)
	{
		// Обнуляем края строк.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * 1);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - 1), 0, sizeof(float) * 1);

		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);

		// Первые [0, 2*1[ колонки
		for (iCol = 0; iCol < (1 << 1); iCol++)
		{
			iCmin = max(1, static_cast <int>(iCol - 1));
			iCmax = iCol + 1;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				pbBrightnessRow[iCol] = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
				{
					pbBrightnessRow[iCol] = u32ValueAdd = 255;
					pHistAdd = pHistAll + (255 * ar_cmmIn.m_i64W);
				}
				else
				{
					pbBrightnessRow[iCol] = static_cast<uint8_t>(u32ValueAdd);
					pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W);
				}
				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
		}

		// Средние [2*Win_cen, ar_cmmIn.m_i64W - Win_size] колонки
		puiSumCurr = puiSum + 1;
		for (; iCol <= iColMax; iCol++, puiSumCurr++)
		{
			iCmin = iCol - 1;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				pbBrightnessRow[iCol] = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
				{
					pbBrightnessRow[iCol] = u32ValueAdd = 255;
					pHistAdd = pHistAll + (255 * ar_cmmIn.m_i64W + iCmin);
				}
				else
				{
					pbBrightnessRow[iCol] = static_cast<uint8_t>(u32ValueAdd);
					pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W + iCmin);
				}
				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = 0; i < 3; i++)
					puiSumCurr[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
			if ((3 - 1) == iRow)
			{
				pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
				pfRowOut++;
			}
		}

		// Последние [ar_cmmIn.m_i64W - 2*Win_cen, ar_cmmIn.m_i64W - 1] колонки
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - 1;
			iCmax = min(static_cast <int>(ar_cmmIn.m_i64W - 1 - 1), static_cast <int>(iCol + 1));

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				pbBrightnessRow[iCol] = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
				{
					pbBrightnessRow[iCol] = u32ValueAdd = 255;
					pHistAdd = pHistAll + (255 * ar_cmmIn.m_i64W);
				}
				else
				{
					pbBrightnessRow[iCol] = static_cast<uint8_t>(u32ValueAdd);
					pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W);
				}
				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
			if ((3 - 1) == iRow)
			{
				pfRowOut[0] = static_cast<float>(puiSum[iCmin] * Size_obratn);
				pfRowOut++;
			}
		}
	}

	// Последующие строки [3, ar_cmmIn.m_i64H[
	for (iRow = 3; iRow < ar_cmmIn.m_i64H; iRow++)
	{
		// Обнуляем края строк.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * 1);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - 1), 0, sizeof(float) * 1);

		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);
		float* pfRowOut = ar_cmmOut.pfGetRow(iRow - 1) + 1;
		iPos = (iRow - 3) % 3;
		pbBrightnessRow = pbBrightness + (iPos * ar_cmmIn.m_i64W);

		// Первые [0, 2*1[ колонки
		for (iCol = 0; iCol < (1 << 1); iCol++)
		{
			iCmin = max(1, static_cast <int>(iCol - 1));
			iCmax = iCol + 1;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				u32ValueAdd = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
					u32ValueAdd = 255;
			}
			if (u32ValueAdd != (u32ValueSub = pbBrightnessRow[iCol]))
			{
				pHistSub = pHistAll + (u32ValueSub * ar_cmmIn.m_i64W);
				pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W);
				pbBrightnessRow[iCol] = u32ValueAdd;
				uint32_t u32ValueAddSub = u32ValueAdd + u32ValueSub;

				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1)) - u32ValueSub * ((static_cast<uint32_t>(pHistSub[i]--) << 1) - 1);
			}
		}

		// Средние [2*Win_cen, ar_cmmIn.m_i64W - Win_size] колонки
		puiSumCurr = puiSum + 1;
		pHist = pHistAll + (iCol - 1);
		for (; iCol <= iColMax; iCol++, puiSumCurr++, pHist++)
		{
			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				u32ValueAdd = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
					u32ValueAdd = 255;
			}
			if (u32ValueAdd != (u32ValueSub = pbBrightnessRow[iCol]))
			{
				pHistSub = pHist + (u32ValueSub * ar_cmmIn.m_i64W);
				pHistAdd = pHist + (u32ValueAdd * ar_cmmIn.m_i64W);
				pbBrightnessRow[iCol] = u32ValueAdd;
				uint32_t u32ValueAddSub = u32ValueAdd + u32ValueSub;

				// Добавляем яркость в Win_size гистограмм и сумм
#pragma omp simd
				for (i = 0; i < 3; i++)
					puiSumCurr[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1)) - u32ValueSub * ((static_cast<uint32_t>(pHistSub[i]--) << 1) - 1);
			}
			pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
			pfRowOut++;
		}

		// Последние [ar_cmmIn.m_i64W - 2*Win_cen, ar_cmmIn.m_i64W - 1] колонки
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - 1;
			iCmax = min(static_cast <int>(ar_cmmIn.m_i64W - 1 - 1), static_cast <int>(iCol + 1));

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				u32ValueAdd = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
					u32ValueAdd = 255;
			}
			if (u32ValueAdd != (u32ValueSub = pbBrightnessRow[iCol]))
			{
				pHistSub = pHistAll + (u32ValueSub * ar_cmmIn.m_i64W);
				pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W);
				pbBrightnessRow[iCol] = u32ValueAdd;
				uint32_t u32ValueAddSub = u32ValueAdd + u32ValueSub;

				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1)) - u32ValueSub * ((static_cast<uint32_t>(pHistSub[i]--) << 1) - 1);
			}
			pfRowOut[0] = static_cast<float>(puiSum[iCmin] * Size_obratn);
			pfRowOut++;
		}
	}

	// Обнуляем края. Последние строки.
	for (iRow = ar_cmmIn.m_i64H - 1; iRow < ar_cmmOut.m_i64H; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

	if (nullptr != pHistAll)
		delete[] pHistAll;
	if (nullptr != puiSum)
		delete[] puiSum;
	if (nullptr != pbBrightness)
		delete[] pbBrightness;
}


void TextureFiltr_Mean_V8_5(MU16Data& ar_cmmIn, MFData& ar_cmmOut, double pseudo_min, double kfct)
{
	double Size_obratn = 1.0 / 25;

	// Кэш для гистограмм
	uint16_t* pHistAll = new uint16_t[256 * ar_cmmIn.m_i64W]; // [256] [ar_cmmIn.m_i64W]
	memset(pHistAll, 0, sizeof(uint16_t) * 256 * ar_cmmIn.m_i64W);
	uint16_t* pHist, * pHistAdd, * pHistSub;

	// Кэш для сумм
	uint32_t* puiSum = new uint32_t[ar_cmmIn.m_i64W], * puiSumCurr;
	memset(puiSum, 0, sizeof(uint32_t) * ar_cmmIn.m_i64W);

	// Кэш для яркостей
	uint8_t* pbBrightness = new uint8_t[ar_cmmIn.m_i64W * 5];
	uint8_t* pbBrightnessRow;

	int64_t iRow, iCol, iColMax = ar_cmmIn.m_i64W - 5;
	ar_cmmOut.iCreate(ar_cmmIn.m_i64W, ar_cmmIn.m_i64H, 6);

	// Обнуляем края. Первые строки.
	for (iRow = 0; iRow < 2; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

	int64_t i, iCmin, iCmax, iPos = 0;
	uint32_t u32ValueAdd, u32ValueSub;
	double d;

	// Первые [0, 5 - 1] строки
	pbBrightnessRow = pbBrightness;
	float* pfRowOut = ar_cmmOut.pfGetRow(2) + 2;
	for (iRow = 0; iRow < 5; iRow++, pbBrightnessRow += ar_cmmIn.m_i64W)
	{
		// Обнуляем края строк.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * 2);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - 2), 0, sizeof(float) * 2);

		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);

		// Первые [0, 2*2[ колонки
		for (iCol = 0; iCol < (2 << 1); iCol++)
		{
			iCmin = max(2, static_cast <int>(iCol - 2));
			iCmax = iCol + 2;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				pbBrightnessRow[iCol] = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
				{
					pbBrightnessRow[iCol] = u32ValueAdd = 255;
					pHistAdd = pHistAll + (255 * ar_cmmIn.m_i64W);
				}
				else
				{
					pbBrightnessRow[iCol] = static_cast<uint8_t>(u32ValueAdd);
					pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W);
				}
				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
		}

		// Средние [2*Win_cen, ar_cmmIn.m_i64W - Win_size] колонки
		puiSumCurr = puiSum + 2;
		for (; iCol <= iColMax; iCol++, puiSumCurr++)
		{
			iCmin = iCol - 2;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				pbBrightnessRow[iCol] = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
				{
					pbBrightnessRow[iCol] = u32ValueAdd = 255;
					pHistAdd = pHistAll + (255 * ar_cmmIn.m_i64W + iCmin);
				}
				else
				{
					pbBrightnessRow[iCol] = static_cast<uint8_t>(u32ValueAdd);
					pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W + iCmin);
				}
				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = 0; i < 5; i++)
					puiSumCurr[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
			if ((5 - 1) == iRow)
			{
				pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
				pfRowOut++;
			}
		}

		// Последние [ar_cmmIn.m_i64W - 2*Win_cen, ar_cmmIn.m_i64W - 1] колонки
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - 2;
			iCmax = min(static_cast <int>(ar_cmmIn.m_i64W - 2 - 1), static_cast <int>(iCol + 2));

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				pbBrightnessRow[iCol] = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
				{
					pbBrightnessRow[iCol] = u32ValueAdd = 255;
					pHistAdd = pHistAll + (255 * ar_cmmIn.m_i64W);
				}
				else
				{
					pbBrightnessRow[iCol] = static_cast<uint8_t>(u32ValueAdd);
					pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W);
				}
				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
			if ((5 - 1) == iRow)
			{
				pfRowOut[0] = static_cast<float>(puiSum[iCmin] * Size_obratn);
				pfRowOut++;
			}
		}
	}

	// Последующие строки [5, ar_cmmIn.m_i64H[
	for (iRow = 5; iRow < ar_cmmIn.m_i64H; iRow++)
	{
		// Обнуляем края строк.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * 2);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - 2), 0, sizeof(float) * 2);

		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);
		float* pfRowOut = ar_cmmOut.pfGetRow(iRow - 2) + 2;
		iPos = (iRow - 5) % 5;
		pbBrightnessRow = pbBrightness + (iPos * ar_cmmIn.m_i64W);

		// Первые [0, 2*2[ колонки
		for (iCol = 0; iCol < (2 << 1); iCol++)
		{
			iCmin = max(2, static_cast <int>(iCol - 2));
			iCmax = iCol + 2;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				u32ValueAdd = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
					u32ValueAdd = 255;
			}
			if (u32ValueAdd != (u32ValueSub = pbBrightnessRow[iCol]))
			{
				pHistSub = pHistAll + (u32ValueSub * ar_cmmIn.m_i64W);
				pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W);
				pbBrightnessRow[iCol] = u32ValueAdd;
				uint32_t u32ValueAddSub = u32ValueAdd + u32ValueSub;

				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1)) - u32ValueSub * ((static_cast<uint32_t>(pHistSub[i]--) << 1) - 1);
			}
		}

		// Средние [2*Win_cen, ar_cmmIn.m_i64W - Win_size] колонки
		puiSumCurr = puiSum + 2;
		pHist = pHistAll + (iCol - 2);
		for (; iCol <= iColMax; iCol++, puiSumCurr++, pHist++)
		{
			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				u32ValueAdd = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
					u32ValueAdd = 255;
			}
			if (u32ValueAdd != (u32ValueSub = pbBrightnessRow[iCol]))
			{
				pHistSub = pHist + (u32ValueSub * ar_cmmIn.m_i64W);
				pHistAdd = pHist + (u32ValueAdd * ar_cmmIn.m_i64W);
				pbBrightnessRow[iCol] = u32ValueAdd;
				uint32_t u32ValueAddSub = u32ValueAdd + u32ValueSub;

				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = 0; i < 5; i++)
					puiSumCurr[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1)) - u32ValueSub * ((static_cast<uint32_t>(pHistSub[i]--) << 1) - 1);
			}
			pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
			pfRowOut++;
		}

		// Последние [ar_cmmIn.m_i64W - 2*Win_cen, ar_cmmIn.m_i64W - 1] колонки
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - 2;
			iCmax = min(static_cast <int>(ar_cmmIn.m_i64W - 2 - 1), static_cast <int>(iCol + 2));

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				u32ValueAdd = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
					u32ValueAdd = 255;
			}
			if (u32ValueAdd != (u32ValueSub = pbBrightnessRow[iCol]))
			{
				pHistSub = pHistAll + (u32ValueSub * ar_cmmIn.m_i64W);
				pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W);
				pbBrightnessRow[iCol] = u32ValueAdd;
				uint32_t u32ValueAddSub = u32ValueAdd + u32ValueSub;

				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1)) - u32ValueSub * ((static_cast<uint32_t>(pHistSub[i]--) << 1) - 1);
			}
			pfRowOut[0] = static_cast<float>(puiSum[iCmin] * Size_obratn);
			pfRowOut++;
		}
	}

	// Обнуляем края. Последние строки.
	for (iRow = ar_cmmIn.m_i64H - 2; iRow < ar_cmmOut.m_i64H; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

	if (nullptr != pHistAll)
		delete[] pHistAll;
	if (nullptr != puiSum)
		delete[] puiSum;
	if (nullptr != pbBrightness)
		delete[] pbBrightness;
}


void TextureFiltr_Mean_V8_7(MU16Data& ar_cmmIn, MFData& ar_cmmOut, double pseudo_min, double kfct)
{
	double Size_obratn = 1.0 / 49;

	// Кэш для гистограмм
	uint16_t* pHistAll = new uint16_t[256 * ar_cmmIn.m_i64W]; // [256] [ar_cmmIn.m_i64W]
	memset(pHistAll, 0, sizeof(uint16_t) * 256 * ar_cmmIn.m_i64W);
	uint16_t* pHist, * pHistAdd, * pHistSub;

	// Кэш для сумм
	uint32_t* puiSum = new uint32_t[ar_cmmIn.m_i64W], * puiSumCurr;
	memset(puiSum, 0, sizeof(uint32_t) * ar_cmmIn.m_i64W);

	// Кэш для яркостей
	uint8_t* pbBrightness = new uint8_t[ar_cmmIn.m_i64W * 7];
	uint8_t* pbBrightnessRow;

	int64_t iRow, iCol, iColMax = ar_cmmIn.m_i64W - 7;
	ar_cmmOut.iCreate(ar_cmmIn.m_i64W, ar_cmmIn.m_i64H, 6);

	// Обнуляем края. Первые строки.
	for (iRow = 0; iRow < 3; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

	int64_t i, iCmin, iCmax, iPos = 0;
	uint32_t u32ValueAdd, u32ValueSub;
	double d;

	// Первые [0, 7 - 1] строки
	pbBrightnessRow = pbBrightness;
	float* pfRowOut = ar_cmmOut.pfGetRow(3) + 3;
	for (iRow = 0; iRow < 7; iRow++, pbBrightnessRow += ar_cmmIn.m_i64W)
	{
		// Обнуляем края строк.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * 3);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - 3), 0, sizeof(float) * 3);

		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);

		// Первые [0, 2*3[ колонки
		for (iCol = 0; iCol < (3 << 1); iCol++)
		{
			iCmin = max(3, static_cast <int>(iCol - 3));
			iCmax = iCol + 3;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				pbBrightnessRow[iCol] = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
				{
					pbBrightnessRow[iCol] = u32ValueAdd = 255;
					pHistAdd = pHistAll + (255 * ar_cmmIn.m_i64W);
				}
				else
				{
					pbBrightnessRow[iCol] = static_cast<uint8_t>(u32ValueAdd);
					pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W);
				}
				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
		}

		// Средние [2*Win_cen, ar_cmmIn.m_i64W - Win_size] колонки
		puiSumCurr = puiSum + 3;
		for (; iCol <= iColMax; iCol++, puiSumCurr++)
		{
			iCmin = iCol - 3;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				pbBrightnessRow[iCol] = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
				{
					pbBrightnessRow[iCol] = u32ValueAdd = 255;
					pHistAdd = pHistAll + (255 * ar_cmmIn.m_i64W + iCmin);
				}
				else
				{
					pbBrightnessRow[iCol] = static_cast<uint8_t>(u32ValueAdd);
					pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W + iCmin);
				}
				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = 0; i < 7; i++)
					puiSumCurr[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
			if ((7 - 1) == iRow)
			{
				pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
				pfRowOut++;
			}
		}

		// Последние [ar_cmmIn.m_i64W - 2*Win_cen, ar_cmmIn.m_i64W - 1] колонки
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - 3;
			iCmax = min(static_cast <int>(ar_cmmIn.m_i64W - 3 - 1), static_cast <int>(iCol + 3));

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				pbBrightnessRow[iCol] = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
				{
					pbBrightnessRow[iCol] = u32ValueAdd = 255;
					pHistAdd = pHistAll + (255 * ar_cmmIn.m_i64W);
				}
				else
				{
					pbBrightnessRow[iCol] = static_cast<uint8_t>(u32ValueAdd);
					pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W);
				}
				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
			if ((7 - 1) == iRow)
			{
				pfRowOut[0] = static_cast<float>(puiSum[iCmin] * Size_obratn);
				pfRowOut++;
			}
		}
	}

	// Последующие строки [7, ar_cmmIn.m_i64H[
	for (iRow = 7; iRow < ar_cmmIn.m_i64H; iRow++)
	{
		// Обнуляем края строк.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * 3);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - 3), 0, sizeof(float) * 3);

		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);
		float* pfRowOut = ar_cmmOut.pfGetRow(iRow - 3) + 3;
		iPos = (iRow - 7) % 7;
		pbBrightnessRow = pbBrightness + (iPos * ar_cmmIn.m_i64W);

		// Первые [0, 2*3[ колонки
		for (iCol = 0; iCol < (3 << 1); iCol++)
		{
			iCmin = max(3, static_cast <int>(iCol - 3));
			iCmax = iCol + 3;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				u32ValueAdd = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
					u32ValueAdd = 255;
			}
			if (u32ValueAdd != (u32ValueSub = pbBrightnessRow[iCol]))
			{
				pHistSub = pHistAll + (u32ValueSub * ar_cmmIn.m_i64W);
				pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W);
				pbBrightnessRow[iCol] = u32ValueAdd;
				uint32_t u32ValueAddSub = u32ValueAdd + u32ValueSub;

				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1)) - u32ValueSub * ((static_cast<uint32_t>(pHistSub[i]--) << 1) - 1);
			}
		}

		// Средние [2*Win_cen, ar_cmmIn.m_i64W - Win_size] колонки
		puiSumCurr = puiSum + 3;
		pHist = pHistAll + (iCol - 3);
		for (; iCol <= iColMax; iCol++, puiSumCurr++, pHist++)
		{
			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				u32ValueAdd = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
					u32ValueAdd = 255;
			}
			if (u32ValueAdd != (u32ValueSub = pbBrightnessRow[iCol]))
			{
				pHistSub = pHist + (u32ValueSub * ar_cmmIn.m_i64W);
				pHistAdd = pHist + (u32ValueAdd * ar_cmmIn.m_i64W);
				pbBrightnessRow[iCol] = u32ValueAdd;
				uint32_t u32ValueAddSub = u32ValueAdd + u32ValueSub;

				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = 0; i < 7; i++)
					puiSumCurr[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1)) - u32ValueSub * ((static_cast<uint32_t>(pHistSub[i]--) << 1) - 1);
			}
			pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
			pfRowOut++;
		}

		// Последние [ar_cmmIn.m_i64W - 2*Win_cen, ar_cmmIn.m_i64W - 1] колонки
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - 3;
			iCmax = min(static_cast <int>(ar_cmmIn.m_i64W - 3 - 1), static_cast <int>(iCol + 3));

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				u32ValueAdd = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
					u32ValueAdd = 255;
			}
			if (u32ValueAdd != (u32ValueSub = pbBrightnessRow[iCol]))
			{
				pHistSub = pHistAll + (u32ValueSub * ar_cmmIn.m_i64W);
				pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W);
				pbBrightnessRow[iCol] = u32ValueAdd;
				uint32_t u32ValueAddSub = u32ValueAdd + u32ValueSub;

				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1)) - u32ValueSub * ((static_cast<uint32_t>(pHistSub[i]--) << 1) - 1);
			}
			pfRowOut[0] = static_cast<float>(puiSum[iCmin] * Size_obratn);
			pfRowOut++;
		}
	}

	// Обнуляем края. Последние строки.
	for (iRow = ar_cmmIn.m_i64H - 3; iRow < ar_cmmOut.m_i64H; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

	if (nullptr != pHistAll)
		delete[] pHistAll;
	if (nullptr != puiSum)
		delete[] puiSum;
	if (nullptr != pbBrightness)
		delete[] pbBrightness;
}


void TextureFiltr_Mean_V8_9(MU16Data& ar_cmmIn, MFData& ar_cmmOut, double pseudo_min, double kfct)
{
	double Size_obratn = 1.0 / 81;

	// Кэш для гистограмм
	uint16_t* pHistAll = new uint16_t[256 * ar_cmmIn.m_i64W]; // [256] [ar_cmmIn.m_i64W]
	memset(pHistAll, 0, sizeof(uint16_t) * 256 * ar_cmmIn.m_i64W);
	uint16_t* pHist, * pHistAdd, * pHistSub;

	// Кэш для сумм
	uint32_t* puiSum = new uint32_t[ar_cmmIn.m_i64W], * puiSumCurr;
	memset(puiSum, 0, sizeof(uint32_t) * ar_cmmIn.m_i64W);

	// Кэш для яркостей
	uint8_t* pbBrightness = new uint8_t[ar_cmmIn.m_i64W * 9];
	uint8_t* pbBrightnessRow;

	int64_t iRow, iCol, iColMax = ar_cmmIn.m_i64W - 9;
	ar_cmmOut.iCreate(ar_cmmIn.m_i64W, ar_cmmIn.m_i64H, 6);

	// Обнуляем края. Первые строки.
	for (iRow = 0; iRow < 4; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

	int64_t i, iCmin, iCmax, iPos = 0;
	uint32_t u32ValueAdd, u32ValueSub;
	double d;

	// Первые [0, 9 - 1] строки
	pbBrightnessRow = pbBrightness;
	float* pfRowOut = ar_cmmOut.pfGetRow(4) + 4;
	for (iRow = 0; iRow < 9; iRow++, pbBrightnessRow += ar_cmmIn.m_i64W)
	{
		// Обнуляем края строк.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * 4);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - 4), 0, sizeof(float) * 4);

		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);

		// Первые [0, 2*4[ колонки
		for (iCol = 0; iCol < (4 << 1); iCol++)
		{
			iCmin = max(4, static_cast <int>(iCol - 4));
			iCmax = iCol + 4;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				pbBrightnessRow[iCol] = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
				{
					pbBrightnessRow[iCol] = u32ValueAdd = 255;
					pHistAdd = pHistAll + (255 * ar_cmmIn.m_i64W);
				}
				else
				{
					pbBrightnessRow[iCol] = static_cast<uint8_t>(u32ValueAdd);
					pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W);
				}
				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
		}

		// Средние [2*Win_cen, ar_cmmIn.m_i64W - Win_size] колонки
		puiSumCurr = puiSum + 4;
		for (; iCol <= iColMax; iCol++, puiSumCurr++)
		{
			iCmin = iCol - 4;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				pbBrightnessRow[iCol] = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
				{
					pbBrightnessRow[iCol] = u32ValueAdd = 255;
					pHistAdd = pHistAll + (255 * ar_cmmIn.m_i64W + iCmin);
				}
				else
				{
					pbBrightnessRow[iCol] = static_cast<uint8_t>(u32ValueAdd);
					pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W + iCmin);
				}
				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = 0; i < 9; i++)
					puiSumCurr[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
			if ((9 - 1) == iRow)
			{
				pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
				pfRowOut++;
			}
		}

		// Последние [ar_cmmIn.m_i64W - 2*Win_cen, ar_cmmIn.m_i64W - 1] колонки
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - 4;
			iCmax = min(static_cast <int>(ar_cmmIn.m_i64W - 4 - 1), static_cast <int>(iCol + 4));

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				pbBrightnessRow[iCol] = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
				{
					pbBrightnessRow[iCol] = u32ValueAdd = 255;
					pHistAdd = pHistAll + (255 * ar_cmmIn.m_i64W);
				}
				else
				{
					pbBrightnessRow[iCol] = static_cast<uint8_t>(u32ValueAdd);
					pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W);
				}
				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
			if ((9 - 1) == iRow)
			{
				pfRowOut[0] = static_cast<float>(puiSum[iCmin] * Size_obratn);
				pfRowOut++;
			}
		}
	}

	// Последующие строки [9, ar_cmmIn.m_i64H[
	for (iRow = 9; iRow < ar_cmmIn.m_i64H; iRow++)
	{
		// Обнуляем края строк.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * 4);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - 4), 0, sizeof(float) * 4);

		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);
		float* pfRowOut = ar_cmmOut.pfGetRow(iRow - 4) + 4;
		iPos = (iRow - 9) % 9;
		pbBrightnessRow = pbBrightness + (iPos * ar_cmmIn.m_i64W);

		// Первые [0, 2*4[ колонки
		for (iCol = 0; iCol < (4 << 1); iCol++)
		{
			iCmin = max(4, static_cast <int>(iCol - 4));
			iCmax = iCol + 4;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				u32ValueAdd = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
					u32ValueAdd = 255;
			}
			if (u32ValueAdd != (u32ValueSub = pbBrightnessRow[iCol]))
			{
				pHistSub = pHistAll + (u32ValueSub * ar_cmmIn.m_i64W);
				pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W);
				pbBrightnessRow[iCol] = u32ValueAdd;
				uint32_t u32ValueAddSub = u32ValueAdd + u32ValueSub;

				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1)) - u32ValueSub * ((static_cast<uint32_t>(pHistSub[i]--) << 1) - 1);
			}
		}

		// Средние [2*Win_cen, ar_cmmIn.m_i64W - Win_size] колонки
		puiSumCurr = puiSum + 4;
		pHist = pHistAll + (iCol - 4);
		for (; iCol <= iColMax; iCol++, puiSumCurr++, pHist++)
		{
			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				u32ValueAdd = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
					u32ValueAdd = 255;
			}
			if (u32ValueAdd != (u32ValueSub = pbBrightnessRow[iCol]))
			{
				pHistSub = pHist + (u32ValueSub * ar_cmmIn.m_i64W);
				pHistAdd = pHist + (u32ValueAdd * ar_cmmIn.m_i64W);
				pbBrightnessRow[iCol] = u32ValueAdd;
				uint32_t u32ValueAddSub = u32ValueAdd + u32ValueSub;

				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = 0; i < 9; i++)
					puiSumCurr[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1)) - u32ValueSub * ((static_cast<uint32_t>(pHistSub[i]--) << 1) - 1);
			}
			pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
			pfRowOut++;
		}

		// Последние [ar_cmmIn.m_i64W - 2*Win_cen, ar_cmmIn.m_i64W - 1] колонки
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - 4;
			iCmax = min(static_cast <int>(ar_cmmIn.m_i64W - 4 - 1), static_cast <int>(iCol + 4));

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				u32ValueAdd = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
					u32ValueAdd = 255;
			}
			if (u32ValueAdd != (u32ValueSub = pbBrightnessRow[iCol]))
			{
				pHistSub = pHistAll + (u32ValueSub * ar_cmmIn.m_i64W);
				pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W);
				pbBrightnessRow[iCol] = u32ValueAdd;
				uint32_t u32ValueAddSub = u32ValueAdd + u32ValueSub;

				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1)) - u32ValueSub * ((static_cast<uint32_t>(pHistSub[i]--) << 1) - 1);
			}
			pfRowOut[0] = static_cast<float>(puiSum[iCmin] * Size_obratn);
			pfRowOut++;
		}
	}

	// Обнуляем края. Последние строки.
	for (iRow = ar_cmmIn.m_i64H - 4; iRow < ar_cmmOut.m_i64H; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

	if (nullptr != pHistAll)
		delete[] pHistAll;
	if (nullptr != puiSum)
		delete[] puiSum;
	if (nullptr != pbBrightness)
		delete[] pbBrightness;
}


void TextureFiltr_Mean_V8_11(MU16Data& ar_cmmIn, MFData& ar_cmmOut, double pseudo_min, double kfct)
{
	double Size_obratn = 1.0 / 121;

	// Кэш для гистограмм
	uint16_t* pHistAll = new uint16_t[256 * ar_cmmIn.m_i64W]; // [256] [ar_cmmIn.m_i64W]
	memset(pHistAll, 0, sizeof(uint16_t) * 256 * ar_cmmIn.m_i64W);
	uint16_t* pHist, * pHistAdd, * pHistSub;

	// Кэш для сумм
	uint32_t* puiSum = new uint32_t[ar_cmmIn.m_i64W], * puiSumCurr;
	memset(puiSum, 0, sizeof(uint32_t) * ar_cmmIn.m_i64W);

	// Кэш для яркостей
	uint8_t* pbBrightness = new uint8_t[ar_cmmIn.m_i64W * 11];
	uint8_t* pbBrightnessRow;

	int64_t iRow, iCol, iColMax = ar_cmmIn.m_i64W - 11;
	ar_cmmOut.iCreate(ar_cmmIn.m_i64W, ar_cmmIn.m_i64H, 6);

	// Обнуляем края. Первые строки.
	for (iRow = 0; iRow < 5; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

	int64_t i, iCmin, iCmax, iPos = 0;
	uint32_t u32ValueAdd, u32ValueSub;
	double d;

	// Первые [0, 11 - 1] строки
	pbBrightnessRow = pbBrightness;
	float* pfRowOut = ar_cmmOut.pfGetRow(5) + 5;
	for (iRow = 0; iRow < 11; iRow++, pbBrightnessRow += ar_cmmIn.m_i64W)
	{
		// Обнуляем края строк.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * 5);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - 5), 0, sizeof(float) * 5);

		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);

		// Первые [0, 2*5[ колонки
		for (iCol = 0; iCol < (5 << 1); iCol++)
		{
			iCmin = max(5, static_cast <int>(iCol - 5));
			iCmax = iCol + 5;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				pbBrightnessRow[iCol] = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
				{
					pbBrightnessRow[iCol] = u32ValueAdd = 255;
					pHistAdd = pHistAll + (255 * ar_cmmIn.m_i64W);
				}
				else
				{
					pbBrightnessRow[iCol] = static_cast<uint8_t>(u32ValueAdd);
					pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W);
				}
				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
		}

		// Средние [2*Win_cen, ar_cmmIn.m_i64W - Win_size] колонки
		puiSumCurr = puiSum + 5;
		for (; iCol <= iColMax; iCol++, puiSumCurr++)
		{
			iCmin = iCol - 5;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				pbBrightnessRow[iCol] = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
				{
					pbBrightnessRow[iCol] = u32ValueAdd = 255;
					pHistAdd = pHistAll + (255 * ar_cmmIn.m_i64W + iCmin);
				}
				else
				{
					pbBrightnessRow[iCol] = static_cast<uint8_t>(u32ValueAdd);
					pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W + iCmin);
				}
				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = 0; i < 11; i++)
					puiSumCurr[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
			if ((11 - 1) == iRow)
			{
				pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
				pfRowOut++;
			}
		}

		// Последние [ar_cmmIn.m_i64W - 2*Win_cen, ar_cmmIn.m_i64W - 1] колонки
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - 5;
			iCmax = min(static_cast <int>(ar_cmmIn.m_i64W - 5 - 1), static_cast <int>(iCol + 5));

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				pbBrightnessRow[iCol] = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
				{
					pbBrightnessRow[iCol] = u32ValueAdd = 255;
					pHistAdd = pHistAll + (255 * ar_cmmIn.m_i64W);
				}
				else
				{
					pbBrightnessRow[iCol] = static_cast<uint8_t>(u32ValueAdd);
					pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W);
				}
				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
			if ((11 - 1) == iRow)
			{
				pfRowOut[0] = static_cast<float>(puiSum[iCmin] * Size_obratn);
				pfRowOut++;
			}
		}
	}

	// Последующие строки [11, ar_cmmIn.m_i64H[
	for (iRow = 11; iRow < ar_cmmIn.m_i64H; iRow++)
	{
		// Обнуляем края строк.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * 5);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - 5), 0, sizeof(float) * 5);

		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);
		float* pfRowOut = ar_cmmOut.pfGetRow(iRow - 5) + 5;
		iPos = (iRow - 11) % 11;
		pbBrightnessRow = pbBrightness + (iPos * ar_cmmIn.m_i64W);

		// Первые [0, 2*5[ колонки
		for (iCol = 0; iCol < (5 << 1); iCol++)
		{
			iCmin = max(5, static_cast <int>(iCol - 5));
			iCmax = iCol + 5;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				u32ValueAdd = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
					u32ValueAdd = 255;
			}
			if (u32ValueAdd != (u32ValueSub = pbBrightnessRow[iCol]))
			{
				pHistSub = pHistAll + (u32ValueSub * ar_cmmIn.m_i64W);
				pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W);
				pbBrightnessRow[iCol] = u32ValueAdd;
				uint32_t u32ValueAddSub = u32ValueAdd + u32ValueSub;

				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1)) - u32ValueSub * ((static_cast<uint32_t>(pHistSub[i]--) << 1) - 1);
			}
		}

		// Средние [2*Win_cen, ar_cmmIn.m_i64W - Win_size] колонки
		puiSumCurr = puiSum + 5;
		pHist = pHistAll + (iCol - 5);
		for (; iCol <= iColMax; iCol++, puiSumCurr++, pHist++)
		{
			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				u32ValueAdd = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
					u32ValueAdd = 255;
			}
			if (u32ValueAdd != (u32ValueSub = pbBrightnessRow[iCol]))
			{
				pHistSub = pHist + (u32ValueSub * ar_cmmIn.m_i64W);
				pHistAdd = pHist + (u32ValueAdd * ar_cmmIn.m_i64W);
				pbBrightnessRow[iCol] = u32ValueAdd;
				uint32_t u32ValueAddSub = u32ValueAdd + u32ValueSub;

				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = 0; i < 11; i++)
					puiSumCurr[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1)) - u32ValueSub * ((static_cast<uint32_t>(pHistSub[i]--) << 1) - 1);
			}
			pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
			pfRowOut++;
		}

		// Последние [ar_cmmIn.m_i64W - 2*Win_cen, ar_cmmIn.m_i64W - 1] колонки
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - 5;
			iCmax = min(static_cast <int>(ar_cmmIn.m_i64W - 5 - 1), static_cast <int>(iCol + 5));

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				u32ValueAdd = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
					u32ValueAdd = 255;
			}
			if (u32ValueAdd != (u32ValueSub = pbBrightnessRow[iCol]))
			{
				pHistSub = pHistAll + (u32ValueSub * ar_cmmIn.m_i64W);
				pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W);
				pbBrightnessRow[iCol] = u32ValueAdd;
				uint32_t u32ValueAddSub = u32ValueAdd + u32ValueSub;

				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1)) - u32ValueSub * ((static_cast<uint32_t>(pHistSub[i]--) << 1) - 1);
			}
			pfRowOut[0] = static_cast<float>(puiSum[iCmin] * Size_obratn);
			pfRowOut++;
		}
	}

	// Обнуляем края. Последние строки.
	for (iRow = ar_cmmIn.m_i64H - 5; iRow < ar_cmmOut.m_i64H; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

	if (nullptr != pHistAll)
		delete[] pHistAll;
	if (nullptr != puiSum)
		delete[] puiSum;
	if (nullptr != pbBrightness)
		delete[] pbBrightness;
}


void TextureFiltr_Mean_V8_13(MU16Data& ar_cmmIn, MFData& ar_cmmOut, double pseudo_min, double kfct)
{
	double Size_obratn = 1.0 / 169;

	// Кэш для гистограмм
	uint16_t* pHistAll = new uint16_t[256 * ar_cmmIn.m_i64W]; // [256] [ar_cmmIn.m_i64W]
	memset(pHistAll, 0, sizeof(uint16_t) * 256 * ar_cmmIn.m_i64W);
	uint16_t* pHist, * pHistAdd, * pHistSub;

	// Кэш для сумм
	uint32_t* puiSum = new uint32_t[ar_cmmIn.m_i64W], * puiSumCurr;
	memset(puiSum, 0, sizeof(uint32_t) * ar_cmmIn.m_i64W);

	// Кэш для яркостей
	uint8_t* pbBrightness = new uint8_t[ar_cmmIn.m_i64W * 13];
	uint8_t* pbBrightnessRow;

	int64_t iRow, iCol, iColMax = ar_cmmIn.m_i64W - 13;
	ar_cmmOut.iCreate(ar_cmmIn.m_i64W, ar_cmmIn.m_i64H, 6);

	// Обнуляем края. Первые строки.
	for (iRow = 0; iRow < 6; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

	int64_t i, iCmin, iCmax, iPos = 0;
	uint32_t u32ValueAdd, u32ValueSub;
	double d;

	// Первые [0, 13 - 1] строки
	pbBrightnessRow = pbBrightness;
	float* pfRowOut = ar_cmmOut.pfGetRow(6) + 6;
	for (iRow = 0; iRow < 13; iRow++, pbBrightnessRow += ar_cmmIn.m_i64W)
	{
		// Обнуляем края строк.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * 6);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - 6), 0, sizeof(float) * 6);

		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);

		// Первые [0, 2*6[ колонки
		for (iCol = 0; iCol < (6 << 1); iCol++)
		{
			iCmin = max(6, static_cast <int>(iCol - 6));
			iCmax = iCol + 6;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				pbBrightnessRow[iCol] = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
				{
					pbBrightnessRow[iCol] = u32ValueAdd = 255;
					pHistAdd = pHistAll + (255 * ar_cmmIn.m_i64W);
				}
				else
				{
					pbBrightnessRow[iCol] = static_cast<uint8_t>(u32ValueAdd);
					pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W);
				}
				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
		}

		// Средние [2*Win_cen, ar_cmmIn.m_i64W - Win_size] колонки
		puiSumCurr = puiSum + 6;
		for (; iCol <= iColMax; iCol++, puiSumCurr++)
		{
			iCmin = iCol - 6;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				pbBrightnessRow[iCol] = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
				{
					pbBrightnessRow[iCol] = u32ValueAdd = 255;
					pHistAdd = pHistAll + (255 * ar_cmmIn.m_i64W + iCmin);
				}
				else
				{
					pbBrightnessRow[iCol] = static_cast<uint8_t>(u32ValueAdd);
					pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W + iCmin);
				}
				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = 0; i < 13; i++)
					puiSumCurr[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
			if ((13 - 1) == iRow)
			{
				pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
				pfRowOut++;
			}
		}

		// Последние [ar_cmmIn.m_i64W - 2*Win_cen, ar_cmmIn.m_i64W - 1] колонки
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - 6;
			iCmax = min(static_cast <int>(ar_cmmIn.m_i64W - 6 - 1), static_cast <int>(iCol + 6));

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				pbBrightnessRow[iCol] = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
				{
					pbBrightnessRow[iCol] = u32ValueAdd = 255;
					pHistAdd = pHistAll + (255 * ar_cmmIn.m_i64W);
				}
				else
				{
					pbBrightnessRow[iCol] = static_cast<uint8_t>(u32ValueAdd);
					pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W);
				}
				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
			if ((13 - 1) == iRow)
			{
				pfRowOut[0] = static_cast<float>(puiSum[iCmin] * Size_obratn);
				pfRowOut++;
			}
		}
	}

	// Последующие строки [13, ar_cmmIn.m_i64H[
	for (iRow = 13; iRow < ar_cmmIn.m_i64H; iRow++)
	{
		// Обнуляем края строк.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * 6);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - 6), 0, sizeof(float) * 6);

		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);
		float* pfRowOut = ar_cmmOut.pfGetRow(iRow - 6) + 6;
		iPos = (iRow - 13) % 13;
		pbBrightnessRow = pbBrightness + (iPos * ar_cmmIn.m_i64W);

		// Первые [0, 2*6[ колонки
		for (iCol = 0; iCol < (6 << 1); iCol++)
		{
			iCmin = max(6, static_cast <int>(iCol - 6));
			iCmax = iCol + 6;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				u32ValueAdd = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
					u32ValueAdd = 255;
			}
			if (u32ValueAdd != (u32ValueSub = pbBrightnessRow[iCol]))
			{
				pHistSub = pHistAll + (u32ValueSub * ar_cmmIn.m_i64W);
				pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W);
				pbBrightnessRow[iCol] = u32ValueAdd;
				uint32_t u32ValueAddSub = u32ValueAdd + u32ValueSub;

				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1)) - u32ValueSub * ((static_cast<uint32_t>(pHistSub[i]--) << 1) - 1);
			}
		}

		// Средние [2*Win_cen, ar_cmmIn.m_i64W - Win_size] колонки
		puiSumCurr = puiSum + 6;
		pHist = pHistAll + (iCol - 6);
		for (; iCol <= iColMax; iCol++, puiSumCurr++, pHist++)
		{
			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				u32ValueAdd = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
					u32ValueAdd = 255;
			}
			if (u32ValueAdd != (u32ValueSub = pbBrightnessRow[iCol]))
			{
				pHistSub = pHist + (u32ValueSub * ar_cmmIn.m_i64W);
				pHistAdd = pHist + (u32ValueAdd * ar_cmmIn.m_i64W);
				pbBrightnessRow[iCol] = u32ValueAdd;
				uint32_t u32ValueAddSub = u32ValueAdd + u32ValueSub;

				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = 0; i < 13; i++)
					puiSumCurr[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1)) - u32ValueSub * ((static_cast<uint32_t>(pHistSub[i]--) << 1) - 1);
			}
			pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
			pfRowOut++;
		}

		// Последние [ar_cmmIn.m_i64W - 2*Win_cen, ar_cmmIn.m_i64W - 1] колонки
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - 6;
			iCmax = min(static_cast <int>(ar_cmmIn.m_i64W - 6 - 1), static_cast <int>(iCol + 6));

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				u32ValueAdd = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
					u32ValueAdd = 255;
			}
			if (u32ValueAdd != (u32ValueSub = pbBrightnessRow[iCol]))
			{
				pHistSub = pHistAll + (u32ValueSub * ar_cmmIn.m_i64W);
				pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W);
				pbBrightnessRow[iCol] = u32ValueAdd;
				uint32_t u32ValueAddSub = u32ValueAdd + u32ValueSub;

				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1)) - u32ValueSub * ((static_cast<uint32_t>(pHistSub[i]--) << 1) - 1);
			}
			pfRowOut[0] = static_cast<float>(puiSum[iCmin] * Size_obratn);
			pfRowOut++;
		}
	}

	// Обнуляем края. Последние строки.
	for (iRow = ar_cmmIn.m_i64H - 6; iRow < ar_cmmOut.m_i64H; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

	if (nullptr != pHistAll)
		delete[] pHistAll;
	if (nullptr != puiSum)
		delete[] puiSum;
	if (nullptr != pbBrightness)
		delete[] pbBrightness;
}


void TextureFiltr_Mean_V8_15(MU16Data& ar_cmmIn, MFData& ar_cmmOut, double pseudo_min, double kfct)
{
	double Size_obratn = 1.0 / 225;

	// Кэш для гистограмм
	uint16_t* pHistAll = new uint16_t[256 * ar_cmmIn.m_i64W]; // [256] [ar_cmmIn.m_i64W]
	memset(pHistAll, 0, sizeof(uint16_t) * 256 * ar_cmmIn.m_i64W);
	uint16_t* pHist, * pHistAdd, * pHistSub;

	// Кэш для сумм
	uint32_t* puiSum = new uint32_t[ar_cmmIn.m_i64W], * puiSumCurr;
	memset(puiSum, 0, sizeof(uint32_t) * ar_cmmIn.m_i64W);

	// Кэш для яркостей
	uint8_t* pbBrightness = new uint8_t[ar_cmmIn.m_i64W * 15];
	uint8_t* pbBrightnessRow;

	int64_t iRow, iCol, iColMax = ar_cmmIn.m_i64W - 15;
	ar_cmmOut.iCreate(ar_cmmIn.m_i64W, ar_cmmIn.m_i64H, 6);

	// Обнуляем края. Первые строки.
	for (iRow = 0; iRow < 7; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

	int64_t i, iCmin, iCmax, iPos = 0;
	uint32_t u32ValueAdd, u32ValueSub;
	double d;

	// Первые [0, 15 - 1] строки
	pbBrightnessRow = pbBrightness;
	float* pfRowOut = ar_cmmOut.pfGetRow(7) + 7;
	for (iRow = 0; iRow < 15; iRow++, pbBrightnessRow += ar_cmmIn.m_i64W)
	{
		// Обнуляем края строк.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * 7);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - 7), 0, sizeof(float) * 7);

		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);

		// Первые [0, 2*7[ колонки
		for (iCol = 0; iCol < (7 << 1); iCol++)
		{
			iCmin = max(7, static_cast <int>(iCol - 7));
			iCmax = iCol + 7;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				pbBrightnessRow[iCol] = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
				{
					pbBrightnessRow[iCol] = u32ValueAdd = 255;
					pHistAdd = pHistAll + (255 * ar_cmmIn.m_i64W);
				}
				else
				{
					pbBrightnessRow[iCol] = static_cast<uint8_t>(u32ValueAdd);
					pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W);
				}
				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
		}

		// Средние [2*Win_cen, ar_cmmIn.m_i64W - Win_size] колонки
		puiSumCurr = puiSum + 7;
		for (; iCol <= iColMax; iCol++, puiSumCurr++)
		{
			iCmin = iCol - 7;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				pbBrightnessRow[iCol] = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
				{
					pbBrightnessRow[iCol] = u32ValueAdd = 255;
					pHistAdd = pHistAll + (255 * ar_cmmIn.m_i64W + iCmin);
				}
				else
				{
					pbBrightnessRow[iCol] = static_cast<uint8_t>(u32ValueAdd);
					pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W + iCmin);
				}
				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = 0; i < 15; i++)
					puiSumCurr[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
			if ((15 - 1) == iRow)
			{
				pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
				pfRowOut++;
			}
		}

		// Последние [ar_cmmIn.m_i64W - 2*Win_cen, ar_cmmIn.m_i64W - 1] колонки
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - 7;
			iCmax = min(static_cast <int>(ar_cmmIn.m_i64W - 7 - 1), static_cast <int>(iCol + 7));

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				pbBrightnessRow[iCol] = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
				{
					pbBrightnessRow[iCol] = u32ValueAdd = 255;
					pHistAdd = pHistAll + (255 * ar_cmmIn.m_i64W);
				}
				else
				{
					pbBrightnessRow[iCol] = static_cast<uint8_t>(u32ValueAdd);
					pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W);
				}
				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
			if ((15 - 1) == iRow)
			{
				pfRowOut[0] = static_cast<float>(puiSum[iCmin] * Size_obratn);
				pfRowOut++;
			}
		}
	}

	// Последующие строки [15, ar_cmmIn.m_i64H[
	for (iRow = 15; iRow < ar_cmmIn.m_i64H; iRow++)
	{
		// Обнуляем края строк.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * 7);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - 7), 0, sizeof(float) * 7);

		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);
		float* pfRowOut = ar_cmmOut.pfGetRow(iRow - 7) + 7;
		iPos = (iRow - 15) % 15;
		pbBrightnessRow = pbBrightness + (iPos * ar_cmmIn.m_i64W);

		// Первые [0, 2*7[ колонки
		for (iCol = 0; iCol < (7 << 1); iCol++)
		{
			iCmin = max(7, static_cast <int>(iCol - 7));
			iCmax = iCol + 7;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				u32ValueAdd = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
					u32ValueAdd = 255;
			}
			if (u32ValueAdd != (u32ValueSub = pbBrightnessRow[iCol]))
			{
				pHistSub = pHistAll + (u32ValueSub * ar_cmmIn.m_i64W);
				pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W);
				pbBrightnessRow[iCol] = u32ValueAdd;
				uint32_t u32ValueAddSub = u32ValueAdd + u32ValueSub;

				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1)) - u32ValueSub * ((static_cast<uint32_t>(pHistSub[i]--) << 1) - 1);
			}
		}

		// Средние [2*Win_cen, ar_cmmIn.m_i64W - Win_size] колонки
		puiSumCurr = puiSum + 7;
		pHist = pHistAll + (iCol - 7);
		for (; iCol <= iColMax; iCol++, puiSumCurr++, pHist++)
		{
			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				u32ValueAdd = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
					u32ValueAdd = 255;
			}
			if (u32ValueAdd != (u32ValueSub = pbBrightnessRow[iCol]))
			{
				pHistSub = pHist + (u32ValueSub * ar_cmmIn.m_i64W);
				pHistAdd = pHist + (u32ValueAdd * ar_cmmIn.m_i64W);
				pbBrightnessRow[iCol] = u32ValueAdd;
				uint32_t u32ValueAddSub = u32ValueAdd + u32ValueSub;

				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = 0; i < 15; i++)
					puiSumCurr[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1)) - u32ValueSub * ((static_cast<uint32_t>(pHistSub[i]--) << 1) - 1);
			}
			pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
			pfRowOut++;
		}

		// Последние [ar_cmmIn.m_i64W - 2*Win_cen, ar_cmmIn.m_i64W - 1] колонки
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - 7;
			iCmax = min(static_cast <int>(ar_cmmIn.m_i64W - 7 - 1), static_cast <int>(iCol + 7));

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				u32ValueAdd = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
					u32ValueAdd = 255;
			}
			if (u32ValueAdd != (u32ValueSub = pbBrightnessRow[iCol]))
			{
				pHistSub = pHistAll + (u32ValueSub * ar_cmmIn.m_i64W);
				pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W);
				pbBrightnessRow[iCol] = u32ValueAdd;
				uint32_t u32ValueAddSub = u32ValueAdd + u32ValueSub;

				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1)) - u32ValueSub * ((static_cast<uint32_t>(pHistSub[i]--) << 1) - 1);
			}
			pfRowOut[0] = static_cast<float>(puiSum[iCmin] * Size_obratn);
			pfRowOut++;
		}
	}

	// Обнуляем края. Последние строки.
	for (iRow = ar_cmmIn.m_i64H - 7; iRow < ar_cmmOut.m_i64H; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

	if (nullptr != pHistAll)
		delete[] pHistAll;
	if (nullptr != puiSum)
		delete[] puiSum;
	if (nullptr != pbBrightness)
		delete[] pbBrightness;
}


void TextureFiltr_Mean_V8_17(MU16Data& ar_cmmIn, MFData& ar_cmmOut, double pseudo_min, double kfct)
{
	double Size_obratn = 1.0 / 289;

	// Кэш для гистограмм
	uint16_t* pHistAll = new uint16_t[256 * ar_cmmIn.m_i64W]; // [256] [ar_cmmIn.m_i64W]
	memset(pHistAll, 0, sizeof(uint16_t) * 256 * ar_cmmIn.m_i64W);
	uint16_t* pHist, * pHistAdd, * pHistSub;

	// Кэш для сумм
	uint32_t* puiSum = new uint32_t[ar_cmmIn.m_i64W], * puiSumCurr;
	memset(puiSum, 0, sizeof(uint32_t) * ar_cmmIn.m_i64W);

	// Кэш для яркостей
	uint8_t* pbBrightness = new uint8_t[ar_cmmIn.m_i64W * 17];
	uint8_t* pbBrightnessRow;

	int64_t iRow, iCol, iColMax = ar_cmmIn.m_i64W - 17;
	ar_cmmOut.iCreate(ar_cmmIn.m_i64W, ar_cmmIn.m_i64H, 6);

	// Обнуляем края. Первые строки.
	for (iRow = 0; iRow < 8; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

	int64_t i, iCmin, iCmax, iPos = 0;
	uint32_t u32ValueAdd, u32ValueSub;
	double d;

	// Первые [0, 17 - 1] строки
	pbBrightnessRow = pbBrightness;
	float* pfRowOut = ar_cmmOut.pfGetRow(8) + 8;
	for (iRow = 0; iRow < 17; iRow++, pbBrightnessRow += ar_cmmIn.m_i64W)
	{
		// Обнуляем края строк.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * 8);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - 8), 0, sizeof(float) * 8);

		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);

		// Первые [0, 2*8[ колонки
		for (iCol = 0; iCol < (8 << 1); iCol++)
		{
			iCmin = max(8, static_cast <int>(iCol - 8));
			iCmax = iCol + 8;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				pbBrightnessRow[iCol] = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
				{
					pbBrightnessRow[iCol] = u32ValueAdd = 255;
					pHistAdd = pHistAll + (255 * ar_cmmIn.m_i64W);
				}
				else
				{
					pbBrightnessRow[iCol] = static_cast<uint8_t>(u32ValueAdd);
					pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W);
				}
				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
		}

		// Средние [2*Win_cen, ar_cmmIn.m_i64W - Win_size] колонки
		puiSumCurr = puiSum + 8;
		for (; iCol <= iColMax; iCol++, puiSumCurr++)
		{
			iCmin = iCol - 8;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				pbBrightnessRow[iCol] = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
				{
					pbBrightnessRow[iCol] = u32ValueAdd = 255;
					pHistAdd = pHistAll + (255 * ar_cmmIn.m_i64W + iCmin);
				}
				else
				{
					pbBrightnessRow[iCol] = static_cast<uint8_t>(u32ValueAdd);
					pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W + iCmin);
				}
				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = 0; i < 17; i++)
					puiSumCurr[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
			if ((17 - 1) == iRow)
			{
				pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
				pfRowOut++;
			}
		}

		// Последние [ar_cmmIn.m_i64W - 2*Win_cen, ar_cmmIn.m_i64W - 1] колонки
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - 8;
			iCmax = min(static_cast <int>(ar_cmmIn.m_i64W - 8 - 1), static_cast <int>(iCol + 8));

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				pbBrightnessRow[iCol] = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
				{
					pbBrightnessRow[iCol] = u32ValueAdd = 255;
					pHistAdd = pHistAll + (255 * ar_cmmIn.m_i64W);
				}
				else
				{
					pbBrightnessRow[iCol] = static_cast<uint8_t>(u32ValueAdd);
					pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W);
				}
				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
			if ((17 - 1) == iRow)
			{
				pfRowOut[0] = static_cast<float>(puiSum[iCmin] * Size_obratn);
				pfRowOut++;
			}
		}
	}

	// Последующие строки [17, ar_cmmIn.m_i64H[
	for (iRow = 17; iRow < ar_cmmIn.m_i64H; iRow++)
	{
		// Обнуляем края строк.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * 8);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - 8), 0, sizeof(float) * 8);

		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);
		float* pfRowOut = ar_cmmOut.pfGetRow(iRow - 8) + 8;
		iPos = (iRow - 17) % 17;
		pbBrightnessRow = pbBrightness + (iPos * ar_cmmIn.m_i64W);

		// Первые [0, 2*8[ колонки
		for (iCol = 0; iCol < (8 << 1); iCol++)
		{
			iCmin = max(8, static_cast <int>(iCol - 8));
			iCmax = iCol + 8;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				u32ValueAdd = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
					u32ValueAdd = 255;
			}
			if (u32ValueAdd != (u32ValueSub = pbBrightnessRow[iCol]))
			{
				pHistSub = pHistAll + (u32ValueSub * ar_cmmIn.m_i64W);
				pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W);
				pbBrightnessRow[iCol] = u32ValueAdd;
				uint32_t u32ValueAddSub = u32ValueAdd + u32ValueSub;

				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1)) - u32ValueSub * ((static_cast<uint32_t>(pHistSub[i]--) << 1) - 1);
			}
		}

		// Средние [2*Win_cen, ar_cmmIn.m_i64W - Win_size] колонки
		puiSumCurr = puiSum + 8;
		pHist = pHistAll + (iCol - 8);
		for (; iCol <= iColMax; iCol++, puiSumCurr++, pHist++)
		{
			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				u32ValueAdd = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
					u32ValueAdd = 255;
			}
			if (u32ValueAdd != (u32ValueSub = pbBrightnessRow[iCol]))
			{
				pHistSub = pHist + (u32ValueSub * ar_cmmIn.m_i64W);
				pHistAdd = pHist + (u32ValueAdd * ar_cmmIn.m_i64W);
				pbBrightnessRow[iCol] = u32ValueAdd;
				uint32_t u32ValueAddSub = u32ValueAdd + u32ValueSub;

				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = 0; i < 17; i++)
					puiSumCurr[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1)) - u32ValueSub * ((static_cast<uint32_t>(pHistSub[i]--) << 1) - 1);
			}
			pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
			pfRowOut++;
		}

		// Последние [ar_cmmIn.m_i64W - 2*Win_cen, ar_cmmIn.m_i64W - 1] колонки
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - 8;
			iCmax = min(static_cast <int>(ar_cmmIn.m_i64W - 8 - 1), static_cast <int>(iCol + 8));

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				u32ValueAdd = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
					u32ValueAdd = 255;
			}
			if (u32ValueAdd != (u32ValueSub = pbBrightnessRow[iCol]))
			{
				pHistSub = pHistAll + (u32ValueSub * ar_cmmIn.m_i64W);
				pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W);
				pbBrightnessRow[iCol] = u32ValueAdd;
				uint32_t u32ValueAddSub = u32ValueAdd + u32ValueSub;

				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1)) - u32ValueSub * ((static_cast<uint32_t>(pHistSub[i]--) << 1) - 1);
			}
			pfRowOut[0] = static_cast<float>(puiSum[iCmin] * Size_obratn);
			pfRowOut++;
		}
	}

	// Обнуляем края. Последние строки.
	for (iRow = ar_cmmIn.m_i64H - 8; iRow < ar_cmmOut.m_i64H; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

	if (nullptr != pHistAll)
		delete[] pHistAll;
	if (nullptr != puiSum)
		delete[] puiSum;
	if (nullptr != pbBrightness)
		delete[] pbBrightness;
}


void TextureFiltr_Mean_V8_19(MU16Data& ar_cmmIn, MFData& ar_cmmOut, double pseudo_min, double kfct)
{
	double Size_obratn = 1.0 / 361;

	// Кэш для гистограмм
	uint16_t* pHistAll = new uint16_t[256 * ar_cmmIn.m_i64W]; // [256] [ar_cmmIn.m_i64W]
	memset(pHistAll, 0, sizeof(uint16_t) * 256 * ar_cmmIn.m_i64W);
	uint16_t* pHist, * pHistAdd, * pHistSub;

	// Кэш для сумм
	uint32_t* puiSum = new uint32_t[ar_cmmIn.m_i64W], * puiSumCurr;
	memset(puiSum, 0, sizeof(uint32_t) * ar_cmmIn.m_i64W);

	// Кэш для яркостей
	uint8_t* pbBrightness = new uint8_t[ar_cmmIn.m_i64W * 19];
	uint8_t* pbBrightnessRow;

	int64_t iRow, iCol, iColMax = ar_cmmIn.m_i64W - 19;
	ar_cmmOut.iCreate(ar_cmmIn.m_i64W, ar_cmmIn.m_i64H, 6);

	// Обнуляем края. Первые строки.
	for (iRow = 0; iRow < 9; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

	int64_t i, iCmin, iCmax, iPos = 0;
	uint32_t u32ValueAdd, u32ValueSub;
	double d;

	// Первые [0, 19 - 1] строки
	pbBrightnessRow = pbBrightness;
	float* pfRowOut = ar_cmmOut.pfGetRow(9) + 9;
	for (iRow = 0; iRow < 19; iRow++, pbBrightnessRow += ar_cmmIn.m_i64W)
	{
		// Обнуляем края строк.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * 9);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - 9), 0, sizeof(float) * 9);

		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);

		// Первые [0, 2*9[ колонки
		for (iCol = 0; iCol < (9 << 1); iCol++)
		{
			iCmin = max(9, static_cast <int>(iCol - 9));
			iCmax = iCol + 9;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				pbBrightnessRow[iCol] = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
				{
					pbBrightnessRow[iCol] = u32ValueAdd = 255;
					pHistAdd = pHistAll + (255 * ar_cmmIn.m_i64W);
				}
				else
				{
					pbBrightnessRow[iCol] = static_cast<uint8_t>(u32ValueAdd);
					pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W);
				}
				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
		}

		// Средние [2*Win_cen, ar_cmmIn.m_i64W - Win_size] колонки
		puiSumCurr = puiSum + 9;
		for (; iCol <= iColMax; iCol++, puiSumCurr++)
		{
			iCmin = iCol - 9;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				pbBrightnessRow[iCol] = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
				{
					pbBrightnessRow[iCol] = u32ValueAdd = 255;
					pHistAdd = pHistAll + (255 * ar_cmmIn.m_i64W + iCmin);
				}
				else
				{
					pbBrightnessRow[iCol] = static_cast<uint8_t>(u32ValueAdd);
					pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W + iCmin);
				}
				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = 0; i < 19; i++)
					puiSumCurr[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
			if ((19 - 1) == iRow)
			{
				pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
				pfRowOut++;
			}
		}

		// Последние [ar_cmmIn.m_i64W - 2*Win_cen, ar_cmmIn.m_i64W - 1] колонки
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - 9;
			iCmax = min(static_cast <int>(ar_cmmIn.m_i64W - 9 - 1), static_cast <int>(iCol + 9));

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				pbBrightnessRow[iCol] = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
				{
					pbBrightnessRow[iCol] = u32ValueAdd = 255;
					pHistAdd = pHistAll + (255 * ar_cmmIn.m_i64W);
				}
				else
				{
					pbBrightnessRow[iCol] = static_cast<uint8_t>(u32ValueAdd);
					pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W);
				}
				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
			if ((19 - 1) == iRow)
			{
				pfRowOut[0] = static_cast<float>(puiSum[iCmin] * Size_obratn);
				pfRowOut++;
			}
		}
	}

	// Последующие строки [19, ar_cmmIn.m_i64H[
	for (iRow = 19; iRow < ar_cmmIn.m_i64H; iRow++)
	{
		// Обнуляем края строк.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * 9);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - 9), 0, sizeof(float) * 9);

		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);
		float* pfRowOut = ar_cmmOut.pfGetRow(iRow - 9) + 9;
		iPos = (iRow - 19) % 19;
		pbBrightnessRow = pbBrightness + (iPos * ar_cmmIn.m_i64W);

		// Первые [0, 2*9[ колонки
		for (iCol = 0; iCol < (9 << 1); iCol++)
		{
			iCmin = max(9, static_cast <int>(iCol - 9));
			iCmax = iCol + 9;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				u32ValueAdd = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
					u32ValueAdd = 255;
			}
			if (u32ValueAdd != (u32ValueSub = pbBrightnessRow[iCol]))
			{
				pHistSub = pHistAll + (u32ValueSub * ar_cmmIn.m_i64W);
				pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W);
				pbBrightnessRow[iCol] = u32ValueAdd;
				uint32_t u32ValueAddSub = u32ValueAdd + u32ValueSub;

				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1)) - u32ValueSub * ((static_cast<uint32_t>(pHistSub[i]--) << 1) - 1);
			}
		}

		// Средние [2*Win_cen, ar_cmmIn.m_i64W - Win_size] колонки
		puiSumCurr = puiSum + 9;
		pHist = pHistAll + (iCol - 9);
		for (; iCol <= iColMax; iCol++, puiSumCurr++, pHist++)
		{
			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				u32ValueAdd = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
					u32ValueAdd = 255;
			}
			if (u32ValueAdd != (u32ValueSub = pbBrightnessRow[iCol]))
			{
				pHistSub = pHist + (u32ValueSub * ar_cmmIn.m_i64W);
				pHistAdd = pHist + (u32ValueAdd * ar_cmmIn.m_i64W);
				pbBrightnessRow[iCol] = u32ValueAdd;
				uint32_t u32ValueAddSub = u32ValueAdd + u32ValueSub;

				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = 0; i < 19; i++)
					puiSumCurr[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1)) - u32ValueSub * ((static_cast<uint32_t>(pHistSub[i]--) << 1) - 1);
			}
			pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
			pfRowOut++;
		}

		// Последние [ar_cmmIn.m_i64W - 2*Win_cen, ar_cmmIn.m_i64W - 1] колонки
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - 9;
			iCmax = min(static_cast <int>(ar_cmmIn.m_i64W - 9 - 1), static_cast <int>(iCol + 9));

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				u32ValueAdd = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
					u32ValueAdd = 255;
			}
			if (u32ValueAdd != (u32ValueSub = pbBrightnessRow[iCol]))
			{
				pHistSub = pHistAll + (u32ValueSub * ar_cmmIn.m_i64W);
				pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W);
				pbBrightnessRow[iCol] = u32ValueAdd;
				uint32_t u32ValueAddSub = u32ValueAdd + u32ValueSub;

				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1)) - u32ValueSub * ((static_cast<uint32_t>(pHistSub[i]--) << 1) - 1);
			}
			pfRowOut[0] = static_cast<float>(puiSum[iCmin] * Size_obratn);
			pfRowOut++;
		}
	}

	// Обнуляем края. Последние строки.
	for (iRow = ar_cmmIn.m_i64H - 9; iRow < ar_cmmOut.m_i64H; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

	if (nullptr != pHistAll)
		delete[] pHistAll;
	if (nullptr != puiSum)
		delete[] puiSum;
	if (nullptr != pbBrightness)
		delete[] pbBrightness;
}


void TextureFiltr_Mean_V8_21(MU16Data& ar_cmmIn, MFData& ar_cmmOut, double pseudo_min, double kfct)
{
	double Size_obratn = 1.0 / 441;

	// Кэш для гистограмм
	uint16_t* pHistAll = new uint16_t[256 * ar_cmmIn.m_i64W]; // [256] [ar_cmmIn.m_i64W]
	memset(pHistAll, 0, sizeof(uint16_t) * 256 * ar_cmmIn.m_i64W);
	uint16_t* pHist, * pHistAdd, * pHistSub;

	// Кэш для сумм
	uint32_t* puiSum = new uint32_t[ar_cmmIn.m_i64W], * puiSumCurr;
	memset(puiSum, 0, sizeof(uint32_t) * ar_cmmIn.m_i64W);

	// Кэш для яркостей
	uint8_t* pbBrightness = new uint8_t[ar_cmmIn.m_i64W * 21];
	uint8_t* pbBrightnessRow;

	int64_t iRow, iCol, iColMax = ar_cmmIn.m_i64W - 21;
	ar_cmmOut.iCreate(ar_cmmIn.m_i64W, ar_cmmIn.m_i64H, 6);

	// Обнуляем края. Первые строки.
	for (iRow = 0; iRow < 10; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

	int64_t i, iCmin, iCmax, iPos = 0;
	uint32_t u32ValueAdd, u32ValueSub;
	double d;

	// Первые [0, 21 - 1] строки
	pbBrightnessRow = pbBrightness;
	float* pfRowOut = ar_cmmOut.pfGetRow(10) + 10;
	for (iRow = 0; iRow < 21; iRow++, pbBrightnessRow += ar_cmmIn.m_i64W)
	{
		// Обнуляем края строк.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * 10);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - 10), 0, sizeof(float) * 10);

		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);

		// Первые [0, 2*10[ колонки
		for (iCol = 0; iCol < (10 << 1); iCol++)
		{
			iCmin = max(10, static_cast <int>(iCol - 10));
			iCmax = iCol + 10;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				pbBrightnessRow[iCol] = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
				{
					pbBrightnessRow[iCol] = u32ValueAdd = 255;
					pHistAdd = pHistAll + (255 * ar_cmmIn.m_i64W);
				}
				else
				{
					pbBrightnessRow[iCol] = static_cast<uint8_t>(u32ValueAdd);
					pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W);
				}
				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
		}

		// Средние [2*Win_cen, ar_cmmIn.m_i64W - Win_size] колонки
		puiSumCurr = puiSum + 10;
		for (; iCol <= iColMax; iCol++, puiSumCurr++)
		{
			iCmin = iCol - 10;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				pbBrightnessRow[iCol] = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
				{
					pbBrightnessRow[iCol] = u32ValueAdd = 255;
					pHistAdd = pHistAll + (255 * ar_cmmIn.m_i64W + iCmin);
				}
				else
				{
					pbBrightnessRow[iCol] = static_cast<uint8_t>(u32ValueAdd);
					pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W + iCmin);
				}
				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = 0; i < 21; i++)
					puiSumCurr[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
			if ((21 - 1) == iRow)
			{
				pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
				pfRowOut++;
			}
		}

		// Последние [ar_cmmIn.m_i64W - 2*Win_cen, ar_cmmIn.m_i64W - 1] колонки
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - 10;
			iCmax = min(static_cast <int>(ar_cmmIn.m_i64W - 10 - 1), static_cast <int>(iCol + 10));

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				pbBrightnessRow[iCol] = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
				{
					pbBrightnessRow[iCol] = u32ValueAdd = 255;
					pHistAdd = pHistAll + (255 * ar_cmmIn.m_i64W);
				}
				else
				{
					pbBrightnessRow[iCol] = static_cast<uint8_t>(u32ValueAdd);
					pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W);
				}
				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
			if ((21 - 1) == iRow)
			{
				pfRowOut[0] = static_cast<float>(puiSum[iCmin] * Size_obratn);
				pfRowOut++;
			}
		}
	}

	// Последующие строки [21, ar_cmmIn.m_i64H[
	for (iRow = 21; iRow < ar_cmmIn.m_i64H; iRow++)
	{
		// Обнуляем края строк.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * 10);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - 10), 0, sizeof(float) * 10);

		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);
		float* pfRowOut = ar_cmmOut.pfGetRow(iRow - 10) + 10;
		iPos = (iRow - 21) % 21;
		pbBrightnessRow = pbBrightness + (iPos * ar_cmmIn.m_i64W);

		// Первые [0, 2*10[ колонки
		for (iCol = 0; iCol < (10 << 1); iCol++)
		{
			iCmin = max(10, static_cast <int>(iCol - 10));
			iCmax = iCol + 10;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				u32ValueAdd = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
					u32ValueAdd = 255;
			}
			if (u32ValueAdd != (u32ValueSub = pbBrightnessRow[iCol]))
			{
				pHistSub = pHistAll + (u32ValueSub * ar_cmmIn.m_i64W);
				pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W);
				pbBrightnessRow[iCol] = u32ValueAdd;
				uint32_t u32ValueAddSub = u32ValueAdd + u32ValueSub;

				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1)) - u32ValueSub * ((static_cast<uint32_t>(pHistSub[i]--) << 1) - 1);
			}
		}

		// Средние [2*Win_cen, ar_cmmIn.m_i64W - Win_size] колонки
		puiSumCurr = puiSum + 10;
		pHist = pHistAll + (iCol - 10);
		for (; iCol <= iColMax; iCol++, puiSumCurr++, pHist++)
		{
			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				u32ValueAdd = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
					u32ValueAdd = 255;
			}
			if (u32ValueAdd != (u32ValueSub = pbBrightnessRow[iCol]))
			{
				pHistSub = pHist + (u32ValueSub * ar_cmmIn.m_i64W);
				pHistAdd = pHist + (u32ValueAdd * ar_cmmIn.m_i64W);
				pbBrightnessRow[iCol] = u32ValueAdd;
				uint32_t u32ValueAddSub = u32ValueAdd + u32ValueSub;

				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = 0; i < 21; i++)
					puiSumCurr[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1)) - u32ValueSub * ((static_cast<uint32_t>(pHistSub[i]--) << 1) - 1);
			}
			pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
			pfRowOut++;
		}

		// Последние [ar_cmmIn.m_i64W - 2*Win_cen, ar_cmmIn.m_i64W - 1] колонки
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - 10;
			iCmax = min(static_cast <int>(ar_cmmIn.m_i64W - 10 - 1), static_cast <int>(iCol + 10));

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
				u32ValueAdd = 0;
			else
			{
				if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)
					u32ValueAdd = 255;
			}
			if (u32ValueAdd != (u32ValueSub = pbBrightnessRow[iCol]))
			{
				pHistSub = pHistAll + (u32ValueSub * ar_cmmIn.m_i64W);
				pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W);
				pbBrightnessRow[iCol] = u32ValueAdd;
				uint32_t u32ValueAddSub = u32ValueAdd + u32ValueSub;

				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1)) - u32ValueSub * ((static_cast<uint32_t>(pHistSub[i]--) << 1) - 1);
			}
			pfRowOut[0] = static_cast<float>(puiSum[iCmin] * Size_obratn);
			pfRowOut++;
		}
	}

	// Обнуляем края. Последние строки.
	for (iRow = ar_cmmIn.m_i64H - 10; iRow < ar_cmmOut.m_i64H; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

	if (nullptr != pHistAll)
		delete[] pHistAll;
	if (nullptr != puiSum)
		delete[] puiSum;
	if (nullptr != pbBrightness)
		delete[] pbBrightness;
}

tdTextureFiltr_Mean_V8 g_afunTextureFiltr_Mean_V8[10] =
{
 TextureFiltr_Mean_V8_3, // 0
 TextureFiltr_Mean_V8_5, // 1
 TextureFiltr_Mean_V8_7, // 2
 TextureFiltr_Mean_V8_9, // 3
 TextureFiltr_Mean_V8_11, // 4
 TextureFiltr_Mean_V8_13, // 5
 TextureFiltr_Mean_V8_15, // 6
 TextureFiltr_Mean_V8_17, // 7
 TextureFiltr_Mean_V8_19, // 8
 TextureFiltr_Mean_V8_21 // 9
};
