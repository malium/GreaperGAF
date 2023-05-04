/***********************************************************************************
*   Copyright 2022 Marcos Sánchez Torrent.                                         *
*   All Rights Reserved.                                                           *
***********************************************************************************/

#include "../ImplPrerequisites.h"
#include "WinWindow.h"
#include "../WindowManager.h"
#include "../GreaperGALDLL.h"
#include "../../../GreaperCore/Public/Win/Win32Base.h"
#include "../../../GreaperCore/Public/StringUtils.h"
#include "../../../GreaperCore/Public/SlimTaskScheduler.h"
#include "../../../GreaperCore/Public/StringUtils.h"

using namespace greaper;
using namespace greaper::gal;

extern SPtr<WindowManager> gWindowManager;
extern SPtr<GreaperGALLibrary> gGALLibrary;

static constexpr WStringView gWindowClassID = L"GreaperWindow"sv;

/*
Message order:
WM_GETMINMAXINFO
WM_NCCREATE
WM_NCCALCSIZE
WM_CREATE
WM_SHOWWINDOW
WM_WINDOWPOSCHANGING
WM_WINDOWPOSCHANGING
WM_ACTIVATEAPP
WM_NCACTIVATE
WM_GETICON
WM_GETICON
WM_GETICON
WM_ACTIVATE
WM_IME_SETCONTEXT
WM_IME_NOTIFY
WM_SETFOCUS
WM_NCPAINT
WM_ERASEBKGND
WM_WINDOWPOSCHANGED
WM_SIZE
WM_MOVE
WM_PAINT
0x0090
WM_WINDOWPOSCHANGING
WM_WINDOWPOSCHANGED
WM_NCACTIVATE
WM_ACTIVATE
WM_ACTIVATEAPP
WM_KILLFOCUS
WM_IME_SETCONTEXT
WM_IME_NOTIFY
WM_DESTROY
WM_NCDESTROY
*/

static LRESULT CALLBACK WindowMessageProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	auto* wnd = reinterpret_cast<WinWindowImpl*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
	if (wnd != nullptr)
		return wnd->OnWindowProc(hWnd, uMsg, wParam, lParam);

	return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

LRESULT WinWindowImpl::WM_CLOSE_MSG(UNUSED WPARAM wParam, UNUSED LPARAM lParam)
{
	m_ShouldClose = true;
	DestroyWindow(m_WindowHandle);
	return 0;
}

LRESULT WinWindowImpl::WM_DESTROYQUIT_MSG(WPARAM wParam, LPARAM lParam)
{
	m_ShouldClose = true;
	m_WindowHandle = nullptr;
	return DefWindowProcW(m_WindowHandle, m_LastMessageID, wParam, lParam);
}


void greaper::gal::WinWindowImpl::AddWinMessages() noexcept
{
	LOCK(m_MessageMutex);
#define SETMSG(command, function)\
_SetWinMessage(command, [this](WPARAM wParam, LPARAM lParam) { return function(wParam, lParam); })

	SETMSG(WM_CLOSE, WM_CLOSE_MSG);
	SETMSG(WM_DESTROY, WM_DESTROYQUIT_MSG);
	SETMSG(WM_QUIT, WM_DESTROYQUIT_MSG);

#undef SETMSG
}

EmptyResult WinWindowImpl::Create(const WindowDesc& windowDesc)noexcept
{
	WinWindowDesc desc = (const WinWindowDesc&)windowDesc;

	m_ThreadID = CUR_THID();
	m_TaskScheduler = desc.Scheduler;
	m_Mode = desc.Mode;
	m_State = desc.State;
	m_ResizingEnabled = desc.ResizingEnabled;

	// retrieve monitor
	if (windowDesc.Monitor != nullptr)
		desc.Monitor = windowDesc.Monitor;
	else
		desc.Monitor = gWindowManager->GetMainMonitor();

	WNDCLASSEXW wc;
	ClearMemory(wc);
	wc.cbSize = sizeof(wc);
	RECT windowRect;
	ClearMemory(windowRect);
	DWORD dwStyle = 0, dwStyleEx = 0;
	m_Position.SetZero();

	if (desc.Size.X > 0 && desc.Size.Y > 0)
		m_Size = desc.Size;
	else
		m_Size.Set(1280, 720);
	

	switch(desc.Mode)
	{
	case WindowMode_t::FullScreen:
	{
		dwStyle = WS_POPUP;
		dwStyleEx = WS_EX_APPWINDOW;
	}
	break;
	case WindowMode_t::Borderless:
	{
		dwStyle = WS_POPUP;
		dwStyleEx = WS_EX_APPWINDOW;
	}
	break;
	default:
		gGALLibrary->LogWarning(Format("Trying to create a Windows window, but an invalid WindowMode %s was given, falling back to Windowed.", TEnum<WindowMode_t>::ToString(desc.Mode).data()));
		desc.Mode = WindowMode_t::Windowed;
	case WindowMode_t::Windowed:
	{
		dwStyle = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
		if (m_ResizingEnabled)
			dwStyle |= WS_THICKFRAME;

		if (m_State == WindowState_t::Minimized)
			dwStyle |= WS_ICONIC;
		if (m_State == WindowState_t::Maximized)
			dwStyle |= WS_MAXIMIZE;

		dwStyleEx = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
	}
	}

	auto wApp = gGALLibrary->GetApplication();
	if (!wApp.expired())
	{
		auto app = wApp.lock();
		auto wAppInstanceProp = app->GetAppInstance();
		if (!wAppInstanceProp.expired())
		{
			auto appInstanceProp = wAppInstanceProp.lock();
			wc.hInstance = (HINSTANCE)appInstanceProp->GetValueCopy();
		}
	}
	
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = &WindowMessageProc;
	wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
	wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = gWindowClassID.data();
	wc.hIconSm = LoadIconW(nullptr, IDI_APPLICATION);
	wc.cbWndExtra = sizeof(this);

	const auto regFailed = RegisterClassExW(&wc) == 0;

	if (regFailed)
	{
		const auto errCode = GetLastError();
		return Result::CreateFailure(Format("RegisterClassExW failed with error code " I32_HEX_FMT ", error message '%S'.", errCode,
			OSPlatform::GetLastErrorAsString(errCode).c_str()));
	}

	auto wTitle = StringUtils::ToWIDE(desc.Title);

	HWND parentWindow = nullptr;
	if (desc.ParentWindow != nullptr)
		parentWindow = ((SPtr<WinWindow>)desc.ParentWindow)->GetOSHandle();

	AddWinMessages();

	m_WindowHandle = CreateWindowExW(dwStyleEx, gWindowClassID.data(), wTitle.c_str(),
		WS_CLIPCHILDREN | WS_CLIPSIBLINGS | dwStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, m_Size.X, m_Size.Y, parentWindow, nullptr, wc.hInstance, nullptr);

	if (m_WindowHandle == nullptr)
	{
		const auto errCode = GetLastError();
		return Result::CreateFailure(Format("CreateWindowExW failed with code " I32_HEX_FMT ", error message '%S'.", errCode,
			OSPlatform::GetLastErrorAsString(errCode).c_str()));
	}

	auto swlpRet = SetWindowLongPtrW(m_WindowHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
	if (swlpRet == 0)
	{
		const auto errorCode = GetLastError();
		if (errorCode != 0)
		{
			DestroyWindow(m_WindowHandle);
			m_WindowHandle = nullptr;
			return Result::CreateFailure(Format("SetWindowLongPtrW failed with code " I32_HEX_FMT ", error message '%S'.", errorCode,
				OSPlatform::GetLastErrorAsString(errorCode).c_str()));
		}
	}

	ChangeWindowMessageFilterEx(m_WindowHandle, WM_DROPFILES, MSGFLT_ALLOW, nullptr);
	ChangeWindowMessageFilterEx(m_WindowHandle, WM_COPYDATA, MSGFLT_ALLOW, nullptr);
	ChangeWindowMessageFilterEx(m_WindowHandle, WM_COPYGLOBALDATA, MSGFLT_ALLOW, nullptr);

	if (desc.StartVisible)
	{
		switch (m_State)
		{
		case WindowState_t::Maximized:
			::ShowWindow(m_WindowHandle, SW_MAXIMIZE);
			break;
		case WindowState_t::Minimized:
			::ShowWindow(m_WindowHandle, SW_MINIMIZE);
			break;
		default:
			::ShowWindow(m_WindowHandle, SW_SHOWNORMAL);
			SetForegroundWindow(m_WindowHandle);
			break;
		}
	}

	if (desc.StartFocused)
	{
		SetFocus(m_WindowHandle);
	}
	
	UpdateWindow(m_WindowHandle);

	
	//DestroyWindow(m_WindowHandle);
	//m_WindowHandle = nullptr;
	//return Result::CreateFailure("Not implemented"sv);
	return Result::CreateSuccess();
}

EmptyResult greaper::gal::WinWindowImpl::_ChangeWindowSize(math::Vector2i size)
{
	return Result::CreateFailure("Not implemented"sv);
}

EmptyResult greaper::gal::WinWindowImpl::_ChangeWindowPosition(math::Vector2i size)
{
	return Result::CreateFailure("Not implemented"sv);
}

TResult<String> greaper::gal::WinWindowImpl::_GetWindowTitle() const
{
	const auto lengthRtn = GetWindowTextLengthW(m_WindowHandle);
	if (lengthRtn == 0)
	{
		const auto err = GetLastError();
		if (err != 0)
			return Result::CreateFailure<String>(Format("GetWindowTextLengthW failed, errorCode %" I32_HEX_FMT " errorMessage %S.", OSPlatform::GetLastErrorAsString(err).c_str()));
	}
	WString title;
	title.resize(lengthRtn);
	const auto rtn = GetWindowTextW(m_WindowHandle, title.data(), (int)title.size());
	if (rtn == 0 && lengthRtn != rtn)
	{
		const auto err = GetLastError();
		if (err != 0)
			return Result::CreateFailure<String>(Format("GetWindowTextW failed, errorCode %" I32_HEX_FMT " errorMessage %S.", OSPlatform::GetLastErrorAsString(err).c_str()));
	}
	return Result::CreateSuccess(StringUtils::FromWIDE(title));
}

EmptyResult greaper::gal::WinWindowImpl::_ChangeWindowPositionAnchor(AnchoredPosition_t anchor)
{
	return Result::CreateFailure("Not implemented"sv);
}

EmptyResult greaper::gal::WinWindowImpl::_ChangeWindowTitle(StringView title)
{
	auto rtn = SetWindowTextW(m_WindowHandle, StringUtils::ToWIDE(title).c_str());
	if (rtn != 0)
		return Result::CreateSuccess();
	const auto err = GetLastError();
	return Result::CreateFailure(Format("SetWindowTextW failed, errorCode %" I32_HEX_FMT " errorMessage %S.", OSPlatform::GetLastErrorAsString(err).c_str()));
}

EmptyResult greaper::gal::WinWindowImpl::_ChangeWindowMode(WindowMode_t mode)
{
	return Result::CreateFailure("Not implemented"sv);
}

EmptyResult greaper::gal::WinWindowImpl::_ChangeWindowState(WindowState_t state)
{
	return Result::CreateFailure("Not implemented"sv);
}

EmptyResult greaper::gal::WinWindowImpl::_ShowWindow()
{
	return Result::CreateFailure("Not implemented"sv);
}

EmptyResult greaper::gal::WinWindowImpl::_HideWindow()
{
	return Result::CreateFailure("Not implemented"sv);
}

EmptyResult greaper::gal::WinWindowImpl::_RequestFocus()
{
	return Result::CreateFailure("Not implemented"sv);
}

EmptyResult greaper::gal::WinWindowImpl::_EnableResizing(bool enable)
{
	return Result::CreateFailure("Not implemented"sv);
}

EmptyResult greaper::gal::WinWindowImpl::_ChangeResizingAspectRatio(math::Vector2i aspectRatio, bool changeCurrent)
{
	return Result::CreateFailure("Not implemented"sv);
}

EmptyResult greaper::gal::WinWindowImpl::_ChangeMaxWindowSize(math::Vector2i maxSize, bool changeCurrent)
{
	return Result::CreateFailure("Not implemented"sv);
}

EmptyResult greaper::gal::WinWindowImpl::_ChangeMinWindowSize(math::Vector2i minSize, bool changeCurrent)
{
	return Result::CreateFailure("Not implemented"sv);
}

EmptyResult greaper::gal::WinWindowImpl::_ChangeMonitor(PMonitor monitor)
{
	return Result::CreateFailure("Not implemented"sv);
}

TResult<String> greaper::gal::WinWindowImpl::_GetClipboardText() const
{
	return Result::CreateFailure<String>("Not implemented"sv);
}

EmptyResult greaper::gal::WinWindowImpl::_SetClipboardText(StringView text)const
{
	return Result::CreateFailure("Not implemented"sv);
}

TResult<bool> greaper::gal::WinWindowImpl::_HasClipboardText()const
{
	return Result::CreateFailure<bool>("Not implemented"sv);
}

EmptyResult greaper::gal::WinWindowImpl::_PollEvents()const
{
	MSG msg;
	while (PeekMessageW(&msg, m_WindowHandle, 0, 0, PM_REMOVE) != 0)
	{
		TranslateMessage(&msg);

		DispatchMessageW(&msg);
	}
	return Result::CreateSuccess();
}

EmptyResult greaper::gal::WinWindowImpl::_WaitForEvents() const
{
	MSG msg;
	auto msgRtn = GetMessageW(&msg, m_WindowHandle, 0, 0);
	if (msgRtn > -1)
	{
		TranslateMessage(&msg);

		DispatchMessageW(&msg);

		return Result::CreateSuccess();
	}
	const auto err = GetLastError();
	return Result::CreateFailure(Format("GetMessageW failed, errorCode %" I32_HEX_FMT " errorMessage %S.", OSPlatform::GetLastErrorAsString(err).c_str()));
}

EmptyResult greaper::gal::WinWindowImpl::_SwapWindow()const
{
	/* No-op */
	return Result::CreateSuccess();
}

EmptyResult greaper::gal::WinWindowImpl::_CloseWindow()
{
	return Result::CreateFailure("Not implemented"sv);
}

RenderBackend_t greaper::gal::WinWindowImpl::GetRenderBackend() const
{
	return RenderBackend_t::Native;
}

LRESULT greaper::gal::WinWindowImpl::OnWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//gGALLibrary->LogVerbose(Format("Message from window " I32_HEX_FMT " wParam %" PRIuPTR " lParam %" PRIiPTR, uMsg, wParam, lParam));


	//if (!m_ShouldClose)
	//{
	//	if (uMsg == WM_CLOSE)
	//	{
	//		LOCK(m_Mutex);
	//		m_ShouldClose = true;
	//		DestroyWindow(m_WindowHandle);
	//		m_WindowHandle = nullptr;
	//		return 0;
	//	}
	//	if (uMsg == WM_DESTROY || uMsg == WM_QUIT)
	//	{
	//		LOCK(m_Mutex);
	//		m_ShouldClose = true;
	//	}
	//}
	MessageFn* fn = nullptr;
	{
		SHAREDLOCK(m_MessageMutex);
		const auto it = m_MessageMap.find(uMsg);
		if (it != m_MessageMap.end())
			fn = &it->second;
		m_LastMessageID = uMsg;
	}
	if (fn != nullptr)
		return (*fn)(wParam, lParam);
	
	return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}