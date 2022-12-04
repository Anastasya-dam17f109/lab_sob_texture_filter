// textureFilterCodoGen.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <iostream>
#include <cstdlib>


int iCodeGenRowsCols_V8(FILE* pf, int Win_cen, int iFlagRows, int iFlagCols)
{
    const char* pcAdd_iCmin = "";
    const char* pcAdd_iCminOr0 = "";
    const char* pcCurr = "";
    const char* pcCorrSum = "";
    const char* pcAll = "";
    int Win_size = (Win_cen << 1) + 1;
    int Win_lin = Win_size * Win_size;
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
        fprintf(pf, " iCmin = max(%d, static_cast <int>(iCol - %d));\n", Win_cen, Win_cen);
        fprintf(pf, " iCmax = iCol + %d;\n", Win_cen);
        fprintf(pf, "\n");
    }
    else if (1 == iFlagCols)
    {
        pcAdd_iCmin = " + iCmin";
        pcCurr = "Curr";
        pcAdd_iCminOr0 = "0";
        fprintf(pf, " // Средние [2*Win_cen, ar_cmmIn.m_i64W - Win_size] колонки\n");
        fprintf(pf, " puiSumCurr = puiSum + %d;\n", Win_cen);
        if (0 != iFlagRows)
        {
            fprintf(pf, " pHist = pHistAll + (iCol - %d);\n", Win_cen);
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
        fprintf(pf, " // Последние [ar_cmmIn.m_i64W - 2*Win_cen, ar_cmmIn.m_i64W - 1] колонки\n");
        fprintf(pf, " for (; iCol < ar_cmmIn.m_i64W; iCol++)\n");
        fprintf(pf, " {\n");
        fprintf(pf, " iCmin = iCol - %d;\n", Win_cen);
        fprintf(pf, " iCmax = min(static_cast <int>(ar_cmmIn.m_i64W - %d - 1), static_cast <int>(iCol + %d));\n", Win_cen, Win_cen);
        fprintf(pf, "\n");
    }
    fprintf(pf, " if ((d = (pRowIn[iCol] - pseudo_min)*kfct) < 0.5) //Так как при яркости 0 вклад в  сумму будет 0 при любом pHist[0], то pHist[0] можно просто нигде не учитывать, но яркость в кэше нужно обнулить!\n");
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
        fprintf(pf, " pHistAdd = pHistAll + (255 * ar_cmmIn.m_i64W%s);\n", pcAdd_iCmin);
        fprintf(pf, " }\n");
        fprintf(pf, " else\n");
        fprintf(pf, " {\n");
        fprintf(pf, " pbBrightnessRow[iCol] = static_cast<uint8_t>(u32ValueAdd);\n");
        fprintf(pf, " pHistAdd = pHistAll + (u32ValueAdd * ar_cmmIn.m_i64W%s);\n",
            pcAdd_iCmin);
        fprintf(pf, " }\n");
    }
    else
    {
        fprintf(pf, " u32ValueAdd = 255;\n");
        fprintf(pf, " }\n");
        fprintf(pf, " if (u32ValueAdd != (u32ValueSub = pbBrightnessRow[iCol]))\n");
        fprintf(pf, " {\n");
        fprintf(pf, " pHistSub = pHist%s + (u32ValueSub * ar_cmmIn.m_i64W);\n", pcAll);
        fprintf(pf, " pHistAdd = pHist%s + (u32ValueAdd * ar_cmmIn.m_i64W);\n", pcAll);
        fprintf(pf, " pbBrightnessRow[iCol] = u32ValueAdd;\n");
       // if ('\0' != pcCorrSum[0])
         //   fprintf(pf, " uint32_t u32ValueAddSub = u32ValueAdd + u32ValueSub;\n");
        
        fprintf(pf, "\n");
    }
    fprintf(pf, " // Добавляем яркость в Win_size гистограмм и сумм\n");
    if (1 != iFlagCols)
        fprintf(pf, " for (i = iCmin; i <= iCmax; i++)\n");
    else
        fprintf(pf, " for (i = 0; i < %d; i++)\n", Win_size);
    fprintf(pf, " puiSum%s[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) <<  1)) % s; \n", pcCurr, pcCorrSum);

        fprintf(pf, " }\n");
    if (0 != iFlagCols)
    {
        if (0 == iFlagRows)
        {
            fprintf(pf, " if ((%d - 1) == iRow)\n", Win_size);
            fprintf(pf, " {\n");
            fprintf(pf, " pfRowOut[0] = static_cast<float>(puiSum%s[%s] * Size_obratn);\n",
                pcCurr, pcAdd_iCminOr0);
            fprintf(pf, " pfRowOut++;\n");
            fprintf(pf, " }\n");
        }
        else
        {
            fprintf(pf, " pfRowOut[0] = static_cast<float>(puiSum%s[%s] * Size_obratn);\n", pcCurr,
                pcAdd_iCminOr0);
            fprintf(pf, " pfRowOut++;\n");
        }
    }
    fprintf(pf, " }\n");
    return 0;
}
int iCodeGenRows_V8(FILE* pf, int Win_cen, int iFlagRows)
{
    int Win_size = (Win_cen << 1) + 1;
    int Win_lin = Win_size * Win_size;
    fprintf(pf, "\n");
    if (0 == iFlagRows)
    {
        fprintf(pf, " // Первые [0, %d - 1] строки\n", Win_size);
        fprintf(pf, " pbBrightnessRow = pbBrightness;\n");
        fprintf(pf, " float *pfRowOut = ar_cmmOut.pfGetRow(%d) + %d;\n", Win_cen, Win_cen);
        fprintf(pf, " for (iRow = 0; iRow <  %d; iRow++, pbBrightnessRow += ar_cmmIn.m_i64W)\n", Win_size);
    }
    else
    {
        fprintf(pf, " // Последующие строки [%d, ar_cmmIn.m_i64H[\n", Win_size);
        fprintf(pf, " for (iRow = %d; iRow < ar_cmmIn.m_i64H; iRow++)\n", Win_size);
    }
    fprintf(pf, " {\n");
    fprintf(pf, " // Обнуляем края строк.\n");
    fprintf(pf, " memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * %d);\n", Win_cen);
    fprintf(pf, " memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - %d), 0, sizeof(float) *  %d); \n", Win_cen, Win_cen);
        fprintf(pf, "\n");
    fprintf(pf, " uint16_t *pRowIn = ar_cmmIn.pu16GetRow(iRow);\n");
    if (0 != iFlagRows)
    {
        fprintf(pf, " float *pfRowOut = ar_cmmOut.pfGetRow(iRow - %d) + %d;\n", Win_cen, Win_cen);
        fprintf(pf, " iPos = (iRow - %d) %% %d;\n", Win_size, Win_size);
        fprintf(pf, " pbBrightnessRow = pbBrightness + (iPos * ar_cmmIn.m_i64W);\n");
    }
    // Первые колонки
    if (0 != iCodeGenRowsCols_V8(pf, Win_cen, iFlagRows, 0))
        return 10;
    // Средние колонки
    if (0 != iCodeGenRowsCols_V8(pf, Win_cen, iFlagRows, 1))
        return 20;
    // Последние колонки
    if (0 != iCodeGenRowsCols_V8(pf, Win_cen, iFlagRows, 2))
        return 30;
    fprintf(pf, " }\n");
    return 0;
}

// кодогенератор 7 версии

int iCodeGen_V8(const char* a_pcFilePath)
{
    FILE* pf; fopen_s(&pf,a_pcFilePath, "wt");
    if (nullptr == pf)
        return 10;
    // Пролог
    fprintf(pf, "#define _CRT_SECURE_NO_WARNINGS\n");
    fprintf(pf, "\n");
    fprintf(pf, "#include <iostream>\n");
    fprintf(pf, "#include <cstdlib>\n");
    fprintf(pf, "#include <conio.h>\n");
    fprintf(pf, "#include <string>\n");
    fprintf(pf, "#include <omp.h>\n");
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
        fprintf(pf, "void TextureFiltr_Mean_V8_%d(MU16Data &ar_cmmIn, MFData &ar_cmmOut, double pseudo_min,    double kfct)\n", Win_size);
            fprintf(pf, "{\n");
        fprintf(pf, " double Size_obratn = 1.0 / %d;\n", Win_lin);
        fprintf(pf, "\n");
        fprintf(pf, " // Кэш для гистограмм\n");
        fprintf(pf, " uint16_t *pHistAll = new uint16_t[256 * ar_cmmIn.m_i64W]; // [256] [ar_cmmIn.m_i64W]\n");
            fprintf(pf, " memset(pHistAll, 0, sizeof(uint16_t) * 256 * ar_cmmIn.m_i64W);\n");
        fprintf(pf, " uint16_t *pHist, *pHistAdd, *pHistSub;\n");
        fprintf(pf, "\n");
        fprintf(pf, " // Кэш для сумм\n");
        fprintf(pf, " uint32_t *puiSum = new uint32_t[ar_cmmIn.m_i64W], *puiSumCurr;\n");
        fprintf(pf, " memset(puiSum, 0, sizeof(uint32_t) * ar_cmmIn.m_i64W);\n");
        fprintf(pf, "\n");
        fprintf(pf, " // Кэш для яркостей\n");
        fprintf(pf, " uint8_t *pbBrightness = new uint8_t[ar_cmmIn.m_i64W * %d];\n", Win_size);
        fprintf(pf, " uint8_t *pbBrightnessRow;\n");
        fprintf(pf, "\n");
        fprintf(pf, " int64_t iRow, iCol, iColMax = ar_cmmIn.m_i64W - %d;\n", Win_size);
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
        if (0 != iCodeGenRows_V8(pf, Win_cen, 0))
        {
            fclose(pf);
            return 20;
        }
        // Последующие строки
        if (0 != iCodeGenRows_V8(pf, Win_cen, 1))
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
        fprintf(pf, " if (nullptr != pbBrightness)\n");
        fprintf(pf, " delete[] pbBrightness;\n");
        fprintf(pf, "}\n");
    }
    fprintf(pf, "\n");
    fprintf(pf, "tdTextureFiltr_Mean_V8 g_afunTextureFiltr_Mean_V8[10] = \n");
    fprintf(pf, "{\n");
    fprintf(pf, " TextureFiltr_Mean_V8_3, // 0\n");
    fprintf(pf, " TextureFiltr_Mean_V8_5, // 1\n");
    fprintf(pf, " TextureFiltr_Mean_V8_7, // 2\n");
    fprintf(pf, " TextureFiltr_Mean_V8_9, // 3\n");
    fprintf(pf, " TextureFiltr_Mean_V8_11, // 4\n");
    fprintf(pf, " TextureFiltr_Mean_V8_13, // 5\n");
    fprintf(pf, " TextureFiltr_Mean_V8_15, // 6\n");
    fprintf(pf, " TextureFiltr_Mean_V8_17, // 7\n");
    fprintf(pf, " TextureFiltr_Mean_V8_19, // 8\n");
    fprintf(pf, " TextureFiltr_Mean_V8_21 // 9\n");
    fprintf(pf, "};\n");
    fclose(pf);
    return 0;
}
int main()
{
    std::cout << "Hello World!\n";
    std::string a_pcFilePath = "TextureFiltr_Mean_V8_CodeGen.cpp";
    iCodeGen_V8(a_pcFilePath.c_str());

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
