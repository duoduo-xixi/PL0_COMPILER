// 预处理: windows.h 的 TokenType 枚举值与我们的 TokenType typedef 冲突
#define TokenType TokenType_WIN32
#include "gui.h"             // → includes windows.h
#undef TokenType

#include "lexer.h"           // → includes common.h (defines our TokenType)
#include "lr_parser.h"
#include "semantic.h"
#include "../include/common.h"
#include <stdio.h>
#include <io.h>
#include <richedit.h>        // Rich Edit 控件支持

// ==================== 控件 ID ====================
#define IDC_SOURCE_EDIT    1001
#define IDC_OUTPUT_EDIT    1002
#define IDC_RUN_BUTTON     1003
#define IDC_TEST_LIST      1004
#define IDC_RADIO_LEXER    1005
#define IDC_RADIO_PARSER   1006
#define IDC_RADIO_SEMANTIC 1007

// ==================== 布局常量 ====================
#define MARGIN        10
#define GAP           8
#define LABEL_H       20
#define MIDDLE_W      130
#define TEST_LIST_H   150

// ==================== 全局状态 ====================
static HWND g_hSourceEdit;
static HWND g_hOutputEdit;
static HWND g_hTestList;
static HWND g_hRunButton;
static HWND g_hRadioLexer;
static HWND g_hRadioParser;
static HWND g_hRadioSemantic;
static HWND g_hMainWnd;

static HFONT g_hCodeFont;
static HFONT g_hArrowFont;
static HFONT g_hLabelFont;

// 0=词法, 1=语法, 2=语义
static int g_analysis_mode = 2;

// ==================== 编码转换 ====================
static char* w2n(LPCWSTR wstr) {
    if (!wstr) return NULL;
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    char *buf = (char*)malloc(len);
    if (buf) WideCharToMultiByte(CP_UTF8, 0, wstr, -1, buf, len, NULL, NULL);
    return buf;
}

static WCHAR* n2w(const char *str) {
    if (!str) return NULL;
    int len = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
    WCHAR *buf = (WCHAR*)malloc(len * sizeof(WCHAR));
    if (buf) MultiByteToWideChar(CP_UTF8, 0, str, -1, buf, len);
    return buf;
}

// ==================== 换行符规范化 ====================
static char* fix_newlines(const char *str) {
    if (!str) return NULL;
    int extra = 0;
    for (const char *p = str; *p; p++) {
        if (*p == '\n' && (p == str || *(p-1) != '\r'))
            extra++;
    }
    if (extra == 0) return _strdup(str);

    size_t orig_len = strlen(str);
    char *result = (char*)malloc(orig_len + extra + 1);
    if (!result) return NULL;

    char *out = result;
    for (const char *p = str; *p; p++) {
        if (*p == '\n' && (p == str || *(p-1) != '\r'))
            *out++ = '\r';
        *out++ = *p;
    }
    *out = '\0';
    return result;
}

// ==================== 文件读取 ====================
static char* read_file(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) return NULL;

    char *buffer = (char*)malloc(MAX_SOURCE_SIZE);
    if (!buffer) { fclose(fp); return NULL; }

    size_t len = fread(buffer, 1, MAX_SOURCE_SIZE - 1, fp);
    buffer[len] = '\0';
    fclose(fp);
    return buffer;
}

// ==================== 输出捕获 ====================
typedef void (*AnalysisFunc)(const char*);

static char* capture_and_run(AnalysisFunc func, const char *source) {
    WCHAR temp_path[MAX_PATH];
    WCHAR output_file[MAX_PATH];

    if (!GetTempPathW(MAX_PATH, temp_path))
        return NULL;
    if (!GetTempFileNameW(temp_path, L"pl0", 0, output_file))
        return NULL;

    char *outfile_n = w2n(output_file);
    if (!outfile_n) return NULL;

    int saved_fd = _dup(1);
    if (saved_fd == -1) { free(outfile_n); return NULL; }

    FILE *fp = freopen(outfile_n, "wb", stdout);
    if (!fp) {
        _close(saved_fd);
        free(outfile_n);
        return NULL;
    }

    func(source);

    fflush(stdout);
    _dup2(saved_fd, 1);
    _close(saved_fd);

    char *result = read_file(outfile_n);
    DeleteFileW(output_file);
    free(outfile_n);

    if (!result) {
        result = (char*)malloc(256);
        if (result) strcpy(result, "(分析完成，无输出)\r\n");
    }
    return result;
}

// ==================== 分析管线 ====================
static void run_lexer_only(const char *source) {
    TokenList tl;
    tokenize_all(source, &tl);

    printf("\r\n========== 词法分析结果 ==========\r\n");
    for (int i = 0; i < tl.count; i++)
        print_token_for_lexer(&tl.tokens[i]);
    print_token_stats();

    if (has_lex_error)
        printf("\r\n词法分析发现错误。\r\n");
}

static void run_lexer_and_parser(const char *source) {
    TokenList tl;
    tokenize_all(source, &tl);

    printf("\r\n========== 词法分析结果 ==========\r\n");
    for (int i = 0; i < tl.count; i++)
        print_token_for_lexer(&tl.tokens[i]);
    print_token_stats();

    if (has_lex_error) {
        printf("\r\n词法分析发现错误，停止后续分析。\r\n");
        return;
    }

    printf("\r\n========== LR 语法分析 ==========\r\n");
    ParseResult pr = lr_parse(&tl);
    if (!pr.success)
        printf("语法分析发现错误。\r\n");
}

static void run_full_compiler(const char *source) {
    TokenList tl;
    tokenize_all(source, &tl);

    printf("\r\n========== 词法分析结果 ==========\r\n");
    for (int i = 0; i < tl.count; i++)
        print_token_for_lexer(&tl.tokens[i]);
    print_token_stats();

    if (has_lex_error) {
        printf("\r\n词法分析发现错误，停止后续分析。\r\n");
        return;
    }

    printf("\r\n========== LR 语法分析 ==========\r\n");
    ParseResult pr = lr_parse(&tl);
    if (!pr.success) {
        printf("语法分析发现错误。\r\n");
        return;
    }

    printf("\r\n========== 语义分析 ==========\r\n");
    semantic_analyze(&tl);
}

// ==================== 测试用例发现 ====================
static void populate_test_list(void) {
    const char *exp_dirs[] = {
        "tests/experiment4_lexer",
        "tests/experiment5_parser",
        "tests/experiment6_semantic"
    };
    const WCHAR *exp_labels[] = {
        L"── 实验四：词法分析 ──",
        L"── 实验五：语法分析 ──",
        L"── 实验六：语义分析 ──"
    };

    for (int e = 0; e < 3; e++) {
        SendMessageW(g_hTestList, LB_ADDSTRING, 0, (LPARAM)exp_labels[e]);

        char pattern[MAX_PATH];
        snprintf(pattern, sizeof(pattern), "%s/*.pl0", exp_dirs[e]);

        WIN32_FIND_DATAA fd;
        HANDLE hFind = FindFirstFileA(pattern, &fd);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                char fullpath[512];
                snprintf(fullpath, sizeof(fullpath), "%s/%s", exp_dirs[e], fd.cFileName);

                char *path_copy = _strdup(fullpath);

                WCHAR wname[MAX_PATH];
                MultiByteToWideChar(CP_UTF8, 0, fd.cFileName, -1, wname, MAX_PATH);

                int idx = (int)SendMessageW(g_hTestList, LB_ADDSTRING, 0, (LPARAM)wname);
                SendMessageW(g_hTestList, LB_SETITEMDATA, idx, (LPARAM)path_copy);
            } while (FindNextFileA(hFind, &fd));
            FindClose(hFind);
        }
    }
}

// ==================== 设置 Rich Edit 字体 ====================
static void set_richedit_font(HWND hRichEdit) {
    CHARFORMAT2W cf = {0};
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_FACE | CFM_SIZE | CFM_BOLD;
    cf.yHeight = 200;  // 10pt (200 / 20 = 10)
    cf.dwEffects = 0;
    wcscpy(cf.szFaceName, L"Consolas");
    SendMessageW(hRichEdit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
}

// ==================== 执行分析 ====================
static void do_analysis(void) {
    int src_len = GetWindowTextLengthW(g_hSourceEdit);
    if (src_len == 0) {
        MessageBoxW(g_hMainWnd, L"请先输入源代码或选择测试用例。",
                    L"提示", MB_OK | MB_ICONINFORMATION);
        return;
    }

    WCHAR *wsrc = (WCHAR*)malloc((src_len + 1) * sizeof(WCHAR));
    if (!wsrc) return;
    GetWindowTextW(g_hSourceEdit, wsrc, src_len + 1);

    char *source = w2n(wsrc);
    free(wsrc);
    if (!source) return;

    AnalysisFunc func;
    switch (g_analysis_mode) {
        case 0: func = run_lexer_only;      break;
        case 1: func = run_lexer_and_parser; break;
        default: func = run_full_compiler;   break;
    }

    SetWindowTextW(g_hOutputEdit, L"正在分析...\r\n");
    EnableWindow(g_hRunButton, FALSE);

    char *output = capture_and_run(func, source);
    free(source);

    if (output) {
        char *fixed = fix_newlines(output);
        WCHAR *woutput = n2w(fixed);
        free(fixed);
        
        // 清空并设置新文本
        SetWindowTextW(g_hOutputEdit, L"");
        SetWindowTextW(g_hOutputEdit, woutput ? woutput : L"(编码转换失败)\r\n");
        
        // Rich Edit 自动处理滚动，无需额外操作
        free(woutput);
        free(output);
    } else {
        SetWindowTextW(g_hOutputEdit, L"分析出错：无法捕获输出。\r\n");
    }

    EnableWindow(g_hRunButton, TRUE);
}

// ==================== 加载测试用例 ====================
static void load_test_case(int idx) {
    if (idx == LB_ERR) return;

    char *path = (char*)SendMessageW(g_hTestList, LB_GETITEMDATA, idx, 0);
    if (!path || (intptr_t)path == LB_ERR) return;

    char *source = read_file(path);
    if (!source) {
        MessageBoxW(g_hMainWnd, L"无法读取测试文件。", L"错误", MB_OK | MB_ICONERROR);
        return;
    }

    char *fixed = fix_newlines(source);
    free(source);
    WCHAR *wsource = n2w(fixed);
    free(fixed);
    if (wsource) {
        SetWindowTextW(g_hSourceEdit, wsource);
        free(wsource);
    }

    if (strstr(path, "experiment4")) {
        g_analysis_mode = 0;
        SendMessageW(g_hRadioLexer,    BM_SETCHECK, BST_CHECKED, 0);
        SendMessageW(g_hRadioParser,   BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessageW(g_hRadioSemantic, BM_SETCHECK, BST_UNCHECKED, 0);
    } else if (strstr(path, "experiment5")) {
        g_analysis_mode = 1;
        SendMessageW(g_hRadioLexer,    BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessageW(g_hRadioParser,   BM_SETCHECK, BST_CHECKED, 0);
        SendMessageW(g_hRadioSemantic, BM_SETCHECK, BST_UNCHECKED, 0);
    } else {
        g_analysis_mode = 2;
        SendMessageW(g_hRadioLexer,    BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessageW(g_hRadioParser,   BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessageW(g_hRadioSemantic, BM_SETCHECK, BST_CHECKED, 0);
    }
}

// ==================== 控件重排 ====================
static void layout_controls(int client_w, int client_h) {
    int left_w    = (client_w - 4 * MARGIN - MIDDLE_W) * 55 / 100;
    int right_w   = (client_w - 4 * MARGIN - MIDDLE_W) * 45 / 100;
    int middle_x  = MARGIN + left_w + GAP;
    int right_x   = middle_x + MIDDLE_W + GAP;

    int src_edit_h = client_h - 2 * MARGIN - LABEL_H - GAP - TEST_LIST_H - MARGIN;
    if (src_edit_h < 100) src_edit_h = 100;

    // 左栏
    HWND h;
    h = GetDlgItem(g_hMainWnd, 2000);
    if (h) SetWindowPos(h, NULL, MARGIN, MARGIN, left_w, LABEL_H,
                        SWP_NOZORDER | SWP_NOACTIVATE);

    SetWindowPos(g_hSourceEdit, NULL,
                 MARGIN, MARGIN + LABEL_H, left_w, src_edit_h,
                 SWP_NOZORDER | SWP_NOACTIVATE);

    int test_label_y = MARGIN + LABEL_H + src_edit_h + GAP;
    h = GetDlgItem(g_hMainWnd, 2001);
    if (h) SetWindowPos(h, NULL, MARGIN, test_label_y,
                        left_w, LABEL_H, SWP_NOZORDER | SWP_NOACTIVATE);

    SetWindowPos(g_hTestList, NULL,
                 MARGIN, test_label_y + LABEL_H, left_w, TEST_LIST_H,
                 SWP_NOZORDER | SWP_NOACTIVATE);

    // 中间栏
    int mid_y = MARGIN + LABEL_H + 30;
    h = GetDlgItem(g_hMainWnd, 2002);
    if (h) SetWindowPos(h, NULL, middle_x, mid_y,
                        MIDDLE_W, 50, SWP_NOZORDER | SWP_NOACTIVATE);

    int ry = mid_y + 55;
    SetWindowPos(g_hRadioLexer,    NULL, middle_x - 5, ry,
                 MIDDLE_W + 24, 22, SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(g_hRadioParser,   NULL, middle_x - 5, ry + 28,
                 MIDDLE_W + 24, 22, SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(g_hRadioSemantic, NULL, middle_x - 5, ry + 56,
                 MIDDLE_W + 24, 22, SWP_NOZORDER | SWP_NOACTIVATE);

    int btn_y = ry + 100;
    SetWindowPos(g_hRunButton, NULL, middle_x - 5, btn_y,
                 MIDDLE_W + 24, 32, SWP_NOZORDER | SWP_NOACTIVATE);

    // 右栏
    h = GetDlgItem(g_hMainWnd, 2003);
    if (h) SetWindowPos(h, NULL, right_x, MARGIN,
                        right_w, LABEL_H, SWP_NOZORDER | SWP_NOACTIVATE);

    int out_h = client_h - 2 * MARGIN - LABEL_H;
    SetWindowPos(g_hOutputEdit, NULL,
                 right_x, MARGIN + LABEL_H, right_w, out_h,
                 SWP_NOZORDER | SWP_NOACTIVATE);
}

// ==================== 字体 ====================
static void create_fonts(void) {
    g_hCodeFont = CreateFontW(
        -15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");

    g_hArrowFont = CreateFontW(
        -36, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, FF_DONTCARE, L"Arial");

    g_hLabelFont = CreateFontW(
        -13, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, FF_DONTCARE, L"Microsoft YaHei UI");
}

// ==================== 窗口过程 ====================
static LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            g_hMainWnd = hWnd;
            create_fonts();

            HINSTANCE hInst = (HINSTANCE)GetWindowLongPtrW(hWnd, GWLP_HINSTANCE);

            // --- 左栏：源代码输入（普通 Edit）---
            CreateWindowW(L"STATIC", L"源代码输入",
                          WS_CHILD | WS_VISIBLE,
                          MARGIN, MARGIN, 300, LABEL_H,
                          hWnd, (HMENU)2000, hInst, NULL);

            g_hSourceEdit = CreateWindowExW(
                WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
                ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_WANTRETURN,
                MARGIN, MARGIN + LABEL_H, 300, 300,
                hWnd, (HMENU)IDC_SOURCE_EDIT, hInst, NULL);

            CreateWindowW(L"STATIC", L"测试用例（点击加载到输入框）",
                          WS_CHILD | WS_VISIBLE,
                          MARGIN, 360, 300, LABEL_H,
                          hWnd, (HMENU)2001, hInst, NULL);

            g_hTestList = CreateWindowExW(
                WS_EX_CLIENTEDGE, L"LISTBOX", L"",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY,
                MARGIN, 380, 300, TEST_LIST_H,
                hWnd, (HMENU)IDC_TEST_LIST, hInst, NULL);

            // --- 中间栏 ---
            HWND hArrow = CreateWindowW(L"STATIC", L"\x2192",
                          WS_CHILD | WS_VISIBLE | SS_CENTER,
                          400, 80, MIDDLE_W, 50,
                          hWnd, (HMENU)2002, hInst, NULL);

            g_hRadioLexer = CreateWindowW(L"BUTTON", L"词法分析",
                              WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
                              395, 140, MIDDLE_W + 24, 22,
                              hWnd, (HMENU)IDC_RADIO_LEXER, hInst, NULL);
            g_hRadioParser = CreateWindowW(L"BUTTON", L"语法分析",
                              WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                              395, 168, MIDDLE_W + 24, 22,
                              hWnd, (HMENU)IDC_RADIO_PARSER, hInst, NULL);
            g_hRadioSemantic = CreateWindowW(L"BUTTON", L"语义分析",
                                WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                                395, 196, MIDDLE_W + 24, 22,
                                hWnd, (HMENU)IDC_RADIO_SEMANTIC, hInst, NULL);

            SendMessageW(g_hRadioSemantic, BM_SETCHECK, BST_CHECKED, 0);

            g_hRunButton = CreateWindowExW(
                WS_EX_CLIENTEDGE, L"BUTTON", L"运行分析",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                395, 250, MIDDLE_W + 10, 32,
                hWnd, (HMENU)IDC_RUN_BUTTON, hInst, NULL);

            // --- 右栏：输出框（使用 Rich Edit）---
            CreateWindowW(L"STATIC", L"分析输出",
                          WS_CHILD | WS_VISIBLE,
                          500, MARGIN, 300, LABEL_H,
                          hWnd, (HMENU)2003, hInst, NULL);

            // 使用 Rich Edit 控件（MSFTEDIT_CLASS）
            g_hOutputEdit = CreateWindowExW(
                WS_EX_CLIENTEDGE,
                MSFTEDIT_CLASS,  // Microsoft Rich Edit 4.1
                L"",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
                ES_MULTILINE | ES_READONLY,
                500, MARGIN + LABEL_H, 300, 400,
                hWnd, (HMENU)IDC_OUTPUT_EDIT, hInst, NULL);

            // 设置 Rich Edit 字体
            set_richedit_font(g_hOutputEdit);

            // 设置字体
            SendMessageW(g_hSourceEdit, WM_SETFONT, (WPARAM)g_hCodeFont, TRUE);
            SendMessageW(g_hTestList,   WM_SETFONT, (WPARAM)g_hCodeFont, TRUE);
            SendMessageW(hArrow,        WM_SETFONT, (WPARAM)g_hArrowFont, TRUE);
            SendMessageW(g_hRunButton,  WM_SETFONT, (WPARAM)g_hLabelFont, TRUE);

            // 源代码编辑框 Tab 宽度
            DWORD tab = 16;
            SendMessageW(g_hSourceEdit, EM_SETTABSTOPS, 1, (LPARAM)&tab);

            populate_test_list();
            return 0;
        }

        case WM_SIZE: {
            layout_controls(LOWORD(lParam), HIWORD(lParam));
            return 0;
        }

        case WM_GETMINMAXINFO: {
            MINMAXINFO *mmi = (MINMAXINFO*)lParam;
            mmi->ptMinTrackSize.x = 820;
            mmi->ptMinTrackSize.y = 500;
            return 0;
        }

        case WM_COMMAND: {
            int id = LOWORD(wParam), code = HIWORD(wParam);
            switch (id) {
                case IDC_RUN_BUTTON:
                    if (code == BN_CLICKED) do_analysis();
                    break;
                case IDC_RADIO_LEXER:
                    if (code == BN_CLICKED) g_analysis_mode = 0;
                    break;
                case IDC_RADIO_PARSER:
                    if (code == BN_CLICKED) g_analysis_mode = 1;
                    break;
                case IDC_RADIO_SEMANTIC:
                    if (code == BN_CLICKED) g_analysis_mode = 2;
                    break;
                case IDC_TEST_LIST:
                    if (code == LBN_SELCHANGE) {
                        int idx = (int)SendMessageW(g_hTestList, LB_GETCURSEL, 0, 0);
                        if (idx != LB_ERR) load_test_case(idx);
                    }
                    break;
            }
            return 0;
        }

        case WM_CTLCOLORSTATIC: {
            HDC hdc = (HDC)wParam;
            int cid = GetDlgCtrlID((HWND)lParam);
            SetBkMode(hdc, TRANSPARENT);
            if (cid == 2002) {
                SetTextColor(hdc, RGB(0, 102, 204));
            } else {
                SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
            }
            return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
        }

        case WM_DESTROY: {
            int count = (int)SendMessageW(g_hTestList, LB_GETCOUNT, 0, 0);
            for (int i = 0; i < count; i++) {
                char *p = (char*)SendMessageW(g_hTestList, LB_GETITEMDATA, i, 0);
                if (p && (intptr_t)p != LB_ERR) free(p);
            }
            DeleteObject(g_hCodeFont);
            DeleteObject(g_hArrowFont);
            DeleteObject(g_hLabelFont);
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// ==================== 入口点 ====================
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine, int nCmdShow) {
    SetProcessDPIAware();

    // 加载 Rich Edit 控件库（必须！否则 Rich Edit 控件无法创建）
    HMODULE hRichEdit = LoadLibraryW(L"msftedit.dll");
    if (!hRichEdit) {
        MessageBoxW(NULL, L"无法加载 Rich Edit 控件库。\n程序将无法正常显示输出。",
                    L"错误", MB_OK | MB_ICONERROR);
        return 1;
    }

    WNDCLASSW wc = {0};
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"PL0CompilerGUI";
    wc.hIcon         = LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);

    if (!RegisterClassW(&wc)) return 1;

    HWND hWnd = CreateWindowW(
        L"PL0CompilerGUI",
        L"PL/0 编译器 - 图形化分析工具",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1100, 680,
        NULL, NULL, hInstance, NULL);

    if (!hWnd) return 1;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    FreeLibrary(hRichEdit);
    return (int)msg.wParam;
}