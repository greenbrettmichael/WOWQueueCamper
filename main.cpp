#include <string>
#include <Windows.h>
#include <gdiplus.h>
#pragma comment(lib,"gdiplus.lib")
#pragma comment(lib, "winmm.lib")
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#include <opencv2/opencv.hpp>
#include <filesystem>
#include <boost/program_options.hpp>
namespace po = boost::program_options;

int alarmQueue = 1000;
int badRecFail = 10;
int badRecCount = 0;
int startDelayMS = 10000;
int checkDelayMS = 1000;
int rleft = 1035;
int rbottom = 492;
int rwidth = 75;
int rheight = 30;
std::string alarmPath = std::string("default.wav");
tesseract::TessBaseAPI* api = new tesseract::TessBaseAPI();

/* https://github.com/vtempest/tesseract-ocr-sample/blob/master/tesseract-sample/tesseract_sample.cpp */
bool tesseract_preprocess(std::string source_file, std::string procFile) {
	cv::Mat sourceImg = cv::imread(source_file.c_str(), cv::IMREAD_COLOR);
	cv::cvtColor(sourceImg, sourceImg, cv::COLOR_BGR2HSV);
	cv::GaussianBlur(sourceImg, sourceImg, { 5,5 }, 1);
	cv::Mat yellowMask;
	cv::inRange(sourceImg, cv::Scalar(15, 100, 100), cv::Scalar(35, 255, 255), yellowMask);
	cv::bitwise_not(sourceImg, sourceImg, yellowMask);
	yellowMask = 255 - yellowMask;
	return cv::imwrite(procFile.c_str(), yellowMask);
}

/* https://github.com/vtempest/tesseract-ocr-sample/blob/master/tesseract-sample/tesseract_sample.cpp */
std::string tesseract_ocr_get_queue(std::string preprocessed_file)
{
	char* outText;
	Pix* image = pixRead(preprocessed_file.c_str());
	api->SetPageSegMode(tesseract::PSM_SINGLE_LINE);
	api->SetImage(image);
	std::string result = std::string(api->GetUTF8Text());
	tesseract::ResultIterator* ri = api->GetIterator();
	tesseract::PageIteratorLevel level = tesseract::RIL_WORD;
	std::string out;
	if (ri != 0) {
		do {
			if (ri->Confidence(level) < 80.f)
			{
				continue;
			}
			if (ri->WordIsNumeric())
			{
				const char* word = ri->GetUTF8Text(level);
				out.append(std::string(word));
				delete[] word;
			}
		} while (ri->Next(level));
	}
	delete image;
	return out;
}

bool tesseract_ocr_get_line(std::string preprocessed_file)
{
	char* outText;
	Pix* image = pixRead(preprocessed_file.c_str());
	api->SetPageSegMode(tesseract::PSM_SPARSE_TEXT);
	api->SetImage(image);
	std::string result = std::string(api->GetUTF8Text());
	tesseract::ResultIterator* ri = api->GetIterator();
	tesseract::PageIteratorLevel level = tesseract::RIL_TEXTLINE;
	std::string successLine("Position in queue");
	bool foundLine = false;
	int bboxLeft, bboxRight, bboxBottom, bboxTop;
	if (ri != 0) {
		do {
			const char* line = ri->GetUTF8Text(level);
			std::string lineStr(line);
			if (lineStr.find(successLine) != std::string::npos) {
				foundLine = true;
				ri->BoundingBox(level, &bboxLeft, &bboxTop, &bboxRight, &bboxBottom);
			}
			delete[] line;
		} while (ri->Next(level) && !foundLine);
	}
	if (foundLine)
	{
		rbottom = bboxTop - 5;
		rleft = (bboxLeft) - 5;
		rwidth = (bboxRight - bboxLeft) + 5;
		rheight = (bboxBottom - bboxTop) + 5;
	}
	delete image;
	return foundLine;
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
bool ConvertBMPToPNG(const char* bmpFile, const char* pngFile)
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
	HDC	sourceDc = GetWindowDC(GetDesktopWindow());
	HDC hDc = CreateCompatibleDC(0);
	HBITMAP hBmp = CreateCompatibleBitmap(sourceDc, width, height);
	SelectObject(hDc, hBmp);
	BitBlt(hDc, 0, 0, width, height, sourceDc, x, y, SRCCOPY);
	bool ret = SaveBMPFile(filename, hBmp, hDc, width, height);
	DeleteObject(hBmp);
	return ret;
}

bool findDims()
{
	rleft = GetSystemMetrics(SM_XVIRTUALSCREEN);
	rbottom = GetSystemMetrics(SM_YVIRTUALSCREEN);
	rwidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	rheight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	struct TempPaths
	{
		~TempPaths()
		{
			std::filesystem::remove_all(tmpDir);
		}
		std::filesystem::path tmpDir;
		std::filesystem::path bmpFile;
		std::filesystem::path pngFile;
		std::filesystem::path procFile;
	};
	badRecCount++;
	TempPaths workingImages;
	workingImages.tmpDir = std::filesystem::temp_directory_path();
	workingImages.tmpDir /= "WOWQueueCamper";
	std::filesystem::create_directory(workingImages.tmpDir);
	workingImages.bmpFile = workingImages.tmpDir;
	workingImages.bmpFile /= "color.bmp";
	workingImages.pngFile = workingImages.tmpDir;
	workingImages.pngFile /= "color.png";
	workingImages.procFile = workingImages.tmpDir;
	workingImages.procFile /= "bw.png";
	if (!ScreenCapture(rleft, rbottom, rwidth, rheight, strdup(workingImages.bmpFile.string().c_str())))
		return false;
	if (!ConvertBMPToPNG(workingImages.bmpFile.string().c_str(), workingImages.pngFile.string().c_str()))
		return false;
	if (!tesseract_preprocess(workingImages.pngFile.string(), workingImages.procFile.string()))
		return false;
	return tesseract_ocr_get_line(workingImages.procFile.string());
}

bool checkScreen(int x, int y, int width, int height)
{
	struct TempPaths
	{
		~TempPaths()
		{
			std::filesystem::remove_all(tmpDir);
		}
		std::filesystem::path tmpDir;
		std::filesystem::path bmpFile;
		std::filesystem::path pngFile;
		std::filesystem::path procFile;
	};
	badRecCount++;
	TempPaths workingImages;
	workingImages.tmpDir = std::filesystem::temp_directory_path();
	workingImages.tmpDir /= "WOWQueueCamper";
	std::filesystem::create_directory(workingImages.tmpDir);
	workingImages.bmpFile = workingImages.tmpDir;
	workingImages.bmpFile /= "color.bmp";
	workingImages.pngFile = workingImages.tmpDir;
	workingImages.pngFile /= "color.png";
	workingImages.procFile = workingImages.tmpDir;
	workingImages.procFile /= "bw.png";
	if (!ScreenCapture(x, y, width, height, strdup(workingImages.bmpFile.string().c_str())))
		return false;
	if (!ConvertBMPToPNG(workingImages.bmpFile.string().c_str(), workingImages.pngFile.string().c_str()))
		return false;
	if (!tesseract_preprocess(workingImages.pngFile.string(), workingImages.procFile.string()))
	{
		return false;
	}
	std::string ocr_result = tesseract_ocr_get_queue(workingImages.procFile.string());
	int queueNo = INT_MAX;
	try
	{
		queueNo = std::stoi(ocr_result);
		badRecCount = 0;
		std::cout << queueNo << std::endl;
	}
	catch (...)
	{
		std::cout << "BAD READ: " << ocr_result << std::endl;
	}
	return queueNo < alarmQueue;
}

void printHelp(po::options_description const& options)
{
	std::cout << "Usage: WOWQueueCamper [options]" << std::endl;
	std::cout << options;
}

/* https://stackoverflow.com/questions/38289101/boost-program-options-dependent-options */
void option_dependency(const boost::program_options::variables_map& vm,
	const std::string& for_what, const std::string& required_option)
{
	if (vm.count(for_what) && !vm[for_what].defaulted())
		if (vm.count(required_option) == 0 || vm[required_option].defaulted())
			throw std::logic_error(std::string("Option '") + for_what
				+ "' requires option '" + required_option + "'.");
}

int main(int argc, const char* argv[])
{
	po::options_description options("Options");
	options.add_options()
		("rleft", po::value<int>(), "x value of farthest left pixel on fullscreen.Requires other r parameters, default 1035")
		("rbottom", po::value<int>(), "y value of farthest bottom pixel on fullscreen.Requires other r parameters, default 492")
		("rwidth", po::value<int>(), "width of x values of farthest left pixel to farthest right pixel on fullscreen. Requires other r parameters, default 75")
		("rheight", po::value<int>(), "height of y values of farthest left pixel to farthest right pixel on fullscreen.Requires other r parameters, default 30")
		("rheight", po::value<int>(), "height of y values of farthest left pixel to farthest right pixel on fullscreen.Requires other r parameters, default 30")
		("alarm_number", po::value<int>(), "Amount of players in queue to trigger alarm. default 1000")
		("start_delay", po::value<int>(), "Time before running program to allow time to switch to WOW queue in ms. default 10000")
		("check_delay", po::value<int>(), "Time between checking queue in ms. default 1000")
		("alarm_file", po::value<std::string>(), "Absolute path to wav file to play for alarm. default fire truck air horn")
		("help,h", "Prints help menu");
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, options), vm);
	option_dependency(vm, "rleft", "rbottom");
	option_dependency(vm, "rleft", "rwidth");
	option_dependency(vm, "rleft", "rheight");
	option_dependency(vm, "rbottom", "rleft");
	option_dependency(vm, "rbottom", "rwidth");
	option_dependency(vm, "rbottom", "rheight");
	option_dependency(vm, "rwidth", "rleft");
	option_dependency(vm, "rwidth", "rbottom");
	option_dependency(vm, "rwidth", "rheight");
	option_dependency(vm, "rheight", "rleft");
	option_dependency(vm, "rheight", "rbottom");
	option_dependency(vm, "rheight", "rwidth");
	if (vm.count("help"))
	{
		printHelp(options);
		return 0;
	}
	try
	{
		po::notify(vm);
	}
	catch (std::exception const& e)
	{
		std::cerr << "Bad Arguments: " << e.what() << std::endl;
		printHelp(options);
		return 2;
	}
	if (vm.count("alarm_number"))
	{
		alarmQueue = vm["alarm_number"].as<int>();
	}
	if (vm.count("start_delay"))
	{
		startDelayMS = vm["start_delay"].as<int>();
	}
	if (vm.count("check_delay"))
	{
		checkDelayMS = vm["check_delay"].as<int>();
	}
	if (vm.count("alarm_file"))
	{
		std::filesystem::path alarmFilePath(vm["alarm_file"].as<std::string>());
		if (std::filesystem::exists(alarmFilePath) && alarmFilePath.extension() == ".wav")
		{
			alarmPath = alarmFilePath.string();
		}
		else
		{
			std::cout << alarmFilePath.string() << " does not exist or is not a .wav file " << std::endl;
			return 3;
		}
	}
	Sleep(startDelayMS);
	std::cout << "Starting the WOWQueueCamper! Waiting for queue to reach " << alarmQueue << std::endl;
	HMODULE hModule = GetModuleHandleW(NULL);
	WCHAR path[MAX_PATH];
	GetModuleFileNameW(hModule, path, MAX_PATH);
	char exePath[MAX_PATH];
	wcstombs(exePath, path, MAX_PATH);
	std::filesystem::path tessDataPath(exePath);
	tessDataPath = tessDataPath.parent_path();
	tessDataPath /= "tessdata";
	api->Init(tessDataPath.string().c_str(), "eng");
	if (vm.count("rleft") && vm.count("rbottom") && vm.count("rwidth") && vm.count("rheight"))
	{
		rleft = vm["rleft"].as<int>();
		rbottom = vm["rbottom"].as<int>();
		rwidth = vm["rwidth"].as<int>();
		rheight = vm["rheight"].as<int>();
	}
	else
	{
		findDims();
	}
	while (!checkScreen(rleft, rbottom, rwidth, rheight) && badRecCount < badRecFail)
	{
		Sleep(checkDelayMS);
	}
	PlaySound(alarmPath.c_str(), NULL, SND_SYNC);
	delete api;
    return 0;
}