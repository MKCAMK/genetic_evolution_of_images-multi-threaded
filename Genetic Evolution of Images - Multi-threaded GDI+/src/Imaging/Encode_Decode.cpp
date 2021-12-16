#include "Program/Program.h"
#include "common/CoInitialize.h"
#include "common/Settings.h"

#include <wincodec.h>
#pragma comment(lib, "Windowscodecs.lib")
#include <atlbase.h>

struct WRAPPER_IStream
{
	IStream* Item;
	explicit WRAPPER_IStream(const std::wstring&, HRESULT&, bool);
	~WRAPPER_IStream();
	void KeepFile();

private:
	bool IsThisTemporaryFile;
	std::wstring mFilePath;
};

WRAPPER_IStream::WRAPPER_IStream(const std::wstring& FilePath, HRESULT& Result, bool WritingToNewFile = false) : Item(NULL), IsThisTemporaryFile(false), mFilePath()
{
	if (WritingToNewFile)
	{
		if (DoesFileExists(FilePath) == false)
		{
			mFilePath = FilePath;
			IsThisTemporaryFile = true;
		}
		Result = SHCreateStreamOnFileEx(FilePath.c_str(), STGM_WRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE, NULL, NULL, NULL, &Item);
	}
	else
	{
		Result = SHCreateStreamOnFileEx(FilePath.c_str(), STGM_READ | STGM_SHARE_DENY_WRITE | STGM_FAILIFTHERE, NULL, false, NULL, &Item);
	}
}

WRAPPER_IStream::~WRAPPER_IStream()
{
	if (Item != NULL)
	{
		Item->Release();
		if (IsThisTemporaryFile)
			DeleteFile(mFilePath.c_str());
	}
}

void WRAPPER_IStream::KeepFile()
{
	IsThisTemporaryFile = false;
}

struct WRAPPER_PROPVARIANT
{
	PROPVARIANT Item;
	WRAPPER_PROPVARIANT();
	~WRAPPER_PROPVARIANT();
};

WRAPPER_PROPVARIANT::WRAPPER_PROPVARIANT()
{
	PropVariantInit(&Item);
}

WRAPPER_PROPVARIANT::~WRAPPER_PROPVARIANT()
{
	PropVariantClear(&Item);
}

WICBitmapTransformOptions GetImageOrientation(IWICBitmapFlipRotator* RotatedBitmap, IWICBitmapSource* BitmapToRotate, IWICBitmapFrameDecode* FrameWithMetadata, const std::wstring& ImageFormat)
{
	if (ImageFormat == L".tiff" || ImageFormat == L".tif" || ImageFormat == L".jpg" || ImageFormat == L".jpeg" || ImageFormat == L".jpe" || ImageFormat == L".jfif")
	{
		HRESULT Result;

		CComPtr<IWICMetadataQueryReader> MetadataReader;
		Result = FrameWithMetadata->GetMetadataQueryReader(&MetadataReader);
		if (FAILED(Result))
			return WICBitmapTransformRotate0;

		WRAPPER_PROPVARIANT Value;

		std::wstring MetadataName = L"";
		
		if (ImageFormat == L".tiff" || ImageFormat == L".tif")
		{
			MetadataName = L"/ifd/{ushort=274}";
		}
		else
		{
			MetadataName = L"/app1/ifd/{ushort=274}";
		}

		Result = MetadataReader->GetMetadataByName(MetadataName.c_str(), &Value.Item);
		if (FAILED(Result))
			return WICBitmapTransformRotate0;

		if (Value.Item.vt != VT_UI2)
			return WICBitmapTransformRotate0;

		USHORT Orientation = Value.Item.uiVal;

		WICBitmapTransformOptions Transformation = WICBitmapTransformRotate0;
		switch (Orientation)
		{
			case 1:
				Transformation = WICBitmapTransformRotate0;
				break;
			case 2:
				Transformation = WICBitmapTransformFlipHorizontal;
				break;
			case 3:
				Transformation = WICBitmapTransformRotate180;
				break;
			case 4:
				Transformation = WICBitmapTransformFlipVertical;
				break;
			case 5:
				Transformation = (WICBitmapTransformOptions)(WICBitmapTransformFlipVertical | WICBitmapTransformRotate90);
				break;
			case 6:
				Transformation = WICBitmapTransformRotate90;
				break;
			case 7:
				Transformation = (WICBitmapTransformOptions)(WICBitmapTransformFlipHorizontal | WICBitmapTransformRotate90);
				break;
			case 8:
				Transformation = WICBitmapTransformRotate270;
				break;
			default:
				return WICBitmapTransformRotate0;
		}

		return Transformation;
	}

	return WICBitmapTransformRotate0;
}

bool MAIN_PROGRAM_CLASS::DecodeImage(const std::wstring& FilePath)
{
	HRESULT Result;



	WRAPPER_CoInitializeEx wrCOM_Initalization(Result);
	if (FAILED(Result))
		return false;



	WRAPPER_IStream wrStreamFromFile(FilePath, Result);
	if (FAILED(Result))
		return false;



	std::wstring ImageFormat;
	GetExtensionFromFilePath(FilePath, ImageFormat);

	IID InputFormat;
	if (ImageFormat == L".png")
		InputFormat = CLSID_WICPngDecoder;
	else if (ImageFormat == L".tiff" || ImageFormat == L".tif")
		InputFormat = CLSID_WICTiffDecoder;
	else if (ImageFormat == L".jpg" || ImageFormat == L".jpeg" || ImageFormat == L".jpe" || ImageFormat == L".jfif")
		InputFormat = CLSID_WICJpegDecoder;
	else if (ImageFormat == L".gif")
		InputFormat = CLSID_WICGifDecoder;
	else if (ImageFormat == L".bmp" || ImageFormat == L".dib")
		InputFormat = CLSID_WICBmpDecoder;
	else
		InputFormat = CLSID_WICBmpDecoder;//default; atttempt to open as a bitmap



	CComPtr<IWICBitmapDecoder> Decoder;
	Result = Decoder.CoCreateInstance(InputFormat, NULL, CLSCTX_INPROC_SERVER);
	if (FAILED(Result))
		return false;

	Result = Decoder->Initialize(wrStreamFromFile.Item, WICDecodeMetadataCacheOnLoad);
	if (FAILED(Result))
		return false;

	UINT FrameCount = 0;
	Result = Decoder->GetFrameCount(&FrameCount);
	if (FAILED(Result) || FrameCount != 1)
		return false;



	CComPtr<IWICBitmapFrameDecode> Frame;
	Result = Decoder->GetFrame(0, &Frame);
	if (FAILED(Result))
		return false;



	CComPtr<IWICBitmapSource> DecodedBitmap;
	Result = WICConvertBitmapSource(GUID_WICPixelFormat32bppBGR, Frame, &DecodedBitmap);
	if (FAILED(Result))
		return false;



	CComPtr<IWICImagingFactory> Factory;
	Result = Factory.CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER);
	if (FAILED(Result))
		return false;

	CComPtr<IWICBitmapFlipRotator> DecodedRotatedBitmap;
	Factory->CreateBitmapFlipRotator(&DecodedRotatedBitmap);
	if (FAILED(Result))
		return false;

	Result = DecodedRotatedBitmap->Initialize(DecodedBitmap, GetImageOrientation(DecodedRotatedBitmap.p, DecodedBitmap.p, Frame.p, ImageFormat));
	if (FAILED(Result))
		return false;



	UINT BitmapWidth = 0;
	UINT BitmapHeight = 0;
	Result = DecodedRotatedBitmap->GetSize(&BitmapWidth, &BitmapHeight);
	if (FAILED(Result) || BitmapWidth == 0 || BitmapHeight == 0)
		return false;

	ZeroMemory(&EvolutionImages.OriginalImage_Info, sizeof(EvolutionImages.OriginalImage_Info));
	EvolutionImages.OriginalImage_Info.bmWidth = BitmapWidth;
	EvolutionImages.OriginalImage_Info.bmHeight = BitmapHeight;
	EvolutionImages.OriginalImage_Info.bmPlanes = 1;
	EvolutionImages.OriginalImage_Info.bmBitsPixel = CONSTANT_x_BYTES_PER_PIXEL * 8;
	const int NumberOfBytesPerPixel = EvolutionImages.OriginalImage_Info.bmBitsPixel / 8;
	EvolutionImages.OriginalImage_Info.bmWidthBytes = EvolutionImages.OriginalImage_Info.bmWidth * NumberOfBytesPerPixel;

	ZeroMemory(&EvolutionImages.DeviceIndependentBitmap_Format, sizeof(EvolutionImages.DeviceIndependentBitmap_Format));
	EvolutionImages.DeviceIndependentBitmap_Format.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	EvolutionImages.DeviceIndependentBitmap_Format.bmiHeader.biWidth = EvolutionImages.OriginalImage_Info.bmWidth;
	EvolutionImages.DeviceIndependentBitmap_Format.bmiHeader.biHeight = -EvolutionImages.OriginalImage_Info.bmHeight;// The negative value here represents a top-down DI bitmap.
	EvolutionImages.DeviceIndependentBitmap_Format.bmiHeader.biPlanes = EvolutionImages.OriginalImage_Info.bmPlanes;
	EvolutionImages.DeviceIndependentBitmap_Format.bmiHeader.biBitCount = EvolutionImages.OriginalImage_Info.bmBitsPixel;
	EvolutionImages.DeviceIndependentBitmap_Format.bmiHeader.biCompression = BI_RGB;

	EvolutionImages.OriginalImage_ComparisonBuffer_DeviceIndependentBitmapHandle = NULL;
	HDC DisplayContext = GetDC(NULL);
	EvolutionImages.OriginalImage_ComparisonBuffer_DeviceIndependentBitmapHandle = CreateDIBSection(DisplayContext, &EvolutionImages.DeviceIndependentBitmap_Format, DIB_RGB_COLORS, (void**)&EvolutionImages.OriginalImage_ComparisonBuffer_DeviceIndependentBitmapPointer, NULL, NULL);
	ReleaseDC(NULL, DisplayContext);
	if (EvolutionImages.OriginalImage_ComparisonBuffer_DeviceIndependentBitmapHandle == NULL)
		return false;



	const UINT BitmapStride = EvolutionImages.OriginalImage_Info.bmWidthBytes;
	const UINT BufferSize = BitmapStride * BitmapHeight;
	Result = DecodedRotatedBitmap->CopyPixels(NULL, BitmapStride, BufferSize, EvolutionImages.OriginalImage_ComparisonBuffer_DeviceIndependentBitmapPointer);
	if (FAILED(Result))
	{
		DeleteObject(EvolutionImages.OriginalImage_ComparisonBuffer_DeviceIndependentBitmapHandle);
		EvolutionImages.OriginalImage_ComparisonBuffer_DeviceIndependentBitmapHandle = NULL;
		return false;
	}



	EvolutionImages.OriginalImage_Info_NumberOfPixels = EvolutionImages.OriginalImage_Info.bmWidth * EvolutionImages.OriginalImage_Info.bmHeight;

	EvolutionImages.OriginalImage_Info_AreaRect.left = 0;
	EvolutionImages.OriginalImage_Info_AreaRect.top = 0;
	EvolutionImages.OriginalImage_Info_AreaRect.right = EvolutionImages.OriginalImage_Info.bmWidth;
	EvolutionImages.OriginalImage_Info_AreaRect.bottom = EvolutionImages.OriginalImage_Info.bmHeight;

	return true;
}

bool MAIN_PROGRAM_CLASS::EncodeImage(const std::wstring& FilePath, const std::wstring& FileExtension) const
{
	HRESULT Result;



	WRAPPER_CoInitializeEx wrCOM_Initalization(Result);
	if (FAILED(Result))
		return false;



	CComPtr<IWICImagingFactory> Factory;
	Result = Factory.CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER);
	if (FAILED(Result))
		return false;



	CComPtr<IWICBitmap> BitmapBeforeConversion;
	CComPtr<IWICBitmapSource> BitmapAfterConversion;

	Result = Factory->CreateBitmapFromHBITMAP(EvolutionImages.EvolvedImage_ComparisonBuffer_DeviceIndependentBitmapHandle, NULL, WICBitmapIgnoreAlpha, &BitmapBeforeConversion);
	if (FAILED(Result))
		return false;

	Result = WICConvertBitmapSource(GUID_WICPixelFormat24bppBGR, BitmapBeforeConversion, &BitmapAfterConversion);
	if (FAILED(Result))
		return false;



	WRAPPER_IStream wrStreamToFile(FilePath.c_str(), Result, true);
	if (FAILED(Result))
		return false;



	IID OutputFormat;
	if (FileExtension == L".png")
		OutputFormat = CLSID_WICPngEncoder;
	else if (FileExtension == L".bmp" || FileExtension == L".dib")
		OutputFormat = CLSID_WICBmpEncoder;
	else
		OutputFormat = CLSID_WICBmpEncoder;//default; atttempt to save as a bitmap

	CComPtr<IWICBitmapEncoder> Encoder;
	Result = Encoder.CoCreateInstance(OutputFormat, NULL, CLSCTX_INPROC_SERVER);
	if (FAILED(Result))
		return false;

	Result = Encoder->Initialize(wrStreamToFile.Item, WICBitmapEncoderNoCache);
	if (FAILED(Result))
		return false;



	CComPtr<IWICBitmapFrameEncode> Frame;

	Result = Encoder->CreateNewFrame(&Frame, NULL);
	if (FAILED(Result))
		return false;

	Result = Frame->Initialize(NULL);
	if (FAILED(Result))
		return false;



	WICPixelFormatGUID InputFormat;
	Result = BitmapAfterConversion->GetPixelFormat(&InputFormat);
	if (FAILED(Result))
		return false;
	WICPixelFormatGUID AvailableFormat = InputFormat;

	Result = Frame->SetPixelFormat(&AvailableFormat);
	if (FAILED(Result)) return false;

	if (IsEqualGUID(InputFormat, AvailableFormat) == false)
		return false;



	Result = Frame->WriteSource(BitmapAfterConversion, NULL);
	if (FAILED(Result))
		return false;

	if (FAILED(Frame->Commit()))
		return false;

	if (FAILED(Encoder->Commit()))
		return false;



	wrStreamToFile.KeepFile();



	return true;
}