#include <string>
#include <Windows.h>
#include <gdiplus.h>
#pragma comment(lib,"gdiplus.lib")
#pragma comment(lib, "winmm.lib")
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>

int alarmQueue = 1000;
int badRecFail = 10;
int badRecCount = 0;

/* https://github.com/vtempest/tesseract-ocr-sample/blob/master/tesseract-sample/tesseract_sample.cpp */
std::string tesseract_preprocess(std::string source_file) {
	char tempPath[128];
	GetTempPathA(128, tempPath);
	strcat_s(tempPath, "test2.png");
	char preprocessed_file[MAX_PATH];
	strcpy_s(preprocessed_file, tempPath);
	BOOL perform_negate = FALSE;
	l_float32 dark_bg_threshold = 0.75f; // From 0.0 to 1.0, with 0 being all white and 1 being all black 
	int perform_scale = 1;
	l_float32 scale_factor = 10.0f;
	int perform_unsharp_mask = 1;
	l_int32 usm_halfwidth = 5;
	l_float32 usm_fract = 2.5f;
	int perform_otsu_binarize = 1;
	l_int32 otsu_sx = 2000;
	l_int32 otsu_sy = 2000;
	l_int32 otsu_smoothx = 0;
	l_int32 otsu_smoothy = 0;
	l_float32 otsu_scorefract = 0.0f;
	l_int32 status = 1;
	l_float32 border_avg = 0.0f;
	PIX* pixs = NULL;
	char* ext = NULL;
	pixs = pixRead(source_file.c_str());
	pixs = pixConvertRGBToGray(pixs, 0.0f, 0.0f, 0.0f);
	if (perform_negate)
	{
		PIX* otsu_pixs = NULL;
		status = pixOtsuAdaptiveThreshold(pixs, otsu_sx, otsu_sy, otsu_smoothx, otsu_smoothy, otsu_scorefract, NULL, &otsu_pixs);
		if (!otsu_pixs)
		{
			return "";
		}
		border_avg = pixAverageOnLine(otsu_pixs, 0, 0, otsu_pixs->w - 1, 0, 1);                               // Top 
		border_avg += pixAverageOnLine(otsu_pixs, 0, otsu_pixs->h - 1, otsu_pixs->w - 1, otsu_pixs->h - 1, 1); // Bottom 
		border_avg += pixAverageOnLine(otsu_pixs, 0, 0, 0, otsu_pixs->h - 1, 1);                               // Left 
		border_avg += pixAverageOnLine(otsu_pixs, otsu_pixs->w - 1, 0, otsu_pixs->w - 1, otsu_pixs->h - 1, 1); // Right 
		border_avg /= 4.0f;
		pixDestroy(&otsu_pixs);
		if (border_avg > dark_bg_threshold)
		{
			pixInvert(pixs, pixs);

		}
	}
	if (perform_scale)
	{
		pixs = pixScaleGrayLI(pixs, scale_factor, scale_factor);
	}
	if (perform_unsharp_mask)
	{
		pixs = pixUnsharpMaskingGray(pixs, usm_halfwidth, usm_fract);
	}
	if (perform_otsu_binarize)
	{
		status = pixOtsuAdaptiveThreshold(pixs, otsu_sx, otsu_sy, otsu_smoothx, otsu_smoothy, otsu_scorefract, NULL, &pixs);
	}
	status = pixWriteImpliedFormat(preprocessed_file, pixs, 0, 0);
	std::string out(preprocessed_file);
	return out;
}

/* https://github.com/vtempest/tesseract-ocr-sample/blob/master/tesseract-sample/tesseract_sample.cpp */
std::string tesseract_ocr(std::string preprocessed_file)
{
	char* outText;
	Pix* image = pixRead(preprocessed_file.c_str());
	tesseract::TessBaseAPI* api = new tesseract::TessBaseAPI();
	TCHAR CurDir[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, CurDir);
	api->Init(CurDir, "eng");
	api->SetPageSegMode(tesseract::PSM_SINGLE_LINE);
	api->SetImage(image);
	outText = api->GetUTF8Text();
	std::string out(outText);
	return out;
}

/* https://gist.github.com/rdp/9821698 */
inline int GetFilePointer(HANDLE FileHandle) {
	return SetFilePointer(FileHandle, 0, 0, FILE_CURRENT);
}
#include <iostream>
/* https://gist.github.com/rdp/9821698 */
extern bool SaveBMPFile(char* filename, HBITMAP bitmap, HDC bitmapDC, int width, int height) {
	struct BMPProps
	{
		~BMPProps() 
		{
			delete[] lpbi;
			delete[] lpvBits;
		}
		bool Success = false;
		HDC SurfDC = NULL;        // GDI-compatible device context for the surface
		HBITMAP OffscrBmp = NULL; // bitmap that is converted to a DIB
		HDC OffscrDC = NULL;      // offscreen DC that we can select OffscrBmp into
		LPBITMAPINFO lpbi = NULL; // bitmap format info; used by GetDIBits
		LPVOID lpvBits = NULL;    // pointer to bitmap bits array
		HANDLE BmpFile = INVALID_HANDLE_VALUE;    // destination .bmp file
		BITMAPFILEHEADER bmfh;  // .bmp file header
	};
	BMPProps myProps;
	if ((myProps.OffscrBmp = CreateCompatibleBitmap(bitmapDC, width, height)) == NULL)
		return false;
	if ((myProps.OffscrDC = CreateCompatibleDC(bitmapDC)) == NULL)
		return false;
	HBITMAP OldBmp = (HBITMAP)SelectObject(myProps.OffscrDC, myProps.OffscrBmp);
	BitBlt(myProps.OffscrDC, 0, 0, width, height, bitmapDC, 0, 0, SRCCOPY);
	if ((myProps.lpbi = (LPBITMAPINFO)(new char[sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD)])) == NULL)
		return false;
	ZeroMemory(&myProps.lpbi->bmiHeader, sizeof(BITMAPINFOHEADER));
	myProps.lpbi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	SelectObject(myProps.OffscrDC, OldBmp);
	if (!GetDIBits(myProps.OffscrDC, myProps.OffscrBmp, 0, height, NULL, myProps.lpbi, DIB_RGB_COLORS))
		return false;
	if ((myProps.lpvBits = new char[myProps.lpbi->bmiHeader.biSizeImage]) == NULL)
		return false;
	if (!GetDIBits(myProps.OffscrDC, myProps.OffscrBmp, 0, height, myProps.lpvBits, myProps.lpbi, DIB_RGB_COLORS))
		return false;
	if ((myProps.BmpFile = CreateFile(filename,
		GENERIC_WRITE,
		0, NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL)) == INVALID_HANDLE_VALUE)

		return false;
	DWORD Written;
	myProps.bmfh.bfType = 19778;
	myProps.bmfh.bfReserved1 = myProps.bmfh.bfReserved2 = 0;
	if (!WriteFile(myProps.BmpFile, &myProps.bmfh, sizeof(myProps.bmfh), &Written, NULL))
		return false;
	if (Written < sizeof(myProps.bmfh))
		return false;
	if (!WriteFile(myProps.BmpFile, &myProps.lpbi->bmiHeader, sizeof(BITMAPINFOHEADER), &Written, NULL))
		return false;
	if (Written < sizeof(BITMAPINFOHEADER))
		return false;
	int PalEntries;
	if (myProps.lpbi->bmiHeader.biCompression == BI_BITFIELDS)
		PalEntries = 3;
	else
		PalEntries = (myProps.lpbi->bmiHeader.biBitCount <= 8) ?
		(int)(1 << myProps.lpbi->bmiHeader.biBitCount)
		: 0;
	if (myProps.lpbi->bmiHeader.biClrUsed)
		PalEntries = myProps.lpbi->bmiHeader.biClrUsed;
	if (PalEntries) {
		if (!WriteFile(myProps.BmpFile, &myProps.lpbi->bmiColors, PalEntries * sizeof(RGBQUAD), &Written, NULL))
			return false;
		if (Written < PalEntries * sizeof(RGBQUAD))
			return false;
	}
	myProps.bmfh.bfOffBits = GetFilePointer(myProps.BmpFile);
	if (!WriteFile(myProps.BmpFile, myProps.lpvBits, myProps.lpbi->bmiHeader.biSizeImage, &Written, NULL))
		return false;
	if (Written < myProps.lpbi->bmiHeader.biSizeImage)
		return false;
	myProps.bmfh.bfSize = GetFilePointer(myProps.BmpFile);
	SetFilePointer(myProps.BmpFile, 0, 0, FILE_BEGIN);
	if (!WriteFile(myProps.BmpFile, &myProps.bmfh, sizeof(myProps.bmfh), &Written, NULL))
		return false;
	if (Written < sizeof(myProps.bmfh))
		return false;
	CloseHandle(myProps.BmpFile);
	return true;
}

/* https://docs.microsoft.com/en-us/windows/win32/gdiplus/-gdiplus-retrieving-the-class-identifier-for-an-encoder-use */
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes
	Gdiplus::ImageCodecInfo* pImageCodecInfo = NULL;
	Gdiplus::GetImageEncodersSize(&num, &size);
	if (size == 0)
		return -1;  // Failure
	pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
	if (pImageCodecInfo == NULL)
		return -1;  // Failure
	GetImageEncoders(num, size, pImageCodecInfo);
	for (UINT j = 0; j < num; ++j)
	{
		if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;  // Success
		}
	}
	free(pImageCodecInfo);
	return -1;  // Failure
}

/* https://docs.microsoft.com/en-us/windows/win32/gdiplus/-gdiplus-converting-a-bmp-image-to-a-png-image-use */
bool ConvertBMPToPNG(char* bmpFile, char* pngFile)
{
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	CLSID   encoderClsid;
	Gdiplus::Status  stat;
	std::string bmpFileStr(bmpFile);
	std::wstring bmpFileWStr(bmpFileStr.size(), L'#');
	mbstowcs(&bmpFileWStr[0], bmpFileStr.data(), bmpFileStr.size());
	std::string pngFileStr(pngFile);
	std::wstring pngFileWStr(bmpFileStr.size(), L'#');
	mbstowcs(&pngFileWStr[0], pngFileStr.data(), pngFileStr.size());
	Gdiplus::Image* image = new Gdiplus::Image(bmpFileWStr.c_str());
	GetEncoderClsid(L"image/png", &encoderClsid);
	stat = image->Save(pngFileWStr.c_str(), &encoderClsid, NULL);
	delete image;
	Gdiplus::GdiplusShutdown(gdiplusToken);
	return stat == Gdiplus::Ok;
}

/* https://gist.github.com/rdp/9821698 */
bool ScreenCapture(int x, int y, int width, int height, char* filename) {
	HDC hDc = CreateCompatibleDC(0);
	HBITMAP hBmp = CreateCompatibleBitmap(GetDC(0), width, height);
	SelectObject(hDc, hBmp);
	BitBlt(hDc, 0, 0, width, height, GetDC(0), x, y, SRCCOPY);
	bool ret = SaveBMPFile(filename, hBmp, hDc, width, height);
	DeleteObject(hBmp);
	return ret;
}


bool checkScreen(int x, int y, int width, int height)
{
	if (!ScreenCapture(x, y, width, height, "C:\\test.bmp"))
		return false;
	if (!ConvertBMPToPNG("C:\\test.bmp", "C:\\test.png"))
		return false;
	std::string preprocessed_file = tesseract_preprocess("C:/test.png");
	std::string ocr_result = tesseract_ocr(preprocessed_file);
	int queueNo = alarmQueue * 10;
	try
	{
		int queueNo = std::stoi(ocr_result);
		badRecCount = 0;
		std::cout << queueNo << std::endl;
	}
	catch (...)
	{
		badRecCount++;
		std::cout << ocr_result << std::endl;
	}
	return queueNo < alarmQueue;
}

int main()
{
	Sleep(100);
	std::cout << "Starting the WOWQueueCamper! Waiting for queue to reach " << alarmQueue << std::endl;
	RECT screenRect;
	const HWND hDesktop = GetDesktopWindow();
	GetWindowRect(hDesktop, &screenRect);
	int width = screenRect.right;
	int height = screenRect.bottom;
	int baseW = .539 * width;
	int baseH = .455 * height;
	int rwidth = (.578 * width) - baseW;
	int rheight = (.483 * height) - baseH;
	while (!checkScreen(baseW, baseH, rwidth, rheight) && badRecCount < badRecFail)
	{
		Sleep(1000);
	}
	PlaySound("default.wav", NULL, SND_SYNC);
    return 0;
}