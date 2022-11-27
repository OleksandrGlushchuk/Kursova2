#include "window/Application.h"
#include "math/Timer.h"
#include <thread>
#include <chrono>
#include "render/singletones/globals.hpp"
#include <tuple>

const float FRAME_DURATION = 1.f / 60.f;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

engine::windows::Window window;
engine::windows::Renderer renderer;
engine::windows::Application application(window, renderer);
Timer timer;

int WINAPI wWinMain(_In_ HINSTANCE hinstance, _In_opt_ HINSTANCE hprev, _In_ LPWSTR cmdline, _In_ int cmdshow)
{
	engine::connection::init();
	application.InitConnection(cmdline);

	engine::init();

	window = engine::windows::Window(L"Kursova2", hinstance, WndProc);
	application.Init();
	window.Show();

	MSG msg;
	timer.RestartTimer();
	float delta_time = 0;
	while (true)
	{
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT) {
				engine::connection::deinit();
				engine::deinit();
				return msg.wParam;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);

		}

		if (timer.TimeElapsed(FRAME_DURATION))
		{
			application.Draw();
			application.delta_time = timer.GetDeltaTime();
			timer.RestartTimer();
		}
		
		std::this_thread::yield();
	}
	engine::connection::deinit();
	engine::deinit();
	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{

	case WM_ENTERSIZEMOVE:
		application.OnEnterSizeMove();
		break;

	case WM_EXITSIZEMOVE:
		application.OnExitSizeMove();
		timer.RestartTimer();
		break;

	case WM_SIZE:
		application.OnChangeWindowSize();
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_KEYDOWN:
		if ((HIWORD(lparam) & KF_REPEAT) != KF_REPEAT)
			application.OnKeyDown(wparam);
		break;
	case WM_KEYUP:
		application.OnKeyUp(wparam);
		break;

	case WM_MOUSEMOVE:
	{
		WORD mx = LOWORD(lparam), my = HIWORD(lparam);
		application.OnMouseMove(mx, my);
		break;
	}

	case WM_SETCURSOR:
		application.OnSetCursor(lparam);
		break;

	case WM_MOUSEWHEEL:
		application.OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wparam));
		break;
	default: return DefWindowProc(hwnd, msg, wparam, lparam);
	}
	return 0;
}