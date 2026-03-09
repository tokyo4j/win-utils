// compiled with clang -mwindows -o remap.exe remap.c
#include <stdio.h>
#include <windows.h>

struct keybind {
	DWORD key;
	DWORD sent_keycode;

	BOOL activated;
};

struct context {
	HHOOK hook;
	BOOL muhenkan_pressed;
	struct keybind keybinds[32];
};

static struct context ctx = {
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

// TRUE when a includes b
#define INCLUDES(a, b) (((a) & (b)) == (b))

static BOOL
match_keybind(DWORD keycode, BOOL is_down)
{
	if (is_down) {
		for (size_t i = 0;; i++) {
			struct keybind *kb = &ctx.keybinds[i];
			if (!kb->key) {
				break;
			}
			if (kb->key == keycode && ctx.muhenkan_pressed) {
				send_key(kb->sent_keycode, TRUE);
				kb->activated = TRUE;
				return TRUE;
			}
		}
	} else {
		for (size_t i = 0;; i++) {
			struct keybind *kb = &ctx.keybinds[i];
			if (!kb->key) {
				break;
			}
			if (kb->activated
				&& (kb->key == keycode
					|| !ctx.muhenkan_pressed)) {
				send_key(kb->sent_keycode, FALSE);
				kb->activated = FALSE;
				return TRUE;
			}
		}
	}
	return FALSE;
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
	CHAR home_dir[256] = {0};
	DWORD home_len = GetEnvironmentVariableA("USERPROFILE", home_dir,
		sizeof(home_dir));
	if (home_len > 0) {
		SetCurrentDirectoryA(home_dir);
	}

	ctx.hook = SetWindowsHookExW(WH_KEYBOARD_LL, handle_key_event,
		GetModuleHandle(NULL), 0);
	if (!ctx.hook) {
		fprintf(stderr, "SetWindowsHookEx failed: %lu\n",
			GetLastError());
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
