// ConsoleApplication1.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <iostream>
#include <conio.h>
#include <string>
#include <omp.h>
#include <fstream>
#include "MU16Data.h"
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
	ofstream outTime("calcTime_many_threads.csv");
	
	outTime << "thrAmount" << ";";
	for (Win_cen = 1; Win_cen < 11; ++Win_cen)
	{
		Win_size = 1 + 2 * Win_cen;
		outTime << std::to_string(Win_size) + "x" + std::to_string(Win_size) << ";";
	}
	outTime << endl;
	for (int thrAmount = 2; thrAmount < 13; thrAmount += 2)
	{
		outTime << thrAmount << ";";
		for (Win_cen = 5; Win_cen < 6; ++Win_cen)
		{
			Win_size = 1 + 2 * Win_cen;
			cout << "calc win_size = " << Win_size << endl;
			dStart = omp_get_wtime();

			TextureFiltr_Mean_omp1(m, mOut, Win_cen, pseudo_min, kfct, thrAmount);
			dEnd = omp_get_wtime();
			//outTime << pcFilePath + std::string("Out") + std::to_string(Win_size) + "x" + std::to_string(Win_size) <<
				//" - " << dEnd - dStart << endl;
			string time_dif = std::to_string(dEnd - dStart);
			cout << time_dif << endl;
			auto pos = time_dif.find(".");
			outTime << time_dif.replace(pos, pos + strPoint.length(), ",") << ";";
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
							notEquivalCount++;
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
