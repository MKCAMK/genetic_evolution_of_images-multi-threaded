#pragma once
#include "common/Windows_include.h"
#include <string>

bool DoesFileExists(const std::wstring&);

void GetExtensionFromFilePath(const std::wstring&, std::wstring&);

enum struct PROGRAM_STATUS { STARTUP, WORKING, PAUSED, SHUTDOWN };

struct LABELED_INPUT_FIELD
{
	int Position_X		{ 0 };
	int Position_Y		{ 0 };
	bool Error			{ false };
	int MaximumValue	{ 0 };
	int MinimumValue	{ 0 };
	HWND StaticText		{ NULL };
	HWND InputField		{ NULL };
};

struct WINDOW_CONTROLS
{
	LABELED_INPUT_FIELD NumberOfImages				{};

	LABELED_INPUT_FIELD NumberOfEllipses			{};

	LABELED_INPUT_FIELD EllipseMutationChance		{};

	LABELED_INPUT_FIELD EllipseSizeAtCreation		{};

	LABELED_INPUT_FIELD EllipseShapeMutationRange	{};

	LABELED_INPUT_FIELD EllipseColorMutationRange	{};

	LABELED_INPUT_FIELD BoundingBoxMargin			{};

	LABELED_INPUT_FIELD ComparisonPrecision			{};

	LABELED_INPUT_FIELD FileSelect					{};
	HWND FileSelect_Button				{ NULL };

	HWND Start_Button					{ NULL };

	HWND Pause_Resume_Button			{ NULL };

	HWND SaveImage_Button				{ NULL };

	bool WorkWindowButtonsAreVisible	{ false };

	HFONT ControlsFont					{ NULL };

	HBRUSH BackgroundBrush				{ NULL };
	COLORREF BackgroundBrushColor		{};

	HICON EvolvedIcon					{ NULL };
};