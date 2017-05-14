#include "stdafx.h"
#include "omp.h"

void createConsole(LPCSTR consoleTitle)
{
	AllocConsole();
	SetConsoleTitle(consoleTitle);
	freopen("CONIN$", "r", stdin);
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	omp_set_num_threads(omp_get_max_threads());

#if _DEBUG
	createConsole("ShaderLab Debug");
#endif


	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	ShaderLab theApp(hInstance, nCmdShow);
	if (!theApp.Initialize())
		return 0;

	return theApp.Run();
}
