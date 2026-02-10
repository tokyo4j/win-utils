// compiled with clang -mwindows -o remap.exe remap.c
#include <stdio.h>
#include <windows.h>

enum modifier {
	MOD_NONE = 0,
	MOD_MUHENKAN = 1 << 0,
	MOD_CAPS = 1 << 1,
};

enum action_type {
	ACTION_KEY,
	ACTION_CMD,
};

struct action {
	enum action_type type;
	union {
		DWORD keycode;
		const char *cmd;
	};
};

struct keybind {
	DWORD key;
	DWORD modifiers;
	struct action action;

	BOOL activated;
};

struct context {
	HHOOK hook;
	DWORD active_mods;
	struct keybind keybinds[32];
};

static struct context ctx = {
	.keybinds = {
		{
			.key =  'H',
			.modifiers = MOD_CAPS | MOD_MUHENKAN,
			.action = { .type = ACTION_KEY, .keycode = VK_F14 },
		},
		{
			.key =  'J',
			.modifiers = MOD_CAPS | MOD_MUHENKAN,
			.action = { .type = ACTION_KEY, .keycode = VK_F15 },
		},
		{
			.key =  'K',
			.modifiers = MOD_CAPS | MOD_MUHENKAN,
			.action = { .type = ACTION_KEY, .keycode = VK_F16 },
		},
		{
			.key =  'L',
			.modifiers = MOD_CAPS | MOD_MUHENKAN,
			.action = { .type = ACTION_KEY, .keycode = VK_F17 },
		},
		{
			.key =  'H',
			.modifiers = MOD_MUHENKAN,
			.action = { .type = ACTION_KEY, .keycode = VK_LEFT },
		},
		{
			.key =  'J',
			.modifiers = MOD_MUHENKAN,
			.action = { .type = ACTION_KEY, .keycode = VK_DOWN },
		},
		{
			.key =  'K',
			.modifiers = MOD_MUHENKAN,
			.action = { .type = ACTION_KEY, .keycode = VK_UP },
		},
		{
			.key =  'L',
			.modifiers = MOD_MUHENKAN,
			.action = { .type = ACTION_KEY, .keycode = VK_RIGHT },
		},
		{
			.key = 'A',
			.modifiers = MOD_CAPS,
			.action = { .type = ACTION_KEY, .keycode = VK_LWIN },
		},
		{
			.key = 'S',
			.modifiers = MOD_CAPS,
			.action = { .type = ACTION_CMD, .cmd = "alacritty" },
		},
		{
			.key = 'W',
			.modifiers = MOD_CAPS,
			.action = { .type = ACTION_CMD, .cmd = "explorer" },
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

static enum modifier
get_mod(DWORD code)
{
	switch (code) {
	case VK_F13:
		// Capslock is mapped to F13 via scancode map
		return MOD_CAPS;
	case VK_NONCONVERT:
		return MOD_MUHENKAN;
	default:
		return MOD_NONE;
	}
}

static void
run_action(const struct action *action, BOOL is_down)
{
	switch (action->type) {
	case ACTION_KEY:
		send_key(action->keycode, is_down);
		break;
	case ACTION_CMD:
		if (is_down) {
			ShellExecuteA(NULL, "open", action->cmd,
				NULL, NULL, SW_SHOWNORMAL);
		}
		break;
	}
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
			if (kb->key == keycode
				&& INCLUDES(ctx.active_mods, kb->modifiers)) {
				run_action(&kb->action, TRUE);
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
					|| !INCLUDES(ctx.active_mods,
						kb->modifiers))) {
				run_action(&kb->action, FALSE);
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

	enum modifier mod = get_mod(kb_hook->vkCode);
	if (mod) {
		if (is_down) {
			ctx.active_mods |= mod;
		} else {
			ctx.active_mods &= ~mod;
		}
	}
	if (match_keybind(kb_hook->vkCode, is_down)) {
		return 1;
	}
	if (mod) {
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
