#include "Program/Program.h"
#include "common/Settings.h"
#include "resources/resource.h"
#include "common/Windows_include.h"

constexpr int RefreshRate = SETTING_x_FRAMES_PER_SECOND;
constexpr UINT_PTR TimerID = 1;

HBRUSH BackgroundBrush;

HICON EvolvedIcon;

MAIN_PROGRAM_CLASS Program;

LRESULT WINAPI MainWindowProcedure(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
		case WM_CREATE:
		{
			BOOL SetBehaviorTo = false;
			SetUserObjectInformation(GetCurrentProcess(), UOI_TIMERPROC_EXCEPTION_SUPPRESSION, &SetBehaviorTo, sizeof(BOOL));

			if (SetTimer(hWnd, TimerID, 1000 / RefreshRate, NULL) == 0)
			{
				DestroyWindow(hWnd);
				break;
			}

			Program.StartupWindow(hWnd, BackgroundBrush, EvolvedIcon);
			break;
		}
		case WM_DESTROY:
		{
			KillTimer(hWnd, TimerID);

			Program.Shutdown();

			PostQuitMessage(0);
			return 0;
		}
		case WM_COMMAND:
		{
			Program.Button((HWND)lParam);
			break;
		}
		case WM_DROPFILES:
		{
			Program.FileDrop((HDROP)wParam);
			break;
		}
		case WM_CTLCOLORSTATIC:
		{
			HBRUSH Result;
			if (Program.StaticTextColor((HDC)wParam, (HWND)lParam, &Result) == true)
				return (INT_PTR)Result;
			break;
		}
		case WM_TIMER:
		{
			InvalidateRect(hWnd, NULL, false);
			break;
		}
		case WM_PAINT:
		{
			Program.Paint(hWnd);
			break;
		}
	}
	return DefWindowProc(hWnd, Msg, wParam, lParam);
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPWSTR lpCmdLine, _In_ int nShowCmd)
{
	HWND WindowHandle;
	WNDCLASSEX WindowClass;
	TCHAR WindowClassName[] = L"GeneticEvolutionOfImages_2020_06_28";
	BackgroundBrush = CreateSolidBrush(SETTING_x_STARTUP_WINDOW_BACKGROUND_COLOR);

	ZeroMemory(&WindowClass, sizeof(WNDCLASSEX));
	WindowClass.hInstance = hInstance;
	WindowClass.lpszClassName = WindowClassName;
	WindowClass.lpfnWndProc = MainWindowProcedure;
	WindowClass.cbSize = sizeof(WNDCLASSEX);
	WindowClass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	EvolvedIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON2));
	WindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WindowClass.hbrBackground = BackgroundBrush;
	WindowClass.style = CS_HREDRAW | CS_VREDRAW;

	if (!RegisterClassEx(&WindowClass)) return 0;

	WindowHandle = CreateWindowEx(0, WindowClassName, L"Genetic Evolution Of Images", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);

	ShowWindow(WindowHandle, nShowCmd);

	MSG Msg;

	for (;;)
	{
		GetMessage(&Msg, NULL, 0, 0);
		if (Msg.message == WM_QUIT)
			break;
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}

	DeleteObject(BackgroundBrush);

	return static_cast<int>(Msg.wParam);
}