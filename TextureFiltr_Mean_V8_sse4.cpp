#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <conio.h>
#include <string>
#include <omp.h>
#include <cstdlib>
#include "MU16Data.h"

#include "TextureFiltr_Mean_V8_CodeGen.h"
using namespace std;

void TextureFiltr_Mean_V8_3_sse4(MU16Data& ar_cmmIn, MFData& ar_cmmOut, double pseudo_min, double kfct)
{
	double Size_obratn = 1.0 / 9;

	// ��� ��� ����������
	uint16_t* pHistAll = new uint16_t[256 * ar_cmmIn.m_i64W]; // [256][ar_cmmIn.m_i64W]
	memset(pHistAll, 0, sizeof(uint16_t) * 256 * ar_cmmIn.m_i64W);
	uint16_t* pHist, * pHistAdd, * pHistSub;

	// ��� ��� ����
	uint32_t* puiSum = new uint32_t[ar_cmmIn.m_i64W], * puiSumCurr;
	memset(puiSum, 0, sizeof(uint32_t) * ar_cmmIn.m_i64W);

	// ��� ��� ��������
	uint8_t* pbBrightness = new uint8_t[ar_cmmIn.m_i64W * 3];
	uint8_t* pbBrightnessRow;

	int64_t iRow, iCol, iColMax = ar_cmmIn.m_i64W - 3;
	ar_cmmOut.iCreate(ar_cmmIn.m_i64W, ar_cmmIn.m_i64H, 6);

	// �������� ����. ������ ������.
	for (iRow = 0; iRow < 1; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

	int64_t i, iCmin, iCmax, iPos = 0;
	uint32_t u32ValueAdd, u32ValueSub;
	double d;

	// ������ [0, 3 - 1] ������
	pbBrightnessRow = pbBrightness;
	float* pfRowOut = ar_cmmOut.pfGetRow(1) + 1;
	for (iRow = 0; iRow < 3; iRow++, pbBrightnessRow += ar_cmmIn.m_i64W)
	{
		// �������� ���� �����.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * 1);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - 1), 0, sizeof(float) * 1);

		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);

		// ������ [0, 2*1[ �������
		for (iCol = 0; iCol < (1 << 1); iCol++)
		{
			iCmin = max(1, static_cast<int>(iCol - 1));
			iCmax = iCol + 1;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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
				// ��������� ������� � 3 ���������� � ����
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
		}

		// ������� [2*1, ar_cmmIn.m_i64W - 3] �������
		puiSumCurr = puiSum + 1;
		for (; iCol <= iColMax; iCol++, puiSumCurr++)
		{
			iCmin = iCol - 1;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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
				// ��������� ������� � 3 ���������� � ����
				for (i = 0; i < 3; i++)
					puiSumCurr[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
			if ((3 - 1) == iRow)
			{
				pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
				pfRowOut++;
			}
		}

		// ��������� [ar_cmmIn.m_i64W - 2*1, ar_cmmIn.m_i64W - 1] �������
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - 1;
			iCmax = min(static_cast<int>(ar_cmmIn.m_i64W - 1 - 1), static_cast<int>(iCol + 1));

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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
				// ��������� ������� � 3 ���������� � ����
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

	__m128i xmmHistOne_3 = _mm_set_epi16(0, 0, 0, 0, 0, 1, 1, 1); // ��� inc ��� dec ����� ��� 3 ����������

	// ����������� ������ [3, ar_cmmIn.m_i64H[
	for (iRow = 3; iRow < ar_cmmIn.m_i64H; iRow++)
	{
		// �������� ���� �����.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * 1);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - 1), 0, sizeof(float) * 1);

		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);
		float* pfRowOut = ar_cmmOut.pfGetRow(iRow - 1) + 1;
		iPos = (iRow - 3) % 3;
		pbBrightnessRow = pbBrightness + (iPos * ar_cmmIn.m_i64W);

		// ������ [0, 2*1[ �������
		for (iCol = 0; iCol < (1 << 1); iCol++)
		{
			iCmin = max(1, static_cast<int>(iCol - 1));
			iCmax = iCol + 1;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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

				// ��������� ������� � 3 ���������� � ����
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAddSub + ((u32ValueAdd * static_cast<uint32_t>(pHistAdd[i]++) - u32ValueSub * static_cast<uint32_t>(pHistSub[i]--)) << 1);
			}
		}

		// ������� [2*1, ar_cmmIn.m_i64W - 3] �������
		puiSumCurr = puiSum + 1;
		pHist = pHistAll + (iCol - 1);
		for (; iCol <= iColMax; iCol++, puiSumCurr++, pHist++)
		{
			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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

				// ��������� ������� � 3 ���������� � ����
				//============================
				// ������ 3 �����
				//============================
				__m128i xmmHistSub = _mm_lddqu_si128(reinterpret_cast<__m128i*>(pHistSub));// ��������� 128 ��� (���������� 48 ��� - 3 ��. �������)
				__m128i xmmHistAdd = _mm_lddqu_si128(reinterpret_cast<__m128i*>(pHistAdd));

				// 3 �������� �����������
				__m128i xmmSumCurr = _mm_lddqu_si128(reinterpret_cast<__m128i*> (puiSumCurr)); // ��������� 128 ��� (���������� 96 ��� - 3 ��. �������)

				__m128i xmmHistSub32 = _mm_cvtepu16_epi32(xmmHistSub); // HistSub32[3], HistSub32[2], HistSub32[1], HistSub32[0](Hi < ->Lo)
				__m128i xmmHistAdd32 = _mm_cvtepu16_epi32(xmmHistAdd);

				__m128i xmmValueAdd = _mm_loadu_si32(&u32ValueAdd); // 0, 0, 0, u32ValueAdd(Hi < ->Lo)
				__m128i xmmValueSub = _mm_loadu_si32(&u32ValueSub);

				xmmValueAdd = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(xmmValueAdd), _mm_castsi128_ps(xmmValueAdd), 0x0C0)); // 0, u32ValueAdd, u32ValueAdd, u32ValueAdd
				xmmValueSub = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(xmmValueSub), _mm_castsi128_ps(xmmValueSub), 0x0C0));

				xmmHistAdd32 = _mm_sub_epi32(_mm_mullo_epi32(xmmHistAdd32, xmmValueAdd), _mm_mullo_epi32(xmmHistSub32, xmmValueSub));
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmHistAdd32); // <<= 1
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmValueAdd);
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmValueSub);

				_mm_storeu_si128(reinterpret_cast<__m128i*>(puiSumCurr), _mm_add_epi32(xmmSumCurr, xmmHistAdd32)); // ��������� puiSumCurr[0,1,2,3]
				_mm_storeu_si128(reinterpret_cast<__m128i*>(pHistAdd), _mm_add_epi16(xmmHistAdd, xmmHistOne_3)); // pHistAdd[0-2]++; ��������� 128 ��� (���������� 128 - 8 ��. �������)
				_mm_storeu_si128(reinterpret_cast<__m128i*>(pHistSub), _mm_sub_epi16(xmmHistSub, xmmHistOne_3)); // pHistSub[0-2]--; ��������� 128 ��� (���������� 128 - 8 ��. �������)
			}
			pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
			pfRowOut++;
		}

		// ��������� [ar_cmmIn.m_i64W - 2*1, ar_cmmIn.m_i64W - 1] �������
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - 1;
			iCmax = min(static_cast<int>(ar_cmmIn.m_i64W - 1 - 1), static_cast<int>(iCol + 1));

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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

				// ��������� ������� � 3 ���������� � ����
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAddSub + ((u32ValueAdd * static_cast<uint32_t>(pHistAdd[i]++) - u32ValueSub * static_cast<uint32_t>(pHistSub[i]--)) << 1);
			}
			pfRowOut[0] = static_cast<float>(puiSum[iCmin] * Size_obratn);
			pfRowOut++;
		}
	}

	// �������� ����. ��������� ������.
	for (iRow = ar_cmmIn.m_i64H - 1; iRow < ar_cmmOut.m_i64H; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

	if (nullptr != pHistAll)
		delete[] pHistAll;
	if (nullptr != puiSum)
		delete[] puiSum;
	if (nullptr != pbBrightness)
		delete[] pbBrightness;
}


void TextureFiltr_Mean_V8_5_sse4(MU16Data& ar_cmmIn, MFData& ar_cmmOut, double pseudo_min, double kfct)
{
	double Size_obratn = 1.0 / 25;

	// ��� ��� ����������
	uint16_t* pHistAll = new uint16_t[256 * ar_cmmIn.m_i64W]; // [256][ar_cmmIn.m_i64W]
	memset(pHistAll, 0, sizeof(uint16_t) * 256 * ar_cmmIn.m_i64W);
	uint16_t* pHist, * pHistAdd, * pHistSub;

	// ��� ��� ����
	uint32_t* puiSum = new uint32_t[ar_cmmIn.m_i64W], * puiSumCurr;
	memset(puiSum, 0, sizeof(uint32_t) * ar_cmmIn.m_i64W);

	// ��� ��� ��������
	uint8_t* pbBrightness = new uint8_t[ar_cmmIn.m_i64W * 5];
	uint8_t* pbBrightnessRow;

	int64_t iRow, iCol, iColMax = ar_cmmIn.m_i64W - 5;
	ar_cmmOut.iCreate(ar_cmmIn.m_i64W, ar_cmmIn.m_i64H, 6);

	// �������� ����. ������ ������.
	for (iRow = 0; iRow < 2; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

	int64_t i, iCmin, iCmax, iPos = 0;
	uint32_t u32ValueAdd, u32ValueSub;
	double d;

	// ������ [0, 5 - 1] ������
	pbBrightnessRow = pbBrightness;
	float* pfRowOut = ar_cmmOut.pfGetRow(2) + 2;
	for (iRow = 0; iRow < 5; iRow++, pbBrightnessRow += ar_cmmIn.m_i64W)
	{
		// �������� ���� �����.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * 2);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - 2), 0, sizeof(float) * 2);

		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);

		// ������ [0, 2*2[ �������
		for (iCol = 0; iCol < (2 << 1); iCol++)
		{
			iCmin = max(2, static_cast<int>(iCol - 2));
			iCmax = iCol + 2;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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
				// ��������� ������� � 5 ���������� � ����
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
		}

		// ������� [2*2, ar_cmmIn.m_i64W - 5] �������
		puiSumCurr = puiSum + 2;
		for (; iCol <= iColMax; iCol++, puiSumCurr++)
		{
			iCmin = iCol - 2;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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
				// ��������� ������� � 5 ���������� � ����
				for (i = 0; i < 5; i++)
					puiSumCurr[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
			if ((5 - 1) == iRow)
			{
				pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
				pfRowOut++;
			}
		}

		// ��������� [ar_cmmIn.m_i64W - 2*2, ar_cmmIn.m_i64W - 1] �������
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - 2;
			iCmax = min(static_cast<int>(ar_cmmIn.m_i64W - 2 - 1), static_cast<int>(iCol + 2));

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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
				// ��������� ������� � 5 ���������� � ����
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

	__m128i xmmHistOne_5 = _mm_set_epi16(0, 0, 0, 1, 1, 1, 1, 1); // ��� inc ��� dec ����� ��� 5 ����������

	// ����������� ������ [5, ar_cmmIn.m_i64H[
	for (iRow = 5; iRow < ar_cmmIn.m_i64H; iRow++)
	{
		// �������� ���� �����.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * 2);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - 2), 0, sizeof(float) * 2);

		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);
		float* pfRowOut = ar_cmmOut.pfGetRow(iRow - 2) + 2;
		iPos = (iRow - 5) % 5;
		pbBrightnessRow = pbBrightness + (iPos * ar_cmmIn.m_i64W);

		// ������ [0, 2*2[ �������
		for (iCol = 0; iCol < (2 << 1); iCol++)
		{
			iCmin = max(2, static_cast<int>(iCol - 2));
			iCmax = iCol + 2;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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

				// ��������� ������� � 5 ���������� � ����
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAddSub + ((u32ValueAdd * static_cast<uint32_t>(pHistAdd[i]++) - u32ValueSub * static_cast<uint32_t>(pHistSub[i]--)) << 1);
			}
		}

		// ������� [2*2, ar_cmmIn.m_i64W - 5] �������
		puiSumCurr = puiSum + 2;
		pHist = pHistAll + (iCol - 2);
		for (; iCol <= iColMax; iCol++, puiSumCurr++, pHist++)
		{
			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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

				// ��������� ������� � 5 ���������� � ����
				//============================
				// ������ 5 �����
				//============================
				__m128i xmmHistSub = _mm_lddqu_si128(reinterpret_cast<__m128i*>(pHistSub));// ��������� 128 ��� (���������� 80 ��� - 5 ��. �������)
				__m128i xmmHistAdd = _mm_lddqu_si128(reinterpret_cast<__m128i*>(pHistAdd));

				// ������� 4 �������� �����������
				__m128i xmmSumCurr = _mm_lddqu_si128(reinterpret_cast<__m128i*> (puiSumCurr)); // ��������� 128 ��� (���������� 128 ��� - 4 ��. �������)

				__m128i xmmHistSub32 = _mm_cvtepu16_epi32(xmmHistSub); // HistSub32[3], HistSub32[2], HistSub32[1], HistSub32[0](Hi < ->Lo)
				__m128i xmmHistAdd32 = _mm_cvtepu16_epi32(xmmHistAdd);

				__m128i xmmValueAdd = _mm_loadu_si32(&u32ValueAdd); // 0, 0, 0, u32ValueAdd(Hi < ->Lo)
				__m128i xmmValueSub = _mm_loadu_si32(&u32ValueSub);

				xmmValueAdd = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(xmmValueAdd), _mm_castsi128_ps(xmmValueAdd), 0x0)); // 0, u32ValueAdd, u32ValueAdd, u32ValueAdd
				xmmValueSub = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(xmmValueSub), _mm_castsi128_ps(xmmValueSub), 0x0));

				__m128i xmmValueAddSub = _mm_add_epi32(xmmValueAdd, xmmValueSub);

				xmmHistAdd32 = _mm_sub_epi32(_mm_mullo_epi32(xmmHistAdd32, xmmValueAdd), _mm_mullo_epi32(xmmHistSub32, xmmValueSub));
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmHistAdd32); // <<= 1
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmValueAddSub);

				_mm_storeu_si128(reinterpret_cast<__m128i*>(puiSumCurr), _mm_add_epi32(xmmSumCurr, xmmHistAdd32)); // ��������� puiSumCurr[0,1,2,3]

				// ������� 1 ������� �����������
				xmmSumCurr = _mm_lddqu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 4)); // ��������� 128 ��� (���������� 32 - 1 ��. �������)

				xmmHistSub32 = _mm_cvtepu16_epi32(_mm_bsrli_si128(xmmHistSub, 8));// HistSub32[7], HistSub32[6], HistSub32[5], HistSub32[4] (Hi <-> Lo)
				xmmHistAdd32 = _mm_cvtepu16_epi32(_mm_bsrli_si128(xmmHistAdd, 8));

				// ������������ �������� �����������
				_mm_storeu_si128(reinterpret_cast<__m128i*>(pHistAdd), _mm_add_epi16(xmmHistAdd, xmmHistOne_5)); // pHistAdd[0-4]++; ��������� 128 ��� (���������� 128 - 8 ��. �������)
				_mm_storeu_si128(reinterpret_cast<__m128i*>(pHistSub), _mm_sub_epi16(xmmHistSub, xmmHistOne_5)); // pHistSub[0-4]--; ��������� 128 ��� (���������� 128 - 8 ��. �������)

				xmmValueAdd = _mm_bsrli_si128(xmmValueAdd, 12); // 0, 0, 0, xmmValueAdd(Hi < ->Lo)
				xmmValueSub = _mm_bsrli_si128(xmmValueSub, 12); // 0, 0, 	0, xmmValueSub(Hi < ->Lo)
				xmmValueAddSub = _mm_bsrli_si128(xmmValueAddSub, 12);

				xmmHistAdd32 = _mm_sub_epi32(_mm_mullo_epi32(xmmHistAdd32, xmmValueAdd), _mm_mullo_epi32(xmmHistSub32, xmmValueSub));
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmHistAdd32); // <<= 	1
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmValueAddSub);

				_mm_storeu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 4), _mm_add_epi32(xmmSumCurr, xmmHistAdd32)); // ��������� puiSumCurr[0,1,2,3]
			}
			pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
			pfRowOut++;
		}

		// ��������� [ar_cmmIn.m_i64W - 2*2, ar_cmmIn.m_i64W - 1] �������
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - 2;
			iCmax = min(static_cast<int>(ar_cmmIn.m_i64W - 2 - 1), static_cast<int>(iCol + 2));

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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

				// ��������� ������� � 5 ���������� � ����
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAddSub + ((u32ValueAdd * static_cast<uint32_t>(pHistAdd[i]++) - u32ValueSub * static_cast<uint32_t>(pHistSub[i]--)) << 1);
			}
			pfRowOut[0] = static_cast<float>(puiSum[iCmin] * Size_obratn);
			pfRowOut++;
		}
	}

	// �������� ����. ��������� ������.
	for (iRow = ar_cmmIn.m_i64H - 2; iRow < ar_cmmOut.m_i64H; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

	if (nullptr != pHistAll)
		delete[] pHistAll;
	if (nullptr != puiSum)
		delete[] puiSum;
	if (nullptr != pbBrightness)
		delete[] pbBrightness;
}


void TextureFiltr_Mean_V8_7_sse4(MU16Data& ar_cmmIn, MFData& ar_cmmOut, double pseudo_min, double kfct)
{
	double Size_obratn = 1.0 / 49;

	// ��� ��� ����������
	uint16_t* pHistAll = new uint16_t[256 * ar_cmmIn.m_i64W]; // [256][ar_cmmIn.m_i64W]
	memset(pHistAll, 0, sizeof(uint16_t) * 256 * ar_cmmIn.m_i64W);
	uint16_t* pHist, * pHistAdd, * pHistSub;

	// ��� ��� ����
	uint32_t* puiSum = new uint32_t[ar_cmmIn.m_i64W], * puiSumCurr;
	memset(puiSum, 0, sizeof(uint32_t) * ar_cmmIn.m_i64W);

	// ��� ��� ��������
	uint8_t* pbBrightness = new uint8_t[ar_cmmIn.m_i64W * 7];
	uint8_t* pbBrightnessRow;

	int64_t iRow, iCol, iColMax = ar_cmmIn.m_i64W - 7;
	ar_cmmOut.iCreate(ar_cmmIn.m_i64W, ar_cmmIn.m_i64H, 6);

	// �������� ����. ������ ������.
	for (iRow = 0; iRow < 3; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

	int64_t i, iCmin, iCmax, iPos = 0;
	uint32_t u32ValueAdd, u32ValueSub;
	double d;

	// ������ [0, 7 - 1] ������
	pbBrightnessRow = pbBrightness;
	float* pfRowOut = ar_cmmOut.pfGetRow(3) + 3;
	for (iRow = 0; iRow < 7; iRow++, pbBrightnessRow += ar_cmmIn.m_i64W)
	{
		// �������� ���� �����.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * 3);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - 3), 0, sizeof(float) * 3);

		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);

		// ������ [0, 2*3[ �������
		for (iCol = 0; iCol < (3 << 1); iCol++)
		{
			iCmin = max(3, static_cast<int>(iCol - 3));
			iCmax = iCol + 3;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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
				// ��������� ������� � 7 ���������� � ����
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
		}

		// ������� [2*3, ar_cmmIn.m_i64W - 7] �������
		puiSumCurr = puiSum + 3;
		for (; iCol <= iColMax; iCol++, puiSumCurr++)
		{
			iCmin = iCol - 3;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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
				// ��������� ������� � 7 ���������� � ����
				for (i = 0; i < 7; i++)
					puiSumCurr[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
			if ((7 - 1) == iRow)
			{
				pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
				pfRowOut++;
			}
		}

		// ��������� [ar_cmmIn.m_i64W - 2*3, ar_cmmIn.m_i64W - 1] �������
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - 3;
			iCmax = min(static_cast<int>(ar_cmmIn.m_i64W - 3 - 1), static_cast<int>(iCol + 3));

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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
				// ��������� ������� � 7 ���������� � ����
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

	__m128i xmmHistOne_7 = _mm_set_epi16(0, 1, 1, 1, 1, 1, 1, 1); // ��� inc ��� dec ����� ��� 7 ����������

	// ����������� ������ [7, ar_cmmIn.m_i64H[
	for (iRow = 7; iRow < ar_cmmIn.m_i64H; iRow++)
	{
		// �������� ���� �����.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * 3);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - 3), 0, sizeof(float) * 3);

		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);
		float* pfRowOut = ar_cmmOut.pfGetRow(iRow - 3) + 3;
		iPos = (iRow - 7) % 7;
		pbBrightnessRow = pbBrightness + (iPos * ar_cmmIn.m_i64W);

		// ������ [0, 2*3[ �������
		for (iCol = 0; iCol < (3 << 1); iCol++)
		{
			iCmin = max(3, static_cast<int>(iCol - 3));
			iCmax = iCol + 3;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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

				// ��������� ������� � 7 ���������� � ����
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAddSub + ((u32ValueAdd * static_cast<uint32_t>(pHistAdd[i]++) - u32ValueSub * static_cast<uint32_t>(pHistSub[i]--)) << 1);
			}
		}

		// ������� [2*3, ar_cmmIn.m_i64W - 7] �������
		puiSumCurr = puiSum + 3;
		pHist = pHistAll + (iCol - 3);
		for (; iCol <= iColMax; iCol++, puiSumCurr++, pHist++)
		{
			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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

				// ��������� ������� � 7 ���������� � ����
				//============================
				// ������ 7 �����
				//============================
				__m128i xmmHistSub = _mm_lddqu_si128(reinterpret_cast<__m128i*>(pHistSub));// ��������� 128 ��� (���������� 112 ��� - 7 ��. �������)
				__m128i xmmHistAdd = _mm_lddqu_si128(reinterpret_cast<__m128i*>(pHistAdd));

				// ������� 4 �������� �����������
				__m128i xmmSumCurr = _mm_lddqu_si128(reinterpret_cast<__m128i*> (puiSumCurr)); // ��������� 128 ��� (���������� 128 ��� - 4 ��. �������)

				__m128i xmmHistSub32 = _mm_cvtepu16_epi32(xmmHistSub); // HistSub32[3], HistSub32[2], HistSub32[1], HistSub32[0](Hi < ->Lo)
				__m128i xmmHistAdd32 = _mm_cvtepu16_epi32(xmmHistAdd);

				__m128i xmmValueAdd = _mm_loadu_si32(&u32ValueAdd); // 0, 0, 0, u32ValueAdd(Hi < ->Lo)
				__m128i xmmValueSub = _mm_loadu_si32(&u32ValueSub);

				xmmValueAdd = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(xmmValueAdd), _mm_castsi128_ps(xmmValueAdd), 0x0)); // 0, u32ValueAdd, u32ValueAdd, u32ValueAdd
				xmmValueSub = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(xmmValueSub), _mm_castsi128_ps(xmmValueSub), 0x0));

				__m128i xmmValueAddSub = _mm_add_epi32(xmmValueAdd, xmmValueSub);

				xmmHistAdd32 = _mm_sub_epi32(_mm_mullo_epi32(xmmHistAdd32, xmmValueAdd), _mm_mullo_epi32(xmmHistSub32, xmmValueSub));
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmHistAdd32); // <<= 1
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmValueAddSub);

				_mm_storeu_si128(reinterpret_cast<__m128i*>(puiSumCurr), _mm_add_epi32(xmmSumCurr, xmmHistAdd32)); // ��������� puiSumCurr[0,1,2,3]

				// ������� 3 �������� �����������
				xmmSumCurr = _mm_lddqu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 4)); // ��������� 128 ��� (���������� 96 - 3 ��. �������)

				xmmHistSub32 = _mm_cvtepu16_epi32(_mm_bsrli_si128(xmmHistSub, 8));// HistSub32[7], HistSub32[6], HistSub32[5], HistSub32[4] (Hi <-> Lo)
				xmmHistAdd32 = _mm_cvtepu16_epi32(_mm_bsrli_si128(xmmHistAdd, 8));

				// ������������ �������� �����������
				_mm_storeu_si128(reinterpret_cast<__m128i*>(pHistAdd), _mm_add_epi16(xmmHistAdd, xmmHistOne_7)); // pHistAdd[0-6]++; ��������� 128 ��� (���������� 128 - 8 ��. �������)
				_mm_storeu_si128(reinterpret_cast<__m128i*>(pHistSub), _mm_sub_epi16(xmmHistSub, xmmHistOne_7)); // pHistSub[0-6]--; ��������� 128 ��� (���������� 128 - 8 ��. �������)

				xmmValueAdd = _mm_bsrli_si128(xmmValueAdd, 4); // 0, xmmValueAdd, xmmValueAdd, xmmValueAdd(Hi < ->Lo)
				xmmValueSub = _mm_bsrli_si128(xmmValueSub, 4); // 0, xmmValueSub, xmmValueSub, xmmValueSub(Hi < ->Lo)
				xmmValueAddSub = _mm_bsrli_si128(xmmValueAddSub, 4);

				xmmHistAdd32 = _mm_sub_epi32(_mm_mullo_epi32(xmmHistAdd32, xmmValueAdd), _mm_mullo_epi32(xmmHistSub32, xmmValueSub));
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmHistAdd32); // <<= 	1
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmValueAddSub);

				_mm_storeu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 4), _mm_add_epi32(xmmSumCurr, xmmHistAdd32)); // ��������� puiSumCurr[0,1,2,3]
			}
			pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
			pfRowOut++;
		}

		// ��������� [ar_cmmIn.m_i64W - 2*3, ar_cmmIn.m_i64W - 1] �������
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - 3;
			iCmax = min(static_cast<int>(ar_cmmIn.m_i64W - 3 - 1), static_cast<int>(iCol + 3));

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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

				// ��������� ������� � 7 ���������� � ����
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAddSub + ((u32ValueAdd * static_cast<uint32_t>(pHistAdd[i]++) - u32ValueSub * static_cast<uint32_t>(pHistSub[i]--)) << 1);
			}
			pfRowOut[0] = static_cast<float>(puiSum[iCmin] * Size_obratn);
			pfRowOut++;
		}
	}

	// �������� ����. ��������� ������.
	for (iRow = ar_cmmIn.m_i64H - 3; iRow < ar_cmmOut.m_i64H; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

	if (nullptr != pHistAll)
		delete[] pHistAll;
	if (nullptr != puiSum)
		delete[] puiSum;
	if (nullptr != pbBrightness)
		delete[] pbBrightness;
}


void TextureFiltr_Mean_V8_9_sse4(MU16Data& ar_cmmIn, MFData& ar_cmmOut, double pseudo_min, double kfct)
{
	double Size_obratn = 1.0 / 81;

	// ��� ��� ����������
	uint16_t* pHistAll = new uint16_t[256 * ar_cmmIn.m_i64W]; // [256][ar_cmmIn.m_i64W]
	memset(pHistAll, 0, sizeof(uint16_t) * 256 * ar_cmmIn.m_i64W);
	uint16_t* pHist, * pHistAdd, * pHistSub;

	// ��� ��� ����
	uint32_t* puiSum = new uint32_t[ar_cmmIn.m_i64W], * puiSumCurr;
	memset(puiSum, 0, sizeof(uint32_t) * ar_cmmIn.m_i64W);

	// ��� ��� ��������
	uint8_t* pbBrightness = new uint8_t[ar_cmmIn.m_i64W * 9];
	uint8_t* pbBrightnessRow;

	int64_t iRow, iCol, iColMax = ar_cmmIn.m_i64W - 9;
	ar_cmmOut.iCreate(ar_cmmIn.m_i64W, ar_cmmIn.m_i64H, 6);

	// �������� ����. ������ ������.
	for (iRow = 0; iRow < 4; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

	int64_t i, iCmin, iCmax, iPos = 0;
	uint32_t u32ValueAdd, u32ValueSub;
	double d;

	// ������ [0, 9 - 1] ������
	pbBrightnessRow = pbBrightness;
	float* pfRowOut = ar_cmmOut.pfGetRow(4) + 4;
	for (iRow = 0; iRow < 9; iRow++, pbBrightnessRow += ar_cmmIn.m_i64W)
	{
		// �������� ���� �����.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * 4);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - 4), 0, sizeof(float) * 4);

		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);

		// ������ [0, 2*4[ �������
		for (iCol = 0; iCol < (4 << 1); iCol++)
		{
			iCmin = max(4, static_cast<int>(iCol - 4));
			iCmax = iCol + 4;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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
				// ��������� ������� � 9 ���������� � ����
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
		}

		// ������� [2*4, ar_cmmIn.m_i64W - 9] �������
		puiSumCurr = puiSum + 4;
		for (; iCol <= iColMax; iCol++, puiSumCurr++)
		{
			iCmin = iCol - 4;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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
				// ��������� ������� � 9 ���������� � ����
				for (i = 0; i < 9; i++)
					puiSumCurr[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
			if ((9 - 1) == iRow)
			{
				pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
				pfRowOut++;
			}
		}

		// ��������� [ar_cmmIn.m_i64W - 2*4, ar_cmmIn.m_i64W - 1] �������
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - 4;
			iCmax = min(static_cast<int>(ar_cmmIn.m_i64W - 4 - 1), static_cast<int>(iCol + 4));

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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
				// ��������� ������� � 9 ���������� � ����
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

	__m128i xmmHistOne_1 = _mm_set_epi16(0, 0, 0, 0, 0, 0, 0, 1); // ��� inc ��� dec ����� ��� 1 ����������
	__m128i xmmHistOne_All = _mm_set_epi16(1, 1, 1, 1, 1, 1, 1, 1); // ��� inc ��� dec ����� ��� ���� ����������

	// ����������� ������ [9, ar_cmmIn.m_i64H[
	for (iRow = 9; iRow < ar_cmmIn.m_i64H; iRow++)
	{
		// �������� ���� �����.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * 4);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - 4), 0, sizeof(float) * 4);

		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);
		float* pfRowOut = ar_cmmOut.pfGetRow(iRow - 4) + 4;
		iPos = (iRow - 9) % 9;
		pbBrightnessRow = pbBrightness + (iPos * ar_cmmIn.m_i64W);

		// ������ [0, 2*4[ �������
		for (iCol = 0; iCol < (4 << 1); iCol++)
		{
			iCmin = max(4, static_cast<int>(iCol - 4));
			iCmax = iCol + 4;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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

				// ��������� ������� � 9 ���������� � ����
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAddSub + ((u32ValueAdd * static_cast<uint32_t>(pHistAdd[i]++) - u32ValueSub * static_cast<uint32_t>(pHistSub[i]--)) << 1);
			}
		}

		// ������� [2*4, ar_cmmIn.m_i64W - 9] �������
		puiSumCurr = puiSum + 4;
		pHist = pHistAll + (iCol - 4);
		for (; iCol <= iColMax; iCol++, puiSumCurr++, pHist++)
		{
			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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

				// ��������� ������� � 9 ���������� � ����
				//============================
				// ������ 8 �����
				//============================
				__m128i xmmHistSub = _mm_lddqu_si128(reinterpret_cast<__m128i*>(pHistSub));// ��������� 128 ��� (���������� 128 ��� - 8 ��. �������)
				__m128i xmmHistAdd = _mm_lddqu_si128(reinterpret_cast<__m128i*>(pHistAdd));

				// ������� 4 �������� �����������
				__m128i xmmSumCurr = _mm_lddqu_si128(reinterpret_cast<__m128i*> (puiSumCurr)); // ��������� 128 ��� (���������� 128 ��� - 4 ��. �������)

				__m128i xmmHistSub32 = _mm_cvtepu16_epi32(xmmHistSub); // HistSub32[3], HistSub32[2], HistSub32[1], HistSub32[0](Hi < ->Lo)
				__m128i xmmHistAdd32 = _mm_cvtepu16_epi32(xmmHistAdd);

				__m128i xmmValueAdd = _mm_loadu_si32(&u32ValueAdd); // 0, 0, 0, u32ValueAdd(Hi < ->Lo)
				__m128i xmmValueSub = _mm_loadu_si32(&u32ValueSub);

				xmmValueAdd = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(xmmValueAdd), _mm_castsi128_ps(xmmValueAdd), 0x0)); // 0, u32ValueAdd, u32ValueAdd, u32ValueAdd
				xmmValueSub = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(xmmValueSub), _mm_castsi128_ps(xmmValueSub), 0x0));

				__m128i xmmValueAddSub = _mm_add_epi32(xmmValueAdd, xmmValueSub);

				xmmHistAdd32 = _mm_sub_epi32(_mm_mullo_epi32(xmmHistAdd32, xmmValueAdd), _mm_mullo_epi32(xmmHistSub32, xmmValueSub));
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmHistAdd32); // <<= 1
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmValueAddSub);

				_mm_storeu_si128(reinterpret_cast<__m128i*>(puiSumCurr), _mm_add_epi32(xmmSumCurr, xmmHistAdd32)); // ��������� puiSumCurr[0,1,2,3]

				// ������� 4 �������� �����������
				xmmSumCurr = _mm_lddqu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 4)); // ��������� 128 ��� (���������� 128 - 4 ��. �������)

				xmmHistSub32 = _mm_cvtepu16_epi32(_mm_bsrli_si128(xmmHistSub, 8));// HistSub32[7], HistSub32[6], HistSub32[5], HistSub32[4] (Hi <-> Lo)
				xmmHistAdd32 = _mm_cvtepu16_epi32(_mm_bsrli_si128(xmmHistAdd, 8));

				// ������������ �������� �����������
				_mm_storeu_si128(reinterpret_cast<__m128i*>(pHistAdd), _mm_add_epi16(xmmHistAdd, xmmHistOne_All)); // pHistAdd[0-7]++; ��������� 128 ��� (���������� 128 - 8 ��. �������)
				_mm_storeu_si128(reinterpret_cast<__m128i*>(pHistSub), _mm_sub_epi16(xmmHistSub, xmmHistOne_All)); // pHistSub[0-7]--; ��������� 128 ��� (���������� 128 - 8 ��. �������)

				xmmHistAdd32 = _mm_sub_epi32(_mm_mullo_epi32(xmmHistAdd32, xmmValueAdd), _mm_mullo_epi32(xmmHistSub32, xmmValueSub));
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmHistAdd32); // <<= 	1
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmValueAddSub);

				_mm_storeu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 4), _mm_add_epi32(xmmSumCurr, xmmHistAdd32)); // ��������� puiSumCurr[0,1,2,3]
				//============================
				// ��������� 1 �����
				//============================
				xmmHistSub = _mm_lddqu_si128(reinterpret_cast<__m128i*>(pHistSub + 8)); // ��������� 128 ��� (���������� 16 ��� - 1 ��. �������)
				xmmHistAdd = _mm_lddqu_si128(reinterpret_cast<__m128i*>(pHistAdd + 8));

				// 1 ������� �����������
				xmmSumCurr = _mm_lddqu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 8)); // ��������� 128 ��� (���������� 32 ��� - 1 ��. �������)

				xmmHistSub32 = _mm_cvtepu16_epi32(xmmHistSub); // HistSub32[3], HistSub32[2], HistSub32[1], HistSub32[0](Hi < ->Lo)
				xmmHistAdd32 = _mm_cvtepu16_epi32(xmmHistAdd);

				xmmValueAdd = _mm_bsrli_si128(xmmValueAdd, 12); // 0, 0, 0, xmmValueAdd(Hi < ->Lo)
				xmmValueSub = _mm_bsrli_si128(xmmValueSub, 12); // 0, 0, 0, xmmValueSub(Hi < ->Lo)
				xmmValueAddSub = _mm_bsrli_si128(xmmValueAddSub, 12);

				xmmHistAdd32 = _mm_sub_epi32(_mm_mullo_epi32(xmmHistAdd32, xmmValueAdd), _mm_mullo_epi32(xmmHistSub32, xmmValueSub));
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmHistAdd32); // <<= 1
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmValueAddSub);

				_mm_storeu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 8), _mm_add_epi32(xmmSumCurr, xmmHistAdd32)); // ��������� puiSumCurr[0,1,2,3]
				_mm_storeu_si128(reinterpret_cast<__m128i*>(pHistAdd + 8), _mm_add_epi16(xmmHistAdd, xmmHistOne_1)); // pHistAdd[0-0]++; ��������� 128 ��� (���������� 128 - 8 ��. �������)
				_mm_storeu_si128(reinterpret_cast<__m128i*>(pHistSub + 8), _mm_sub_epi16(xmmHistSub, xmmHistOne_1)); // pHistSub[0-0]--; ��������� 128 ��� (���������� 128 - 8 ��. �������)
			}
			pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
			pfRowOut++;
		}

		// ��������� [ar_cmmIn.m_i64W - 2*4, ar_cmmIn.m_i64W - 1] �������
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - 4;
			iCmax = min(static_cast<int>(ar_cmmIn.m_i64W - 4 - 1), static_cast<int>(iCol + 4));

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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

				// ��������� ������� � 9 ���������� � ����
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAddSub + ((u32ValueAdd * static_cast<uint32_t>(pHistAdd[i]++) - u32ValueSub * static_cast<uint32_t>(pHistSub[i]--)) << 1);
			}
			pfRowOut[0] = static_cast<float>(puiSum[iCmin] * Size_obratn);
			pfRowOut++;
		}
	}

	// �������� ����. ��������� ������.
	for (iRow = ar_cmmIn.m_i64H - 4; iRow < ar_cmmOut.m_i64H; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

	if (nullptr != pHistAll)
		delete[] pHistAll;
	if (nullptr != puiSum)
		delete[] puiSum;
	if (nullptr != pbBrightness)
		delete[] pbBrightness;
}


void TextureFiltr_Mean_V8_11_sse4(MU16Data& ar_cmmIn, MFData& ar_cmmOut, double pseudo_min, double kfct)
{
	double Size_obratn = 1.0 / 121;

	// ��� ��� ����������
	uint16_t* pHistAll = new uint16_t[256 * ar_cmmIn.m_i64W]; // [256][ar_cmmIn.m_i64W]
	memset(pHistAll, 0, sizeof(uint16_t) * 256 * ar_cmmIn.m_i64W);
	uint16_t* pHist, * pHistAdd, * pHistSub;

	// ��� ��� ����
	uint32_t* puiSum = new uint32_t[ar_cmmIn.m_i64W], * puiSumCurr;
	memset(puiSum, 0, sizeof(uint32_t) * ar_cmmIn.m_i64W);

	// ��� ��� ��������
	uint8_t* pbBrightness = new uint8_t[ar_cmmIn.m_i64W * 11];
	uint8_t* pbBrightnessRow;

	int64_t iRow, iCol, iColMax = ar_cmmIn.m_i64W - 11;
	ar_cmmOut.iCreate(ar_cmmIn.m_i64W, ar_cmmIn.m_i64H, 6);

	// �������� ����. ������ ������.
	for (iRow = 0; iRow < 5; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

	int64_t i, iCmin, iCmax, iPos = 0;
	uint32_t u32ValueAdd, u32ValueSub;
	double d;

	// ������ [0, 11 - 1] ������
	pbBrightnessRow = pbBrightness;
	float* pfRowOut = ar_cmmOut.pfGetRow(5) + 5;
	for (iRow = 0; iRow < 11; iRow++, pbBrightnessRow += ar_cmmIn.m_i64W)
	{
		// �������� ���� �����.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * 5);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - 5), 0, sizeof(float) * 5);

		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);

		// ������ [0, 2*5[ �������
		for (iCol = 0; iCol < (5 << 1); iCol++)
		{
			iCmin = max(5, static_cast<int>(iCol - 5));
			iCmax = iCol + 5;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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
				// ��������� ������� � 11 ���������� � ����
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
		}

		// ������� [2*5, ar_cmmIn.m_i64W - 11] �������
		puiSumCurr = puiSum + 5;
		for (; iCol <= iColMax; iCol++, puiSumCurr++)
		{
			iCmin = iCol - 5;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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
				// ��������� ������� � 11 ���������� � ����
				for (i = 0; i < 11; i++)
					puiSumCurr[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
			if ((11 - 1) == iRow)
			{
				pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
				pfRowOut++;
			}
		}

		// ��������� [ar_cmmIn.m_i64W - 2*5, ar_cmmIn.m_i64W - 1] �������
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - 5;
			iCmax = min(static_cast<int>(ar_cmmIn.m_i64W - 5 - 1), static_cast<int>(iCol + 5));

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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
				// ��������� ������� � 11 ���������� � ����
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

	__m128i xmmHistOne_3 = _mm_set_epi16(0, 0, 0, 0, 0, 1, 1, 1); // ��� inc ��� dec ����� ��� 3 ����������
	__m128i xmmHistOne_All = _mm_set_epi16(1, 1, 1, 1, 1, 1, 1, 1); // ��� inc ��� dec ����� ��� ���� ����������

	// ����������� ������ [11, ar_cmmIn.m_i64H[
	for (iRow = 11; iRow < ar_cmmIn.m_i64H; iRow++)
	{
		// �������� ���� �����.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * 5);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - 5), 0, sizeof(float) * 5);

		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);
		float* pfRowOut = ar_cmmOut.pfGetRow(iRow - 5) + 5;
		iPos = (iRow - 11) % 11;
		pbBrightnessRow = pbBrightness + (iPos * ar_cmmIn.m_i64W);

		// ������ [0, 2*5[ �������
		for (iCol = 0; iCol < (5 << 1); iCol++)
		{
			iCmin = max(5, static_cast<int>(iCol - 5));
			iCmax = iCol + 5;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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

				// ��������� ������� � 11 ���������� � ����
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAddSub + ((u32ValueAdd * static_cast<uint32_t>(pHistAdd[i]++) - u32ValueSub * static_cast<uint32_t>(pHistSub[i]--)) << 1);
			}
		}

		// ������� [2*5, ar_cmmIn.m_i64W - 11] �������
		puiSumCurr = puiSum + 5;
		pHist = pHistAll + (iCol - 5);
		for (; iCol <= iColMax; iCol++, puiSumCurr++, pHist++)
		{
			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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

				// ��������� ������� � 11 ���������� � ����
				//============================
				// ������ 8 �����
				//============================
				__m128i xmmHistSub = _mm_lddqu_si128(reinterpret_cast<__m128i*>(pHistSub));// ��������� 128 ��� (���������� 128 ��� - 8 ��. �������)
				__m128i xmmHistAdd = _mm_lddqu_si128(reinterpret_cast<__m128i*>(pHistAdd));

				// ������� 4 �������� �����������
				__m128i xmmSumCurr = _mm_lddqu_si128(reinterpret_cast<__m128i*> (puiSumCurr)); // ��������� 128 ��� (���������� 128 ��� - 4 ��. �������)

				__m128i xmmHistSub32 = _mm_cvtepu16_epi32(xmmHistSub); // HistSub32[3], HistSub32[2], HistSub32[1], HistSub32[0](Hi < ->Lo)
				__m128i xmmHistAdd32 = _mm_cvtepu16_epi32(xmmHistAdd);

				__m128i xmmValueAdd = _mm_loadu_si32(&u32ValueAdd); // 0, 0, 0, u32ValueAdd(Hi < ->Lo)
				__m128i xmmValueSub = _mm_loadu_si32(&u32ValueSub);

				xmmValueAdd = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(xmmValueAdd), _mm_castsi128_ps(xmmValueAdd), 0x0)); // 0, u32ValueAdd, u32ValueAdd, u32ValueAdd
				xmmValueSub = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(xmmValueSub), _mm_castsi128_ps(xmmValueSub), 0x0));

				__m128i xmmValueAddSub = _mm_add_epi32(xmmValueAdd, xmmValueSub);

				xmmHistAdd32 = _mm_sub_epi32(_mm_mullo_epi32(xmmHistAdd32, xmmValueAdd), _mm_mullo_epi32(xmmHistSub32, xmmValueSub));
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmHistAdd32); // <<= 1
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmValueAddSub);

				_mm_storeu_si128(reinterpret_cast<__m128i*>(puiSumCurr), _mm_add_epi32(xmmSumCurr, xmmHistAdd32)); // ��������� puiSumCurr[0,1,2,3]

				// ������� 4 �������� �����������
				xmmSumCurr = _mm_lddqu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 4)); // ��������� 128 ��� (���������� 128 - 4 ��. �������)

				xmmHistSub32 = _mm_cvtepu16_epi32(_mm_bsrli_si128(xmmHistSub, 8));// HistSub32[7], HistSub32[6], HistSub32[5], HistSub32[4] (Hi <-> Lo)
				xmmHistAdd32 = _mm_cvtepu16_epi32(_mm_bsrli_si128(xmmHistAdd, 8));

				// ������������ �������� �����������
				_mm_storeu_si128(reinterpret_cast<__m128i*>(pHistAdd), _mm_add_epi16(xmmHistAdd, xmmHistOne_All)); // pHistAdd[0-7]++; ��������� 128 ��� (���������� 128 - 8 ��. �������)
				_mm_storeu_si128(reinterpret_cast<__m128i*>(pHistSub), _mm_sub_epi16(xmmHistSub, xmmHistOne_All)); // pHistSub[0-7]--; ��������� 128 ��� (���������� 128 - 8 ��. �������)

				xmmHistAdd32 = _mm_sub_epi32(_mm_mullo_epi32(xmmHistAdd32, xmmValueAdd), _mm_mullo_epi32(xmmHistSub32, xmmValueSub));
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmHistAdd32); // <<= 	1
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmValueAddSub);

				_mm_storeu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 4), _mm_add_epi32(xmmSumCurr, xmmHistAdd32)); // ��������� puiSumCurr[0,1,2,3]
				//============================
				// ��������� 3 �����
				//============================
				xmmHistSub = _mm_lddqu_si128(reinterpret_cast<__m128i*>(pHistSub + 8)); // ��������� 128 ��� (���������� 48 ��� - 3 ��. �������)
				xmmHistAdd = _mm_lddqu_si128(reinterpret_cast<__m128i*>(pHistAdd + 8));

				// 3 �������� �����������
				xmmSumCurr = _mm_lddqu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 8)); // ��������� 128 ��� (���������� 96 ��� - 3 ��. �������)

				xmmHistSub32 = _mm_cvtepu16_epi32(xmmHistSub); // HistSub32[3], HistSub32[2], HistSub32[1], HistSub32[0](Hi < ->Lo)
				xmmHistAdd32 = _mm_cvtepu16_epi32(xmmHistAdd);

				xmmValueAdd = _mm_bsrli_si128(xmmValueAdd, 4); // 0, xmmValueAdd, xmmValueAdd, xmmValueAdd(Hi < ->Lo)
				xmmValueSub = _mm_bsrli_si128(xmmValueSub, 4); // 0, xmmValueSub, xmmValueSub, xmmValueSub(Hi < ->Lo)
				xmmValueAddSub = _mm_bsrli_si128(xmmValueAddSub, 4);

				xmmHistAdd32 = _mm_sub_epi32(_mm_mullo_epi32(xmmHistAdd32, xmmValueAdd), _mm_mullo_epi32(xmmHistSub32, xmmValueSub));
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmHistAdd32); // <<= 1
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmValueAddSub);

				_mm_storeu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 8), _mm_add_epi32(xmmSumCurr, xmmHistAdd32)); // ��������� puiSumCurr[0,1,2,3]
				_mm_storeu_si128(reinterpret_cast<__m128i*>(pHistAdd + 8), _mm_add_epi16(xmmHistAdd, xmmHistOne_3)); // pHistAdd[0-2]++; ��������� 128 ��� (���������� 128 - 8 ��. �������)
				_mm_storeu_si128(reinterpret_cast<__m128i*>(pHistSub + 8), _mm_sub_epi16(xmmHistSub, xmmHistOne_3)); // pHistSub[0-2]--; ��������� 128 ��� (���������� 128 - 8 ��. �������)
			}
			pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
			pfRowOut++;
		}

		// ��������� [ar_cmmIn.m_i64W - 2*5, ar_cmmIn.m_i64W - 1] �������
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - 5;
			iCmax = min(static_cast<int>(ar_cmmIn.m_i64W - 5 - 1), static_cast<int>(iCol + 5));

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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

				// ��������� ������� � 11 ���������� � ����
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAddSub + ((u32ValueAdd * static_cast<uint32_t>(pHistAdd[i]++) - u32ValueSub * static_cast<uint32_t>(pHistSub[i]--)) << 1);
			}
			pfRowOut[0] = static_cast<float>(puiSum[iCmin] * Size_obratn);
			pfRowOut++;
		}
	}

	// �������� ����. ��������� ������.
	for (iRow = ar_cmmIn.m_i64H - 5; iRow < ar_cmmOut.m_i64H; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

	if (nullptr != pHistAll)
		delete[] pHistAll;
	if (nullptr != puiSum)
		delete[] puiSum;
	if (nullptr != pbBrightness)
		delete[] pbBrightness;
}


void TextureFiltr_Mean_V8_13_sse4(MU16Data& ar_cmmIn, MFData& ar_cmmOut, double pseudo_min, double kfct)
{
	double Size_obratn = 1.0 / 169;

	// ��� ��� ����������
	uint16_t* pHistAll = new uint16_t[256 * ar_cmmIn.m_i64W]; // [256][ar_cmmIn.m_i64W]
	memset(pHistAll, 0, sizeof(uint16_t) * 256 * ar_cmmIn.m_i64W);
	uint16_t* pHist, * pHistAdd, * pHistSub;

	// ��� ��� ����
	uint32_t* puiSum = new uint32_t[ar_cmmIn.m_i64W], * puiSumCurr;
	memset(puiSum, 0, sizeof(uint32_t) * ar_cmmIn.m_i64W);

	// ��� ��� ��������
	uint8_t* pbBrightness = new uint8_t[ar_cmmIn.m_i64W * 13];
	uint8_t* pbBrightnessRow;

	int64_t iRow, iCol, iColMax = ar_cmmIn.m_i64W - 13;
	ar_cmmOut.iCreate(ar_cmmIn.m_i64W, ar_cmmIn.m_i64H, 6);

	// �������� ����. ������ ������.
	for (iRow = 0; iRow < 6; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

	int64_t i, iCmin, iCmax, iPos = 0;
	uint32_t u32ValueAdd, u32ValueSub;
	double d;

	// ������ [0, 13 - 1] ������
	pbBrightnessRow = pbBrightness;
	float* pfRowOut = ar_cmmOut.pfGetRow(6) + 6;
	for (iRow = 0; iRow < 13; iRow++, pbBrightnessRow += ar_cmmIn.m_i64W)
	{
		// �������� ���� �����.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * 6);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - 6), 0, sizeof(float) * 6);

		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);

		// ������ [0, 2*6[ �������
		for (iCol = 0; iCol < (6 << 1); iCol++)
		{
			iCmin = max(6, static_cast<int>(iCol - 6));
			iCmax = iCol + 6;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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
				// ��������� ������� � 13 ���������� � ����
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
		}

		// ������� [2*6, ar_cmmIn.m_i64W - 13] �������
		puiSumCurr = puiSum + 6;
		for (; iCol <= iColMax; iCol++, puiSumCurr++)
		{
			iCmin = iCol - 6;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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
				// ��������� ������� � 13 ���������� � ����
				for (i = 0; i < 13; i++)
					puiSumCurr[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
			if ((13 - 1) == iRow)
			{
				pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
				pfRowOut++;
			}
		}

		// ��������� [ar_cmmIn.m_i64W - 2*6, ar_cmmIn.m_i64W - 1] �������
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - 6;
			iCmax = min(static_cast<int>(ar_cmmIn.m_i64W - 6 - 1), static_cast<int>(iCol + 6));

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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
				// ��������� ������� � 13 ���������� � ����
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

	__m128i xmmHistOne_5 = _mm_set_epi16(0, 0, 0, 1, 1, 1, 1, 1); // ��� inc ��� dec ����� ��� 5 ����������
	__m128i xmmHistOne_All = _mm_set_epi16(1, 1, 1, 1, 1, 1, 1, 1); // ��� inc ��� dec ����� ��� ���� ����������

	// ����������� ������ [13, ar_cmmIn.m_i64H[
	for (iRow = 13; iRow < ar_cmmIn.m_i64H; iRow++)
	{
		// �������� ���� �����.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * 6);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - 6), 0, sizeof(float) * 6);

		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);
		float* pfRowOut = ar_cmmOut.pfGetRow(iRow - 6) + 6;
		iPos = (iRow - 13) % 13;
		pbBrightnessRow = pbBrightness + (iPos * ar_cmmIn.m_i64W);

		// ������ [0, 2*6[ �������
		for (iCol = 0; iCol < (6 << 1); iCol++)
		{
			iCmin = max(6, static_cast<int>(iCol - 6));
			iCmax = iCol + 6;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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

				// ��������� ������� � 13 ���������� � ����
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAddSub + ((u32ValueAdd * static_cast<uint32_t>(pHistAdd[i]++) - u32ValueSub * static_cast<uint32_t>(pHistSub[i]--)) << 1);
			}
		}

		// ������� [2*6, ar_cmmIn.m_i64W - 13] �������
		puiSumCurr = puiSum + 6;
		pHist = pHistAll + (iCol - 6);
		for (; iCol <= iColMax; iCol++, puiSumCurr++, pHist++)
		{
			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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

				// ��������� ������� � 13 ���������� � ����
				//============================
				// ������ 8 �����
				//============================
				__m128i xmmHistSub = _mm_lddqu_si128(reinterpret_cast<__m128i*>(pHistSub));// ��������� 128 ��� (���������� 128 ��� - 8 ��. �������)
				__m128i xmmHistAdd = _mm_lddqu_si128(reinterpret_cast<__m128i*>(pHistAdd));

				// ������� 4 �������� �����������
				__m128i xmmSumCurr = _mm_lddqu_si128(reinterpret_cast<__m128i*> (puiSumCurr)); // ��������� 128 ��� (���������� 128 ��� - 4 ��. �������)

				__m128i xmmHistSub32 = _mm_cvtepu16_epi32(xmmHistSub); // HistSub32[3], HistSub32[2], HistSub32[1], HistSub32[0](Hi < ->Lo)
				__m128i xmmHistAdd32 = _mm_cvtepu16_epi32(xmmHistAdd);

				__m128i xmmValueAdd = _mm_loadu_si32(&u32ValueAdd); // 0, 0, 0, u32ValueAdd(Hi < ->Lo)
				__m128i xmmValueSub = _mm_loadu_si32(&u32ValueSub);

				xmmValueAdd = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(xmmValueAdd), _mm_castsi128_ps(xmmValueAdd), 0x0)); // 0, u32ValueAdd, u32ValueAdd, u32ValueAdd
				xmmValueSub = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(xmmValueSub), _mm_castsi128_ps(xmmValueSub), 0x0));

				__m128i xmmValueAddSub = _mm_add_epi32(xmmValueAdd, xmmValueSub);

				xmmHistAdd32 = _mm_sub_epi32(_mm_mullo_epi32(xmmHistAdd32, xmmValueAdd), _mm_mullo_epi32(xmmHistSub32, xmmValueSub));
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmHistAdd32); // <<= 1
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmValueAddSub);

				_mm_storeu_si128(reinterpret_cast<__m128i*>(puiSumCurr), _mm_add_epi32(xmmSumCurr, xmmHistAdd32)); // ��������� puiSumCurr[0,1,2,3]

				// ������� 4 �������� �����������
				xmmSumCurr = _mm_lddqu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 4)); // ��������� 128 ��� (���������� 128 - 4 ��. �������)

				xmmHistSub32 = _mm_cvtepu16_epi32(_mm_bsrli_si128(xmmHistSub, 8));// HistSub32[7], HistSub32[6], HistSub32[5], HistSub32[4] (Hi <-> Lo)
				xmmHistAdd32 = _mm_cvtepu16_epi32(_mm_bsrli_si128(xmmHistAdd, 8));

				// ������������ �������� �����������
				_mm_storeu_si128(reinterpret_cast<__m128i*>(pHistAdd), _mm_add_epi16(xmmHistAdd, xmmHistOne_All)); // pHistAdd[0-7]++; ��������� 128 ��� (���������� 128 - 8 ��. �������)
				_mm_storeu_si128(reinterpret_cast<__m128i*>(pHistSub), _mm_sub_epi16(xmmHistSub, xmmHistOne_All)); // pHistSub[0-7]--; ��������� 128 ��� (���������� 128 - 8 ��. �������)

				xmmHistAdd32 = _mm_sub_epi32(_mm_mullo_epi32(xmmHistAdd32, xmmValueAdd), _mm_mullo_epi32(xmmHistSub32, xmmValueSub));
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmHistAdd32); // <<= 	1
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmValueAddSub);

				_mm_storeu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 4), _mm_add_epi32(xmmSumCurr, xmmHistAdd32)); // ��������� puiSumCurr[0,1,2,3]
				//============================
				// ��������� 5 �����
				//============================
				xmmHistSub = _mm_lddqu_si128(reinterpret_cast<__m128i*>(pHistSub + 8)); // ��������� 128 ��� (���������� 80 ��� - 5 ��. �������)
				xmmHistAdd = _mm_lddqu_si128(reinterpret_cast<__m128i*>(pHistAdd + 8));

				// ������� 4 �������� �����������
				xmmSumCurr = _mm_lddqu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 8)); // ��������� 128 ��� (���������� 128 ��� - 4 ��. �������)

				xmmHistSub32 = _mm_cvtepu16_epi32(xmmHistSub); // HistSub32[3], HistSub32[2], HistSub32[1], HistSub32[0](Hi < ->Lo)
				xmmHistAdd32 = _mm_cvtepu16_epi32(xmmHistAdd);

				xmmHistAdd32 = _mm_sub_epi32(_mm_mullo_epi32(xmmHistAdd32, xmmValueAdd), _mm_mullo_epi32(xmmHistSub32, xmmValueSub));
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmHistAdd32); // <<= 1
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmValueAddSub);

				_mm_storeu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 8), _mm_add_epi32(xmmSumCurr, xmmHistAdd32)); // ��������� puiSumCurr[0,1,2,3]

				// ������� 1 ������� �����������
				xmmSumCurr = _mm_lddqu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 12)); // ��������� 128 ��� (���������� 32 - 1 ��. �������)

				xmmHistSub32 = _mm_cvtepu16_epi32(_mm_bsrli_si128(xmmHistSub, 8));// HistSub32[7], HistSub32[6], HistSub32[5], HistSub32[4] (Hi <-> Lo)
				xmmHistAdd32 = _mm_cvtepu16_epi32(_mm_bsrli_si128(xmmHistAdd, 8));

				// ������������ �������� �����������
				_mm_storeu_si128(reinterpret_cast<__m128i*>(pHistAdd + 8), _mm_add_epi16(xmmHistAdd, xmmHistOne_5)); // pHistAdd[0-4]++; ��������� 128 ��� (���������� 128 - 8 ��. �������)
				_mm_storeu_si128(reinterpret_cast<__m128i*>(pHistSub + 8), _mm_sub_epi16(xmmHistSub, xmmHistOne_5)); // pHistSub[0-4]--; ��������� 128 ��� (���������� 128 - 8 ��. �������)

				xmmValueAdd = _mm_bsrli_si128(xmmValueAdd, 12); // 0, 0, 0, xmmValueAdd(Hi < ->Lo)
				xmmValueSub = _mm_bsrli_si128(xmmValueSub, 12); // 0, 0, 	0, xmmValueSub(Hi < ->Lo)
				xmmValueAddSub = _mm_bsrli_si128(xmmValueAddSub, 12);

				xmmHistAdd32 = _mm_sub_epi32(_mm_mullo_epi32(xmmHistAdd32, xmmValueAdd), _mm_mullo_epi32(xmmHistSub32, xmmValueSub));
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmHistAdd32); // <<= 	1
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmValueAddSub);

				_mm_storeu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 12), _mm_add_epi32(xmmSumCurr, xmmHistAdd32)); // ��������� puiSumCurr[0,1,2,3]
			}
			pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
			pfRowOut++;
		}

		// ��������� [ar_cmmIn.m_i64W - 2*6, ar_cmmIn.m_i64W - 1] �������
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - 6;
			iCmax = min(static_cast<int>(ar_cmmIn.m_i64W - 6 - 1), static_cast<int>(iCol + 6));

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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

				// ��������� ������� � 13 ���������� � ����
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAddSub + ((u32ValueAdd * static_cast<uint32_t>(pHistAdd[i]++) - u32ValueSub * static_cast<uint32_t>(pHistSub[i]--)) << 1);
			}
			pfRowOut[0] = static_cast<float>(puiSum[iCmin] * Size_obratn);
			pfRowOut++;
		}
	}

	// �������� ����. ��������� ������.
	for (iRow = ar_cmmIn.m_i64H - 6; iRow < ar_cmmOut.m_i64H; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

	if (nullptr != pHistAll)
		delete[] pHistAll;
	if (nullptr != puiSum)
		delete[] puiSum;
	if (nullptr != pbBrightness)
		delete[] pbBrightness;
}


void TextureFiltr_Mean_V8_15_sse4(MU16Data& ar_cmmIn, MFData& ar_cmmOut, double pseudo_min, double kfct)
{
	double Size_obratn = 1.0 / 225;

	// ��� ��� ����������
	uint16_t* pHistAll = new uint16_t[256 * ar_cmmIn.m_i64W]; // [256][ar_cmmIn.m_i64W]
	memset(pHistAll, 0, sizeof(uint16_t) * 256 * ar_cmmIn.m_i64W);
	uint16_t* pHist, * pHistAdd, * pHistSub;

	// ��� ��� ����
	uint32_t* puiSum = new uint32_t[ar_cmmIn.m_i64W], * puiSumCurr;
	memset(puiSum, 0, sizeof(uint32_t) * ar_cmmIn.m_i64W);

	// ��� ��� ��������
	uint8_t* pbBrightness = new uint8_t[ar_cmmIn.m_i64W * 15];
	uint8_t* pbBrightnessRow;

	int64_t iRow, iCol, iColMax = ar_cmmIn.m_i64W - 15;
	ar_cmmOut.iCreate(ar_cmmIn.m_i64W, ar_cmmIn.m_i64H, 6);

	// �������� ����. ������ ������.
	for (iRow = 0; iRow < 7; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

	int64_t i, iCmin, iCmax, iPos = 0;
	uint32_t u32ValueAdd, u32ValueSub;
	double d;

	// ������ [0, 15 - 1] ������
	pbBrightnessRow = pbBrightness;
	float* pfRowOut = ar_cmmOut.pfGetRow(7) + 7;
	for (iRow = 0; iRow < 15; iRow++, pbBrightnessRow += ar_cmmIn.m_i64W)
	{
		// �������� ���� �����.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * 7);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - 7), 0, sizeof(float) * 7);

		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);

		// ������ [0, 2*7[ �������
		for (iCol = 0; iCol < (7 << 1); iCol++)
		{
			iCmin = max(7, static_cast<int>(iCol - 7));
			iCmax = iCol + 7;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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
				// ��������� ������� � 15 ���������� � ����
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
		}

		// ������� [2*7, ar_cmmIn.m_i64W - 15] �������
		puiSumCurr = puiSum + 7;
		for (; iCol <= iColMax; iCol++, puiSumCurr++)
		{
			iCmin = iCol - 7;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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
				// ��������� ������� � 15 ���������� � ����
				for (i = 0; i < 15; i++)
					puiSumCurr[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
			if ((15 - 1) == iRow)
			{
				pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
				pfRowOut++;
			}
		}

		// ��������� [ar_cmmIn.m_i64W - 2*7, ar_cmmIn.m_i64W - 1] �������
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - 7;
			iCmax = min(static_cast<int>(ar_cmmIn.m_i64W - 7 - 1), static_cast<int>(iCol + 7));

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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
				// ��������� ������� � 15 ���������� � ����
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

	__m128i xmmHistOne_7 = _mm_set_epi16(0, 1, 1, 1, 1, 1, 1, 1); // ��� inc ��� dec ����� ��� 7 ����������
	__m128i xmmHistOne_All = _mm_set_epi16(1, 1, 1, 1, 1, 1, 1, 1); // ��� inc ��� dec ����� ��� ���� ����������

	// ����������� ������ [15, ar_cmmIn.m_i64H[
	for (iRow = 15; iRow < ar_cmmIn.m_i64H; iRow++)
	{
		// �������� ���� �����.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * 7);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - 7), 0, sizeof(float) * 7);

		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);
		float* pfRowOut = ar_cmmOut.pfGetRow(iRow - 7) + 7;
		iPos = (iRow - 15) % 15;
		pbBrightnessRow = pbBrightness + (iPos * ar_cmmIn.m_i64W);

		// ������ [0, 2*7[ �������
		for (iCol = 0; iCol < (7 << 1); iCol++)
		{
			iCmin = max(7, static_cast<int>(iCol - 7));
			iCmax = iCol + 7;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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

				// ��������� ������� � 15 ���������� � ����
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAddSub + ((u32ValueAdd * static_cast<uint32_t>(pHistAdd[i]++) - u32ValueSub * static_cast<uint32_t>(pHistSub[i]--)) << 1);
			}
		}

		// ������� [2*7, ar_cmmIn.m_i64W - 15] �������
		puiSumCurr = puiSum + 7;
		pHist = pHistAll + (iCol - 7);
		for (; iCol <= iColMax; iCol++, puiSumCurr++, pHist++)
		{
			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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

				// ��������� ������� � 15 ���������� � ����
				//============================
				// ������ 8 �����
				//============================
				__m128i xmmHistSub = _mm_lddqu_si128(reinterpret_cast<__m128i*>(pHistSub));// ��������� 128 ��� (���������� 128 ��� - 8 ��. �������)
				__m128i xmmHistAdd = _mm_lddqu_si128(reinterpret_cast<__m128i*>(pHistAdd));

				// ������� 4 �������� �����������
				__m128i xmmSumCurr = _mm_lddqu_si128(reinterpret_cast<__m128i*> (puiSumCurr)); // ��������� 128 ��� (���������� 128 ��� - 4 ��. �������)

				__m128i xmmHistSub32 = _mm_cvtepu16_epi32(xmmHistSub); // HistSub32[3], HistSub32[2], HistSub32[1], HistSub32[0](Hi < ->Lo)
				__m128i xmmHistAdd32 = _mm_cvtepu16_epi32(xmmHistAdd);

				__m128i xmmValueAdd = _mm_loadu_si32(&u32ValueAdd); // 0, 0, 0, u32ValueAdd(Hi < ->Lo)
				__m128i xmmValueSub = _mm_loadu_si32(&u32ValueSub);

				xmmValueAdd = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(xmmValueAdd), _mm_castsi128_ps(xmmValueAdd), 0x0)); // 0, u32ValueAdd, u32ValueAdd, u32ValueAdd
				xmmValueSub = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(xmmValueSub), _mm_castsi128_ps(xmmValueSub), 0x0));

				__m128i xmmValueAddSub = _mm_add_epi32(xmmValueAdd, xmmValueSub);

				xmmHistAdd32 = _mm_sub_epi32(_mm_mullo_epi32(xmmHistAdd32, xmmValueAdd), _mm_mullo_epi32(xmmHistSub32, xmmValueSub));
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmHistAdd32); // <<= 1
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmValueAddSub);

				_mm_storeu_si128(reinterpret_cast<__m128i*>(puiSumCurr), _mm_add_epi32(xmmSumCurr, xmmHistAdd32)); // ��������� puiSumCurr[0,1,2,3]

				// ������� 4 �������� �����������
				xmmSumCurr = _mm_lddqu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 4)); // ��������� 128 ��� (���������� 128 - 4 ��. �������)

				xmmHistSub32 = _mm_cvtepu16_epi32(_mm_bsrli_si128(xmmHistSub, 8));// HistSub32[7], HistSub32[6], HistSub32[5], HistSub32[4] (Hi <-> Lo)
				xmmHistAdd32 = _mm_cvtepu16_epi32(_mm_bsrli_si128(xmmHistAdd, 8));

				// ������������ �������� �����������
				_mm_storeu_si128(reinterpret_cast<__m128i*>(pHistAdd), _mm_add_epi16(xmmHistAdd, xmmHistOne_All)); // pHistAdd[0-7]++; ��������� 128 ��� (���������� 128 - 8 ��. �������)
				_mm_storeu_si128(reinterpret_cast<__m128i*>(pHistSub), _mm_sub_epi16(xmmHistSub, xmmHistOne_All)); // pHistSub[0-7]--; ��������� 128 ��� (���������� 128 - 8 ��. �������)

				xmmHistAdd32 = _mm_sub_epi32(_mm_mullo_epi32(xmmHistAdd32, xmmValueAdd), _mm_mullo_epi32(xmmHistSub32, xmmValueSub));
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmHistAdd32); // <<= 	1
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmValueAddSub);

				_mm_storeu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 4), _mm_add_epi32(xmmSumCurr, xmmHistAdd32)); // ��������� puiSumCurr[0,1,2,3]
				//============================
				// ��������� 7 �����
				//============================
				xmmHistSub = _mm_lddqu_si128(reinterpret_cast<__m128i*>(pHistSub + 8)); // ��������� 128 ��� (���������� 112 ��� - 7 ��. �������)
				xmmHistAdd = _mm_lddqu_si128(reinterpret_cast<__m128i*>(pHistAdd + 8));

				// ������� 4 �������� �����������
				xmmSumCurr = _mm_lddqu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 8)); // ��������� 128 ��� (���������� 128 ��� - 4 ��. �������)

				xmmHistSub32 = _mm_cvtepu16_epi32(xmmHistSub); // HistSub32[3], HistSub32[2], HistSub32[1], HistSub32[0](Hi < ->Lo)
				xmmHistAdd32 = _mm_cvtepu16_epi32(xmmHistAdd);

				xmmHistAdd32 = _mm_sub_epi32(_mm_mullo_epi32(xmmHistAdd32, xmmValueAdd), _mm_mullo_epi32(xmmHistSub32, xmmValueSub));
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmHistAdd32); // <<= 1
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmValueAddSub);

				_mm_storeu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 8), _mm_add_epi32(xmmSumCurr, xmmHistAdd32)); // ��������� puiSumCurr[0,1,2,3]

				// ������� 3 �������� �����������
				xmmSumCurr = _mm_lddqu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 12)); // ��������� 128 ��� (���������� 96 - 3 ��. �������)

				xmmHistSub32 = _mm_cvtepu16_epi32(_mm_bsrli_si128(xmmHistSub, 8));// HistSub32[7], HistSub32[6], HistSub32[5], HistSub32[4] (Hi <-> Lo)
				xmmHistAdd32 = _mm_cvtepu16_epi32(_mm_bsrli_si128(xmmHistAdd, 8));

				// ������������ �������� �����������
				_mm_storeu_si128(reinterpret_cast<__m128i*>(pHistAdd + 8), _mm_add_epi16(xmmHistAdd, xmmHistOne_7)); // pHistAdd[0-6]++; ��������� 128 ��� (���������� 128 - 8 ��. �������)
				_mm_storeu_si128(reinterpret_cast<__m128i*>(pHistSub + 8), _mm_sub_epi16(xmmHistSub, xmmHistOne_7)); // pHistSub[0-6]--; ��������� 128 ��� (���������� 128 - 8 ��. �������)

				xmmValueAdd = _mm_bsrli_si128(xmmValueAdd, 4); // 0, xmmValueAdd, xmmValueAdd, xmmValueAdd(Hi < ->Lo)
				xmmValueSub = _mm_bsrli_si128(xmmValueSub, 4); // 0, xmmValueSub, xmmValueSub, xmmValueSub(Hi < ->Lo)
				xmmValueAddSub = _mm_bsrli_si128(xmmValueAddSub, 4);

				xmmHistAdd32 = _mm_sub_epi32(_mm_mullo_epi32(xmmHistAdd32, xmmValueAdd), _mm_mullo_epi32(xmmHistSub32, xmmValueSub));
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmHistAdd32); // <<= 	1
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmValueAddSub);

				_mm_storeu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 12), _mm_add_epi32(xmmSumCurr, xmmHistAdd32)); // ��������� puiSumCurr[0,1,2,3]
			}
			pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
			pfRowOut++;
		}

		// ��������� [ar_cmmIn.m_i64W - 2*7, ar_cmmIn.m_i64W - 1] �������
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - 7;
			iCmax = min(static_cast<int>(ar_cmmIn.m_i64W - 7 - 1), static_cast<int>(iCol + 7));

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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

				// ��������� ������� � 15 ���������� � ����
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAddSub + ((u32ValueAdd * static_cast<uint32_t>(pHistAdd[i]++) - u32ValueSub * static_cast<uint32_t>(pHistSub[i]--)) << 1);
			}
			pfRowOut[0] = static_cast<float>(puiSum[iCmin] * Size_obratn);
			pfRowOut++;
		}
	}

	// �������� ����. ��������� ������.
	for (iRow = ar_cmmIn.m_i64H - 7; iRow < ar_cmmOut.m_i64H; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

	if (nullptr != pHistAll)
		delete[] pHistAll;
	if (nullptr != puiSum)
		delete[] puiSum;
	if (nullptr != pbBrightness)
		delete[] pbBrightness;
}


void TextureFiltr_Mean_V8_17_sse4(MU16Data& ar_cmmIn, MFData& ar_cmmOut, double pseudo_min, double kfct)
{
	double Size_obratn = 1.0 / 289;

	// ��� ��� ����������
	uint16_t* pHistAll = new uint16_t[256 * ar_cmmIn.m_i64W]; // [256][ar_cmmIn.m_i64W]
	memset(pHistAll, 0, sizeof(uint16_t) * 256 * ar_cmmIn.m_i64W);
	uint16_t* pHist, * pHistAdd, * pHistSub;

	// ��� ��� ����
	uint32_t* puiSum = new uint32_t[ar_cmmIn.m_i64W], * puiSumCurr;
	memset(puiSum, 0, sizeof(uint32_t) * ar_cmmIn.m_i64W);

	// ��� ��� ��������
	uint8_t* pbBrightness = new uint8_t[ar_cmmIn.m_i64W * 17];
	uint8_t* pbBrightnessRow;

	int64_t iRow, iCol, iColMax = ar_cmmIn.m_i64W - 17;
	ar_cmmOut.iCreate(ar_cmmIn.m_i64W, ar_cmmIn.m_i64H, 6);

	// �������� ����. ������ ������.
	for (iRow = 0; iRow < 8; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

	int64_t i, iCmin, iCmax, iPos = 0;
	uint32_t u32ValueAdd, u32ValueSub;
	double d;

	// ������ [0, 17 - 1] ������
	pbBrightnessRow = pbBrightness;
	float* pfRowOut = ar_cmmOut.pfGetRow(8) + 8;
	for (iRow = 0; iRow < 17; iRow++, pbBrightnessRow += ar_cmmIn.m_i64W)
	{
		// �������� ���� �����.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * 8);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - 8), 0, sizeof(float) * 8);

		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);

		// ������ [0, 2*8[ �������
		for (iCol = 0; iCol < (8 << 1); iCol++)
		{
			iCmin = max(8, static_cast<int>(iCol - 8));
			iCmax = iCol + 8;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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
				// ��������� ������� � 17 ���������� � ����
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
		}

		// ������� [2*8, ar_cmmIn.m_i64W - 17] �������
		puiSumCurr = puiSum + 8;
		for (; iCol <= iColMax; iCol++, puiSumCurr++)
		{
			iCmin = iCol - 8;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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
				// ��������� ������� � 17 ���������� � ����
				for (i = 0; i < 17; i++)
					puiSumCurr[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
			if ((17 - 1) == iRow)
			{
				pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
				pfRowOut++;
			}
		}

		// ��������� [ar_cmmIn.m_i64W - 2*8, ar_cmmIn.m_i64W - 1] �������
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - 8;
			iCmax = min(static_cast<int>(ar_cmmIn.m_i64W - 8 - 1), static_cast<int>(iCol + 8));

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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
				// ��������� ������� � 17 ���������� � ����
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

	__m128i xmmHistOne_1 = _mm_set_epi16(0, 0, 0, 0, 0, 0, 0, 1); // ��� inc ��� dec ����� ��� 1 ����������
	__m128i xmmHistOne_All = _mm_set_epi16(1, 1, 1, 1, 1, 1, 1, 1); // ��� inc ��� dec ����� ��� ���� ����������

	// ����������� ������ [17, ar_cmmIn.m_i64H[
	for (iRow = 17; iRow < ar_cmmIn.m_i64H; iRow++)
	{
		// �������� ���� �����.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * 8);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - 8), 0, sizeof(float) * 8);

		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);
		float* pfRowOut = ar_cmmOut.pfGetRow(iRow - 8) + 8;
		iPos = (iRow - 17) % 17;
		pbBrightnessRow = pbBrightness + (iPos * ar_cmmIn.m_i64W);

		// ������ [0, 2*8[ �������
		for (iCol = 0; iCol < (8 << 1); iCol++)
		{
			iCmin = max(8, static_cast<int>(iCol - 8));
			iCmax = iCol + 8;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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

				// ��������� ������� � 17 ���������� � ����
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAddSub + ((u32ValueAdd * static_cast<uint32_t>(pHistAdd[i]++) - u32ValueSub * static_cast<uint32_t>(pHistSub[i]--)) << 1);
			}
		}

		// ������� [2*8, ar_cmmIn.m_i64W - 17] �������
		puiSumCurr = puiSum + 8;
		pHist = pHistAll + (iCol - 8);
		for (; iCol <= iColMax; iCol++, puiSumCurr++, pHist++)
		{
			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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

				// ��������� ������� � 17 ���������� � ����
				//============================
				// ������ 8 �����
				//============================
				__m128i xmmHistSub = _mm_lddqu_si128(reinterpret_cast<__m128i*>(pHistSub));// ��������� 128 ��� (���������� 128 ��� - 8 ��. �������)
				__m128i xmmHistAdd = _mm_lddqu_si128(reinterpret_cast<__m128i*>(pHistAdd));

				// ������� 4 �������� �����������
				__m128i xmmSumCurr = _mm_lddqu_si128(reinterpret_cast<__m128i*> (puiSumCurr)); // ��������� 128 ��� (���������� 128 ��� - 4 ��. �������)

				__m128i xmmHistSub32 = _mm_cvtepu16_epi32(xmmHistSub); // HistSub32[3], HistSub32[2], HistSub32[1], HistSub32[0](Hi < ->Lo)
				__m128i xmmHistAdd32 = _mm_cvtepu16_epi32(xmmHistAdd);

				__m128i xmmValueAdd = _mm_loadu_si32(&u32ValueAdd); // 0, 0, 0, u32ValueAdd(Hi < ->Lo)
				__m128i xmmValueSub = _mm_loadu_si32(&u32ValueSub);

				xmmValueAdd = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(xmmValueAdd), _mm_castsi128_ps(xmmValueAdd), 0x0)); // 0, u32ValueAdd, u32ValueAdd, u32ValueAdd
				xmmValueSub = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(xmmValueSub), _mm_castsi128_ps(xmmValueSub), 0x0));

				__m128i xmmValueAddSub = _mm_add_epi32(xmmValueAdd, xmmValueSub);

				xmmHistAdd32 = _mm_sub_epi32(_mm_mullo_epi32(xmmHistAdd32, xmmValueAdd), _mm_mullo_epi32(xmmHistSub32, xmmValueSub));
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmHistAdd32); // <<= 1
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmValueAddSub);

				_mm_storeu_si128(reinterpret_cast<__m128i*>(puiSumCurr), _mm_add_epi32(xmmSumCurr, xmmHistAdd32)); // ��������� puiSumCurr[0,1,2,3]

				// ������� 4 �������� �����������
				xmmSumCurr = _mm_lddqu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 4)); // ��������� 128 ��� (���������� 128 - 4 ��. �������)

				xmmHistSub32 = _mm_cvtepu16_epi32(_mm_bsrli_si128(xmmHistSub, 8));// HistSub32[7], HistSub32[6], HistSub32[5], HistSub32[4] (Hi <-> Lo)
				xmmHistAdd32 = _mm_cvtepu16_epi32(_mm_bsrli_si128(xmmHistAdd, 8));

				// ������������ �������� �����������
				_mm_storeu_si128(reinterpret_cast<__m128i*>(pHistAdd), _mm_add_epi16(xmmHistAdd, xmmHistOne_All)); // pHistAdd[0-7]++; ��������� 128 ��� (���������� 128 - 8 ��. �������)
				_mm_storeu_si128(reinterpret_cast<__m128i*>(pHistSub), _mm_sub_epi16(xmmHistSub, xmmHistOne_All)); // pHistSub[0-7]--; ��������� 128 ��� (���������� 128 - 8 ��. �������)

				xmmHistAdd32 = _mm_sub_epi32(_mm_mullo_epi32(xmmHistAdd32, xmmValueAdd), _mm_mullo_epi32(xmmHistSub32, xmmValueSub));
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmHistAdd32); // <<= 	1
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmValueAddSub);

				_mm_storeu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 4), _mm_add_epi32(xmmSumCurr, xmmHistAdd32)); // ��������� puiSumCurr[0,1,2,3]
				//============================
				// ������ 8 �����
				//============================
				xmmHistSub = _mm_lddqu_si128(reinterpret_cast<__m128i*>(pHistSub + 8)); // ��������� 128 ��� (���������� 128 ��� - 8 ��. �������)
				xmmHistAdd = _mm_lddqu_si128(reinterpret_cast<__m128i*>(pHistAdd + 8));

				// ������� 4 �������� �����������
				xmmSumCurr = _mm_lddqu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 8)); // ��������� 128 ��� (���������� 128 ��� - 4 ��. �������)

				xmmHistSub32 = _mm_cvtepu16_epi32(xmmHistSub); // HistSub32[3], HistSub32[2], HistSub32[1], HistSub32[0](Hi < ->Lo)
				xmmHistAdd32 = _mm_cvtepu16_epi32(xmmHistAdd);

				xmmHistAdd32 = _mm_sub_epi32(_mm_mullo_epi32(xmmHistAdd32, xmmValueAdd), _mm_mullo_epi32(xmmHistSub32, xmmValueSub));
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmHistAdd32); // <<= 1
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmValueAddSub);

				_mm_storeu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 8), _mm_add_epi32(xmmSumCurr, xmmHistAdd32)); // ��������� puiSumCurr[0,1,2,3]

				// ������� 4 �������� �����������
				xmmSumCurr = _mm_lddqu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 12)); // ��������� 128 ��� (���������� 128 - 4 ��. �������)

				xmmHistSub32 = _mm_cvtepu16_epi32(_mm_bsrli_si128(xmmHistSub, 8));// HistSub32[7], HistSub32[6], HistSub32[5], HistSub32[4] (Hi <-> Lo)
				xmmHistAdd32 = _mm_cvtepu16_epi32(_mm_bsrli_si128(xmmHistAdd, 8));

				// ������������ �������� �����������
				_mm_storeu_si128(reinterpret_cast<__m128i*>(pHistAdd + 8), _mm_add_epi16(xmmHistAdd, xmmHistOne_All)); // pHistAdd[0-7]++; ��������� 128 ��� (���������� 128 - 8 ��. �������)
				_mm_storeu_si128(reinterpret_cast<__m128i*>(pHistSub + 8), _mm_sub_epi16(xmmHistSub, xmmHistOne_All)); // pHistSub[0-7]--; ��������� 128 ��� (���������� 128 - 8 ��. �������)

				xmmHistAdd32 = _mm_sub_epi32(_mm_mullo_epi32(xmmHistAdd32, xmmValueAdd), _mm_mullo_epi32(xmmHistSub32, xmmValueSub));
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmHistAdd32); // <<= 	1
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmValueAddSub);

				_mm_storeu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 12), _mm_add_epi32(xmmSumCurr, xmmHistAdd32)); // ��������� puiSumCurr[0,1,2,3]
				//============================
				// ��������� 1 �����
				//============================
				xmmHistSub = _mm_lddqu_si128(reinterpret_cast<__m128i*>(pHistSub + 16)); // ��������� 128 ��� (���������� 16 ��� - 1 ��. �������)
				xmmHistAdd = _mm_lddqu_si128(reinterpret_cast<__m128i*>(pHistAdd + 16));

				// 1 ������� �����������
				xmmSumCurr = _mm_lddqu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 16)); // ��������� 128 ��� (���������� 32 ��� - 1 ��. �������)

				xmmHistSub32 = _mm_cvtepu16_epi32(xmmHistSub); // HistSub32[3], HistSub32[2], HistSub32[1], HistSub32[0](Hi < ->Lo)
				xmmHistAdd32 = _mm_cvtepu16_epi32(xmmHistAdd);

				xmmValueAdd = _mm_bsrli_si128(xmmValueAdd, 12); // 0, 0, 0, xmmValueAdd(Hi < ->Lo)
				xmmValueSub = _mm_bsrli_si128(xmmValueSub, 12); // 0, 0, 0, xmmValueSub(Hi < ->Lo)
				xmmValueAddSub = _mm_bsrli_si128(xmmValueAddSub, 12);

				xmmHistAdd32 = _mm_sub_epi32(_mm_mullo_epi32(xmmHistAdd32, xmmValueAdd), _mm_mullo_epi32(xmmHistSub32, xmmValueSub));
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmHistAdd32); // <<= 1
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmValueAddSub);

				_mm_storeu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 16), _mm_add_epi32(xmmSumCurr, xmmHistAdd32)); // ��������� puiSumCurr[0,1,2,3]
				_mm_storeu_si128(reinterpret_cast<__m128i*>(pHistAdd + 16), _mm_add_epi16(xmmHistAdd, xmmHistOne_1)); // pHistAdd[0-0]++; ��������� 128 ��� (���������� 128 - 8 ��. �������)
				_mm_storeu_si128(reinterpret_cast<__m128i*>(pHistSub + 16), _mm_sub_epi16(xmmHistSub, xmmHistOne_1)); // pHistSub[0-0]--; ��������� 128 ��� (���������� 128 - 8 ��. �������)
			}
			pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
			pfRowOut++;
		}

		// ��������� [ar_cmmIn.m_i64W - 2*8, ar_cmmIn.m_i64W - 1] �������
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - 8;
			iCmax = min(static_cast<int>(ar_cmmIn.m_i64W - 8 - 1), static_cast<int>(iCol + 8));

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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

				// ��������� ������� � 17 ���������� � ����
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAddSub + ((u32ValueAdd * static_cast<uint32_t>(pHistAdd[i]++) - u32ValueSub * static_cast<uint32_t>(pHistSub[i]--)) << 1);
			}
			pfRowOut[0] = static_cast<float>(puiSum[iCmin] * Size_obratn);
			pfRowOut++;
		}
	}

	// �������� ����. ��������� ������.
	for (iRow = ar_cmmIn.m_i64H - 8; iRow < ar_cmmOut.m_i64H; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

	if (nullptr != pHistAll)
		delete[] pHistAll;
	if (nullptr != puiSum)
		delete[] puiSum;
	if (nullptr != pbBrightness)
		delete[] pbBrightness;
}


void TextureFiltr_Mean_V8_19_sse4(MU16Data& ar_cmmIn, MFData& ar_cmmOut, double pseudo_min, double kfct)
{
	double Size_obratn = 1.0 / 361;

	// ��� ��� ����������
	uint16_t* pHistAll = new uint16_t[256 * ar_cmmIn.m_i64W]; // [256][ar_cmmIn.m_i64W]
	memset(pHistAll, 0, sizeof(uint16_t) * 256 * ar_cmmIn.m_i64W);
	uint16_t* pHist, * pHistAdd, * pHistSub;

	// ��� ��� ����
	uint32_t* puiSum = new uint32_t[ar_cmmIn.m_i64W], * puiSumCurr;
	memset(puiSum, 0, sizeof(uint32_t) * ar_cmmIn.m_i64W);

	// ��� ��� ��������
	uint8_t* pbBrightness = new uint8_t[ar_cmmIn.m_i64W * 19];
	uint8_t* pbBrightnessRow;

	int64_t iRow, iCol, iColMax = ar_cmmIn.m_i64W - 19;
	ar_cmmOut.iCreate(ar_cmmIn.m_i64W, ar_cmmIn.m_i64H, 6);

	// �������� ����. ������ ������.
	for (iRow = 0; iRow < 9; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

	int64_t i, iCmin, iCmax, iPos = 0;
	uint32_t u32ValueAdd, u32ValueSub;
	double d;

	// ������ [0, 19 - 1] ������
	pbBrightnessRow = pbBrightness;
	float* pfRowOut = ar_cmmOut.pfGetRow(9) + 9;
	for (iRow = 0; iRow < 19; iRow++, pbBrightnessRow += ar_cmmIn.m_i64W)
	{
		// �������� ���� �����.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * 9);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - 9), 0, sizeof(float) * 9);

		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);

		// ������ [0, 2*9[ �������
		for (iCol = 0; iCol < (9 << 1); iCol++)
		{
			iCmin = max(9, static_cast<int>(iCol - 9));
			iCmax = iCol + 9;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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
				// ��������� ������� � 19 ���������� � ����
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
		}

		// ������� [2*9, ar_cmmIn.m_i64W - 19] �������
		puiSumCurr = puiSum + 9;
		for (; iCol <= iColMax; iCol++, puiSumCurr++)
		{
			iCmin = iCol - 9;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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
				// ��������� ������� � 19 ���������� � ����
				for (i = 0; i < 19; i++)
					puiSumCurr[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
			if ((19 - 1) == iRow)
			{
				pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
				pfRowOut++;
			}
		}

		// ��������� [ar_cmmIn.m_i64W - 2*9, ar_cmmIn.m_i64W - 1] �������
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - 9;
			iCmax = min(static_cast<int>(ar_cmmIn.m_i64W - 9 - 1), static_cast<int>(iCol + 9));

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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
				// ��������� ������� � 19 ���������� � ����
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

	__m128i xmmHistOne_3 = _mm_set_epi16(0, 0, 0, 0, 0, 1, 1, 1); // ��� inc ��� dec ����� ��� 3 ����������
	__m128i xmmHistOne_All = _mm_set_epi16(1, 1, 1, 1, 1, 1, 1, 1); // ��� inc ��� dec ����� ��� ���� ����������

	// ����������� ������ [19, ar_cmmIn.m_i64H[
	for (iRow = 19; iRow < ar_cmmIn.m_i64H; iRow++)
	{
		// �������� ���� �����.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * 9);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - 9), 0, sizeof(float) * 9);

		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);
		float* pfRowOut = ar_cmmOut.pfGetRow(iRow - 9) + 9;
		iPos = (iRow - 19) % 19;
		pbBrightnessRow = pbBrightness + (iPos * ar_cmmIn.m_i64W);

		// ������ [0, 2*9[ �������
		for (iCol = 0; iCol < (9 << 1); iCol++)
		{
			iCmin = max(9, static_cast<int>(iCol - 9));
			iCmax = iCol + 9;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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

				// ��������� ������� � 19 ���������� � ����
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAddSub + ((u32ValueAdd * static_cast<uint32_t>(pHistAdd[i]++) - u32ValueSub * static_cast<uint32_t>(pHistSub[i]--)) << 1);
			}
		}

		// ������� [2*9, ar_cmmIn.m_i64W - 19] �������
		puiSumCurr = puiSum + 9;
		pHist = pHistAll + (iCol - 9);
		for (; iCol <= iColMax; iCol++, puiSumCurr++, pHist++)
		{
			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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

				// ��������� ������� � 19 ���������� � ����
				//============================
				// ������ 8 �����
				//============================
				__m128i xmmHistSub = _mm_lddqu_si128(reinterpret_cast<__m128i*>(pHistSub));// ��������� 128 ��� (���������� 128 ��� - 8 ��. �������)
				__m128i xmmHistAdd = _mm_lddqu_si128(reinterpret_cast<__m128i*>(pHistAdd));

				// ������� 4 �������� �����������
				__m128i xmmSumCurr = _mm_lddqu_si128(reinterpret_cast<__m128i*> (puiSumCurr)); // ��������� 128 ��� (���������� 128 ��� - 4 ��. �������)

				__m128i xmmHistSub32 = _mm_cvtepu16_epi32(xmmHistSub); // HistSub32[3], HistSub32[2], HistSub32[1], HistSub32[0](Hi < ->Lo)
				__m128i xmmHistAdd32 = _mm_cvtepu16_epi32(xmmHistAdd);

				__m128i xmmValueAdd = _mm_loadu_si32(&u32ValueAdd); // 0, 0, 0, u32ValueAdd(Hi < ->Lo)
				__m128i xmmValueSub = _mm_loadu_si32(&u32ValueSub);

				xmmValueAdd = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(xmmValueAdd), _mm_castsi128_ps(xmmValueAdd), 0x0)); // 0, u32ValueAdd, u32ValueAdd, u32ValueAdd
				xmmValueSub = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(xmmValueSub), _mm_castsi128_ps(xmmValueSub), 0x0));

				__m128i xmmValueAddSub = _mm_add_epi32(xmmValueAdd, xmmValueSub);

				xmmHistAdd32 = _mm_sub_epi32(_mm_mullo_epi32(xmmHistAdd32, xmmValueAdd), _mm_mullo_epi32(xmmHistSub32, xmmValueSub));
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmHistAdd32); // <<= 1
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmValueAddSub);

				_mm_storeu_si128(reinterpret_cast<__m128i*>(puiSumCurr), _mm_add_epi32(xmmSumCurr, xmmHistAdd32)); // ��������� puiSumCurr[0,1,2,3]

				// ������� 4 �������� �����������
				xmmSumCurr = _mm_lddqu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 4)); // ��������� 128 ��� (���������� 128 - 4 ��. �������)

				xmmHistSub32 = _mm_cvtepu16_epi32(_mm_bsrli_si128(xmmHistSub, 8));// HistSub32[7], HistSub32[6], HistSub32[5], HistSub32[4] (Hi <-> Lo)
				xmmHistAdd32 = _mm_cvtepu16_epi32(_mm_bsrli_si128(xmmHistAdd, 8));

				// ������������ �������� �����������
				_mm_storeu_si128(reinterpret_cast<__m128i*>(pHistAdd), _mm_add_epi16(xmmHistAdd, xmmHistOne_All)); // pHistAdd[0-7]++; ��������� 128 ��� (���������� 128 - 8 ��. �������)
				_mm_storeu_si128(reinterpret_cast<__m128i*>(pHistSub), _mm_sub_epi16(xmmHistSub, xmmHistOne_All)); // pHistSub[0-7]--; ��������� 128 ��� (���������� 128 - 8 ��. �������)

				xmmHistAdd32 = _mm_sub_epi32(_mm_mullo_epi32(xmmHistAdd32, xmmValueAdd), _mm_mullo_epi32(xmmHistSub32, xmmValueSub));
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmHistAdd32); // <<= 	1
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmValueAddSub);

				_mm_storeu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 4), _mm_add_epi32(xmmSumCurr, xmmHistAdd32)); // ��������� puiSumCurr[0,1,2,3]
				//============================
				// ������ 8 �����
				//============================
				xmmHistSub = _mm_lddqu_si128(reinterpret_cast<__m128i*>(pHistSub + 8)); // ��������� 128 ��� (���������� 128 ��� - 8 ��. �������)
				xmmHistAdd = _mm_lddqu_si128(reinterpret_cast<__m128i*>(pHistAdd + 8));

				// ������� 4 �������� �����������
				xmmSumCurr = _mm_lddqu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 8)); // ��������� 128 ��� (���������� 128 ��� - 4 ��. �������)

				xmmHistSub32 = _mm_cvtepu16_epi32(xmmHistSub); // HistSub32[3], HistSub32[2], HistSub32[1], HistSub32[0](Hi < ->Lo)
				xmmHistAdd32 = _mm_cvtepu16_epi32(xmmHistAdd);

				xmmHistAdd32 = _mm_sub_epi32(_mm_mullo_epi32(xmmHistAdd32, xmmValueAdd), _mm_mullo_epi32(xmmHistSub32, xmmValueSub));
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmHistAdd32); // <<= 1
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmValueAddSub);

				_mm_storeu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 8), _mm_add_epi32(xmmSumCurr, xmmHistAdd32)); // ��������� puiSumCurr[0,1,2,3]

				// ������� 4 �������� �����������
				xmmSumCurr = _mm_lddqu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 12)); // ��������� 128 ��� (���������� 128 - 4 ��. �������)

				xmmHistSub32 = _mm_cvtepu16_epi32(_mm_bsrli_si128(xmmHistSub, 8));// HistSub32[7], HistSub32[6], HistSub32[5], HistSub32[4] (Hi <-> Lo)
				xmmHistAdd32 = _mm_cvtepu16_epi32(_mm_bsrli_si128(xmmHistAdd, 8));

				// ������������ �������� �����������
				_mm_storeu_si128(reinterpret_cast<__m128i*>(pHistAdd + 8), _mm_add_epi16(xmmHistAdd, xmmHistOne_All)); // pHistAdd[0-7]++; ��������� 128 ��� (���������� 128 - 8 ��. �������)
				_mm_storeu_si128(reinterpret_cast<__m128i*>(pHistSub + 8), _mm_sub_epi16(xmmHistSub, xmmHistOne_All)); // pHistSub[0-7]--; ��������� 128 ��� (���������� 128 - 8 ��. �������)

				xmmHistAdd32 = _mm_sub_epi32(_mm_mullo_epi32(xmmHistAdd32, xmmValueAdd), _mm_mullo_epi32(xmmHistSub32, xmmValueSub));
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmHistAdd32); // <<= 	1
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmValueAddSub);

				_mm_storeu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 12), _mm_add_epi32(xmmSumCurr, xmmHistAdd32)); // ��������� puiSumCurr[0,1,2,3]
				//============================
				// ��������� 3 �����
				//============================
				xmmHistSub = _mm_lddqu_si128(reinterpret_cast<__m128i*>(pHistSub + 16)); // ��������� 128 ��� (���������� 48 ��� - 3 ��. �������)
				xmmHistAdd = _mm_lddqu_si128(reinterpret_cast<__m128i*>(pHistAdd + 16));

				// 3 �������� �����������
				xmmSumCurr = _mm_lddqu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 16)); // ��������� 128 ��� (���������� 96 ��� - 3 ��. �������)

				xmmHistSub32 = _mm_cvtepu16_epi32(xmmHistSub); // HistSub32[3], HistSub32[2], HistSub32[1], HistSub32[0](Hi < ->Lo)
				xmmHistAdd32 = _mm_cvtepu16_epi32(xmmHistAdd);

				xmmValueAdd = _mm_bsrli_si128(xmmValueAdd, 4); // 0, xmmValueAdd, xmmValueAdd, xmmValueAdd(Hi < ->Lo)
				xmmValueSub = _mm_bsrli_si128(xmmValueSub, 4); // 0, xmmValueSub, xmmValueSub, xmmValueSub(Hi < ->Lo)
				xmmValueAddSub = _mm_bsrli_si128(xmmValueAddSub, 4);

				xmmHistAdd32 = _mm_sub_epi32(_mm_mullo_epi32(xmmHistAdd32, xmmValueAdd), _mm_mullo_epi32(xmmHistSub32, xmmValueSub));
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmHistAdd32); // <<= 1
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmValueAddSub);

				_mm_storeu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 16), _mm_add_epi32(xmmSumCurr, xmmHistAdd32)); // ��������� puiSumCurr[0,1,2,3]
				_mm_storeu_si128(reinterpret_cast<__m128i*>(pHistAdd + 16), _mm_add_epi16(xmmHistAdd, xmmHistOne_3)); // pHistAdd[0-2]++; ��������� 128 ��� (���������� 128 - 8 ��. �������)
				_mm_storeu_si128(reinterpret_cast<__m128i*>(pHistSub + 16), _mm_sub_epi16(xmmHistSub, xmmHistOne_3)); // pHistSub[0-2]--; ��������� 128 ��� (���������� 128 - 8 ��. �������)
			}
			pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
			pfRowOut++;
		}

		// ��������� [ar_cmmIn.m_i64W - 2*9, ar_cmmIn.m_i64W - 1] �������
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - 9;
			iCmax = min(static_cast<int>(ar_cmmIn.m_i64W - 9 - 1), static_cast<int>(iCol + 9));

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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

				// ��������� ������� � 19 ���������� � ����
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAddSub + ((u32ValueAdd * static_cast<uint32_t>(pHistAdd[i]++) - u32ValueSub * static_cast<uint32_t>(pHistSub[i]--)) << 1);
			}
			pfRowOut[0] = static_cast<float>(puiSum[iCmin] * Size_obratn);
			pfRowOut++;
		}
	}

	// �������� ����. ��������� ������.
	for (iRow = ar_cmmIn.m_i64H - 9; iRow < ar_cmmOut.m_i64H; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

	if (nullptr != pHistAll)
		delete[] pHistAll;
	if (nullptr != puiSum)
		delete[] puiSum;
	if (nullptr != pbBrightness)
		delete[] pbBrightness;
}


void TextureFiltr_Mean_V8_21_sse4(MU16Data& ar_cmmIn, MFData& ar_cmmOut, double pseudo_min, double kfct)
{
	double Size_obratn = 1.0 / 441;

	// ��� ��� ����������
	uint16_t* pHistAll = new uint16_t[256 * ar_cmmIn.m_i64W]; // [256][ar_cmmIn.m_i64W]
	memset(pHistAll, 0, sizeof(uint16_t) * 256 * ar_cmmIn.m_i64W);
	uint16_t* pHist, * pHistAdd, * pHistSub;

	// ��� ��� ����
	uint32_t* puiSum = new uint32_t[ar_cmmIn.m_i64W], * puiSumCurr;
	memset(puiSum, 0, sizeof(uint32_t) * ar_cmmIn.m_i64W);

	// ��� ��� ��������
	uint8_t* pbBrightness = new uint8_t[ar_cmmIn.m_i64W * 21];
	uint8_t* pbBrightnessRow;

	int64_t iRow, iCol, iColMax = ar_cmmIn.m_i64W - 21;
	ar_cmmOut.iCreate(ar_cmmIn.m_i64W, ar_cmmIn.m_i64H, 6);

	// �������� ����. ������ ������.
	for (iRow = 0; iRow < 10; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

	int64_t i, iCmin, iCmax, iPos = 0;
	uint32_t u32ValueAdd, u32ValueSub;
	double d;

	// ������ [0, 21 - 1] ������
	pbBrightnessRow = pbBrightness;
	float* pfRowOut = ar_cmmOut.pfGetRow(10) + 10;
	for (iRow = 0; iRow < 21; iRow++, pbBrightnessRow += ar_cmmIn.m_i64W)
	{
		// �������� ���� �����.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * 10);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - 10), 0, sizeof(float) * 10);

		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);

		// ������ [0, 2*10[ �������
		for (iCol = 0; iCol < (10 << 1); iCol++)
		{
			iCmin = max(10, static_cast<int>(iCol - 10));
			iCmax = iCol + 10;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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
				// ��������� ������� � 21 ���������� � ����
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
		}

		// ������� [2*10, ar_cmmIn.m_i64W - 21] �������
		puiSumCurr = puiSum + 10;
		for (; iCol <= iColMax; iCol++, puiSumCurr++)
		{
			iCmin = iCol - 10;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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
				// ��������� ������� � 21 ���������� � ����
				for (i = 0; i < 21; i++)
					puiSumCurr[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
			if ((21 - 1) == iRow)
			{
				pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
				pfRowOut++;
			}
		}

		// ��������� [ar_cmmIn.m_i64W - 2*10, ar_cmmIn.m_i64W - 1] �������
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - 10;
			iCmax = min(static_cast<int>(ar_cmmIn.m_i64W - 10 - 1), static_cast<int>(iCol + 10));

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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
				// ��������� ������� � 21 ���������� � ����
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

	__m128i xmmHistOne_5 = _mm_set_epi16(0, 0, 0, 1, 1, 1, 1, 1); // ��� inc ��� dec ����� ��� 5 ����������
	__m128i xmmHistOne_All = _mm_set_epi16(1, 1, 1, 1, 1, 1, 1, 1); // ��� inc ��� dec ����� ��� ���� ����������

	// ����������� ������ [21, ar_cmmIn.m_i64H[
	for (iRow = 21; iRow < ar_cmmIn.m_i64H; iRow++)
	{
		// �������� ���� �����.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * 10);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - 10), 0, sizeof(float) * 10);

		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);
		float* pfRowOut = ar_cmmOut.pfGetRow(iRow - 10) + 10;
		iPos = (iRow - 21) % 21;
		pbBrightnessRow = pbBrightness + (iPos * ar_cmmIn.m_i64W);

		// ������ [0, 2*10[ �������
		for (iCol = 0; iCol < (10 << 1); iCol++)
		{
			iCmin = max(10, static_cast<int>(iCol - 10));
			iCmax = iCol + 10;

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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

				// ��������� ������� � 21 ���������� � ����
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAddSub + ((u32ValueAdd * static_cast<uint32_t>(pHistAdd[i]++) - u32ValueSub * static_cast<uint32_t>(pHistSub[i]--)) << 1);
			}
		}

		// ������� [2*10, ar_cmmIn.m_i64W - 21] �������
		puiSumCurr = puiSum + 10;
		pHist = pHistAll + (iCol - 10);
		for (; iCol <= iColMax; iCol++, puiSumCurr++, pHist++)
		{
			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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

				// ��������� ������� � 21 ���������� � ����
				//============================
				// ������ 8 �����
				//============================
				__m128i xmmHistSub = _mm_lddqu_si128(reinterpret_cast<__m128i*>(pHistSub));// ��������� 128 ��� (���������� 128 ��� - 8 ��. �������)
				__m128i xmmHistAdd = _mm_lddqu_si128(reinterpret_cast<__m128i*>(pHistAdd));

				// ������� 4 �������� �����������
				__m128i xmmSumCurr = _mm_lddqu_si128(reinterpret_cast<__m128i*> (puiSumCurr)); // ��������� 128 ��� (���������� 128 ��� - 4 ��. �������)

				__m128i xmmHistSub32 = _mm_cvtepu16_epi32(xmmHistSub); // HistSub32[3], HistSub32[2], HistSub32[1], HistSub32[0](Hi < ->Lo)
				__m128i xmmHistAdd32 = _mm_cvtepu16_epi32(xmmHistAdd);

				__m128i xmmValueAdd = _mm_loadu_si32(&u32ValueAdd); // 0, 0, 0, u32ValueAdd(Hi < ->Lo)
				__m128i xmmValueSub = _mm_loadu_si32(&u32ValueSub);

				xmmValueAdd = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(xmmValueAdd), _mm_castsi128_ps(xmmValueAdd), 0x0)); // 0, u32ValueAdd, u32ValueAdd, u32ValueAdd
				xmmValueSub = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(xmmValueSub), _mm_castsi128_ps(xmmValueSub), 0x0));

				__m128i xmmValueAddSub = _mm_add_epi32(xmmValueAdd, xmmValueSub);

				xmmHistAdd32 = _mm_sub_epi32(_mm_mullo_epi32(xmmHistAdd32, xmmValueAdd), _mm_mullo_epi32(xmmHistSub32, xmmValueSub));
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmHistAdd32); // <<= 1
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmValueAddSub);

				_mm_storeu_si128(reinterpret_cast<__m128i*>(puiSumCurr), _mm_add_epi32(xmmSumCurr, xmmHistAdd32)); // ��������� puiSumCurr[0,1,2,3]

				// ������� 4 �������� �����������
				xmmSumCurr = _mm_lddqu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 4)); // ��������� 128 ��� (���������� 128 - 4 ��. �������)

				xmmHistSub32 = _mm_cvtepu16_epi32(_mm_bsrli_si128(xmmHistSub, 8));// HistSub32[7], HistSub32[6], HistSub32[5], HistSub32[4] (Hi <-> Lo)
				xmmHistAdd32 = _mm_cvtepu16_epi32(_mm_bsrli_si128(xmmHistAdd, 8));

				// ������������ �������� �����������
				_mm_storeu_si128(reinterpret_cast<__m128i*>(pHistAdd), _mm_add_epi16(xmmHistAdd, xmmHistOne_All)); // pHistAdd[0-7]++; ��������� 128 ��� (���������� 128 - 8 ��. �������)
				_mm_storeu_si128(reinterpret_cast<__m128i*>(pHistSub), _mm_sub_epi16(xmmHistSub, xmmHistOne_All)); // pHistSub[0-7]--; ��������� 128 ��� (���������� 128 - 8 ��. �������)

				xmmHistAdd32 = _mm_sub_epi32(_mm_mullo_epi32(xmmHistAdd32, xmmValueAdd), _mm_mullo_epi32(xmmHistSub32, xmmValueSub));
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmHistAdd32); // <<= 	1
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmValueAddSub);

				_mm_storeu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 4), _mm_add_epi32(xmmSumCurr, xmmHistAdd32)); // ��������� puiSumCurr[0,1,2,3]
				//============================
				// ������ 8 �����
				//============================
				xmmHistSub = _mm_lddqu_si128(reinterpret_cast<__m128i*>(pHistSub + 8)); // ��������� 128 ��� (���������� 128 ��� - 8 ��. �������)
				xmmHistAdd = _mm_lddqu_si128(reinterpret_cast<__m128i*>(pHistAdd + 8));

				// ������� 4 �������� �����������
				xmmSumCurr = _mm_lddqu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 8)); // ��������� 128 ��� (���������� 128 ��� - 4 ��. �������)

				xmmHistSub32 = _mm_cvtepu16_epi32(xmmHistSub); // HistSub32[3], HistSub32[2], HistSub32[1], HistSub32[0](Hi < ->Lo)
				xmmHistAdd32 = _mm_cvtepu16_epi32(xmmHistAdd);

				xmmHistAdd32 = _mm_sub_epi32(_mm_mullo_epi32(xmmHistAdd32, xmmValueAdd), _mm_mullo_epi32(xmmHistSub32, xmmValueSub));
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmHistAdd32); // <<= 1
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmValueAddSub);

				_mm_storeu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 8), _mm_add_epi32(xmmSumCurr, xmmHistAdd32)); // ��������� puiSumCurr[0,1,2,3]

				// ������� 4 �������� �����������
				xmmSumCurr = _mm_lddqu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 12)); // ��������� 128 ��� (���������� 128 - 4 ��. �������)

				xmmHistSub32 = _mm_cvtepu16_epi32(_mm_bsrli_si128(xmmHistSub, 8));// HistSub32[7], HistSub32[6], HistSub32[5], HistSub32[4] (Hi <-> Lo)
				xmmHistAdd32 = _mm_cvtepu16_epi32(_mm_bsrli_si128(xmmHistAdd, 8));

				// ������������ �������� �����������
				_mm_storeu_si128(reinterpret_cast<__m128i*>(pHistAdd + 8), _mm_add_epi16(xmmHistAdd, xmmHistOne_All)); // pHistAdd[0-7]++; ��������� 128 ��� (���������� 128 - 8 ��. �������)
				_mm_storeu_si128(reinterpret_cast<__m128i*>(pHistSub + 8), _mm_sub_epi16(xmmHistSub, xmmHistOne_All)); // pHistSub[0-7]--; ��������� 128 ��� (���������� 128 - 8 ��. �������)

				xmmHistAdd32 = _mm_sub_epi32(_mm_mullo_epi32(xmmHistAdd32, xmmValueAdd), _mm_mullo_epi32(xmmHistSub32, xmmValueSub));
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmHistAdd32); // <<= 	1
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmValueAddSub);

				_mm_storeu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 12), _mm_add_epi32(xmmSumCurr, xmmHistAdd32)); // ��������� puiSumCurr[0,1,2,3]
				//============================
				// ��������� 5 �����
				//============================
				xmmHistSub = _mm_lddqu_si128(reinterpret_cast<__m128i*>(pHistSub + 16)); // ��������� 128 ��� (���������� 80 ��� - 5 ��. �������)
				xmmHistAdd = _mm_lddqu_si128(reinterpret_cast<__m128i*>(pHistAdd + 16));

				// ������� 4 �������� �����������
				xmmSumCurr = _mm_lddqu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 16)); // ��������� 128 ��� (���������� 128 ��� - 4 ��. �������)

				xmmHistSub32 = _mm_cvtepu16_epi32(xmmHistSub); // HistSub32[3], HistSub32[2], HistSub32[1], HistSub32[0](Hi < ->Lo)
				xmmHistAdd32 = _mm_cvtepu16_epi32(xmmHistAdd);

				xmmHistAdd32 = _mm_sub_epi32(_mm_mullo_epi32(xmmHistAdd32, xmmValueAdd), _mm_mullo_epi32(xmmHistSub32, xmmValueSub));
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmHistAdd32); // <<= 1
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmValueAddSub);

				_mm_storeu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 16), _mm_add_epi32(xmmSumCurr, xmmHistAdd32)); // ��������� puiSumCurr[0,1,2,3]

				// ������� 1 ������� �����������
				xmmSumCurr = _mm_lddqu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 20)); // ��������� 128 ��� (���������� 32 - 1 ��. �������)

				xmmHistSub32 = _mm_cvtepu16_epi32(_mm_bsrli_si128(xmmHistSub, 8));// HistSub32[7], HistSub32[6], HistSub32[5], HistSub32[4] (Hi <-> Lo)
				xmmHistAdd32 = _mm_cvtepu16_epi32(_mm_bsrli_si128(xmmHistAdd, 8));

				// ������������ �������� �����������
				_mm_storeu_si128(reinterpret_cast<__m128i*>(pHistAdd + 16), _mm_add_epi16(xmmHistAdd, xmmHistOne_5)); // pHistAdd[0-4]++; ��������� 128 ��� (���������� 128 - 8 ��. �������)
				_mm_storeu_si128(reinterpret_cast<__m128i*>(pHistSub + 16), _mm_sub_epi16(xmmHistSub, xmmHistOne_5)); // pHistSub[0-4]--; ��������� 128 ��� (���������� 128 - 8 ��. �������)

				xmmValueAdd = _mm_bsrli_si128(xmmValueAdd, 12); // 0, 0, 0, xmmValueAdd(Hi < ->Lo)
				xmmValueSub = _mm_bsrli_si128(xmmValueSub, 12); // 0, 0, 	0, xmmValueSub(Hi < ->Lo)
				xmmValueAddSub = _mm_bsrli_si128(xmmValueAddSub, 12);

				xmmHistAdd32 = _mm_sub_epi32(_mm_mullo_epi32(xmmHistAdd32, xmmValueAdd), _mm_mullo_epi32(xmmHistSub32, xmmValueSub));
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmHistAdd32); // <<= 	1
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmValueAddSub);

				_mm_storeu_si128(reinterpret_cast<__m128i*>(puiSumCurr + 20), _mm_add_epi32(xmmSumCurr, xmmHistAdd32)); // ��������� puiSumCurr[0,1,2,3]
			}
			pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
			pfRowOut++;
		}

		// ��������� [ar_cmmIn.m_i64W - 2*10, ar_cmmIn.m_i64W - 1] �������
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - 10;
			iCmax = min(static_cast<int>(ar_cmmIn.m_i64W - 10 - 1), static_cast<int>(iCol + 10));

			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // ��� ��� ��� ������� 0 ����� � ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� �����	��������!
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

				// ��������� ������� � 21 ���������� � ����
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAddSub + ((u32ValueAdd * static_cast<uint32_t>(pHistAdd[i]++) - u32ValueSub * static_cast<uint32_t>(pHistSub[i]--)) << 1);
			}
			pfRowOut[0] = static_cast<float>(puiSum[iCmin] * Size_obratn);
			pfRowOut++;
		}
	}

	// �������� ����. ��������� ������.
	for (iRow = ar_cmmIn.m_i64H - 10; iRow < ar_cmmOut.m_i64H; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

	if (nullptr != pHistAll)
		delete[] pHistAll;
	if (nullptr != puiSum)
		delete[] puiSum;
	if (nullptr != pbBrightness)
		delete[] pbBrightness;
}

tdTextureFiltr_Mean_V8 g_afunTextureFiltr_Mean_V8_sse4[10] =
{
 TextureFiltr_Mean_V8_3_sse4, // 0
 TextureFiltr_Mean_V8_5_sse4, // 1
 TextureFiltr_Mean_V8_7_sse4, // 2
 TextureFiltr_Mean_V8_9_sse4, // 3
 TextureFiltr_Mean_V8_11_sse4, // 4
 TextureFiltr_Mean_V8_13_sse4, // 5
 TextureFiltr_Mean_V8_15_sse4, // 6
 TextureFiltr_Mean_V8_17_sse4, // 7
 TextureFiltr_Mean_V8_19_sse4, // 8
 TextureFiltr_Mean_V8_21_sse4 // 9
};
