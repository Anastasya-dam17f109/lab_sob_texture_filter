// ConsoleApplication1.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <iostream>
#include <conio.h>
#include <string>
#include <omp.h>
#include <pmmintrin.h>
#include <smmintrin.h>
#include <immintrin.h>
#include <emmintrin.h>
#include <xmmintrin.h>
#include <fstream>
#include "MU16Data.h"
#include "TextureFiltr_Mean_V8_CodeGen.h"
using namespace std;
//================================================================
class BTexture
{
public:
	BTexture();

	void Create(uint8_t *a_pVarRow, int a_Win_cen);
	void CalcHist();
	double Calc_Mean();


	uint8_t *m_pVarRow;
	uint16_t hist[256];
	int Win_cen;
	int Win_size;
	int Win_Lin;
	double Size_obratn;
};
//================================================================

BTexture::BTexture()
{
	m_pVarRow = nullptr;
}

void BTexture::Create(uint8_t *a_pVarRow, int a_Win_cen)
{
	Win_cen = a_Win_cen;
	Win_size = (Win_cen << 1) + 1;
	Win_Lin = Win_size * Win_size;
	Size_obratn = 1.0 / Win_Lin;
	m_pVarRow = a_pVarRow;
}

void BTexture::CalcHist()
{
	memset(hist, 0, sizeof(uint16_t) * 256);

	for (int i = 0; i < Win_Lin; i++)
	{
		hist[m_pVarRow[i]]++;
	}
}

double BTexture::Calc_Mean()
{
	CalcHist();
	double Sum = 0.0;
	for (int i = 0; i < Win_Lin; i++)
	{
		Sum += m_pVarRow[i] * hist[m_pVarRow[i]] * Size_obratn;
	}
	return Sum;
}
//================================================================
//================================================================

void TextureFiltr_Mean(MU16Data &ar_cmmIn, MFData &ar_cmmOut, int Win_cen, double pseudo_min, double kfct)
{
	int i = (Win_cen << 1) + 1;
	int Win_lin = i * i;
	uint8_t *pVarRow = new uint8_t[Win_lin];
	BTexture bt;
	bt.Create(pVarRow, Win_cen);

	int64_t iRow, iCol, iRowMax = ar_cmmIn.m_i64H - Win_cen, iColMax = ar_cmmIn.m_i64W - Win_cen;
	ar_cmmOut.iCreate(ar_cmmIn.m_i64W, ar_cmmIn.m_i64H, 6);

	// Обнуляем края. Первые строки.
	for (iRow = 0; iRow < Win_cen; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

	int ln, rw;
	for (iRow = Win_cen; iRow < iRowMax; iRow++)
	{
		float *pfRowOut = ar_cmmOut.pfGetRow(iRow);
		// Обнуляем края. Левый край.
		memset(pfRowOut, 0, sizeof(float) * Win_cen);

		for (iCol = Win_cen; iCol < iColMax; iCol++)
		{
			int i_num = 0;
			for (rw = -Win_cen; rw <= Win_cen; rw++)
			{
				uint16_t *pRowIn = ar_cmmIn.pu16GetRow(iRow + rw);
				for (ln = -Win_cen; ln <= Win_cen; ln++)
				{
					double d = (pRowIn[iCol + ln] - pseudo_min)*kfct;
					if (d <= 0.)
						pVarRow[i_num] = 0;
					else
					{
						int iVal = static_cast<int>(d + 0.5);
						if (iVal > 255)
							pVarRow[i_num] = 255;
						else
							pVarRow[i_num] = static_cast<uint8_t>(iVal);
					}
					i_num++;
				}
			}
			pfRowOut[iCol] = static_cast<float>(bt.Calc_Mean());
		}

		// Обнуляем края. Правый край.
		memset(pfRowOut + iColMax, 0, sizeof(float) * (ar_cmmOut.m_i64LineSizeEl - iColMax));
	}

	// Обнуляем края. Последние строки.
	for (iRow = iRowMax; iRow < ar_cmmOut.m_i64H; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

	if (nullptr != pVarRow)
		delete[] pVarRow;
}

//

void TextureFiltr_Mean_omp1(MU16Data& ar_cmmIn, MFData& ar_cmmOut, int Win_cen, double pseudo_min, double kfct, int thrAmount)
{
	int i = (Win_cen << 1) + 1;
	int Win_lin = i * i;

	uint8_t* pVarRow = new uint8_t[Win_lin * thrAmount];
	BTexture *bt = new BTexture[thrAmount];
	for(int iter = 0; iter < thrAmount; ++iter)
		bt[iter].Create(pVarRow + iter * Win_lin, Win_cen);

	int64_t iRow, iCol, iRowMax = ar_cmmIn.m_i64H - Win_cen, iColMax = ar_cmmIn.m_i64W - Win_cen;
	ar_cmmOut.iCreate(ar_cmmIn.m_i64W, ar_cmmIn.m_i64H, 6);
	// Обнуляем края.Первые строки.
		for (iRow = 0; iRow < Win_cen; iRow++)
			memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);
		int ln, rw;
	/*
	* распараллеливание фильтрации
	* фильтрованое изображение мы получаем, проходя по строкам исходного изображения
	* поэтому распараллеливаем по строкам
	* Механизм фильтрации:
	* 1. вырезаем из изображения кусочек, размером  равным окну фильтра
	* координата центра этого кусочка на исходном изображении - это координата пикселя на 
	* результирующем изображении, в который мы запишем результат фильтрации
	* 2. считаем гистограмму яркостей этого кусочка ( гистограмма строилась на отрезке от 0 
	* до 255, число интервалов - 256, т е по значениям)
	* 3. Фильтруем: каждый пиксель (яркость) домножаем на частоту появления в гистограмме
	* результаты перемножений суммируем и записываем в нужную ячейку на изображении результате
	* 
	* Дополнительно:
	* в момент, когда вырезается на исходном изображении кусок для фильтрации
	* яркости пикселей в этом куске лежат в диапазоне более широком, чем 0-255
	* перед осушествлением фильтрации значения яркостей нужно домножением на соответвующие коэффициенты
	* привести к диапазону 0-255
	* 
	* замечание:
	* в отличие от прошлой задачи, мы не делаем дополнительных действий для обработки краев.
	* полосы, равные половине ширины на изображении- результате по краям просто составляют из 0
	* 
	* Механизм распараллеливания:
	* для рассчета гистограммы яркостей окна фильтра нам нужен контейнер, в котором она будет лежать.
	* для каждого потока это должен быть отдельный контейнер, чтобы они не писали в одно место памяти и не
	* возникло гонки
	* можно контейнер под гистограмму создавать в каждом потоке перед parallel for
	* но это долго
	* Более оптимальное решение:
	* перед началом параллельной секции создать массив контейнеров под гистограммы
	* число элементов в этом массиве принять равным числу потоков
	* тогда во время исполнения поток будет писать в ячейку этого массива, равную его номеру
	* в коде реализовано чуть сложнее - место записи 
	* uint8_t* pVarRow_thr = pVarRow + offset * Win_lin;
	* вместо взятия по индексу мы сдвигаем указатель на нужную нам ячейку
	* на число байт памяти равную индекс элемента * размер контейнера
	*/
#pragma omp parallel private(ln, rw, iCol) num_threads(thrAmount)
	{
			int offset = omp_get_thread_num();
			uint8_t* pVarRow_thr = pVarRow + offset * Win_lin;

		
#pragma omp for
		for (iRow = Win_cen; iRow < iRowMax; iRow++)
		{
			float* pfRowOut = ar_cmmOut.pfGetRow(iRow);
			// Обнуляем края. Левый край.
			memset(pfRowOut, 0, sizeof(float) * Win_cen);

			for (iCol = Win_cen; iCol < iColMax; iCol++)
			{
				int i_num = 0;
				for (rw = -Win_cen; rw <= Win_cen; rw++)
				{
					uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow + rw);
					for (ln = -Win_cen; ln <= Win_cen; ln++)
					{
						double d = (pRowIn[iCol + ln] - pseudo_min) * kfct;
						if (d <= 0.)
							pVarRow_thr[i_num] = 0;
						else
						{
							int iVal = static_cast<int>(d + 0.5);
							if (iVal > 255)
								pVarRow_thr[i_num] = 255;
							else
								pVarRow_thr[i_num] = static_cast<uint8_t>(iVal);
						}
						i_num++;
					}
				}
				pfRowOut[iCol] = static_cast<float>(bt[offset].Calc_Mean());
			}

			// Обнуляем края. Правый край.
			memset(pfRowOut + iColMax, 0, sizeof(float) * (ar_cmmOut.m_i64LineSizeEl - iColMax));
		}
	}

	// Обнуляем края. Последние строки.
	for (iRow = iRowMax; iRow < ar_cmmOut.m_i64H; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

	if (nullptr != pVarRow)
		delete[] pVarRow;
}

//
// 
void TextureFiltr_Mean_omp2(MU16Data& ar_cmmIn, MFData& ar_cmmOut, int Win_cen, double pseudo_min, double kfct)
{
	int i = (Win_cen << 1) + 1;
	int Win_lin = i * i;
	uint8_t* pVarRow = new uint8_t[Win_lin];
	uint16_t* pHist = new uint16_t[256];
	double Size_obratn = 1.0 / Win_lin;
	int64_t iRow, iCol, iRowMax = ar_cmmIn.m_i64H - Win_cen, iColMax = ar_cmmIn.m_i64W - Win_cen;
	ar_cmmOut.iCreate(ar_cmmIn.m_i64W, ar_cmmIn.m_i64H, 6);
	// Обнуляем края. Первые строки.
	for (iRow = 0; iRow < Win_cen; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);
	int ln, rw;
	for (iRow = Win_cen; iRow < iRowMax; iRow++)
	{
		float* pfRowOut = ar_cmmOut.pfGetRow(iRow);
		// Обнуляем края. Левый край.
		memset(pfRowOut, 0, sizeof(float) * Win_cen);
		for (iCol = Win_cen; iCol < iColMax; iCol++)
		{
			memset(pHist, 0, sizeof(uint16_t) * 256);
			int i_num = 0;
			for (rw = -Win_cen; rw <= Win_cen; rw++)
			{
				uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow + rw);
				for (ln = -Win_cen; ln <= Win_cen; ln++)
				{
					double d = (pRowIn[iCol + ln] - pseudo_min) * kfct;
					if (d <= 0.)
						pHist[pVarRow[i_num] = 0]++;
					else
					{
						int iVal = static_cast<int>(d + 0.5);
						if (iVal > 255)
							pHist[pVarRow[i_num] = 255]++;
						else
							pHist[pVarRow[i_num] = static_cast<uint8_t>(iVal)]++;
					}
					i_num++;
				}
			}
			uint32_t Sum = 0;
			for (int i = 0; i < Win_lin; i++)
			{
				Sum += pVarRow[i] * static_cast<uint32_t>(pHist[pVarRow[i]]);
			}
			pfRowOut[iCol] = static_cast<float>(Sum * Size_obratn);
		}
		// Обнуляем края. Правый край.
		memset(pfRowOut + iColMax, 0, sizeof(float) * (ar_cmmOut.m_i64LineSizeEl - iColMax));
	}
	// Обнуляем края. Последние строки.
	for (iRow = iRowMax; iRow < ar_cmmOut.m_i64H; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);
	if (nullptr != pVarRow)
		delete[] pVarRow;
	if (nullptr != pHist)
		delete[] pHist;

}


void TextureFiltr_Mean_omp2_1(MU16Data& ar_cmmIn, MFData& ar_cmmOut, int Win_cen, double pseudo_min, double kfct)
{
	int i = (Win_cen << 1) + 1;
	int Win_lin = i * i;
	uint8_t* pVarRow = new uint8_t[Win_lin];
	uint16_t* pHist = new uint16_t[256];
	double Size_obratn = 1.0 / Win_lin;
	int64_t iRow, iCol, iRowMax = ar_cmmIn.m_i64H - Win_cen, iColMax = ar_cmmIn.m_i64W - Win_cen;
	ar_cmmOut.iCreate(ar_cmmIn.m_i64W, ar_cmmIn.m_i64H, 6);
	// Обнуляем края. Первые строки.
	for (iRow = 0; iRow < Win_cen; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);
	int ln, rw;
	for (iRow = Win_cen; iRow < iRowMax; iRow++)
	{
		float* pfRowOut = ar_cmmOut.pfGetRow(iRow);
		// Обнуляем края. Левый край.
		memset(pfRowOut, 0, sizeof(float) * Win_cen);
		for (iCol = Win_cen; iCol < iColMax; iCol++)
		{
			memset(pHist, 0, sizeof(uint16_t) * 256);
			int i_num = 0;
			for (rw = -Win_cen; rw <= Win_cen; rw++)
			{
				uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow + rw);
				for (ln = -Win_cen; ln <= Win_cen; ln++)
				{
					double d = (pRowIn[iCol + ln] - pseudo_min) * kfct;
					if (d <= 0.)
						pHist[pVarRow[i_num] = 0]++;
					else
					{
						int iVal = static_cast<int>(d + 0.5);
						if (iVal > 255)
							pHist[pVarRow[i_num] = 255]++;
						else
							pHist[pVarRow[i_num] = static_cast<uint8_t>(iVal)]++;
					}
					i_num++;
				}
			}
			uint32_t Sum = 0;
			for (uint32_t i = 0; i < 256; i++)
			{
				uint32_t iH = pHist[i];
				Sum += i * iH * iH;
			}

			pfRowOut[iCol] = static_cast<float>(Sum * Size_obratn);
		}
		// Обнуляем края. Правый край.
		memset(pfRowOut + iColMax, 0, sizeof(float) * (ar_cmmOut.m_i64LineSizeEl - iColMax));
	}
	// Обнуляем края. Последние строки.
	for (iRow = iRowMax; iRow < ar_cmmOut.m_i64H; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);
	if (nullptr != pVarRow)
		delete[] pVarRow;
	if (nullptr != pHist)
		delete[] pHist;

}

void TextureFiltr_Mean_omp2_2(MU16Data& ar_cmmIn, MFData& ar_cmmOut, int Win_cen, double pseudo_min, double kfct)
{
	int i = (Win_cen << 1) + 1;
	int Win_lin = i * i;

	uint8_t* pVarRow = new uint8_t[Win_lin];
	uint16_t* pHist = new uint16_t[256];
	double Size_obratn = 1.0 / Win_lin;
	int64_t iRow, iCol, iRowMax = ar_cmmIn.m_i64H - Win_cen, iColMax = ar_cmmIn.m_i64W - Win_cen;
	ar_cmmOut.iCreate(ar_cmmIn.m_i64W, ar_cmmIn.m_i64H, 6);
	// Обнуляем края. Первые строки.
	memset(pHist, 0, sizeof(uint16_t) * 256);
	for (iRow = 0; iRow < Win_cen; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);
	int ln, rw;
	for (iRow = Win_cen; iRow < iRowMax; iRow++)
	{
		float* pfRowOut = ar_cmmOut.pfGetRow(iRow);
		// Обнуляем края. Левый край.
		memset(pfRowOut, 0, sizeof(float) * Win_cen);
		for (iCol = Win_cen; iCol < iColMax; iCol++)
		{
			//memset(pHist, 0, sizeof(uint16_t) * 256);
			int i_num = 0;
			for (rw = -Win_cen; rw <= Win_cen; rw++)
			{
				uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow + rw);
				for (ln = -Win_cen; ln <= Win_cen; ln++)
				{
					double d = (pRowIn[iCol + ln] - pseudo_min) * kfct;
					if (d <= 0.)
						pHist[pVarRow[i_num] = 0]++;
					else
					{
						int iVal = static_cast<int>(d + 0.5);
						if (iVal > 255)
							pHist[pVarRow[i_num] = 255]++;
						else
							pHist[pVarRow[i_num] = static_cast<uint8_t>(iVal)]++;
					}
					i_num++;
				}
			}
			uint32_t Sum = 0;
			for (uint32_t i = 0; i < Win_lin; i++)
			{
				uint32_t iValue = static_cast<uint32_t>(pVarRow[i]);
				uint32_t iH = static_cast<uint32_t>(pHist[iValue]);
				Sum += iValue * iH * iH;
				pHist[iValue] = 0;
			}

			pfRowOut[iCol] = static_cast<float>(Sum * Size_obratn);
		}
		// Обнуляем края. Правый край.
		memset(pfRowOut + iColMax, 0, sizeof(float) * (ar_cmmOut.m_i64LineSizeEl - iColMax));
	}
	// Обнуляем края. Последние строки.
	for (iRow = iRowMax; iRow < ar_cmmOut.m_i64H; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);
	if (nullptr != pVarRow)
		delete[] pVarRow;
	if (nullptr != pHist)
		delete[] pHist;

}

//
// 

void TextureFiltr_Mean_omp2_3(MU16Data& ar_cmmIn, MFData& ar_cmmOut, int Win_cen, double pseudo_min, double kfct)
{
	int Win_size = (Win_cen << 1) + 1;
	int Win_lin = Win_size * Win_size;
	uint8_t* pVarRow = new uint8_t[Win_lin];
	uint16_t* pHist = new uint16_t[256];
	double Size_obratn = 1.0 / Win_lin;
	int64_t iRow, iCol, iRowMax = ar_cmmIn.m_i64H - Win_cen, iColMax = ar_cmmIn.m_i64W - Win_cen;
	ar_cmmOut.iCreate(ar_cmmIn.m_i64W, ar_cmmIn.m_i64H, 6);
	// Обнуляем края. Первые строки.
	for (iRow = 0; iRow < Win_cen; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);
	int ln, rw;
	for (iRow = Win_cen; iRow < iRowMax; iRow++)
	{
		float* pfRowOut = ar_cmmOut.pfGetRow(iRow);
		// Обнуляем края. Левый край.
		memset(pfRowOut, 0, sizeof(float) * Win_cen);
		// Первая точка в строке
		int iPos = 0;
		memset(pHist, 0, sizeof(uint16_t) * 256);
		iCol = Win_cen;
		{
			for (rw = -Win_cen; rw <= Win_cen; rw++)
			{
				uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow + rw);
				for (ln = -Win_cen; ln <= Win_cen; ln++)
				{
					double d = (pRowIn[iCol + ln] - pseudo_min) * kfct;
					if (d <= 0.)
						pHist[pVarRow[(ln + Win_cen) * Win_size + (rw + Win_cen)] = 0]++;
					else
					{
						int iVal = static_cast<int>(d + 0.5);
						if (iVal > 255)
							pHist[pVarRow[(ln + Win_cen) * Win_size + (rw + Win_cen)] = 255]++;
						else
							pHist[pVarRow[(ln + Win_cen) * Win_size + (rw + Win_cen)] =
							static_cast<uint8_t>(iVal)]++;
					}
				}
			}
			uint32_t Sum = 0;
			for (int i = 0; i < Win_lin; i++)
			{
				uint32_t iValue = static_cast<uint32_t>(pVarRow[i]);
				Sum += iValue * static_cast<uint32_t>(pHist[iValue]);
			}
			pfRowOut[iCol] = static_cast<float>(Sum * Size_obratn);
		}
		for (++iCol; iCol < iColMax; iCol++)
		{
			uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow - Win_cen) + iCol + Win_cen;
			for (rw = 0; rw < Win_size; rw++, pRowIn += ar_cmmIn.m_i64LineSizeEl)
			{
				// Вычитаем из гистограммы столбец iCol - Win_cen - 1
				pHist[pVarRow[iPos + rw]]--;
				// Добавим в гистограмму столбец iCol + Win_cen
				double d = (pRowIn[0] - pseudo_min) * kfct;
				if (d <= 0.)
					pHist[pVarRow[iPos + rw] = 0]++;
				else
				{
					int iVal = static_cast<int>(d + 0.5);
					if (iVal > 255)
						pHist[pVarRow[iPos + rw] = 255]++;
					else
					{
						pHist[pVarRow[iPos + rw] = static_cast<uint8_t>(iVal)]++;
					}
				}
			}
			iPos += Win_size;
			if (iPos >= Win_lin)
				iPos = 0;
			uint32_t Sum = 0;
			for (int i = 0; i < Win_lin; i++)
			{
				uint32_t iValue = static_cast<uint32_t>(pVarRow[i]);
				Sum += iValue * static_cast<uint32_t>(pHist[iValue]);
			}
			pfRowOut[iCol] = static_cast<float>(Sum * Size_obratn);
		}
		// Обнуляем края. Правый край.
		memset(pfRowOut + iColMax, 0, sizeof(float) * (ar_cmmOut.m_i64LineSizeEl - iColMax));
	}
	// Обнуляем края. Последние строки.
	for (iRow = iRowMax; iRow < ar_cmmOut.m_i64H; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);
	if (nullptr != pVarRow)
		delete[] pVarRow;
	if (nullptr != pHist)
		delete[] pHist;

}

//
//

void TextureFiltr_Mean_omp2_4(MU16Data& ar_cmmIn, MFData& ar_cmmOut, int Win_cen, double pseudo_min, double kfct)
{
	int Win_size = (Win_cen << 1) + 1;
	int Win_lin = Win_size * Win_size;
	uint8_t* pVarRow = new uint8_t[Win_lin];
	uint16_t* pHist = new uint16_t[256];
	double Size_obratn = 1.0 / Win_lin;
	int64_t iRow, iCol, iRowMax = ar_cmmIn.m_i64H - Win_cen, iColMax = ar_cmmIn.m_i64W - Win_cen;
	ar_cmmOut.iCreate(ar_cmmIn.m_i64W, ar_cmmIn.m_i64H, 6);
	// Обнуляем края. Первые строки.
	for (iRow = 0; iRow < Win_cen; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);
	int ln, rw;
	for (iRow = Win_cen; iRow < iRowMax; iRow++)
	{
		float* pfRowOut = ar_cmmOut.pfGetRow(iRow);
		// Обнуляем края. Левый край.
		memset(pfRowOut, 0, sizeof(float) * Win_cen);
		// Первая точка в строке
		int iPos = 0;
		uint32_t Sum = 0;
		memset(pHist, 0, sizeof(uint16_t) * 256);
		iCol = Win_cen;
		{
			for (rw = -Win_cen; rw <= Win_cen; rw++)
			{
				uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow + rw);
				for (ln = -Win_cen; ln <= Win_cen; ln++)
				{
					double d = (pRowIn[iCol + ln] - pseudo_min) * kfct;
					if (d <= 0.)
						pHist[pVarRow[(ln + Win_cen) * Win_size + (rw + Win_cen)] = 0]++;
					else
					{
						int iVal = static_cast<int>(d + 0.5);
						if (iVal > 255)
							pHist[pVarRow[(ln + Win_cen) * Win_size + (rw + Win_cen)] = 255]++;
						else
							pHist[pVarRow[(ln + Win_cen) * Win_size + (rw + Win_cen)] =
							static_cast<uint8_t>(iVal)]++;
					}
				}
			}
			for (int i = 0; i < Win_lin; i++)
			{
				uint32_t iValue = static_cast<uint32_t>(pVarRow[i]);
				Sum += iValue * static_cast<uint32_t>(pHist[iValue]);
			}
			pfRowOut[iCol] = static_cast<float>(Sum * Size_obratn);
		}
		for (++iCol; iCol < iColMax; iCol++)
		{
			uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow - Win_cen) + iCol + Win_cen;
			for (rw = 0; rw < Win_size; rw++, pRowIn += ar_cmmIn.m_i64LineSizeEl)
			{
				// Вычитаем из гистограммы столбец iCol - Win_cen - 1
				uint32_t iValue = pVarRow[iPos + rw];
				uint32_t iH = pHist[iValue]--;
				Sum += iValue * (1 - (iH << 1));
				// Добавим в гистограмму столбец iCol + Win_cen
				double d = (pRowIn[0] - pseudo_min) * kfct;
				if (d <= 0.)
				{// Так как при яркости 0 вклад в сумму будет 0 при любом pHist[0], то pHist[0] можно просто 
					//нигде не учитывать, но pVarRow нужно обнулить!
						pVarRow[iPos + rw] = 0;
				}
				else
				{
					int iVal = static_cast<int>(d + 0.5);
					if (iVal > 255)
					{
						iH = pHist[pVarRow[iPos + rw] = 255]++;
						Sum += 255 * (1 + (iH << 1));
					}
					else
					{
						iH = pHist[pVarRow[iPos + rw] = static_cast<uint8_t>(iVal)]++;
						Sum += iVal * (1 + (iH << 1));
					}
				}
			}
			iPos += Win_size;
			if (iPos >= Win_lin)
				iPos = 0;
			pfRowOut[iCol] = static_cast<float>(Sum * Size_obratn);
		}
		// Обнуляем края. Правый край.
		memset(pfRowOut + iColMax, 0, sizeof(float) * (ar_cmmOut.m_i64LineSizeEl - iColMax));
	}
	// Обнуляем края. Последние строки.
	for (iRow = iRowMax; iRow < ar_cmmOut.m_i64H; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);
	if (nullptr != pVarRow)
		delete[] pVarRow;
	if (nullptr != pHist)
		delete[] pHist;


}

//

void TextureFiltr_Mean_omp2_5(MU16Data& ar_cmmIn, MFData& ar_cmmOut, int Win_cen, double pseudo_min, double kfct)
{
	int Win_size = (Win_cen << 1) + 1;
	int Win_lin = Win_size * Win_size;
	uint8_t* pVarRow = new uint8_t[Win_lin];
	uint16_t* pHist;
	double Size_obratn = 1.0 / Win_lin;
	// Кэш для гистограмм
	uint16_t* pHistAll = new uint16_t[256 * ar_cmmIn.m_i64W];
	size_t uiHistSize = 256 * sizeof(uint16_t);
	// Кэш для сумм
	uint32_t* puiSum = new uint32_t[ar_cmmIn.m_i64W];
	// Кэш для яркостей
	uint8_t* pbBrightness = new uint8_t[ar_cmmIn.m_i64W * (Win_size + 1)];
	uint8_t* pbBrightnessRow;
	int64_t iRow, iCol, iRowMax = ar_cmmIn.m_i64H - Win_cen, iColMax = ar_cmmIn.m_i64W - Win_cen;
	ar_cmmOut.iCreate(ar_cmmIn.m_i64W, ar_cmmIn.m_i64H, 6);
	// Обнуляем края. Первые строки.
	for (iRow = 0; iRow < Win_cen; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);
	int ln, rw, iPos = 0;
	uint32_t iValue, iH, Sum = 0;
	{// Расчет кэшей, первая строка
		iRow = Win_cen;
		// Первая точка в строке
		iCol = Win_cen;
		pHist = pHistAll + 256 * iCol;
		memset(pHist, 0, uiHistSize);
		float* pfRowOut = ar_cmmOut.pfGetRow(iRow);
		// Обнуляем края. Левый край.
		memset(pfRowOut, 0, sizeof(float) * Win_cen);
		{
			pbBrightnessRow = pbBrightness + iCol;
			for (rw = -Win_cen; rw <= Win_cen; rw++, pbBrightnessRow += ar_cmmIn.m_i64W)
			{
				uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow + rw);
				for (ln = -Win_cen; ln <= Win_cen; ln++)
				{
					double d = (pRowIn[iCol + ln] - pseudo_min) * kfct;
					if (d < 0.5) // Так как при яркости 0 вклад в сумму будет 0 при любом pHist[0], то 
						//pHist[0] можно просто нигде не учитывать, но pVarRow нужно обнулить!
						pbBrightnessRow[ln] = pVarRow[(ln + Win_cen) * Win_size + (rw + Win_cen)] = 0;
					else
					{
						if ((iValue = static_cast<uint32_t>(d + 0.5)) > 255)
						{
							pbBrightnessRow[ln] = pVarRow[(ln + Win_cen) * Win_size + (rw + Win_cen)] = 255;
							pHist[255]++;
						}
						else
						{
							pbBrightnessRow[ln] = pVarRow[(ln + Win_cen) * Win_size + (rw + Win_cen)] =
								static_cast<uint8_t>(iValue);
							pHist[iValue]++;
						}
					}
				}
			}
			for (int i = 0; i < Win_lin; i++)
			{
				iValue = static_cast<uint32_t>(pVarRow[i]);
				Sum += iValue * static_cast<uint32_t>(pHist[iValue]);
			}
			pfRowOut[iCol] = static_cast<float>(Sum * Size_obratn);
			puiSum[iCol] = Sum;
		}
		// Следующие точки в строке
		for (++iCol; iCol < iColMax; iCol++)
		{
			// Копируем гистограммы
			pHist = pHistAll + 256 * iCol;
			memcpy(pHist, pHist - 256, uiHistSize);
			uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow - Win_cen) + iCol + Win_cen;
			pbBrightnessRow = pbBrightness + iCol + Win_cen;
			for (rw = 0; rw < Win_size; rw++, pRowIn += ar_cmmIn.m_i64LineSizeEl, pbBrightnessRow +=
				ar_cmmIn.m_i64W)
			{
				// Вычитаем из гистограммы столбец iCol - Win_cen - 1
				iH = pHist[iValue = pVarRow[iPos + rw]]--;
				Sum += iValue * (1 - (iH << 1));
				// Прибавляем в гистограмму столбец iCol + Win_cen
				double d = (pRowIn[0] - pseudo_min) * kfct;
				if (d < 0.5) // Так как при яркости 0 вклад в сумму будет 0 при любом pHist[0], то pHist[0] 
				//	можно просто нигде не учитывать, но pVarRow нужно обнулить!
					pbBrightnessRow[0] = pVarRow[iPos + rw] = 0;
				else
				{
					if ((iValue = static_cast<uint32_t>(d + 0.5)) > 255)
					{
						pbBrightnessRow[0] = pVarRow[iPos + rw] = 255;
						iH = pHist[255]++;
						Sum += 255 * (1 + (iH << 1));
					}
					else
					{
						pbBrightnessRow[0] = pVarRow[iPos + rw] = static_cast<uint8_t>(iValue);
						iH = pHist[iValue]++;
						Sum += iValue * (1 + (iH << 1));
					}
				}
			}
			iPos += Win_size;
			if (iPos >= Win_lin)
				iPos = 0;
			pfRowOut[iCol] = static_cast<float>(Sum * Size_obratn);
			puiSum[iCol] = Sum;
		}
		// Обнуляем края. Правый край.
		memset(pfRowOut + iColMax, 0, sizeof(float) * (ar_cmmOut.m_i64LineSizeEl - iColMax));
	}
	// Последующие строки
	int iBrRowSub = 0; // Из какой строки яркостей будем вычитать [0, Win_size]
	int iBrRowAdd = Win_size; // В какую строку яркостей будем заносить данные [0, Win_size]
	for (++iRow; iRow < iRowMax; iRow++)
	{
		float* pfRowOut = ar_cmmOut.pfGetRow(iRow);
		// Обнуляем края. Левый край.
		memset(pfRowOut, 0, sizeof(float) * Win_cen);
		uint8_t* pbBrightnessRowSub = pbBrightness + iBrRowSub * ar_cmmIn.m_i64W;
		uint8_t* pbBrightnessRowAdd = pbBrightness + iBrRowAdd * ar_cmmIn.m_i64W;
		uint16_t* pRowInAdd = ar_cmmIn.pu16GetRow(iRow + Win_cen);
		// Первая точка в строке
		iCol = Win_cen;
		pHist = pHistAll + 256 * iCol;
		Sum = puiSum[iCol];
		{
			for (ln = 0; ln < Win_size; ln++)
			{
				// Вычитаем из гистограммы строку iRow - Win_cen - 1, колонки [iCol-Win_cen, iCol+Win_cen]
				iH = pHist[iValue = pbBrightnessRowSub[ln]]--;
				Sum += iValue * (1 - (iH << 1));
				// Прибавляем в гистограмму строку iRow + Win_cen, колонки [iCol-Win_cen, iCol+Win_cen]
				double d = (pRowInAdd[ln] - pseudo_min) * kfct;
				if (d < 0.5) // Так как при яркости 0 вклад в сумму будет 0 при любом pHist[0], то pHist[0] 
				//	можно просто нигде не учитывать, но pVarRow нужно обнулить!
					pbBrightnessRowAdd[ln] = 0;
				else
				{
					if ((iValue = static_cast<uint32_t>(d + 0.5)) > 255)
					{
						pbBrightnessRowAdd[ln] = 255;
						iH = pHist[255]++;
						Sum += 255 * (1 + (iH << 1));
					}
					else
					{
						pbBrightnessRowAdd[ln] = static_cast<uint8_t>(iValue);
						iH = pHist[iValue]++;
						Sum += iValue * (1 + (iH << 1));
					}
				}
			}
			pfRowOut[iCol] = static_cast<float>(Sum * Size_obratn);
			puiSum[iCol] = Sum;
		}
		// Следующие точки в строке
		pbBrightnessRowSub++;
		pbBrightnessRowAdd++;
		pRowInAdd++;
		pHist += 256;
		for (++iCol; iCol < iColMax; iCol++, pbBrightnessRowSub++, pbBrightnessRowAdd++, pRowInAdd++, pHist +=
			256)
		{
			Sum = puiSum[iCol];
			for (ln = 0; ln < Win_size - 1; ln++)
			{// Все данные есть в строке яркостей
			// Вычитаем из гистограммы строку iRow - Win_cen - 1, колонки [iCol-Win_cen, iCol+Win_cen]
				iH = pHist[iValue = pbBrightnessRowSub[ln]]--;
				Sum += iValue * (1 - (iH << 1));
				// Прибавляем в гистограмму строку iRow + Win_cen, колонки [iCol-Win_cen, iCol+Win_cen]
				iH = pHist[iValue = pbBrightnessRowAdd[ln]]++;
				Sum += iValue * (1 + (iH << 1));
			}
			// ln = Win_size - 1;
			{// Для добавляемой точки нужно рассчитать яркость
			// Вычитаем из гистограммы строку iRow - Win_cen - 1, колонки [iCol-Win_cen, iCol+Win_cen]
				iH = pHist[iValue = pbBrightnessRowSub[ln]]--;
				Sum += iValue * (1 - (iH << 1));
				// Прибавляем в гистограмму строку iRow + Win_cen, колонки [iCol-Win_cen, iCol+Win_cen]
				double d = (pRowInAdd[ln] - pseudo_min) * kfct;
				if (d < 0.5) // Так как при яркости 0 вклад в сумму будет 0 при любом pHist[0], то pHist[0] 
				//	можно просто нигде не учитывать, но pVarRow нужно обнулить!
					pbBrightnessRowAdd[ln] = 0;
				else
				{
					if ((iValue = static_cast<uint32_t>(d + 0.5)) > 255)
					{
						pbBrightnessRowAdd[ln] = 255;
						iH = pHist[255]++;
						Sum += 255 * (1 + (iH << 1));
					}
					else
					{
						pbBrightnessRowAdd[ln] = static_cast<uint8_t>(iValue);
						iH = pHist[iValue]++;
						Sum += iValue * (1 + (iH << 1));
					}
				}
			}
			pfRowOut[iCol] = static_cast<float>(Sum * Size_obratn);
			puiSum[iCol] = Sum;
		}
		// Обнуляем края. Правый край.
		memset(pfRowOut + iColMax, 0, sizeof(float) * (ar_cmmOut.m_i64LineSizeEl - iColMax));
		++iBrRowSub;
		if (iBrRowSub > Win_size)
			iBrRowSub = 0;
		++iBrRowAdd;
		if (iBrRowAdd > Win_size)
			iBrRowAdd = 0;
	}
	// Обнуляем края. Последние строки.
	for (iRow = iRowMax; iRow < ar_cmmOut.m_i64H; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);
	if (nullptr != pVarRow)
		delete[] pVarRow;
	if (nullptr != pHistAll)
		delete[] pHistAll;
	if (nullptr != puiSum)
		delete[] puiSum;
	if (nullptr != pbBrightness)
		delete[] pbBrightness;
}
//

void TextureFiltr_Mean_omp2_6(MU16Data& ar_cmmIn, MFData& ar_cmmOut, int Win_cen, double pseudo_min, double kfct)
{
	int Win_size = (Win_cen << 1) + 1;
	int Win_lin = Win_size * Win_size;
	double Size_obratn = 1.0 / Win_lin;
	// Кэш для гистограмм
	uint16_t* pHistAll = new uint16_t[256 * ar_cmmIn.m_i64W]; // [256][ar_cmmIn.m_i64W]
	memset(pHistAll, 0, sizeof(uint16_t) * 256 * ar_cmmIn.m_i64W);
	uint16_t* pHist, * pHistAdd, * pHistSub;
	// Кэш для сумм
	uint32_t* puiSum = new uint32_t[ar_cmmIn.m_i64W], * puiSumCurr;
	memset(puiSum, 0, sizeof(uint32_t) * ar_cmmIn.m_i64W);
	// Кэш для яркостей
	uint8_t* pbBrightness = new uint8_t[ar_cmmIn.m_i64W * Win_size];
	uint8_t* pbBrightnessRow;
	int64_t iRow, iCol, iColMax = ar_cmmIn.m_i64W - Win_size;
	ar_cmmOut.iCreate(ar_cmmIn.m_i64W, ar_cmmIn.m_i64H, 6);
	// Обнуляем края. Первые строки.
	for (iRow = 0; iRow < Win_cen; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);
	int64_t i, iCmin, iCmax, iPos = 0;
	uint32_t u32ValueAdd, u32ValueSub;
	double d;
	// Первые [0, Win_size - 1] строки
	pbBrightnessRow = pbBrightness;
	float* pfRowOut = ar_cmmOut.pfGetRow(Win_cen) + Win_cen;
	for (iRow = 0; iRow < Win_size; iRow++, pbBrightnessRow += ar_cmmIn.m_i64W)
	{
		// Обнуляем края строк.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * Win_cen);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - Win_cen), 0, sizeof(float) * Win_cen);
		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);
		// Первые [0, 2*Win_cen[ колонки
		for (iCol = 0; iCol < (Win_cen << 1); iCol++)
		{
			iCmin = max(static_cast<int64_t>(Win_cen), iCol - Win_cen);
			iCmax = iCol + Win_cen;
			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // Так как при яркости 0 вклад в сумму будет 0 
				//при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
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
		puiSumCurr = puiSum + Win_cen;
		for (; iCol <= iColMax; iCol++, puiSumCurr++)
		{
			iCmin = iCol - Win_cen;
			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // Так как при яркости 0 вклад в сумму будет 0 
				//при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
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
				for (i = 0; i < Win_size; i++)
					puiSumCurr[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
			if ((Win_size - 1) == iRow)
			{
				pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
				pfRowOut++;
			}
		}
		// Последние [ar_cmmIn.m_i64W - 2*Win_cen, ar_cmmIn.m_i64W - 1] колонки
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - Win_cen;
			iCmax = min(ar_cmmIn.m_i64W - Win_cen - 1, iCol + Win_cen);
			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // Так как при яркости 0 вклад в сумму будет 0 
				//при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
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
			if ((Win_size - 1) == iRow)
			{
				pfRowOut[0] = static_cast<float>(puiSum[iCmin] * Size_obratn);
				pfRowOut++;
			}
		}
	}
	// Последующие строки [Win_size, ar_cmmIn.m_i64H[
	for (iRow = Win_size; iRow < ar_cmmIn.m_i64H; iRow++)
	{
		// Обнуляем края строк.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * Win_cen);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - Win_cen), 0, sizeof(float) * Win_cen);
		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);
		float* pfRowOut = ar_cmmOut.pfGetRow(iRow - Win_cen) + Win_cen;
		iPos = (iRow - Win_size) % Win_size;
		pbBrightnessRow = pbBrightness + (iPos * ar_cmmIn.m_i64W);
		// Первые [0, 2*Win_cen[ колонки
		for (iCol = 0; iCol < (Win_cen << 1); iCol++)
		{
			iCmin = max(static_cast<int64_t>(Win_cen), iCol - Win_cen);
			iCmax = iCol + Win_cen;
			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // Так как при яркости 0 вклад в сумму будет 0 
				//при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
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
				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1)) -
					u32ValueSub * ((static_cast<uint32_t>(pHistSub[i]--) << 1) - 1);
			}
		}
		// Средние [2*Win_cen, ar_cmmIn.m_i64W - Win_size] колонки
		puiSumCurr = puiSum + Win_cen;
		pHist = pHistAll + (iCol - Win_cen);
		for (; iCol <= iColMax; iCol++, puiSumCurr++, pHist++)
		{
			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // Так как при яркости 0 вклад в сумму будет 0 
				//при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
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
				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = 0; i < Win_size; i++)
					puiSumCurr[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1)) -
					u32ValueSub * ((static_cast<uint32_t>(pHistSub[i]--) << 1) - 1);
			}
			pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
			pfRowOut++;
		}
		// Последние [ar_cmmIn.m_i64W - 2*Win_cen, ar_cmmIn.m_i64W - 1] колонки
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - Win_cen;
			iCmax = min(ar_cmmIn.m_i64W - Win_cen - 1, iCol + Win_cen);
			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // Так как при яркости 0 вклад в сумму будет 0 
				//при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
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
				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1)) -
					u32ValueSub * ((static_cast<uint32_t>(pHistSub[i]--) << 1) - 1);
			}
			pfRowOut[0] = static_cast<float>(puiSum[iCmin] * Size_obratn);
			pfRowOut++;
		}
	}
	// Обнуляем края. Последние строки.
	for (iRow = ar_cmmIn.m_i64H - Win_cen; iRow < ar_cmmOut.m_i64H; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);
	if (nullptr != pHistAll)
		delete[] pHistAll;
	if (nullptr != puiSum)
		delete[] puiSum;
	if (nullptr != pbBrightness)
		delete[] pbBrightness;
}

//

void TextureFiltr_Mean_omp2_7(MU16Data& ar_cmmIn, MFData& ar_cmmOut, int Win_cen, double pseudo_min, double kfct)
{
	int Win_size = (Win_cen << 1) + 1;
	int Win_lin = Win_size * Win_size;
	double Size_obratn = 1.0 / Win_lin;
	// Кэш для гистограмм
	uint16_t* pHistAll = new uint16_t[256 * ar_cmmIn.m_i64W]; // [256][ar_cmmIn.m_i64W]
	memset(pHistAll, 0, sizeof(uint16_t) * 256 * ar_cmmIn.m_i64W);
	uint16_t* pHist, * pHistAdd, * pHistSub;
	// Кэш для сумм
	uint32_t* puiSum = new uint32_t[ar_cmmIn.m_i64W], * puiSumCurr;
	memset(puiSum, 0, sizeof(uint32_t) * ar_cmmIn.m_i64W);
	// Кэш для яркостей
	uint8_t* pbBrightness = new uint8_t[ar_cmmIn.m_i64W * Win_size];
	uint8_t* pbBrightnessRow;
	int64_t iRow, iCol, iColMax = ar_cmmIn.m_i64W - Win_size;
	ar_cmmOut.iCreate(ar_cmmIn.m_i64W, ar_cmmIn.m_i64H, 6);
	// Обнуляем края. Первые строки.
	for (iRow = 0; iRow < Win_cen; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);
	int64_t i, iCmin, iCmax, iPos = 0;
	uint32_t u32ValueAdd, u32ValueSub;
	double d;
	// Первые [0, Win_size - 1] строки
	pbBrightnessRow = pbBrightness;
	float* pfRowOut = ar_cmmOut.pfGetRow(Win_cen) + Win_cen;
	for (iRow = 0; iRow < Win_size; iRow++, pbBrightnessRow += ar_cmmIn.m_i64W)
	{
		// Обнуляем края строк.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * Win_cen);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - Win_cen), 0, sizeof(float) * Win_cen);
		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);
		// Первые [0, 2*Win_cen[ колонки
		for (iCol = 0; iCol < (Win_cen << 1); iCol++)
		{
			iCmin = max(static_cast<int64_t>(Win_cen), iCol - Win_cen);
			iCmax = iCol + Win_cen;
			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // Так как при яркости 0 вклад в сумму будет 0 
				//при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
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
		puiSumCurr = puiSum + Win_cen;
		for (; iCol <= iColMax; iCol++, puiSumCurr++)
		{
			iCmin = iCol - Win_cen;
			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // Так как при яркости 0 вклад в сумму будет 0 
				//при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
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
				for (i = 0; i < Win_size; i++)
					puiSumCurr[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
			if ((Win_size - 1) == iRow)
			{
				pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
				pfRowOut++;
			}
		}
		// Последние [ar_cmmIn.m_i64W - 2*Win_cen, ar_cmmIn.m_i64W - 1] колонки
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - Win_cen;
			iCmax = min(ar_cmmIn.m_i64W - Win_cen - 1, iCol + Win_cen);
			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // Так как при яркости 0 вклад в сумму будет 0 
				//при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
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
			if ((Win_size - 1) == iRow)
			{
				pfRowOut[0] = static_cast<float>(puiSum[iCmin] * Size_obratn);
				pfRowOut++;
			}
		}
	}
	// Последующие строки [Win_size, ar_cmmIn.m_i64H[
	for (iRow = Win_size; iRow < ar_cmmIn.m_i64H; iRow++)
	{
		// Обнуляем края строк.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * Win_cen);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - Win_cen), 0, sizeof(float) * Win_cen);
		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);
		float* pfRowOut = ar_cmmOut.pfGetRow(iRow - Win_cen) + Win_cen;
		iPos = (iRow - Win_size) % Win_size;
		pbBrightnessRow = pbBrightness + (iPos * ar_cmmIn.m_i64W);
		// Первые [0, 2*Win_cen[ колонки
		for (iCol = 0; iCol < (Win_cen << 1); iCol++)
		{
			iCmin = max(static_cast<int64_t>(Win_cen), iCol - Win_cen);
			iCmax = iCol + Win_cen;
			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // Так как при яркости 0 вклад в сумму будет 0 
				//при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
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
				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1)) -
					u32ValueSub * ((static_cast<uint32_t>(pHistSub[i]--) << 1) - 1);
			}
		}
		// Средние [2*Win_cen, ar_cmmIn.m_i64W - Win_size] колонки
		puiSumCurr = puiSum + Win_cen;
		pHist = pHistAll + (iCol - Win_cen);
		for (; iCol <= iColMax; iCol++, puiSumCurr++, pHist++)
		{
			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // Так как при яркости 0 вклад в сумму будет 0 
				//при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
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
				// Добавляем яркость в Win_size гистограмм и сумм
				uint32_t u32ValueAddSub = u32ValueAdd + u32ValueSub;
				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = 0; i < Win_size; i++)
					puiSumCurr[i] += u32ValueAddSub + ((u32ValueAdd * static_cast<uint32_t>(pHistAdd[i]++) -
						u32ValueSub * static_cast<uint32_t>(pHistSub[i]--)) << 1);
			}
			pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
			pfRowOut++;
		}
		// Последние [ar_cmmIn.m_i64W - 2*Win_cen, ar_cmmIn.m_i64W - 1] колонки
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - Win_cen;
			iCmax = min(ar_cmmIn.m_i64W - Win_cen - 1, iCol + Win_cen);
			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // Так как при яркости 0 вклад в сумму будет 0 
				//при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
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
				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1)) -
					u32ValueSub * ((static_cast<uint32_t>(pHistSub[i]--) << 1) - 1);
			}
			pfRowOut[0] = static_cast<float>(puiSum[iCmin] * Size_obratn);
			pfRowOut++;
		}
	}
	// Обнуляем края. Последние строки.
	for (iRow = ar_cmmIn.m_i64H - Win_cen; iRow < ar_cmmOut.m_i64H; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);
	if (nullptr != pHistAll)
		delete[] pHistAll;
	if (nullptr != puiSum)
		delete[] puiSum;
	if (nullptr != pbBrightness)
		delete[] pbBrightness;
}

//

void TextureFiltr_Mean_omp2_7_3(MU16Data& ar_cmmIn, MFData& ar_cmmOut,  double pseudo_min, double kfct)
{
	int Win_cen = 3;
	int Win_size = (Win_cen << 1) + 1;
	int Win_lin = Win_size * Win_size;
	double Size_obratn = 1.0 / Win_lin;
	// Кэш для гистограмм
	uint16_t* pHistAll = new uint16_t[256 * ar_cmmIn.m_i64W]; // [256][ar_cmmIn.m_i64W]
	memset(pHistAll, 0, sizeof(uint16_t) * 256 * ar_cmmIn.m_i64W);
	uint16_t* pHist, * pHistAdd, * pHistSub;
	// Кэш для сумм
	uint32_t* puiSum = new uint32_t[ar_cmmIn.m_i64W], * puiSumCurr;
	memset(puiSum, 0, sizeof(uint32_t) * ar_cmmIn.m_i64W);
	// Кэш для яркостей
	uint8_t* pbBrightness = new uint8_t[ar_cmmIn.m_i64W * Win_size];
	uint8_t* pbBrightnessRow;
	int64_t iRow, iCol, iColMax = ar_cmmIn.m_i64W - Win_size;
	ar_cmmOut.iCreate(ar_cmmIn.m_i64W, ar_cmmIn.m_i64H, 6);
	// Обнуляем края. Первые строки.
	for (iRow = 0; iRow < Win_cen; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);
	int64_t i, iCmin, iCmax, iPos = 0;
	uint32_t u32ValueAdd, u32ValueSub;
	double d;
	// Первые [0, Win_size - 1] строки
	pbBrightnessRow = pbBrightness;
	float* pfRowOut = ar_cmmOut.pfGetRow(Win_cen) + Win_cen;
	for (iRow = 0; iRow < Win_size; iRow++, pbBrightnessRow += ar_cmmIn.m_i64W)
	{
		// Обнуляем края строк.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * Win_cen);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - Win_cen), 0, sizeof(float) * Win_cen);
		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);
		// Первые [0, 2*Win_cen[ колонки
		for (iCol = 0; iCol < (Win_cen << 1); iCol++)
		{
			iCmin = max(static_cast<int64_t>(Win_cen), iCol - Win_cen);
			iCmax = iCol + Win_cen;
			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // Так как при яркости 0 вклад в сумму будет 0 
				//при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
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
		puiSumCurr = puiSum + Win_cen;
		for (; iCol <= iColMax; iCol++, puiSumCurr++)
		{
			iCmin = iCol - Win_cen;
			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // Так как при яркости 0 вклад в сумму будет 0 
				//при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
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
				for (i = 0; i < Win_size; i++)
					puiSumCurr[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
			if ((Win_size - 1) == iRow)
			{
				pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
				pfRowOut++;
			}
		}
		// Последние [ar_cmmIn.m_i64W - 2*Win_cen, ar_cmmIn.m_i64W - 1] колонки
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - Win_cen;
			iCmax = min(ar_cmmIn.m_i64W - Win_cen - 1, iCol + Win_cen);
			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // Так как при яркости 0 вклад в сумму будет 0 
				//при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
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
			if ((Win_size - 1) == iRow)
			{
				pfRowOut[0] = static_cast<float>(puiSum[iCmin] * Size_obratn);
				pfRowOut++;
			}
		}
	}
	// Последующие строки [Win_size, ar_cmmIn.m_i64H[
	for (iRow = Win_size; iRow < ar_cmmIn.m_i64H; iRow++)
	{
		// Обнуляем края строк.
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * Win_cen);
		memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - Win_cen), 0, sizeof(float) * Win_cen);
		uint16_t* pRowIn = ar_cmmIn.pu16GetRow(iRow);
		float* pfRowOut = ar_cmmOut.pfGetRow(iRow - Win_cen) + Win_cen;
		iPos = (iRow - Win_size) % Win_size;
		pbBrightnessRow = pbBrightness + (iPos * ar_cmmIn.m_i64W);
		// Первые [0, 2*Win_cen[ колонки
		for (iCol = 0; iCol < (Win_cen << 1); iCol++)
		{
			iCmin = max(static_cast<int64_t>(Win_cen), iCol - Win_cen);
			iCmax = iCol + Win_cen;
			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // Так как при яркости 0 вклад в сумму будет 0 
				//при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
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
				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1)) -
					u32ValueSub * ((static_cast<uint32_t>(pHistSub[i]--) << 1) - 1);
			}
		}
		// Средние [2*Win_cen, ar_cmmIn.m_i64W - Win_size] колонки
		puiSumCurr = puiSum + Win_cen;
		pHist = pHistAll + (iCol - Win_cen);
		for (; iCol <= iColMax; iCol++, puiSumCurr++, pHist++)
		{
			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // Так как при яркости 0 вклад в сумму будет 0 
				//при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
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
				// Добавляем яркость в Win_size гистограмм и сумм
				uint32_t u32ValueAddSub = u32ValueAdd + u32ValueSub;
				// Добавляем яркость в Win_size гистограмм и сумм
				
					puiSumCurr[0] += u32ValueAddSub + ((u32ValueAdd * static_cast<uint32_t>(pHistAdd[0]++) -
						u32ValueSub * static_cast<uint32_t>(pHistSub[0]--)) << 1);
				
					puiSumCurr[1] += u32ValueAddSub + ((u32ValueAdd * static_cast<uint32_t>(pHistAdd[1]++) -
						u32ValueSub * static_cast<uint32_t>(pHistSub[1]--)) << 1);
				
					puiSumCurr[2] += u32ValueAddSub + ((u32ValueAdd * static_cast<uint32_t>(pHistAdd[2]++) -
						u32ValueSub * static_cast<uint32_t>(pHistSub[2]--)) << 1);
			}
			pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
			pfRowOut++;
		}
		// Последние [ar_cmmIn.m_i64W - 2*Win_cen, ar_cmmIn.m_i64W - 1] колонки
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - Win_cen;
			iCmax = min(ar_cmmIn.m_i64W - Win_cen - 1, iCol + Win_cen);
			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // Так как при яркости 0 вклад в сумму будет 0 
				//при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
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
				// Добавляем яркость в Win_size гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1)) -
					u32ValueSub * ((static_cast<uint32_t>(pHistSub[i]--) << 1) - 1);
			}
			pfRowOut[0] = static_cast<float>(puiSum[iCmin] * Size_obratn);
			pfRowOut++;
		}
	}
	// Обнуляем края. Последние строки.
	for (iRow = ar_cmmIn.m_i64H - Win_cen; iRow < ar_cmmOut.m_i64H; iRow++)
		memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);
	if (nullptr != pHistAll)
		delete[] pHistAll;
	if (nullptr != puiSum)
		delete[] puiSum;
	if (nullptr != pbBrightness)
		delete[] pbBrightness;
}

//

void TextureFiltr_Mean_V8_3_sse(MU16Data& ar_cmmIn, MFData& ar_cmmOut, double pseudo_min, double kfct)
{
	double Size_obratn = 1.0 / 9;
	// Кэш для гистограмм
	uint16_t* pHistAll = new uint16_t[256 * ar_cmmIn.m_i64W]; // [256][ar_cmmIn.m_i64W]
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
			iCmin = max(1, static_cast<int>(iCol - 1));
			iCmax = iCol + 1;
			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // Так как при яркости 0 вклад в сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
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
				// Добавляем яркость в 3 гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
		}
		// Средние [2*1, ar_cmmIn.m_i64W - 3] колонки
		puiSumCurr = puiSum + 1;
		for (; iCol <= iColMax; iCol++, puiSumCurr++)
		{
			iCmin = iCol - 1;
			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // Так как при яркости 0 вклад в сумму будет 0 	при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
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
				// Добавляем яркость в 3 гистограмм и сумм
				for (i = 0; i < 3; i++)
					puiSumCurr[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) << 1));
			}
			if ((3 - 1) == iRow)
			{
				pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
				pfRowOut++;
			}
		}
		// Последние [ar_cmmIn.m_i64W - 2*1, ar_cmmIn.m_i64W - 1] колонки
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - 1;
			iCmax = min(ar_cmmIn.m_i64W - 1 - 1, iCol + 1);
			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // Так как при яркости 0 вклад в сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
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
				// Добавляем яркость в 3 гистограмм и сумм
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
	__m128i xmmHistOne_3 = _mm_set_epi16(0, 0, 0, 0, 0, 1, 1, 1); // Для inc или dec сразу для 3 гистограмм
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
			iCmin = max(1, static_cast<int>(iCol - 1));
			iCmax = iCol + 1;
			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // Так как при яркости 0 вклад в сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
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
				// Добавляем яркость в 3 гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAddSub + ((u32ValueAdd * static_cast<uint32_t>(pHistAdd[i]++) -
						u32ValueSub * static_cast<uint32_t>(pHistSub[i]--)) << 1);
			}
		}
		// Средние [2*1, ar_cmmIn.m_i64W - 3] колонки
		puiSumCurr = puiSum + 1;
		pHist = pHistAll + (iCol - 1);
		for (; iCol <= iColMax; iCol++, puiSumCurr++, pHist++)
		{
			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // Так как при яркости 0 вклад в сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
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
				// Добавляем яркость в 3 гистограмм и сумм
				//============================
				// Первые 3 точки
				//============================
				__m128i xmmHistSub = _mm_lddqu_si128(reinterpret_cast<__m128i*>(pHistSub)); // Загружаем 128 бит(используем 48 бит - 3 эл.массива)
					__m128i xmmHistAdd = _mm_lddqu_si128(reinterpret_cast<__m128i*>(pHistAdd));
				// 3 элемента гистограммы
				__m128i xmmSumCurr = _mm_lddqu_si128(reinterpret_cast<__m128i*>(puiSumCurr)); // Загружаем 128 бит(используем 96 бит - 3 эл.массива)
					__m128i xmmHistSub32 = _mm_cvtepu16_epi32(xmmHistSub); // HistSub32[3], HistSub32[2], HistSub32[1], HistSub32[0](Hi < ->Lo)
					__m128i xmmHistAdd32 = _mm_cvtepu16_epi32(xmmHistAdd);
				__m128i xmmValueAdd = _mm_loadu_si32(&u32ValueAdd); // 0, 0, 0, u32ValueAdd (Hi < ->Lo)
				__m128i xmmValueSub = _mm_loadu_si32(&u32ValueSub);
				xmmValueAdd = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(xmmValueAdd),
					_mm_castsi128_ps(xmmValueAdd), 0x0C0)); // 0, u32ValueAdd, u32ValueAdd, u32ValueAdd
				xmmValueSub = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(xmmValueSub),
					_mm_castsi128_ps(xmmValueSub), 0x0C0));
				xmmHistAdd32 = _mm_sub_epi32(_mm_mullo_epi32(xmmHistAdd32, xmmValueAdd),
					_mm_mullo_epi32(xmmHistSub32, xmmValueSub));
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmHistAdd32); // <<= 1
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmValueAdd);
				xmmHistAdd32 = _mm_add_epi32(xmmHistAdd32, xmmValueSub);
				_mm_storeu_si128(reinterpret_cast<__m128i*>(puiSumCurr), _mm_add_epi32(xmmSumCurr,
					xmmHistAdd32)); // Сохраняем puiSumCurr[0,1,2,3]
				_mm_storeu_si128(reinterpret_cast<__m128i*>(pHistAdd), _mm_add_epi16(xmmHistAdd,
					xmmHistOne_3)); // pHistAdd[0-2]++; Сохраняем 128 бит (используем 128 - 8 эл. массива)
				_mm_storeu_si128(reinterpret_cast<__m128i*>(pHistSub), _mm_sub_epi16(xmmHistSub,
					xmmHistOne_3)); // pHistSub[0-2]--; Сохраняем 128 бит (используем 128 - 8 эл. массива)
			}
			pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
			pfRowOut++;
		}
		// Последние [ar_cmmIn.m_i64W - 2*1, ar_cmmIn.m_i64W - 1] колонки
		for (; iCol < ar_cmmIn.m_i64W; iCol++)
		{
			iCmin = iCol - 1;
			iCmax = min(ar_cmmIn.m_i64W - 1 - 1, iCol + 1);
			if ((d = (pRowIn[iCol] - pseudo_min) * kfct) < 0.5) // Так как при яркости 0 вклад в сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!
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
				// Добавляем яркость в 3 гистограмм и сумм
				for (i = iCmin; i <= iCmax; i++)
					puiSum[i] += u32ValueAddSub + ((u32ValueAdd * static_cast<uint32_t>(pHistAdd[i]++) -
						u32ValueSub * static_cast<uint32_t>(pHistSub[i]--)) << 1);
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

//================================================================
//================================================================

int main()
{
	const char *pcFilePath = "D:/Parallel_processes/TextureFiltr_Start/TextureFiltr/";
	const char *pcFileNameIn = "TextureFiltrIn.mu16";
	std::string strFileIn, strFileOut;


	MU16Data m;
	MFData mOut, etalon;
	int Win_cen = 1;
	int Win_size = 2 * Win_cen + 1;
	double pseudo_min, kfct;
	
	strFileIn = pcFilePath;
	strFileIn += pcFileNameIn;
	m.iRead(strFileIn.c_str());
	
	// Найдем мин и мах
	uint16_t u16Min, u16Max;
	u16Min = u16Max = m.pu16GetRow(0)[0];
	for (int64_t iRow = 0; iRow < m.m_i64H; iRow++)
	{
		uint16_t *pRowIn = m.pu16GetRow(iRow);
		for (int64_t iCol = 0; iCol < m.m_i64W; iCol++)
		{
			if (u16Min > pRowIn[iCol])
				u16Min = pRowIn[iCol];
			else if (u16Max < pRowIn[iCol])
				u16Max = pRowIn[iCol];
		}
	}
	pseudo_min = u16Min;
	kfct = 255. / (u16Max - u16Min);
	double dStart, dEnd;
	string strTab, strPoint = ".", strComma = ",";
	/*
	Запись данных о времени работы программы в файл формата csv
	чтобы exel открывал его корректно разделитель между полями устанавливаем = ;
	также при записи времени нужно заменить в строковом представлении числа . на ,
	Иначе exel может принять это за дату
	*/
	ofstream outTime("calcTime_many_threads_overall.csv", ios_base::out|ios_base::app);
	
	/*outTime << "func_name; thrAmount" << ";";
	for (Win_cen = 1; Win_cen < 11; ++Win_cen)
	{
		Win_size = 1 + 2 * Win_cen;
		outTime << std::to_string(Win_size) + "x" + std::to_string(Win_size) << ";";
	}
	outTime << endl;*/
	for (int thrAmount = 2; thrAmount < 4; thrAmount <<= 1)
	{
		outTime << "2_8_Codogen_version;";
		outTime << thrAmount << ";";
		for (Win_cen = 1; Win_cen < 11; ++Win_cen)
		{
			Win_size = 1 + 2 * Win_cen;
			cout << "calc win_size = " << Win_size << endl;
			dStart = omp_get_wtime();
			g_afunTextureFiltr_Mean_V8_sse4_omp[Win_cen-1](m, mOut, pseudo_min, kfct, thrAmount);
			//g_afunTextureFiltr_Mean_V8_sse4[Win_cen - 1](m, mOut, pseudo_min, kfct);
			//TextureFiltr_Mean_V8_3_sse(m, mOut, pseudo_min, kfct);
			//TextureFiltr_Mean_omp2_7_3(m, mOut,  pseudo_min, kfct);
			dEnd = omp_get_wtime();
			//outTime << pcFilePath + std::string("Out") + std::to_string(Win_size) + "x" + std::to_string(Win_size) <<
				//" - " << dEnd - dStart << endl;
			string time_dif = std::to_string(dEnd - dStart);
			cout << time_dif << endl;
			auto pos = time_dif.find(".");
			outTime << time_dif.replace(pos, pos + strPoint.length()-1, ",") << ";";
			strFileOut = pcFilePath + std::string("Out") + std::to_string(Win_size) + "x" + std::to_string(Win_size) + "_new_src.mfd";
			mOut.iWrite(strFileOut.c_str());
			/*блок, в котором идет сравнение с эталоном
			 вначале считываются эталонные значения из файла эталонов для текущего размера окна
			 после считанные значения и полученные значения сравниваются поэлементно
			 сравнение float требует введения погрешности, на которую могут отличаться 2 числа с плавающей точкой
			 здесь реализован самый простой вариант*/
			{
				MU16Data etalon;
				float eps = 1E-4;
				etalon.iRead(string(pcFilePath
					+ std::string("Out")
					+ std::to_string(Win_size)
					+ "x"
					+ std::to_string(Win_size) + "_src.mfd").c_str());
				// сравнение с эталонами
				int notEquivalCount = 0;
				for (int64_t iRow = 0; iRow < mOut.m_i64H; iRow++)
				{
					
					for (int64_t iCol = 0; iCol < m.m_i64W; iCol++)
					{
						if (fabs(mOut.m_pData[iRow * mOut.m_i64W + iCol] - etalon.m_pData[iRow * mOut.m_i64W + iCol]) > eps)
						{
							notEquivalCount++;
							cout << "idx: " << iRow << " " << iCol << " " << fabs(mOut.m_pData[iRow * mOut.m_i64W + iCol] - etalon.m_pData[iRow * mOut.m_i64W + iCol]) << endl;
						}
					}
				}
				if (notEquivalCount > 0)
					cout << "Results are not equal" << endl;
				else
					cout << "Results are equal" << endl;
				
			}
		}
		outTime << endl;
		
	}
	outTime.close();
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
