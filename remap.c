// compiled with clang -mwindows -o remap.exe remap.c
#include <stdio.h>
#include <windows.h>

#include <shellapi.h>

struct keybind {
	DWORD key;
	DWORD sent_keycode;

	BOOL activated;
};

struct context {
	HHOOK hook;
	BOOL muhenkan_pressed;
	BOOL active;
	struct keybind keybinds[6];
	HWND tray_hwnd;
	NOTIFYICONDATAW nid;
};

static struct context ctx = {
	.active = TRUE,
	.keybinds =
		{
			{
				.key = 'A',
				.sent_keycode = VK_HOME,
			},
			{
				.key = 'D',
				.sent_keycode = VK_END,
			},
			{
				.key = 'H',
				.sent_keycode = VK_LEFT,
			},
			{
				.key = 'J',
				.sent_keycode = VK_DOWN,
			},
			{
				.key = 'K',
				.sent_keycode = VK_UP,
			},
			{
				.key = 'L',
				.sent_keycode = VK_RIGHT,
			},
		},
};

#define WM_TRAYICON (WM_USER + 1)

static LRESULT CALLBACK
tray_wndproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_TRAYICON:
		if (lParam == WM_LBUTTONUP) {
			/* toggle active state */
			ctx.active = !ctx.active;
			if (ctx.active) {
				wcscpy_s(ctx.nid.szTip,
					ARRAYSIZE(ctx.nid.szTip), L"active");
				ctx.nid.hIcon =
					LoadIconA(NULL, IDI_APPLICATION);
			} else {
				wcscpy_s(ctx.nid.szTip,
					ARRAYSIZE(ctx.nid.szTip), L"inactive");
				ctx.nid.hIcon = LoadIconA(NULL, IDI_ERROR);
			}
			Shell_NotifyIconW(NIM_MODIFY, &ctx.nid);
		} else if (lParam == WM_RBUTTONUP) {
			/* remove tray icon and exit */
			if (ctx.nid.cbSize) {
				Shell_NotifyIconW(NIM_DELETE, &ctx.nid);
			}
			PostQuitMessage(0);
			return 0;
		}
		break;
	case WM_DESTROY:
		if (ctx.nid.cbSize) {
			Shell_NotifyIconW(NIM_DELETE, &ctx.nid);
		}
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static int
create_tray_window_and_icon(void)
{
	WNDCLASSW wc = {
		.lpfnWndProc = tray_wndproc,
		.hInstance = GetModuleHandle(NULL),
		.lpszClassName = L"remap_tray_class",
	};

	if (!RegisterClassW(&wc)
		&& GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
		return 1;
	}

	ctx.tray_hwnd = CreateWindowExW(0, wc.lpszClassName, L"remap_tray", 0,
		0, 0, 0, 0, HWND_MESSAGE, NULL, wc.hInstance, NULL);
	if (!ctx.tray_hwnd) {
		return 1;
	}

	NOTIFYICONDATAW nid = {
		.cbSize = sizeof(nid),
		.hWnd = ctx.tray_hwnd,
		.uID = 1,
		.uCallbackMessage = WM_TRAYICON,
		.hIcon = LoadIconA(NULL, IDI_APPLICATION),
		.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP,
	};
	wcscpy_s(nid.szTip, ARRAYSIZE(nid.szTip), L"active");
	ctx.nid = nid;

	if (!Shell_NotifyIconW(NIM_ADD, &ctx.nid)) {
		return 1;
	}
	return 0;
}

static void
send_key(DWORD keycode, BOOL is_down)
{
	INPUT inputs[1] = {{
		.type = INPUT_KEYBOARD,
		.ki.wVk = keycode,
		.ki.dwFlags = is_down ? 0 : KEYEVENTF_KEYUP,
	}};
	SendInput(ARRAYSIZE(inputs), inputs, sizeof(inputs));
}

static BOOL
match_keybind(DWORD keycode, BOOL is_down)
{
	if (!ctx.active) {
		return FALSE;
	}
	BOOL consumed = FALSE;
	if (is_down) {
		for (size_t i = 0; i < ARRAYSIZE(ctx.keybinds); i++) {
			struct keybind *kb = &ctx.keybinds[i];
			if (kb->key == keycode && ctx.muhenkan_pressed) {
				send_key(kb->sent_keycode, TRUE);
				kb->activated = TRUE;
				consumed = TRUE;
			}
		}
	} else {
		for (size_t i = 0; i < ARRAYSIZE(ctx.keybinds); i++) {
			struct keybind *kb = &ctx.keybinds[i];
			if (kb->activated
				&& (kb->key == keycode
					|| !ctx.muhenkan_pressed)) {
				send_key(kb->sent_keycode, FALSE);
				kb->activated = FALSE;
				consumed = TRUE;
			}
		}
	}
	return consumed;
}

static LRESULT CALLBACK
handle_key_event(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode < 0) {
		goto out;
	}

	KBDLLHOOKSTRUCT *kb_hook = (KBDLLHOOKSTRUCT *)lParam;
	BOOL is_down = !(kb_hook->flags & LLKHF_UP);

	if (kb_hook->flags & LLKHF_INJECTED) {
		goto out;
	}

	if (kb_hook->vkCode == VK_NONCONVERT) {
		ctx.muhenkan_pressed = is_down;
	}
	if (match_keybind(kb_hook->vkCode, is_down)) {
		return 1;
	}
	if (kb_hook->vkCode == VK_NONCONVERT) {
		return 1;
	}

out:
	return CallNextHookEx(ctx.hook, nCode, wParam, lParam);
}

int
main(void)
{
	ctx.hook = SetWindowsHookExW(WH_KEYBOARD_LL, handle_key_event,
		GetModuleHandle(NULL), 0);
	if (!ctx.hook) {
		fprintf(stderr, "SetWindowsHookEx failed: %lu\n",
			GetLastError());
		return 1;
	}

	if (create_tray_window_and_icon() != 0) {
		fprintf(stderr, "Failed to create tray icon\n");
		return 1;
	}

	MSG msg;
	while (GetMessageW(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	UnhookWindowsHookEx(ctx.hook);
	return 0;
}
