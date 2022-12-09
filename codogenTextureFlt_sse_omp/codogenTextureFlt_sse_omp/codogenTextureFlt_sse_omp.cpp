// codogenTextureFltSSe.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <iostream>
#include <cstdlib>

void FunV8_8(FILE* pf, int iElOffset, int iLast)
{
	const char* pcDeclare = "__m128i ";
	if (0 != iElOffset)
		pcDeclare = "";
	if (0 == iElOffset)
	{
		fprintf(pf, " %sxmmHistSub = _mm_lddqu_si128(reinterpret_cast<__m128i *>(pHistSub));// Загружаем 128 бит (используем %d бит - %d эл. массива)\n", pcDeclare, 16 * iLast, iLast);
		fprintf(pf, " %sxmmHistAdd = _mm_lddqu_si128(reinterpret_cast<__m128i *>(pHistAdd));\n", pcDeclare);
	}
	else
	{
		fprintf(pf, " %sxmmHistSub = _mm_lddqu_si128(reinterpret_cast<__m128i *>(pHistSub + % d)); // Загружаем 128 бит (используем %d бит - %d эл. массива)\n", pcDeclare, iElOffset, 16 * iLast, iLast);
		fprintf(pf, " %sxmmHistAdd = _mm_lddqu_si128(reinterpret_cast<__m128i *>(pHistAdd + % d)); \n", pcDeclare, iElOffset);
	}
	fprintf(pf, "\n");
	if (iLast > 4)
	{
		fprintf(pf, " // Младшие 4 элемента гистограммы\n");
		if (0 == iElOffset)
			fprintf(pf, " %sxmmSumCurr = _mm_lddqu_si128(reinterpret_cast<__m128i * > (puiSumCurr)); // Загружаем 128 бит (используем 128 бит - 4 эл. массива)\n", pcDeclare);
		else
			fprintf(pf, " %sxmmSumCurr = _mm_lddqu_si128(reinterpret_cast<__m128i *>(puiSumCurr + % d)); // Загружаем 128 бит (используем 128 бит - 4 эл. массива)\n", pcDeclare, iElOffset);
	}
	else
	{
		if (1 == iLast)
			fprintf(pf, " // 1 элемент гистограммы\n");
		else
			fprintf(pf, " // %d элемента гистограммы\n", iLast);
		if (0 == iElOffset)
			fprintf(pf, " %sxmmSumCurr = _mm_lddqu_si128(reinterpret_cast<__m128i * > (puiSumCurr)); // Загружаем 128 бит (используем %d бит - %d эл. массива)\n", pcDeclare, 32 * iLast, iLast);
		else
			fprintf(pf, " %sxmmSumCurr = _mm_lddqu_si128(reinterpret_cast<__m128i *>(puiSumCurr	+ % d)); // Загружаем 128 бит (используем %d бит - %d эл. массива)\n", pcDeclare, iElOffset, 32 * iLast, iLast);
	}
	fprintf(pf, "\n");
	fprintf(pf, " %sxmmHistSub32 = _mm_cvtepu16_epi32(xmmHistSub); // HistSub32[3], HistSub32[2], HistSub32[1], HistSub32[0](Hi < ->Lo)\n", pcDeclare);
	fprintf(pf, " %sxmmHistAdd32 = _mm_cvtepu16_epi32(xmmHistAdd);\n", pcDeclare);
	fprintf(pf, "\n");
	if (0 == iElOffset)
	{
		fprintf(pf, " %sxmmValueAdd = _mm_loadu_si32(&u32ValueAdd); // 0, 0, 0, u32ValueAdd(Hi < ->Lo)\n", pcDeclare);
		fprintf(pf, " %sxmmValueSub = _mm_loadu_si32(&u32ValueSub);\n", pcDeclare);
		fprintf(pf, "\n");
		const char* pcImm8 = "0x0";
		if (iLast > 1)
		{
			if (3 == iLast)
				pcImm8 = "0x0C0";
			fprintf(pf, " xmmValueAdd = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(xmmValueAdd), _mm_castsi128_ps(xmmValueAdd), % s)); // 0, u32ValueAdd, u32ValueAdd, u32ValueAdd\n", pcImm8);
			fprintf(pf, " xmmValueSub = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(xmmValueSub), _mm_castsi128_ps(xmmValueSub), % s)); \n", pcImm8);
			fprintf(pf, "\n");
		}
		if (iLast > 4)
		{
			fprintf(pf, " %sxmmValueAddSub = _mm_add_epi32(xmmValueAdd, xmmValueSub);\n",
				pcDeclare);
			fprintf(pf, "\n");
		}
	}
	else
	{
		if (1 == iLast)
		{
			fprintf(pf, " xmmValueAdd = _mm_bsrli_si128(xmmValueAdd, 12); // 0, 0, 0, xmmValueAdd(Hi < ->Lo)\n");
			fprintf(pf, " xmmValueSub = _mm_bsrli_si128(xmmValueSub, 12); // 0, 0, 0, xmmValueSub(Hi < ->Lo)\n");
			fprintf(pf, " xmmValueAddSub = _mm_bsrli_si128(xmmValueAddSub, 12);\n");
			fprintf(pf, "\n");
		}
		else if (3 == iLast)
		{
			fprintf(pf, " xmmValueAdd = _mm_bsrli_si128(xmmValueAdd, 4); // 0, xmmValueAdd, xmmValueAdd, xmmValueAdd(Hi < ->Lo)\n");
			fprintf(pf, " xmmValueSub = _mm_bsrli_si128(xmmValueSub, 4); // 0, xmmValueSub, xmmValueSub, xmmValueSub(Hi < ->Lo)\n");
			fprintf(pf, " xmmValueAddSub = _mm_bsrli_si128(xmmValueAddSub, 4);\n");
			fprintf(pf, "\n");
		}
	}
	fprintf(pf, " xmmHistAdd32 = _mm_sub_epi32(_mm_mullo_epi32(xmmHistAdd32, xmmValueAdd), _mm_mullo_epi32(xmmHistSub32, xmmValueSub)); \n");
	fprintf(pf, " xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmHistAdd32); // <<= 1\n");
	if ((0 == iElOffset) && (iLast < 4))
	{
		fprintf(pf, " xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmValueAdd);\n");
		fprintf(pf, " xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmValueSub);\n");
	}
	else
		fprintf(pf, " xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmValueAddSub);\n");
	fprintf(pf, "\n");
	if (0 == iElOffset)
		fprintf(pf, " _mm_storeu_si128(reinterpret_cast<__m128i *>(puiSumCurr), _mm_add_epi32(xmmSumCurr, xmmHistAdd32)); // Сохраняем puiSumCurr[0,1,2,3]\n");
	else
		fprintf(pf, " _mm_storeu_si128(reinterpret_cast<__m128i *>(puiSumCurr + %d), _mm_add_epi32(xmmSumCurr, xmmHistAdd32)); // Сохраняем puiSumCurr[0,1,2,3]\n", iElOffset);
	const char* pcHist = "xmmHistOne_All";
	if (1 == iLast)
		pcHist = "xmmHistOne_1";
	else if (3 == iLast)
		pcHist = "xmmHistOne_3";
	else if (5 == iLast)
		pcHist = "xmmHistOne_5";
	else if (7 == iLast)
		pcHist = "xmmHistOne_7";
	if (iLast > 4)
	{
		fprintf(pf, "\n");
		if (iLast - 4 == 1)
			fprintf(pf, " // Старший 1 элемент гистограммы\n");
		else
			fprintf(pf, " // Старшие %d элемента гистограммы\n", iLast - 4);
		fprintf(pf, " xmmSumCurr = _mm_lddqu_si128(reinterpret_cast<__m128i *>(puiSumCurr + % d)); // Загружаем 128 бит (используем %d - %d эл. массива)\n", iElOffset + 4, 32 * (iLast - 4), iLast - 4);
		fprintf(pf, "\n");
		fprintf(pf, " xmmHistSub32 = _mm_cvtepu16_epi32(_mm_bsrli_si128(xmmHistSub, 8));// HistSub32[7], HistSub32[6], HistSub32[5], HistSub32[4] (Hi <-> Lo)\n");
		fprintf(pf, " xmmHistAdd32 = _mm_cvtepu16_epi32(_mm_bsrli_si128(xmmHistAdd, 8));\n");
		fprintf(pf, "\n");
		fprintf(pf, " // Корректируем значение гистограммы\n");
		if (0 == iElOffset)
		{
			fprintf(pf, " _mm_storeu_si128(reinterpret_cast<__m128i *>(pHistAdd), _mm_add_epi16(xmmHistAdd, % s)); // pHistAdd[0-%d]++; Сохраняем 128 бит (используем 128 - 8 эл. массива)\n", pcHist, iLast - 1);
			fprintf(pf, " _mm_storeu_si128(reinterpret_cast<__m128i *>(pHistSub), _mm_sub_epi16(xmmHistSub, % s)); // pHistSub[0-%d]--; Сохраняем 128 бит (используем 128 - 8 эл. массива)\n",
				pcHist, iLast - 1);
		}
		else
		{
			fprintf(pf, " _mm_storeu_si128(reinterpret_cast<__m128i *>(pHistAdd + %d), 	_mm_add_epi16(xmmHistAdd, % s)); // pHistAdd[0-%d]++; Сохраняем 128 бит (используем 128 - 8 эл. массива)\n", iElOffset, pcHist, iLast - 1);
			fprintf(pf, " _mm_storeu_si128(reinterpret_cast<__m128i *>(pHistSub + %d), _mm_sub_epi16(xmmHistSub, % s)); // pHistSub[0-%d]--; Сохраняем 128 бит (используем 128 - 8 эл. массива)\n",
				iElOffset, pcHist, iLast - 1);
		}
		fprintf(pf, "\n");
		if (iLast < 8)
		{
			if (5 == iLast)
			{
				fprintf(pf, " xmmValueAdd = _mm_bsrli_si128(xmmValueAdd, 12); // 0, 0, 0, xmmValueAdd(Hi < ->Lo)\n");
				fprintf(pf, " xmmValueSub = _mm_bsrli_si128(xmmValueSub, 12); // 0, 0, 	0, xmmValueSub(Hi < ->Lo)\n");
				fprintf(pf, " xmmValueAddSub = _mm_bsrli_si128(xmmValueAddSub, 12);\n");
				fprintf(pf, "\n");
			}
			else if (7 == iLast)
			{
				fprintf(pf, " xmmValueAdd = _mm_bsrli_si128(xmmValueAdd, 4); // 0, xmmValueAdd, xmmValueAdd, xmmValueAdd(Hi < ->Lo)\n");
				fprintf(pf, " xmmValueSub = _mm_bsrli_si128(xmmValueSub, 4); // 0, xmmValueSub, xmmValueSub, xmmValueSub(Hi < ->Lo)\n");
				fprintf(pf, " xmmValueAddSub = _mm_bsrli_si128(xmmValueAddSub, 4);\n");
				fprintf(pf, "\n");
			}
		}
		fprintf(pf, " xmmHistAdd32 = _mm_sub_epi32(_mm_mullo_epi32(xmmHistAdd32, xmmValueAdd), _mm_mullo_epi32(xmmHistSub32, xmmValueSub)); \n");
		fprintf(pf, " xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmHistAdd32); // <<= 	1\n");
		fprintf(pf, " xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmValueAddSub);\n");
		fprintf(pf, "\n");
		fprintf(pf, " _mm_storeu_si128(reinterpret_cast<__m128i *>(puiSumCurr + %d), _mm_add_epi32(xmmSumCurr, xmmHistAdd32)); // Сохраняем puiSumCurr[0,1,2,3]\n", iElOffset + 4);
	}
	else
	{
		if (0 == iElOffset)
		{
			fprintf(pf, " _mm_storeu_si128(reinterpret_cast<__m128i *>(pHistAdd), _mm_add_epi16(xmmHistAdd, % s)); // pHistAdd[0-%d]++; Сохраняем 128 бит (используем 128 - 8 эл. массива)\n",
				pcHist, iLast - 1);
			fprintf(pf, " _mm_storeu_si128(reinterpret_cast<__m128i *>(pHistSub), _mm_sub_epi16(xmmHistSub, % s)); // pHistSub[0-%d]--; Сохраняем 128 бит (используем 128 - 8 эл. массива)\n", pcHist, iLast - 1);
		}
		else
		{
			fprintf(pf, " _mm_storeu_si128(reinterpret_cast<__m128i *>(pHistAdd + %d), _mm_add_epi16(xmmHistAdd, % s)); // pHistAdd[0-%d]++; Сохраняем 128 бит (используем 128 - 8 эл. массива)\n", iElOffset, pcHist, iLast - 1);
			fprintf(pf, " _mm_storeu_si128(reinterpret_cast<__m128i *>(pHistSub + %d),	_mm_sub_epi16(xmmHistSub, % s)); // pHistSub[0-%d]--; Сохраняем 128 бит (используем 128 - 8 эл. массива)\n", iElOffset, pcHist, iLast - 1);
		}
	}
}
int iCodeGenRowsCols_V8_sse4_omp(FILE* pf, int Win_cen, int iFlagRows, int iFlagCols)
{
	int Win_size = (Win_cen << 1) + 1;
	int Win_lin = Win_size * Win_size;
	const char* pcAdd_iCmin = "";
	const char* pcAdd_iCminOr0 = "";
	const char* pcCurr = "";
	const char* pcCorrSum = "";
	const char* pcAll = "";
	if (0 != iFlagRows)
	{
		pcCorrSum = " - u32ValueSub * ((static_cast<uint32_t>(pHistSub[i]--) << 1) - 1)";
		if (1 != iFlagCols)
			pcAll = "All";
	}
	fprintf(pf, "\n");
	if (0 == iFlagCols)
	{
		fprintf(pf, " // Первые [0, 2*%d[ колонки\n", Win_cen);
		fprintf(pf, " for (iCol = 0; iCol < (%d << 1); iCol++)\n", Win_cen);
		fprintf(pf, " {\n");
		fprintf(pf, " iCmin = max(%d, static_cast<int>(iCol - %d));\n", Win_cen, Win_cen);
		fprintf(pf, " iCmax = iCol + %d;\n", Win_cen);
		fprintf(pf, "\n");
	}
	else if (1 == iFlagCols)
	{
		pcAdd_iCmin = " + iCmin";
		pcCurr = "Curr";
		pcAdd_iCminOr0 = "0";
		fprintf(pf, " // Средние [2*%d, ar_cmmIn.m_i64W - %d] колонки\n", Win_cen, Win_size);
		fprintf(pf, " puiSumCurr = puiSum+ ar_cmmIn.m_i64W * i_thr+ + %d;\n", Win_cen);
		if (0 != iFlagRows)
		{
			fprintf(pf, " pHist = pHistAll+256 * ar_cmmIn.m_i64W * i_thr + (iCol - %d);\n", Win_cen);
			fprintf(pf, " for (; iCol <= iColMax; iCol++, puiSumCurr++, pHist++)\n");
		}
		else
			fprintf(pf, " for (; iCol <= iColMax; iCol++, puiSumCurr++)\n");
		fprintf(pf, " {\n");
		if (0 == iFlagRows)
		{
			fprintf(pf, " iCmin = iCol - %d;\n", Win_cen);
			fprintf(pf, "\n");
		}
	}
	else
	{
		pcAdd_iCminOr0 = "iCmin";
		fprintf(pf, " // Последние [ar_cmmIn.m_i64W - 2*%d, ar_cmmIn.m_i64W - 1] колонки\n", Win_cen);
		fprintf(pf, " for (; iCol < ar_cmmIn.m_i64W; iCol++)\n");
		fprintf(pf, " {\n");
		fprintf(pf, " iCmin = iCol - %d;\n", Win_cen);
		fprintf(pf, " iCmax = min(static_cast<int>(ar_cmmIn.m_i64W - %d - 1), static_cast<int>(iCol + %d));\n", Win_cen, Win_cen);
		fprintf(pf, "\n");
	}
	fprintf(pf, " if ((d = (pRowIn[iCol] - pseudo_min)*kfct) < 0.5) // Так как при яркости 0 вклад в сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно	обнулить!\n");
	if (0 == iFlagRows)
		fprintf(pf, " pbBrightnessRow[iCol] = 0;\n");
	else
		fprintf(pf, " u32ValueAdd = 0;\n");
	fprintf(pf, " else\n");
	fprintf(pf, " {\n");
	fprintf(pf, " if ((u32ValueAdd = static_cast<uint32_t>(d + 0.5)) > 255)\n");
	if (0 == iFlagRows)
	{
		fprintf(pf, " {\n");
		fprintf(pf, " pbBrightnessRow[iCol] = u32ValueAdd = 255;\n");
		fprintf(pf, " pHistAdd = pHistAll + 256 * ar_cmmIn.m_i64W * i_thr + (255 * ar_cmmIn.m_i64W%s);\n", pcAdd_iCmin);
		fprintf(pf, " }\n");
		fprintf(pf, " else\n");
		fprintf(pf, " {\n");
		fprintf(pf, " pbBrightnessRow[iCol] = static_cast<uint8_t>(u32ValueAdd);\n");
		fprintf(pf, " pHistAdd = pHistAll + 256 * ar_cmmIn.m_i64W * i_thr+ (u32ValueAdd * ar_cmmIn.m_i64W%s);\n",
			pcAdd_iCmin);
		fprintf(pf, " }\n");
	}
	else
	{
		fprintf(pf, " u32ValueAdd = 255;\n");
		fprintf(pf, " }\n");
		fprintf(pf, " if (u32ValueAdd != (u32ValueSub = pbBrightnessRow[iCol]))\n");
		fprintf(pf, " {\n");
		if (pcAll != "All")
		{
			fprintf(pf, " pHistSub = pHist%s + (u32ValueSub * ar_cmmIn.m_i64W);\n", pcAll);
			fprintf(pf, " pHistAdd = pHist%s + (u32ValueAdd * ar_cmmIn.m_i64W);\n", pcAll);
		}
		else
		{
			fprintf(pf, " pHistSub = pHist%s +256 * ar_cmmIn.m_i64W * i_thr+ (u32ValueSub * ar_cmmIn.m_i64W);\n", pcAll);
			fprintf(pf, " pHistAdd = pHist%s +256 * ar_cmmIn.m_i64W * i_thr+ (u32ValueAdd * ar_cmmIn.m_i64W);\n", pcAll);
			
		}
		fprintf(pf, " pbBrightnessRow[iCol] = u32ValueAdd;\n");
		if ('\0' != pcCorrSum[0] && ((1 != iFlagRows) || (1 != iFlagCols)))
			fprintf(pf, " uint32_t u32ValueAddSub = u32ValueAdd + u32ValueSub;\n");
		fprintf(pf, "\n");
	}
	fprintf(pf, " // Добавляем яркость в %d гистограмм и сумм\n", Win_size);
	if (1 != iFlagCols)
		fprintf(pf, " for (i = iCmin; i <= iCmax; i++)\n");
	else
	{// 1 == iFlagCols
		if (1 != iFlagRows)
			fprintf(pf, " for (i = 0; i < %d; i++)\n", Win_size);
		else
		{// SSE*
			int iOffset = 0;
			int iLast = Win_size;
			if (Win_size > 8)
			{
				fprintf(pf, " //============================\n");
				fprintf(pf, " // Первые 8 точек\n");
				fprintf(pf, " //============================\n");
				FunV8_8(pf, 0, 8);
				iOffset = 8;
				iLast -= 8;
				if (Win_size > 16)
				{
					fprintf(pf, " //============================\n");
					fprintf(pf, " // Вторые 8 точек\n");
					fprintf(pf, " //============================\n");
					FunV8_8(pf, 8, 8);
					iOffset = 16;
					iLast -= 8;
				}
				fprintf(pf, " //============================\n");
				if (1 == iLast)
					fprintf(pf, " // Последняя 1 точка\n");
				else if (3 == iLast)
					fprintf(pf, " // Последние 3 точки\n");
				else
					fprintf(pf, " // Последние %d точек\n", iLast);
				fprintf(pf, " //============================\n");
			}
			else
			{
				fprintf(pf, " //============================\n");
				if (3 == iLast)
					fprintf(pf, " // Первые 3 точки\n");
				else
					fprintf(pf, " // Первые %d точек\n", iLast);
				fprintf(pf, " //============================\n");
			}
			FunV8_8(pf, iOffset, iLast);
		}
	}
	if ((1 != iFlagRows) || (1 != iFlagCols))
	{
		if ('\0' == pcCorrSum[0])
		{
			if(pcCurr =="")
			fprintf(pf, " puiSum%s[i+ar_cmmIn.m_i64W* i_thr] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1)); \n", pcCurr);
			else
				fprintf(pf, " puiSum%s[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1)); \n", pcCurr);
		}
		else
			if (pcCurr == "")
			fprintf(pf, " puiSum%s[i+ar_cmmIn.m_i64W* i_thr] += u32ValueAddSub + ((u32ValueAdd * static_cast<uint32_t>(pHistAdd[i]++) - u32ValueSub * static_cast<uint32_t>(pHistSub[i]--)) << 1); \n", pcCurr);
			else
				fprintf(pf, " puiSum%s[i] += u32ValueAddSub + ((u32ValueAdd * static_cast<uint32_t>(pHistAdd[i]++) - u32ValueSub * static_cast<uint32_t>(pHistSub[i]--)) << 1); \n", pcCurr);
	}
	fprintf(pf, " }\n");
	if (0 != iFlagCols)
	{
		if (0 == iFlagRows)
		{
			fprintf(pf, " if ((%d - 1) == iRow)\n", Win_size);
			fprintf(pf, " {\n");
			if (pcCurr == "")
			fprintf(pf, " pfRowOut[0] = static_cast<float>(puiSum%s[%s+ar_cmmIn.m_i64W* i_thr] * Size_obratn);\n",
				pcCurr, pcAdd_iCminOr0);
			else
				fprintf(pf, " pfRowOut[0] = static_cast<float>(puiSum%s[%s+ar_cmmIn.m_i64W* i_thr] * Size_obratn);\n",
					pcCurr, pcAdd_iCminOr0);
			fprintf(pf, " pfRowOut++;\n");
			fprintf(pf, " }\n");
		}
		else
		{
			if (pcCurr == "")
			fprintf(pf, " pfRowOut[0] = static_cast<float>(puiSum%s[%s] * Size_obratn);\n", pcCurr,
				pcAdd_iCminOr0);
			else
				fprintf(pf, " pfRowOut[0] = static_cast<float>(puiSum%s[%s] * Size_obratn);\n", pcCurr,
					pcAdd_iCminOr0);
			fprintf(pf, " pfRowOut++;\n");
		}
	}
	fprintf(pf, " }\n");
	return 0;
}
int iCodeGenRows_V8_sse4_omp(FILE* pf, int Win_cen, int iFlagRows)
{
	int Win_size = (Win_cen << 1) + 1;
	int Win_lin = Win_size * Win_size;
	fprintf(pf, "\n");
	if (0 == iFlagRows)
	{
		fprintf(pf, " // Первые [0, %d - 1] строки для кэша\n", Win_size);
		fprintf(pf, " #pragma omp parallel for private(pbBrightnessRow, iRow, iPos, iCol,  iCmin , iCmax, puiSumCurr, pHist, pHistAdd, pHistSub, i,d,u32ValueAdd, u32ValueSub)\n");
		fprintf(pf, " for(int i_thr = 0; i_thr < thr_amount; ++i_thr){");
		fprintf(pf, " pbBrightnessRow = pbBrightness + ar_cmmIn.m_i64W * %d* i_thr;\n", Win_size);
		fprintf(pf, " float *pfRowOut = ar_cmmOut.pfGetRow(%d + iRowThr[i_thr] -%d) + %d;\n", Win_cen, Win_size,Win_cen);
		fprintf(pf, " for (iRow = iRowThr[i_thr]-%d; iRow < iRowThr[i_thr]; iRow++, pbBrightnessRow += ar_cmmIn.m_i64W)\n", Win_size);
	}
	else
	{
		int i, j8 = Win_size % 8;
		

		fprintf(pf, " __m128i xmmHistOne_%d = _mm_set_epi16(", j8);
		for (i = 0; i < 8 - j8; i++)
		{
			fprintf(pf, "0");
			if (i < 7)
				fprintf(pf, ", ");
		}
		for (; i < 8; i++)
		{
			fprintf(pf, "1");
			if (i < 7)
				fprintf(pf, ", ");
		}
		fprintf(pf, "); // Для inc или dec сразу для %d гистограмм\n", j8);
		if (Win_size > 8)
			fprintf(pf, " __m128i xmmHistOne_All = _mm_set_epi16(1, 1, 1, 1, 1, 1, 1, 1); // Для inc или dec сразу для всех гистограмм\n");
		fprintf(pf, "\n");
		fprintf(pf, " // Последующие строки [%d, ar_cmmIn.m_i64H[\n", Win_size);
		if (1 == iFlagRows)
		{
			//if (Win_size > 8)
			//fprintf(pf, " #pragma omp parallel for private(pbBrightnessRow, iRow, iPos, iCol,  iCmin , iCmax, puiSumCurr, pHist, pHistAdd, pHistSub, i,d)\n");
			//else
				//fprintf(pf, " #pragma omp parallel for private(pbBrightnessRow, iRow, iPos , xmmHistOne_%d , iCol,  iCmin , iCmax, puiSumCurr, pHist, pHistAdd, pHistSub, i,d,u32ValueAdd, u32ValueSub)\n", j8);
			//fprintf(pf, " {\n");
			//fprintf(pf, " int offset = \n");
			//fprintf(pf, " for (int i_thr = 0; i_thr < thr_amount; ++i_thr){\n");
		}
		fprintf(pf, " for (iRow = iRowThr[i_thr]; iRow < iRowThrEnds[i_thr]; iRow++)\n", Win_size);
		//fprintf(pf, " for (iRow = %d; iRow < ar_cmmIn.m_i64H; iRow++)\n", Win_size);
	}
	fprintf(pf, " {\n");
	fprintf(pf, " // Обнуляем края строк.\n");
	fprintf(pf, " memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * %d);\n", Win_cen);
	fprintf(pf, " memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - %d), 0, sizeof(float) * %d);\n",
		Win_cen, Win_cen);
	fprintf(pf, "\n");
	fprintf(pf, " uint16_t *pRowIn = ar_cmmIn.pu16GetRow(iRow);\n");
	if (0 != iFlagRows)
	{
		fprintf(pf, " float *pfRowOut = ar_cmmOut.pfGetRow(iRow - %d) + %d;\n", Win_cen, Win_cen);
		fprintf(pf, " iPos = (iRow - %d) %% %d;\n", Win_size, Win_size);
		fprintf(pf, " pbBrightnessRow = pbBrightness+ar_cmmIn.m_i64W * %d* i_thr + (iPos * ar_cmmIn.m_i64W);\n", Win_size);
	}
	// Первые колонки
	if (0 != iCodeGenRowsCols_V8_sse4_omp(pf, Win_cen, iFlagRows, 0))
		return 10;
	// Средние колонки
	if (0 != iCodeGenRowsCols_V8_sse4_omp(pf, Win_cen, iFlagRows, 1))
		return 20;
	// Последние колонки
	if (0 != iCodeGenRowsCols_V8_sse4_omp(pf, Win_cen, iFlagRows, 2))
		return 30;

	fprintf(pf, " }\n");
	//if (0 == iFlagRows)
	//fprintf(pf, " }\n");
	if (1 == iFlagRows)
		fprintf(pf, " }\n");
	return 0;
}
int iCodeGen_V8_sse4_omp(const char* a_pcFilePath)
{
	FILE* pf;
	fopen_s(&pf, a_pcFilePath, "wt");
	if (nullptr == pf)
		return 10;
	fprintf(pf, "#define _CRT_SECURE_NO_WARNINGS\n");
	fprintf(pf, "\n");
	fprintf(pf, "#include <iostream>\n");
	fprintf(pf, "#include <conio.h>\n");
	fprintf(pf, "#include <string>\n");
	fprintf(pf, "#include <omp.h>\n"); 
	fprintf(pf, "#include <cstdlib>\n"); 
	fprintf(pf, "\n");
	fprintf(pf, "#include \"MU16Data.h\"\n");
	fprintf(pf, "\n");
	fprintf(pf, "#include \"TextureFiltr_Mean_V8_CodeGen.h\"\n");
	fprintf(pf, "using namespace std;\n");
	for (int Win_cen = 1; Win_cen <= 10; Win_cen++)
	{
		fprintf(pf, "\n");
		int Win_size = (Win_cen << 1) + 1;
		int Win_lin = Win_size * Win_size;
		// Пролог
		fprintf(pf, "\n");
		fprintf(pf, "void TextureFiltr_Mean_V8_%d_sse4_omp(MU16Data &ar_cmmIn, MFData &ar_cmmOut, double pseudo_min, double kfct, int thr_amount)\n", Win_size);
		fprintf(pf, "{\n");
		fprintf(pf, " double Size_obratn = 1.0 / %d;\n", Win_lin);
		fprintf(pf, "\n");
		fprintf(pf, " // Кэш для гистограмм\n");
		fprintf(pf, " uint16_t *pHistAll = new uint16_t[256 * ar_cmmIn.m_i64W * thr_amount]; // [256][ar_cmmIn.m_i64W]\n");
		fprintf(pf, " memset(pHistAll, 0, sizeof(uint16_t) * 256 * ar_cmmIn.m_i64W * thr_amount);\n");
		fprintf(pf, " uint16_t *pHist, *pHistAdd, *pHistSub;\n");
		fprintf(pf, "\n");
		fprintf(pf, " // Кэш для сумм\n");
		fprintf(pf, " uint32_t *puiSum = new uint32_t[ar_cmmIn.m_i64W* thr_amount], *puiSumCurr;\n");
		fprintf(pf, " memset(puiSum, 0, sizeof(uint32_t) * ar_cmmIn.m_i64W* thr_amount);\n");
		fprintf(pf, "\n");
		fprintf(pf, " // Кэш для яркостей\n");
		fprintf(pf, " uint8_t *pbBrightness = new uint8_t[ar_cmmIn.m_i64W * %d* thr_amount];\n", Win_size);
		fprintf(pf, " uint8_t *pbBrightnessRow;\n");
		fprintf(pf, "\n");
		fprintf(pf, " int64_t iRow, iCol, iColMax = ar_cmmIn.m_i64W - %d, iThrRowMax = iColMax/thr_amount;\n", Win_size);
		fprintf(pf, " int64_t* iRowThr = new int64_t[thr_amount], *iRowThrEnds = new int64_t[thr_amount];\n");
		fprintf(pf, " iRowThr[0] = %d;\n", Win_size);
		fprintf(pf, " iRowThrEnds[0] = %d + iThrRowMax;\n", Win_size);
		fprintf(pf, " for(int i = 1; i < thr_amount-1; ++i){");
		fprintf(pf, " iRowThr[i] = iRowThr[i-1] + iThrRowMax;\n");
		fprintf(pf, " iRowThrEnds[i] = iRowThrEnds[i-1] + iThrRowMax;}\n");
		fprintf(pf, " iRowThr[thr_amount-1] = iRowThr[thr_amount-1-1] + iThrRowMax;\n");
		fprintf(pf, " iRowThrEnds[thr_amount-1] = ar_cmmIn.m_i64W;\n");
		fprintf(pf, " ar_cmmOut.iCreate(ar_cmmIn.m_i64W, ar_cmmIn.m_i64H, 6);\n");
		fprintf(pf, "\n");
		fprintf(pf, " // Обнуляем края. Первые строки.\n");
		fprintf(pf, " for (iRow = 0; iRow < %d; iRow++)\n", Win_cen);
		fprintf(pf, " memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);\n");
		fprintf(pf, "\n");
		fprintf(pf, " int64_t i, iCmin, iCmax, iPos = 0;\n");
		fprintf(pf, " uint32_t u32ValueAdd, u32ValueSub;\n");
		fprintf(pf, " double d;\n");
		// Начальные строки
		if (0 != iCodeGenRows_V8_sse4_omp(pf, Win_cen, 0))
		{
			fclose(pf);
			return 20;
		}
		// Последующие строки
		if (0 != iCodeGenRows_V8_sse4_omp(pf, Win_cen, 1))
		{
			fclose(pf);
			return 30;
		}
		// Эпилог
		fprintf(pf, "\n");
		fprintf(pf, " // Обнуляем края. Последние строки.\n");
		fprintf(pf, " for (iRow = ar_cmmIn.m_i64H - %d; iRow < ar_cmmOut.m_i64H; iRow++)\n", Win_cen);
		fprintf(pf, " memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);\n");
		fprintf(pf, "\n");
		fprintf(pf, " if (nullptr != pHistAll)\n");
		fprintf(pf, " delete[] pHistAll;\n");
		fprintf(pf, " if (nullptr != puiSum)\n");
		fprintf(pf, " delete[] puiSum;\n");
		fprintf(pf, " if (nullptr != iRowThr)\n");
		fprintf(pf, " delete[] iRowThr;\n");
		fprintf(pf, " if (nullptr != pbBrightness)\n");
		fprintf(pf, " delete[] pbBrightness;\n");
		fprintf(pf, "}\n");
	}
	fprintf(pf, "\n");
	fprintf(pf, "tdTextureFiltr_Mean_V8_omp g_afunTextureFiltr_Mean_V8_sse4_omp[10] = \n");
	fprintf(pf, "{\n");
	fprintf(pf, " TextureFiltr_Mean_V8_3_sse4_omp, // 0\n");
	fprintf(pf, " TextureFiltr_Mean_V8_5_sse4_omp, // 1\n");
	fprintf(pf, " TextureFiltr_Mean_V8_7_sse4_omp, // 2\n");
	fprintf(pf, " TextureFiltr_Mean_V8_9_sse4_omp, // 3\n");
	fprintf(pf, " TextureFiltr_Mean_V8_11_sse4_omp, // 4\n");
	fprintf(pf, " TextureFiltr_Mean_V8_13_sse4_omp, // 5\n");
	fprintf(pf, " TextureFiltr_Mean_V8_15_sse4_omp, // 6\n");
	fprintf(pf, " TextureFiltr_Mean_V8_17_sse4_omp, // 7\n");
	fprintf(pf, " TextureFiltr_Mean_V8_19_sse4_omp, // 8\n");
	fprintf(pf, " TextureFiltr_Mean_V8_21_sse4_omp // 9\n");
	fprintf(pf, "};\n");
	fclose(pf);
	return 0;
}

int main()
{
	std::string a_pcFilePath = "TextureFiltr_Mean_V8_sse4_omp.cpp";
	//iCodeGen_V8(a_pcFilePath.c_str());
	iCodeGen_V8_sse4_omp(a_pcFilePath.c_str());
	return 0;
}
// Запуск программы: CTRL+F5 или меню "Отладка" > "Запуск без отладки"
// Отладка программы: F5 или меню "Отладка" > "Запустить отладку"

// Советы по началу работы 
//   1. В окне обозревателя решений можно добавлять файлы и управлять ими.
//   2. В окне Team Explorer можно подключиться к системе управления версиями.
//   3. В окне "Выходные данные" можно просматривать выходные данные сборки и другие сообщения.
//   4. В окне "Список ошибок" можно просматривать ошибки.
//   5. Последовательно выберите пункты меню "Проект" > "Добавить новый элемент", чтобы создать файлы кода, или "Проект" > "Добавить существующий элемент", чтобы добавить в проект существующие файлы кода.
//   6. Чтобы снова открыть этот проект позже, выберите пункты меню "Файл" > "Открыть" > "Проект" и выберите SLN-файл.
