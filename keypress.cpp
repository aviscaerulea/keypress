/**
 * keypress - キー入力自動化ツール
 *
 * 【概要】
 * コマンドライン引数で指定されたキー入力を実行する。
 * ウィンドウは一切表示しない（SUBSYSTEM:WINDOWS）。
 *
 * 【引数形式】
 * keypress.exe "shift+ctrl+alt+a"
 * keypress.exe "shift+a,(100),ctrl+b"
 * keypress.exe "shift+a,500"  - 500 は "5" "0" "0" と順に入力
 * - `,` でキー操作を区切る
 * - `+` で同時押しを表現
 * - `(数字のみ)` でミリ秒単位のスリープ
 * - 上記以外は文字列として1文字ずつキー入力
 *
 * 【対応キー】
 * - 修飾キー: shift, ctrl, alt, win
 * - アルファベット: a-z（大文字小文字区別なし）
 * - 数字: 0-9
 * - ファンクションキー: f1-f24
 * - 特殊キー: enter, esc, tab, space, backspace, delete, など
 *
 * 【ビルド】
 * cl /EHsc /W4 /O2 /utf-8 keypress.cpp /link /SUBSYSTEM:WINDOWS user32.lib
 */

#include <windows.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <algorithm>
#include <cwctype>
#include <regex>
#include <io.h>
#include <fcntl.h>

// 連続キー入力の間隔（ミリ秒）
constexpr int KEY_INTERVAL_MS = 20;

// キー名→仮想キーコードのマップ
std::unordered_map<std::wstring, WORD> g_keymap;

// エラー出力用
FILE* g_stderr = nullptr;

/**
 * キーマップの初期化
 */
void init_keymap() {
    // 修飾キー
    g_keymap[L"shift"] = VK_SHIFT;
    g_keymap[L"ctrl"] = VK_CONTROL;
    g_keymap[L"alt"] = VK_MENU;
    g_keymap[L"win"] = VK_LWIN;

    // 特殊キー
    g_keymap[L"enter"] = VK_RETURN;
    g_keymap[L"esc"] = VK_ESCAPE;
    g_keymap[L"tab"] = VK_TAB;
    g_keymap[L"space"] = VK_SPACE;
    g_keymap[L"backspace"] = VK_BACK;
    g_keymap[L"delete"] = VK_DELETE;
    g_keymap[L"insert"] = VK_INSERT;
    g_keymap[L"home"] = VK_HOME;
    g_keymap[L"end"] = VK_END;
    g_keymap[L"pageup"] = VK_PRIOR;
    g_keymap[L"pagedown"] = VK_NEXT;
    g_keymap[L"up"] = VK_UP;
    g_keymap[L"down"] = VK_DOWN;
    g_keymap[L"left"] = VK_LEFT;
    g_keymap[L"right"] = VK_RIGHT;
    g_keymap[L"printscreen"] = VK_SNAPSHOT;
    g_keymap[L"scrolllock"] = VK_SCROLL;
    g_keymap[L"pause"] = VK_PAUSE;
    g_keymap[L"numlock"] = VK_NUMLOCK;
    g_keymap[L"capslock"] = VK_CAPITAL;

    // ファンクションキー F1-F24
    for (int i = 1; i <= 24; ++i) {
        std::wstring fname = L"f" + std::to_wstring(i);
        g_keymap[fname] = static_cast<WORD>(VK_F1 + (i - 1));
    }
}

/**
 * 文字列を小文字に変換
 */
std::wstring to_lower(const std::wstring& str) {
    std::wstring result = str;
    std::transform(result.begin(), result.end(), result.begin(),
        [](wchar_t c) { return towlower(c); });
    return result;
}

/**
 * 文字列のトリム
 */
std::wstring trim(const std::wstring& str) {
    size_t start = str.find_first_not_of(L" \t\r\n");
    if (start == std::wstring::npos) return L"";
    size_t end = str.find_last_not_of(L" \t\r\n");
    return str.substr(start, end - start + 1);
}

/**
 * 文字列を指定デリミタで分割
 */
std::vector<std::wstring> split(const std::wstring& str, wchar_t delimiter) {
    std::vector<std::wstring> tokens;
    std::wstringstream ss(str);
    std::wstring token;
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

/**
 * キー名を仮想キーコードに変換
 */
bool keyname_to_vk(const std::wstring& keyname, WORD& vk) {
    std::wstring lower = to_lower(keyname);

    // マップに存在する場合
    auto it = g_keymap.find(lower);
    if (it != g_keymap.end()) {
        vk = it->second;
        return true;
    }

    // 1文字のアルファベット
    if (lower.length() == 1) {
        wchar_t ch = lower[0];
        if (ch >= L'a' && ch <= L'z') {
            vk = static_cast<WORD>(towupper(ch));
            return true;
        }
        // 1文字の数字
        if (ch >= L'0' && ch <= L'9') {
            vk = static_cast<WORD>(ch);
            return true;
        }
    }

    return false;
}

/**
 * キーコンビネーション（同時押し）を送信
 */
void send_keys(const std::vector<WORD>& vkeys) {
    std::vector<INPUT> inputs;

    // 全キーを押下
    for (auto vk : vkeys) {
        INPUT inp = {};
        inp.type = INPUT_KEYBOARD;
        inp.ki.wVk = vk;
        inputs.push_back(inp);
    }

    // 全キーを解放（逆順）
    for (auto it = vkeys.rbegin(); it != vkeys.rend(); ++it) {
        INPUT inp = {};
        inp.type = INPUT_KEYBOARD;
        inp.ki.wVk = *it;
        inp.ki.dwFlags = KEYEVENTF_KEYUP;
        inputs.push_back(inp);
    }

    UINT sent = SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
    if (sent != inputs.size()) {
        if (g_stderr) {
            fwprintf(g_stderr, L"Warning: キー入力の一部が失敗しました (%u/%u)\n",
                     sent, static_cast<UINT>(inputs.size()));
        }
    }
}

/**
 * 1文字をキー入力（VkKeyScanW を使用）
 */
bool send_char(wchar_t ch) {
    SHORT scan = VkKeyScanW(ch);
    if (scan == -1) {
        if (g_stderr) {
            fwprintf(g_stderr, L"Error: 入力できない文字: '%c'\n", ch);
        }
        return false;
    }

    BYTE vk = LOBYTE(scan);
    BYTE shift_state = HIBYTE(scan);

    std::vector<INPUT> inputs;

    // 必要な修飾キーを押下
    if (shift_state & 1) {  // Shift
        INPUT inp = {};
        inp.type = INPUT_KEYBOARD;
        inp.ki.wVk = VK_SHIFT;
        inputs.push_back(inp);
    }
    if (shift_state & 2) {  // Ctrl
        INPUT inp = {};
        inp.type = INPUT_KEYBOARD;
        inp.ki.wVk = VK_CONTROL;
        inputs.push_back(inp);
    }
    if (shift_state & 4) {  // Alt
        INPUT inp = {};
        inp.type = INPUT_KEYBOARD;
        inp.ki.wVk = VK_MENU;
        inputs.push_back(inp);
    }

    // キー押下
    INPUT inp_down = {};
    inp_down.type = INPUT_KEYBOARD;
    inp_down.ki.wVk = vk;
    inputs.push_back(inp_down);

    // キー解放
    INPUT inp_up = {};
    inp_up.type = INPUT_KEYBOARD;
    inp_up.ki.wVk = vk;
    inp_up.ki.dwFlags = KEYEVENTF_KEYUP;
    inputs.push_back(inp_up);

    // 修飾キーを解放（逆順）
    if (shift_state & 4) {  // Alt
        INPUT inp = {};
        inp.type = INPUT_KEYBOARD;
        inp.ki.wVk = VK_MENU;
        inp.ki.dwFlags = KEYEVENTF_KEYUP;
        inputs.push_back(inp);
    }
    if (shift_state & 2) {  // Ctrl
        INPUT inp = {};
        inp.type = INPUT_KEYBOARD;
        inp.ki.wVk = VK_CONTROL;
        inp.ki.dwFlags = KEYEVENTF_KEYUP;
        inputs.push_back(inp);
    }
    if (shift_state & 1) {  // Shift
        INPUT inp = {};
        inp.type = INPUT_KEYBOARD;
        inp.ki.wVk = VK_SHIFT;
        inp.ki.dwFlags = KEYEVENTF_KEYUP;
        inputs.push_back(inp);
    }

    UINT sent = SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
    if (sent != inputs.size()) {
        if (g_stderr) {
            fwprintf(g_stderr, L"Error: 文字 '%c' の入力に失敗しました\n", ch);
        }
        return false;
    }
    return true;
}

/**
 * 文字列を1文字ずつキー入力
 */
bool send_string(const std::wstring& str) {
    for (size_t i = 0; i < str.length(); ++i) {
        if (!send_char(str[i])) {
            return false;
        }
        if (i < str.length() - 1) {
            Sleep(KEY_INTERVAL_MS);
        }
    }
    return true;
}

/**
 * トークンがスリープ指定 (数字のみ) かチェック
 */
bool is_sleep_token(const std::wstring& token, int& ms) {
    static std::wregex pattern(L"^\\([0-9]+\\)$");
    if (!std::regex_match(token, pattern)) {
        return false;
    }

    // (数字) の形式なので、中身を抽出
    std::wstring numstr = token.substr(1, token.length() - 2);
    try {
        ms = std::stoi(numstr);
        return true;
    }
    catch (...) {
        if (g_stderr) {
            fwprintf(g_stderr, L"Warning: スリープ時間が大きすぎます: %s（文字列として入力します）\n",
                     token.c_str());
        }
        return false;
    }
}

/**
 * トークンを処理
 */
bool process_token(const std::wstring& token) {
    std::wstring trimmed = trim(token);
    if (trimmed.empty()) return true;

    // スリープ判定: (数字のみ) の形式
    int sleep_ms;
    if (is_sleep_token(trimmed, sleep_ms)) {
        Sleep(sleep_ms);
        return true;
    }

    // `+` を含む場合は同時押し
    if (trimmed.find(L'+') != std::wstring::npos) {
        std::vector<std::wstring> keynames = split(trimmed, L'+');
        std::vector<WORD> vkeys;

        for (const auto& keyname : keynames) {
            std::wstring kn = trim(keyname);
            if (kn.empty()) continue;

            WORD vk;
            if (!keyname_to_vk(kn, vk)) {
                if (g_stderr) {
                    fwprintf(g_stderr, L"Error: 不明なキー名: %s\n", kn.c_str());
                }
                return false;
            }
            vkeys.push_back(vk);
        }

        if (!vkeys.empty()) {
            send_keys(vkeys);
        }
        return true;
    }

    // キーマップに存在する場合は単一キー送信
    WORD vk;
    if (keyname_to_vk(trimmed, vk)) {
        std::vector<WORD> single_key = { vk };
        send_keys(single_key);
        return true;
    }

    // それ以外は文字列として1文字ずつキー入力
    return send_string(trimmed);
}

/**
 * エントリポイント
 */
int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nShowCmd)
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(nShowCmd);

    // 親コンソールにアタッチして stderr を使えるようにする
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        freopen_s(&g_stderr, "CONOUT$", "w", stderr);
        if (g_stderr) {
            _setmode(_fileno(g_stderr), _O_U16TEXT);
        }
    }

    // キーマップ初期化
    init_keymap();

    // 引数チェック
    std::wstring cmdline = lpCmdLine ? lpCmdLine : L"";
    cmdline = trim(cmdline);

    if (cmdline.empty()) {
        if (g_stderr) {
            fwprintf(g_stderr, L"Usage: keypress.exe \"shift+a,(100),ctrl+b\"\n");
        }
        return 1;
    }

    // 先頭と末尾のダブルクォートを除去（長さ2以上の場合のみ）
    if (cmdline.length() >= 2 && cmdline.front() == L'"' && cmdline.back() == L'"') {
        cmdline = cmdline.substr(1, cmdline.length() - 2);
    }

    // `,` で分割してトークン化
    std::vector<std::wstring> tokens = split(cmdline, L',');

    // 各トークンを処理（間に 20ms の間隔を挿入）
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (!process_token(tokens[i])) {
            return 1;
        }
        if (i < tokens.size() - 1) {
            Sleep(KEY_INTERVAL_MS);
        }
    }

    return 0;
}
