void TextureFiltr_Mean_V7(MU16Data &ar_cmmIn, MFData &ar_cmmOut, int Win_cen, double   pseudo_min, double kfct)
{
 int Win_size = (Win_cen << 1) + 1;
 int Win_lin = Win_size * Win_size;
 double Size_obratn = 1.0 / Win_lin;

 // ��� ��� ����������
 uint16_t *pHistAll = new uint16_t[256 * ar_cmmIn.m_i64W]; // [256][ar_cmmIn.m_i64W]
 memset(pHistAll, 0, sizeof(uint16_t) * 256 * ar_cmmIn.m_i64W);
 uint16_t *pHist, *pHistAdd, *pHistSub;

 // ��� ��� ����
 uint32_t *puiSum = new uint32_t[ar_cmmIn.m_i64W], *puiSumCurr;
 memset(puiSum, 0, sizeof(uint32_t) * ar_cmmIn.m_i64W);

 // ��� ��� ��������
 uint8_t *pbBrightness = new uint8_t[ar_cmmIn.m_i64W * Win_size];
 uint8_t *pbBrightnessRow;

 int64_t iRow, iCol, iColMax = ar_cmmIn.m_i64W - Win_size;
 ar_cmmOut.iCreate(ar_cmmIn.m_i64W, ar_cmmIn.m_i64H, 6);

 // �������� ����. ������ ������.
 for (iRow = 0; iRow < Win_cen; iRow++)
 memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

 int64_t i, iCmin, iCmax, iPos = 0;
 uint32_t u32ValueAdd, u32ValueSub;
 double d;

 // ������ [0, Win_size - 1] ������
 pbBrightnessRow = pbBrightness;
 float *pfRowOut = ar_cmmOut.pfGetRow(Win_cen) + Win_cen;
 for (iRow = 0; iRow < Win_size; iRow++, pbBrightnessRow += ar_cmmIn.m_i64W)
 {
 // �������� ���� �����.
 memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * Win_cen);
 memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - Win_cen), 0, sizeof(float) *     Win_cen); 

 uint16_t *pRowIn = ar_cmmIn.pu16GetRow(iRow);

 // ������ [0, 2*Win_cen[ �������
 for (iCol = 0; iCol < (Win_cen << 1); iCol++)
 {
 iCmin = max(Win_cen, iCol - Win_cen);
 iCmax = iCol + Win_cen;

 if ((d = (pRowIn[iCol] - pseudo_min)*kfct) < 0.5) ��� ��� ��� ������� 0 ����� �  ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� ����� ��������!
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
 // ��������� ������� � Win_size ���������� � ����
 for (i = iCmin; i <= iCmax; i++)
 puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) <<   1)) ; 
 }
 }

 // ������� [2*Win_cen, ar_cmmIn.m_i64W - Win_size] �������
 puiSumCurr = puiSum + Win_cen;
 for (; iCol <= iColMax; iCol++, puiSumCurr++)
 {
 iCmin = iCol - Win_cen;

 if ((d = (pRowIn[iCol] - pseudo_min)*kfct) < 0.5) ��� ��� ��� ������� 0 ����� �  ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� ����� ��������!
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
 // ��������� ������� � Win_size ���������� � ����
 for (i = 0; i < Win_size; i++)
 puiSumCurr[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) <<   1)) ; 
 }
 if ((Win_size - 1) == iRow)
 {
 pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
 pfRowOut++;
 }
 }

 // ��������� [ar_cmmIn.m_i64W - 2*Win_cen, ar_cmmIn.m_i64W - 1] �������
 for (; iCol < ar_cmmIn.m_i64W; iCol++)
 {
 iCmin = iCol - Win_cen;
 iCmax = min(ar_cmmIn.m_i64W - Win_cen - 1, iCol + Win_cen);

 if ((d = (pRowIn[iCol] - pseudo_min)*kfct) < 0.5) ��� ��� ��� ������� 0 ����� �  ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� ����� ��������!
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
 // ��������� ������� � Win_size ���������� � ����
 for (i = iCmin; i <= iCmax; i++)
 puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) <<   1)) ; 
 }
 if ((Win_size - 1) == iRow)
 {
 pfRowOut[0] = static_cast<float>(puiSum[iCmin] * Size_obratn);
 pfRowOut++;
 }
 }
 }

 // ����������� ������ [Win_size, ar_cmmIn.m_i64H[
 for (iRow = Win_size; iRow < ar_cmmIn.m_i64H; iRow++)
 {
 // �������� ���� �����.
 memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * Win_cen);
 memset(ar_cmmOut.pfGetRow(iRow) + (ar_cmmIn.m_i64W - Win_cen), 0, sizeof(float) *     Win_cen); 

 uint16_t *pRowIn = ar_cmmIn.pu16GetRow(iRow);
 float *pfRowOut = ar_cmmOut.pfGetRow(iRow - Win_cen) + Win_cen;
 iPos = (iRow - Win_size) % Win_size;
 pbBrightnessRow = pbBrightness + (iPos * ar_cmmIn.m_i64W);

 // ������ [0, 2*Win_cen[ �������
 for (iCol = 0; iCol < (Win_cen << 1); iCol++)
 {
 iCmin = max(Win_cen, iCol - Win_cen);
 iCmax = iCol + Win_cen;

 if ((d = (pRowIn[iCol] - pseudo_min)*kfct) < 0.5) ��� ��� ��� ������� 0 ����� �  ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� ����� ��������!
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

 // ��������� ������� � Win_size ���������� � ����
 for (i = iCmin; i <= iCmax; i++)
 puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) <<   1))  - u32ValueSub * ((static_cast<uint32_t>(pHistSub[i]--) << 1) - 1); 
 }
 }

 // ������� [2*Win_cen, ar_cmmIn.m_i64W - Win_size] �������
 puiSumCurr = puiSum + Win_cen;
 pHist = pHistAll + (iCol - Win_cen);
 for (; iCol <= iColMax; iCol++, puiSumCurr++, pHist++)
 {
 if ((d = (pRowIn[iCol] - pseudo_min)*kfct) < 0.5) ��� ��� ��� ������� 0 ����� �  ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� ����� ��������!
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

 // ��������� ������� � Win_size ���������� � ����
 for (i = 0; i < Win_size; i++)
 puiSumCurr[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) <<   1))  - u32ValueSub * ((static_cast<uint32_t>(pHistSub[i]--) << 1) - 1); 
 }
 pfRowOut[0] = static_cast<float>(puiSumCurr[0] * Size_obratn);
 pfRowOut++;
 }

 // ��������� [ar_cmmIn.m_i64W - 2*Win_cen, ar_cmmIn.m_i64W - 1] �������
 for (; iCol < ar_cmmIn.m_i64W; iCol++)
 {
 iCmin = iCol - Win_cen;
 iCmax = min(ar_cmmIn.m_i64W - Win_cen - 1, iCol + Win_cen);

 if ((d = (pRowIn[iCol] - pseudo_min)*kfct) < 0.5) ��� ��� ��� ������� 0 ����� �  ����� ����� 0 ��� ����� pHist[0], �� pHist[0] ����� ������ ����� �� ���������, �� ������� � ���� ����� ��������!
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

 // ��������� ������� � Win_size ���������� � ����
 for (i = iCmin; i <= iCmax; i++)
 puiSum[i] += u32ValueAdd * (1 + (static_cast<uint32_t>(pHistAdd[i]++) <<   1))  - u32ValueSub * ((static_cast<uint32_t>(pHistSub[i]--) << 1) - 1); 
 }
 pfRowOut[0] = static_cast<float>(puiSum[iCmin] * Size_obratn);
 pfRowOut++;
 }
 }

 // �������� ����. ��������� ������.
 for (iRow = ar_cmmIn.m_i64H - Win_cen; iRow < ar_cmmOut.m_i64H; iRow++)
 memset(ar_cmmOut.pfGetRow(iRow), 0, sizeof(float) * ar_cmmOut.m_i64LineSizeEl);

 if (nullptr != pHistAll)
 delete[] pHistAll;
 if (nullptr != puiSum)
 delete[] puiSum;
 if (nullptr != pbBrightness)
 delete[] pbBrightness;
}
