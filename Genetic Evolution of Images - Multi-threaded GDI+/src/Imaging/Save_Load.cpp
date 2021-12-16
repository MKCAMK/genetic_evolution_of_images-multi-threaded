#include "Program/Program.h"
#include "common/CoInitialize.h"

#include <shobjidl_core.h>
#include <atlbase.h>

constexpr int FILEPATH_MAX_LENGTH = MAX_PATH;

void MAIN_PROGRAM_CLASS::BrowseForFile(HWND DialogOwnerHandle)
{
	HRESULT Result;

	WRAPPER_CoInitializeEx wrCOM_Initalization(Result);
	if (FAILED(Result))
		return;

	CComPtr<IFileOpenDialog> FileOpenDialog;
	Result = FileOpenDialog.CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER);
	if (FAILED(Result))
		return;

	FILEOPENDIALOGOPTIONS OptionsFlags = FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST | FOS_FORCEPREVIEWPANEON;
	Result = FileOpenDialog->SetOptions(OptionsFlags);
	if (FAILED(Result))
		return;

	COMDLG_FILTERSPEC FileTypes[] =
	{
		{L"All supported image formats", L"*.png;*.jpg;*.jpeg;*.jpe;*.jfif;*.gif;*tif;*.tiff;*.bmp;*.dib"},
		{L"PNG files (*.png)", L"*.png"},
		{L"JPEG files (*.jpg;*.jpeg;*.jpe;*.jfif)", L"*.jpg;*.jpeg;*.jpe;*.jfif"},
		{L"GIF files (*.gif)", L"*.gif"},
		{L"Tiff files (*tif;*.tiff)", L"*tif;*.tiff"},
		{L"Bitmap files (*.bmp;*.dib)", L"*.bmp;*.dib"},
		{L"All files", L"*"}
	};
	Result = FileOpenDialog->SetFileTypes(ARRAYSIZE(FileTypes), FileTypes);
	if (FAILED(Result))
		return;

	Result = FileOpenDialog->SetFileTypeIndex(1);
	if (FAILED(Result))
		return;

	Result = FileOpenDialog->Show(DialogOwnerHandle);
	if (FAILED(Result))
		return;

	CComPtr<IShellItem> FileDialogResults;
	Result = FileOpenDialog->GetResult(&FileDialogResults);
	if (FAILED(Result))
		return;

	LPWSTR ChosenFilePath;
	Result = FileDialogResults->GetDisplayName(SIGDN_FILESYSPATH, &ChosenFilePath);
	if (FAILED(Result))
		return;

	SetWindowText(WindowControls.FileSelect.InputField, ChosenFilePath);

	CoTaskMemFree(ChosenFilePath);
}

void ToCWString(const std::wstring& STDWString, wchar_t* CWString)
{
	memcpy(CWString, STDWString.c_str(), sizeof(wchar_t) * (STDWString.length() + 1));
}

struct WRAPPER_CriticalSection
{
	CRITICAL_SECTION& Item;
	explicit WRAPPER_CriticalSection(CRITICAL_SECTION&);
	~WRAPPER_CriticalSection();
};

WRAPPER_CriticalSection::WRAPPER_CriticalSection(CRITICAL_SECTION& CriticalSection) : Item(CriticalSection)
{
	EnterCriticalSection(&Item);
}

WRAPPER_CriticalSection::~WRAPPER_CriticalSection()
{
	LeaveCriticalSection(&Item);
}

void MAIN_PROGRAM_CLASS::SaveImage(HWND DialogOwnerHandle)
{
	WRAPPER_CriticalSection wrEnterCriticalSection(CriticalSection_CurrentStatus);
	if (CurrentStatus.load(std::memory_order_relaxed) == PROGRAM_STATUS::PAUSED)
	{
		HRESULT Result;

		WRAPPER_CoInitializeEx wrCOM_Initalization(Result);
		if (FAILED(Result))
			return;

		CComPtr<IFileSaveDialog> FileSaveDialog;
		Result = FileSaveDialog.CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_INPROC_SERVER);
		if (FAILED(Result))
			return;

		FILEOPENDIALOGOPTIONS OptionsFlags = FOS_OVERWRITEPROMPT | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_NOREADONLYRETURN;
		Result = FileSaveDialog->SetOptions(OptionsFlags);
		if (FAILED(Result))
			return;

		COMDLG_FILTERSPEC FileTypes[] =
		{
			{L"PNG file (*.png)", L"*.png"},
			{L"Bitmap file (*.bmp; *.dib)", L"* .bmp; *.dib"}
		};
		Result = FileSaveDialog->SetFileTypes(ARRAYSIZE(FileTypes), FileTypes);
		if (FAILED(Result))
			return;

		Result = FileSaveDialog->SetFileTypeIndex(1);
		if (FAILED(Result))
			return;

		Result = FileSaveDialog->SetDefaultExtension(L"png");
		if (FAILED(Result))
			return;

		const std::wstring SaveDialogDefaultFileName = NameOfTheOpenedFile + L"_GEN" + std::to_wstring(ReportedGeneration.load(std::memory_order_relaxed));
		wchar_t FilePathBuffer[FILEPATH_MAX_LENGTH];
		ToCWString(SaveDialogDefaultFileName, FilePathBuffer);
		Result = FileSaveDialog->SetFileName(FilePathBuffer);
		if (FAILED(Result))
			return;

		Result = FileSaveDialog->Show(DialogOwnerHandle);
		if (FAILED(Result))
			return;

		CComPtr<IShellItem> FileDialogResults;
		Result = FileSaveDialog->GetResult(&FileDialogResults);
		if (FAILED(Result))
			return;

		LPWSTR ChosenFilePath;
		Result = FileDialogResults->GetDisplayName(SIGDN_FILESYSPATH, &ChosenFilePath);
		if (FAILED(Result))
			return;

		const std::wstring FilePath = ChosenFilePath;
		std::wstring FileExtension;
		GetExtensionFromFilePath(FilePath, FileExtension);

		EncodeImage(FilePath, FileExtension);

		CoTaskMemFree(ChosenFilePath);
	}
}