#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <windowsx.h>
#include <ole2.h>
#include <shellapi.h>
#include <wtsapi32.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include "resource.h"

#ifndef NIN_POPUPOPEN
#define NIN_POPUPOPEN (WM_USER + 6)
#endif

#ifndef NIN_POPUPCLOSE
#define NIN_POPUPCLOSE (WM_USER + 7)
#endif

#define APP_NAME L"TrayClockTooltip"
#define MUTEX_NAME L"TrayClockTooltip.SingleInstance"
#define ADJUST_ARGUMENT L"--set-local-time"

#define WM_TRAYICON (WM_APP + 1)
#define WM_NTP_DONE (WM_APP + 2)
#define WM_APP_INIT (WM_APP + 3)
#define TIMER_CLOCK 1
#define TIMER_NOTIFICATION 2
#define TIMER_TRAY_INIT 3
#define TIMER_APP_INIT 4
#define TIMER_HOVER_PREVIEW 5
#define ID_TRAY_EXIT 1001
#define ID_TRAY_ADJUST 1002
#define ID_TRAY_REFRESH_NTP 1003
#define ID_TRAY_STATUS 1004
#define ID_POPUP_CLOSE 2001
#define ID_POPUP_EXIT 2002
#define ID_POPUP_ADJUST 2003

#define NTP_PORT "123"
#define NTP_PACKET_LENGTH 48
#define NTP_TIMEOUT_MS 3000
#define NTP_TO_FILETIME_SECONDS 9435484800ULL
#define HNS_PER_SECOND 10000000LL
#define DRIFT_THRESHOLD_HNS HNS_PER_SECOND
#define NOTIFICATION_TIMEOUT_MS 5000
#define TRAY_HOVER_X_MARGIN 4
#define TRAY_HOVER_FALLBACK_HALF_WIDTH 24
#define POPUP_EDGE_MARGIN 2
#define POPUP_ICON_GAP 8
#define POPUP_CURSOR_GAP 12
#define HOVER_PREVIEW_POLL_MS 250
#define TIMESTAMP_BUFFER_CCH 128
#define TIMESTAMP_HORIZONTAL_PADDING 32
#define TIMESTAMP_VERTICAL_PADDING 14
#define OWNER_MENU_HORIZONTAL_PADDING 24
#define OWNER_MENU_VERTICAL_PADDING 8
#define OWNER_MENU_TEXT_GAP 4
#define PINNED_INDICATOR_PEN_WIDTH 3
#define PINNED_INDICATOR_X_MARGIN 4
#define PINNED_INDICATOR_DOT_OFFSET 2
#define PINNED_INDICATOR_MIN_HALF_HEIGHT 7

typedef struct AppTheme {
    COLORREF back;
    COLORREF text;
    COLORREF border;
} AppTheme;

typedef struct NtpResult {
    BOOL success;
    int64_t offsetHns;
    WCHAR source[256];
    WCHAR message[320];
} NtpResult;

static HINSTANCE g_instance;
static HWND g_mainWnd;
static HWND g_popupWnd;
static HWND g_notificationWnd;
static NOTIFYICONDATAW g_nid;
static HICON g_icon;
static HFONT g_messageFont;
static HFONT g_titleFont;
static HFONT g_menuFont;
static DWORD g_notifyIconSize;
static BOOL g_notifyIconAdded;
static int64_t g_clockOffsetHns;
static BOOL g_adjustmentAvailable;
static BOOL g_ntpQueryInProgress;
static BOOL g_notifyNtpSuccessIfNoDrift;
static BOOL g_ownsIcon;
static BOOL g_oleInitialized;
static BOOL g_sessionNotificationRegistered;
static WCHAR g_statusText[320];
static WCHAR g_notificationTitle[128];
static WCHAR g_notificationBody[320];
static BOOL g_notificationClickAdjusts;
static POINT g_dragOffset;
static POINT g_dragStartCursor;
static POINT g_lastTrayHoverPoint;
static BOOL g_dragging;
static UINT g_dragButtonMask;
static BOOL g_popupPinned;
static BOOL g_popupHoverPreview;
static BOOL g_popupUserMoved;
static BOOL g_popupHiddenForTrayMenu;
static BOOL g_contextMenuOpen;
static DWORD g_lastTrayMenuTick;
static UINT g_taskbarCreatedMessage;

static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK PopupWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK NotificationWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL IsMenuActive(void);

static int64_t FileTimeToHns(FILETIME ft)
{
    ULARGE_INTEGER value;
    value.LowPart = ft.dwLowDateTime;
    value.HighPart = ft.dwHighDateTime;
    return (int64_t)value.QuadPart;
}

static FILETIME HnsToFileTime(int64_t hns)
{
    ULARGE_INTEGER value;
    FILETIME ft;
    value.QuadPart = (ULONGLONG)hns;
    ft.dwLowDateTime = value.LowPart;
    ft.dwHighDateTime = value.HighPart;
    return ft;
}

static int64_t GetUtcNowHns(void)
{
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    return FileTimeToHns(ft);
}

static void GetAppLocalSystemTime(SYSTEMTIME *localTime)
{
    FILETIME utcFt = HnsToFileTime(GetUtcNowHns() + g_clockOffsetHns);
    FILETIME localFt;
    FileTimeToLocalFileTime(&utcFt, &localFt);
    FileTimeToSystemTime(&localFt, localTime);
}

static BOOL IsFormatQuote(const WCHAR *format, size_t index, BOOL *inQuote)
{
    if (format[index] != L'\'') {
        return FALSE;
    }
    if (format[index + 1] == L'\'') {
        return TRUE;
    }
    *inQuote = !*inQuote;
    return TRUE;
}

static BOOL FormatHasToken(const WCHAR *format, WCHAR token)
{
    size_t i;
    BOOL inQuote = FALSE;
    for (i = 0; format[i]; i++) {
        if (IsFormatQuote(format, i, &inQuote)) {
            if (format[i + 1] == L'\'') i++;
            continue;
        }
        if (!inQuote && (format[i] == token || format[i] == token - 32)) {
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL FindLastMinuteTokenEnd(const WCHAR *format, size_t *minuteEnd)
{
    size_t i;
    BOOL found = FALSE;
    BOOL inQuote = FALSE;
    for (i = 0; format[i]; i++) {
        if (IsFormatQuote(format, i, &inQuote)) {
            if (format[i + 1] == L'\'') i++;
            continue;
        }
        if (!inQuote && (format[i] == L'm' || format[i] == L'M')) {
            size_t j = i;
            while (format[j] == L'm' || format[j] == L'M') {
                j++;
            }
            *minuteEnd = j;
            found = TRUE;
            i = j - 1;
        }
    }
    return found;
}

static BOOL BuildUserTimeFormatWithSeconds(WCHAR *buffer, size_t cch)
{
    WCHAR separator[8];
    WCHAR addition[12];
    size_t length;
    size_t minuteEnd;
    size_t additionLength;

    if (!GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_STIMEFORMAT,
            buffer, (int)cch)) {
        return FALSE;
    }
    if (FormatHasToken(buffer, L's')) {
        return TRUE;
    }
    if (!GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_STIME,
            separator, ARRAYSIZE(separator))) {
        wcscpy_s(separator, ARRAYSIZE(separator), L":");
    }

    if (!FindLastMinuteTokenEnd(buffer, &minuteEnd)) {
        return FALSE;
    }

    length = wcslen(buffer);
    swprintf(addition, ARRAYSIZE(addition), L"%lsss", separator);
    additionLength = wcslen(addition);
    if (length + additionLength + 1 > cch) {
        return FALSE;
    }

    wmemmove(buffer + minuteEnd + additionLength,
        buffer + minuteEnd,
        length - minuteEnd + 1);
    wmemcpy(buffer + minuteEnd, addition, additionLength);
    return TRUE;
}

static void FormatTimestampFromSystemTime(const SYSTEMTIME *st, WCHAR *buffer, size_t cch)
{
    WCHAR datePart[80];
    WCHAR timePart[80];
    WCHAR timeFormat[80];
    if (!GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, DATE_SHORTDATE, st, NULL,
            datePart, ARRAYSIZE(datePart), NULL)) {
        swprintf(datePart, ARRAYSIZE(datePart), L"%04u/%02u/%02u",
            st->wYear, st->wMonth, st->wDay);
    }
    if (!BuildUserTimeFormatWithSeconds(timeFormat, ARRAYSIZE(timeFormat)) ||
        !GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, 0, st, timeFormat,
            timePart, ARRAYSIZE(timePart))) {
        swprintf(timePart, ARRAYSIZE(timePart), L"%02u:%02u:%02u",
            st->wHour, st->wMinute, st->wSecond);
    }
    swprintf(buffer, cch, L"%ls %ls", datePart, timePart);
}

static void FormatTimestamp(WCHAR *buffer, size_t cch)
{
    SYSTEMTIME st;
    GetAppLocalSystemTime(&st);
    FormatTimestampFromSystemTime(&st, buffer, cch);
}

static void ScheduleNextTick(void)
{
    int64_t appNow = GetUtcNowHns() + g_clockOffsetHns;
    int64_t next = ((appNow / HNS_PER_SECOND) + 1) * HNS_PER_SECOND;
    UINT delay = (UINT)((next - appNow) / 10000 + 15);
    if (delay < 50) {
        delay = 50;
    }
    SetTimer(g_mainWnd, TIMER_CLOCK, delay, NULL);
}

static BOOL IsWindowsDarkMode(void)
{
    HKEY key;
    DWORD value = 1;
    DWORD size = sizeof(value);
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        0, KEY_READ, &key) == ERROR_SUCCESS) {
        RegQueryValueExW(key, L"SystemUsesLightTheme", NULL, NULL, (BYTE *)&value, &size);
        RegCloseKey(key);
    }
    return value == 0;
}

static AppTheme GetCurrentTheme(void)
{
    AppTheme theme;
    if (IsWindowsDarkMode()) {
        theme.back = RGB(32, 32, 32);
        theme.text = RGB(255, 255, 255);
        theme.border = RGB(96, 96, 96);
    } else {
        theme.back = GetSysColor(COLOR_INFOBK);
        theme.text = GetSysColor(COLOR_INFOTEXT);
        theme.border = GetSysColor(COLOR_ACTIVEBORDER);
    }
    return theme;
}

static void InvalidatePopup(void)
{
    if (g_popupWnd) {
        InvalidateRect(g_popupWnd, NULL, FALSE);
    }
}

static void UpdateClockDisplays(void)
{
    InvalidatePopup();
}

static void SetTrayIconVersion(void)
{
    g_nid.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIconW(NIM_SETVERSION, &g_nid);
}

static void ClearTrayTooltip(void)
{
    NOTIFYICONDATAW nid;
    if (!g_notifyIconAdded) {
        return;
    }

    ZeroMemory(&nid, sizeof(nid));
    nid.cbSize = g_notifyIconSize;
    nid.hWnd = g_mainWnd;
    nid.uID = 1;
    nid.uFlags = NIF_TIP;
    nid.szTip[0] = L'\0';
    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

static BOOL TryAddTrayIcon(void)
{
    DWORD sizes[] = {
        sizeof(g_nid),
        NOTIFYICONDATAW_V3_SIZE,
        NOTIFYICONDATAW_V2_SIZE
    };
    int i;

    if (g_notifyIconAdded) {
        g_nid.cbSize = g_notifyIconSize;
        Shell_NotifyIconW(NIM_DELETE, &g_nid);
        g_notifyIconAdded = FALSE;
    }

    ZeroMemory(&g_nid, sizeof(g_nid));
    g_nid.hWnd = g_mainWnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = g_icon;

    for (i = 0; i < (int)ARRAYSIZE(sizes); i++) {
        g_nid.cbSize = sizes[i];
        if (Shell_NotifyIconW(NIM_ADD, &g_nid)) {
            g_notifyIconSize = g_nid.cbSize;
            g_notifyIconAdded = TRUE;
            SetTrayIconVersion();
            return TRUE;
        }
    }
    return FALSE;
}

static void EnsureTrayIcon(void)
{
    if (TryAddTrayIcon()) {
        return;
    }

    SetTimer(g_mainWnd, TIMER_TRAY_INIT, 2000, NULL);
}

static void RemoveTrayIcon(void)
{
    if (g_notifyIconAdded) {
        g_nid.cbSize = g_notifyIconSize;
        Shell_NotifyIconW(NIM_DELETE, &g_nid);
        g_notifyIconAdded = FALSE;
    }
}

static void HideNotification(void)
{
    if (g_notificationWnd) {
        DestroyWindow(g_notificationWnd);
        g_notificationWnd = NULL;
    }
}

static void SetTrayNtpMenuText(const WCHAR *text)
{
    if (text && text[0]) {
        wcscpy_s(g_statusText, ARRAYSIZE(g_statusText), text);
    } else {
        wcscpy_s(g_statusText, ARRAYSIZE(g_statusText), L"NTP: refresh");
    }
}

static void ShowNotification(const WCHAR *title, const WCHAR *body, BOOL clickAdjusts)
{
    HideNotification();
    wcscpy_s(g_notificationTitle, ARRAYSIZE(g_notificationTitle), title);
    wcscpy_s(g_notificationBody, ARRAYSIZE(g_notificationBody), body);
    g_notificationClickAdjusts = clickAdjusts;

    g_notificationWnd = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        L"TrayClockTooltipNotification",
        APP_NAME,
        WS_POPUP,
        CW_USEDEFAULT, CW_USEDEFAULT, 300, 84,
        NULL, NULL, g_instance, NULL);
    if (g_notificationWnd) {
        RECT area;
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &area, 0);
        SetWindowPos(g_notificationWnd, HWND_TOPMOST,
            area.right - 312, area.bottom - 96, 300, 84,
            SWP_SHOWWINDOW);
        SetTimer(g_notificationWnd, TIMER_NOTIFICATION, NOTIFICATION_TIMEOUT_MS, NULL);
    }
}

static void RunElevatedAdjustment(void)
{
    WCHAR exePath[MAX_PATH];
    SHELLEXECUTEINFOW sei;
    DWORD exitCode = 1;
    GetModuleFileNameW(NULL, exePath, ARRAYSIZE(exePath));
    ZeroMemory(&sei, sizeof(sei));
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = L"runas";
    sei.lpFile = exePath;
    sei.lpParameters = ADJUST_ARGUMENT;
    sei.nShow = SW_HIDE;

    if (ShellExecuteExW(&sei)) {
        if (sei.hProcess) {
            WaitForSingleObject(sei.hProcess, INFINITE);
            GetExitCodeProcess(sei.hProcess, &exitCode);
            CloseHandle(sei.hProcess);
        }
        if (exitCode == 0) {
            g_clockOffsetHns = 0;
            g_adjustmentAvailable = FALSE;
            SetTrayNtpMenuText(NULL);
            HideNotification();
            UpdateClockDisplays();
            ShowNotification(APP_NAME, L"Windows time was adjusted.", FALSE);
        } else {
            ShowNotification(APP_NAME, L"Could not adjust Windows time.", FALSE);
        }
    }
}

static void TrimNtpSource(WCHAR *source)
{
    WCHAR *comma;
    WCHAR *space;
    while (*source == L' ' || *source == L'\t') {
        MoveMemory(source, source + 1, (wcslen(source) + 1) * sizeof(WCHAR));
    }
    comma = wcschr(source, L',');
    if (comma) {
        *comma = L'\0';
    }
    space = wcspbrk(source, L" \t");
    if (space) {
        *space = L'\0';
    }
}

static BOOL ReadConfiguredSource(WCHAR *source, DWORD cch)
{
    HKEY key;
    DWORD type = 0;
    DWORD bytes = cch * sizeof(WCHAR);
    source[0] = L'\0';
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
        L"SYSTEM\\CurrentControlSet\\Services\\W32Time\\Parameters",
        0, KEY_READ, &key) != ERROR_SUCCESS) {
        return FALSE;
    }
    if (RegQueryValueExW(key, L"NtpServer", NULL, &type, (BYTE *)source, &bytes) != ERROR_SUCCESS) {
        RegCloseKey(key);
        return FALSE;
    }
    RegCloseKey(key);
    source[cch - 1] = L'\0';
    TrimNtpSource(source);
    return source[0] && wcsstr(source, L"Local CMOS") == NULL;
}

static uint32_t ReadBigEndian32(const BYTE *data)
{
    return ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) | ((uint32_t)data[2] << 8) | data[3];
}

static int64_t ReadNtpTransmitFileTime(const BYTE *data)
{
    uint64_t seconds = ReadBigEndian32(data + 40);
    uint64_t fraction = ReadBigEndian32(data + 44);
    uint64_t hns = (seconds + NTP_TO_FILETIME_SECONDS) * (uint64_t)HNS_PER_SECOND;
    hns += (fraction * (uint64_t)HNS_PER_SECOND) >> 32;
    return (int64_t)hns;
}

static BOOL QueryNtpOffset(const WCHAR *source, int64_t *offsetHns)
{
    char host[256];
    struct addrinfo hints;
    struct addrinfo *addresses = NULL;
    struct addrinfo *addr;
    BOOL ok = FALSE;

    if (!WideCharToMultiByte(CP_UTF8, 0, source, -1, host, sizeof(host), NULL, NULL)) {
        return FALSE;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    if (getaddrinfo(host, NTP_PORT, &hints, &addresses) != 0) {
        return FALSE;
    }

    for (addr = addresses; addr; addr = addr->ai_next) {
        SOCKET s = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        DWORD timeout = NTP_TIMEOUT_MS;
        BYTE request[NTP_PACKET_LENGTH];
        BYTE response[NTP_PACKET_LENGTH];
        int received;
        int64_t before;
        int64_t after;
        int64_t midpoint;
        if (s == INVALID_SOCKET) {
            continue;
        }
        setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));
        setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout, sizeof(timeout));
        ZeroMemory(request, sizeof(request));
        request[0] = (4 << 3) | 3;
        before = GetUtcNowHns();
        if (sendto(s, (const char *)request, sizeof(request), 0, addr->ai_addr, (int)addr->ai_addrlen) == sizeof(request)) {
            received = recvfrom(s, (char *)response, sizeof(response), 0, NULL, NULL);
            after = GetUtcNowHns();
            if (received >= NTP_PACKET_LENGTH) {
                midpoint = before + ((after - before) / 2);
                *offsetHns = ReadNtpTransmitFileTime(response) - midpoint;
                ok = TRUE;
            }
        }
        closesocket(s);
        if (ok) {
            break;
        }
    }

    freeaddrinfo(addresses);
    return ok;
}

static NtpResult GetWindowsTimeOffset(void)
{
    NtpResult result;
    ZeroMemory(&result, sizeof(result));
    if (!ReadConfiguredSource(result.source, ARRAYSIZE(result.source))) {
        return result;
    }
    if (QueryNtpOffset(result.source, &result.offsetHns)) {
        double seconds = (double)result.offsetHns / (double)HNS_PER_SECOND;
        result.success = TRUE;
        swprintf(result.message, ARRAYSIZE(result.message), L"Time: %+.3fs (%ls)", seconds, result.source);
    }
    return result;
}

static DWORD WINAPI NtpThreadProc(LPVOID param)
{
    NtpResult *result = (NtpResult *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(NtpResult));
    (void)param;
    if (result) {
        *result = GetWindowsTimeOffset();
    }
    if (!PostMessageW(g_mainWnd, WM_NTP_DONE, 0, (LPARAM)result) && result) {
        HeapFree(GetProcessHeap(), 0, result);
    }
    return 0;
}

static BOOL SetLocalTimeFromNtp(void)
{
    NtpResult result = GetWindowsTimeOffset();
    FILETIME utcFt;
    FILETIME localFt;
    SYSTEMTIME localSt;
    if (!result.success) {
        return FALSE;
    }
    utcFt = HnsToFileTime(GetUtcNowHns() + result.offsetHns);
    FileTimeToLocalFileTime(&utcFt, &localFt);
    FileTimeToSystemTime(&localFt, &localSt);
    return SetLocalTime(&localSt);
}

static void ApplyNtpResult(const NtpResult *result)
{
    BOOL notifySuccessIfNoDrift = g_notifyNtpSuccessIfNoDrift;
    g_notifyNtpSuccessIfNoDrift = FALSE;
    g_ntpQueryInProgress = FALSE;

    if (!result || !result->success) {
        g_clockOffsetHns = 0;
        SetTrayNtpMenuText(NULL);
        g_adjustmentAvailable = FALSE;
        UpdateClockDisplays();
        ShowNotification(APP_NAME, L"Could not obtain NTP time. The app clock is using the Windows system clock.", FALSE);
        return;
    }

    g_clockOffsetHns = result->offsetHns;
    UpdateClockDisplays();

    if (llabs(result->offsetHns) >= DRIFT_THRESHOLD_HNS) {
        WCHAR body[320];
        double seconds = (double)llabs(result->offsetHns) / (double)HNS_PER_SECOND;
        SetTrayNtpMenuText(result->message);
        g_adjustmentAvailable = TRUE;
        swprintf(body, ARRAYSIZE(body),
            L"Windows time differs by %.3f seconds. The app clock is using NTP time.",
            seconds);
        ShowNotification(APP_NAME, body, TRUE);
    } else {
        SetTrayNtpMenuText(NULL);
        g_adjustmentAvailable = FALSE;
        if (notifySuccessIfNoDrift) {
            ShowNotification(APP_NAME, L"NTP time was refreshed. Windows time is within 1 second.", FALSE);
        }
    }
}

static void StartNtpLoad(BOOL notifySuccessIfNoDrift)
{
    HANDLE thread;
    if (g_ntpQueryInProgress) {
        return;
    }

    g_ntpQueryInProgress = TRUE;
    g_notifyNtpSuccessIfNoDrift = notifySuccessIfNoDrift;
    SetTrayNtpMenuText(NULL);
    thread = CreateThread(NULL, 0, NtpThreadProc, NULL, 0, NULL);
    if (thread) {
        CloseHandle(thread);
    } else {
        g_ntpQueryInProgress = FALSE;
        g_notifyNtpSuccessIfNoDrift = FALSE;
        SetTrayNtpMenuText(NULL);
    }
}

static void AppendTrayRefreshMenuItem(HMENU menu)
{
    UINT flags = g_ntpQueryInProgress ? MF_STRING | MF_DISABLED : MF_STRING;
    AppendMenuW(menu, flags, ID_TRAY_REFRESH_NTP, g_statusText);
    AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
}

static void MeasureTextLine(HDC dc, const WCHAR *text, SIZE *size)
{
    RECT rc = { 0, 0, 0, 0 };
    DrawTextW(dc, text, -1, &rc, DT_CALCRECT | DT_SINGLELINE);
    size->cx = rc.right - rc.left;
    size->cy = rc.bottom - rc.top;
}

static BOOL MeasureTrayStatusMenuItem(MEASUREITEMSTRUCT *measure)
{
    HDC dc;
    HFONT oldFont;
    SIZE statusSize;
    int textOffset;

    if (measure->CtlType != ODT_MENU || measure->itemID != ID_TRAY_STATUS) {
        return FALSE;
    }

    dc = GetDC(g_mainWnd);
    oldFont = (HFONT)SelectObject(dc, g_menuFont ? g_menuFont : GetStockObject(DEFAULT_GUI_FONT));
    MeasureTextLine(dc, g_statusText, &statusSize);
    SelectObject(dc, oldFont);
    ReleaseDC(g_mainWnd, dc);

    textOffset = GetSystemMetrics(SM_CXMENUCHECK) + OWNER_MENU_TEXT_GAP;
    measure->itemWidth = statusSize.cx + textOffset + (OWNER_MENU_HORIZONTAL_PADDING / 2);
    measure->itemHeight = statusSize.cy + OWNER_MENU_VERTICAL_PADDING;
    return TRUE;
}

static BOOL DrawTrayStatusMenuItem(const DRAWITEMSTRUCT *draw)
{
    RECT rc;
    RECT statusRc;
    HFONT oldFont;
    COLORREF oldText;
    int oldBkMode;
    int textOffset;

    if (draw->CtlType != ODT_MENU || draw->itemID != ID_TRAY_STATUS) {
        return FALSE;
    }

    rc = draw->rcItem;
    FillRect(draw->hDC, &rc, GetSysColorBrush(COLOR_MENU));

    oldFont = (HFONT)SelectObject(draw->hDC, g_menuFont ? g_menuFont : GetStockObject(DEFAULT_GUI_FONT));
    oldBkMode = SetBkMode(draw->hDC, TRANSPARENT);
    oldText = SetTextColor(draw->hDC, GetSysColor(COLOR_MENUTEXT));

    statusRc = rc;
    textOffset = GetSystemMetrics(SM_CXMENUCHECK) + OWNER_MENU_TEXT_GAP;
    statusRc.left += textOffset;
    statusRc.right -= OWNER_MENU_HORIZONTAL_PADDING / 2;
    statusRc.top += OWNER_MENU_VERTICAL_PADDING / 2;
    statusRc.bottom = rc.bottom - (OWNER_MENU_VERTICAL_PADDING / 2);

    DrawTextW(draw->hDC, g_statusText, -1, &statusRc, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS);

    SetTextColor(draw->hDC, oldText);
    SetBkMode(draw->hDC, oldBkMode);
    SelectObject(draw->hDC, oldFont);
    return TRUE;
}

static HMENU CreateTrayMenu(void)
{
    HMENU menu = CreatePopupMenu();
    if (g_adjustmentAvailable) {
        AppendMenuW(menu, MF_OWNERDRAW | MF_DISABLED, ID_TRAY_STATUS, NULL);
        AppendMenuW(menu, MF_STRING, ID_TRAY_ADJUST, L"Adjust Windows time (admin)");
        AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    } else {
        AppendTrayRefreshMenuItem(menu);
    }
    AppendMenuW(menu, MF_STRING, ID_TRAY_EXIT, L"Exit");
    return menu;
}

static HMENU CreatePopupMenuForClock(void)
{
    HMENU menu = CreatePopupMenu();
    if (g_adjustmentAvailable) {
        AppendMenuW(menu, MF_STRING, ID_POPUP_ADJUST, L"Adjust Windows time (admin)");
        AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    }
    AppendMenuW(menu, MF_STRING, ID_POPUP_CLOSE, L"Close");
    AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(menu, MF_STRING, ID_POPUP_EXIT, L"Exit");
    return menu;
}

static void IncludeTextExtent(HDC dc, const WCHAR *text, int *maxWidth, int *maxHeight)
{
    RECT rc = { 0, 0, 0, 0 };
    DrawTextW(dc, text, -1, &rc, DT_CALCRECT | DT_SINGLELINE);
    if (rc.right - rc.left > *maxWidth) *maxWidth = rc.right - rc.left;
    if (rc.bottom - rc.top > *maxHeight) *maxHeight = rc.bottom - rc.top;
}

static SIZE MeasureTimestamp(HWND hwnd)
{
    WCHAR timestamp[TIMESTAMP_BUFFER_CCH];
    HDC dc = GetDC(hwnd);
    HFONT oldFont;
    SIZE size;
    int maxWidth = 0;
    int maxHeight = 0;
    SYSTEMTIME sampleAm = { 0 };
    SYSTEMTIME samplePm = { 0 };

    FormatTimestamp(timestamp, ARRAYSIZE(timestamp));
    oldFont = (HFONT)SelectObject(dc, g_messageFont);
    IncludeTextExtent(dc, timestamp, &maxWidth, &maxHeight);

    sampleAm.wYear = 8888;
    sampleAm.wMonth = 12;
    sampleAm.wDay = 28;
    sampleAm.wHour = 11;
    sampleAm.wMinute = 59;
    sampleAm.wSecond = 59;
    FormatTimestampFromSystemTime(&sampleAm, timestamp, ARRAYSIZE(timestamp));
    IncludeTextExtent(dc, timestamp, &maxWidth, &maxHeight);

    samplePm = sampleAm;
    samplePm.wHour = 23;
    FormatTimestampFromSystemTime(&samplePm, timestamp, ARRAYSIZE(timestamp));
    IncludeTextExtent(dc, timestamp, &maxWidth, &maxHeight);

    SelectObject(dc, oldFont);
    ReleaseDC(hwnd, dc);
    size.cx = maxWidth + TIMESTAMP_HORIZONTAL_PADDING;
    size.cy = maxHeight + TIMESTAMP_VERTICAL_PADDING;
    return size;
}

static POINT KeepOnScreen(POINT pt, SIZE size)
{
    HMONITOR monitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi;
    mi.cbSize = sizeof(mi);
    GetMonitorInfoW(monitor, &mi);
    if (pt.x < mi.rcWork.left) pt.x = mi.rcWork.left;
    if (pt.y < mi.rcWork.top) pt.y = mi.rcWork.top;
    if (pt.x + size.cx > mi.rcWork.right) pt.x = mi.rcWork.right - size.cx;
    if (pt.y + size.cy > mi.rcWork.bottom) pt.y = mi.rcWork.bottom - size.cy;
    return pt;
}

static BOOL TryGetTrayIconRect(RECT *iconRect)
{
    NOTIFYICONIDENTIFIER identifier;
    ZeroMemory(&identifier, sizeof(identifier));
    identifier.cbSize = sizeof(identifier);
    identifier.hWnd = g_mainWnd;
    identifier.uID = 1;
    return SUCCEEDED(Shell_NotifyIconGetRect(&identifier, iconRect));
}

static POINT GetPopupInitialLocation(SIZE size)
{
    RECT iconRect;
    POINT location;

    if (TryGetTrayIconRect(&iconRect)) {
        location.x = iconRect.left + ((iconRect.right - iconRect.left) / 2) - (size.cx / 2);
        location.y = iconRect.top - size.cy - POPUP_ICON_GAP;
    } else {
        GetCursorPos(&location);
        location.x -= size.cx / 2;
        location.y -= size.cy + POPUP_CURSOR_GAP;
    }

    return KeepOnScreen(location, size);
}

static void HidePopup(void)
{
    if (g_popupWnd) {
        ShowWindow(g_popupWnd, SW_HIDE);
    }
    g_popupPinned = FALSE;
    g_popupHoverPreview = FALSE;
    g_popupUserMoved = FALSE;
    g_dragging = FALSE;
    g_dragButtonMask = 0;
    KillTimer(g_mainWnd, TIMER_HOVER_PREVIEW);
}

static void HidePopupForTrayMenu(void)
{
    g_popupHiddenForTrayMenu = FALSE;
    if (!g_popupWnd || !IsWindowVisible(g_popupWnd) || g_popupUserMoved) {
        return;
    }

    if (g_popupPinned) {
        ShowWindow(g_popupWnd, SW_HIDE);
        g_popupHiddenForTrayMenu = TRUE;
    } else {
        HidePopup();
    }
}

static void RestorePopupHiddenForTrayMenu(void)
{
    if (g_popupHiddenForTrayMenu && g_popupWnd && g_popupPinned) {
        ShowWindow(g_popupWnd, SW_SHOWNOACTIVATE);
    }
    g_popupHiddenForTrayMenu = FALSE;
}

static void ShowPopupNearCursor(BOOL pinned)
{
    POINT location;
    SIZE size;
    BOOL alreadyVisible = g_popupWnd && IsWindowVisible(g_popupWnd);

    if (IsMenuActive() && !pinned) {
        return;
    }

    if (alreadyVisible && g_popupHoverPreview && !pinned) {
        SetTimer(g_mainWnd, TIMER_HOVER_PREVIEW, HOVER_PREVIEW_POLL_MS, NULL);
        return;
    }

    if (!g_popupWnd) {
        g_popupWnd = CreateWindowExW(
            WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
            L"TrayClockTooltipPopup",
            APP_NAME,
            WS_POPUP,
            CW_USEDEFAULT, CW_USEDEFAULT, 160, 32,
            NULL, NULL, g_instance, NULL);
    }
    size = MeasureTimestamp(g_popupWnd);
    location = GetPopupInitialLocation(size);
    g_popupPinned = pinned;
    g_popupHoverPreview = !pinned;
    g_popupUserMoved = FALSE;
    SetWindowPos(g_popupWnd, HWND_TOPMOST, location.x, location.y, size.cx, size.cy, SWP_SHOWWINDOW | SWP_NOACTIVATE);
    if (g_popupHoverPreview) {
        SetTimer(g_mainWnd, TIMER_HOVER_PREVIEW, HOVER_PREVIEW_POLL_MS, NULL);
    } else {
        KillTimer(g_mainWnd, TIMER_HOVER_PREVIEW);
    }
}

static void PinHoverPopup(void)
{
    if (g_popupWnd && IsWindowVisible(g_popupWnd) && g_popupHoverPreview) {
        g_popupPinned = TRUE;
        g_popupHoverPreview = FALSE;
        KillTimer(g_mainWnd, TIMER_HOVER_PREVIEW);
        InvalidatePopup();
    }
}

static void BeginPopupDrag(HWND hwnd, LPARAM lParam, UINT buttonMask)
{
    PinHoverPopup();
    g_dragging = TRUE;
    g_dragButtonMask = buttonMask;
    SetCapture(hwnd);
    g_dragOffset.x = GET_X_LPARAM(lParam);
    g_dragOffset.y = GET_Y_LPARAM(lParam);
    GetCursorPos(&g_dragStartCursor);
}

static BOOL EndPopupDrag(UINT buttonMask)
{
    if (!g_dragging || g_dragButtonMask != buttonMask) {
        return FALSE;
    }

    g_dragging = FALSE;
    g_dragButtonMask = 0;
    ReleaseCapture();
    return TRUE;
}

static void RestoreHoverPreviewFromPinnedPopup(void)
{
    g_popupPinned = FALSE;
    g_popupHoverPreview = TRUE;
    SetTimer(g_mainWnd, TIMER_HOVER_PREVIEW, HOVER_PREVIEW_POLL_MS, NULL);
    InvalidatePopup();
}

static void TogglePinnedPopup(void)
{
    if (g_popupWnd && IsWindowVisible(g_popupWnd) && g_popupHoverPreview) {
        PinHoverPopup();
    } else if (g_popupWnd && IsWindowVisible(g_popupWnd) && g_popupPinned) {
        if (g_popupUserMoved) {
            HidePopup();
        } else {
            RestoreHoverPreviewFromPinnedPopup();
        }
    } else {
        ShowPopupNearCursor(TRUE);
    }
}

static BOOL IsPointInTrayIconXRange(POINT pt)
{
    RECT rc;
    if (TryGetTrayIconRect(&rc)) {
        return pt.x >= rc.left - TRAY_HOVER_X_MARGIN && pt.x <= rc.right + TRAY_HOVER_X_MARGIN;
    }

    return pt.x >= g_lastTrayHoverPoint.x - TRAY_HOVER_FALLBACK_HALF_WIDTH &&
        pt.x <= g_lastTrayHoverPoint.x + TRAY_HOVER_FALLBACK_HALF_WIDTH;
}

static BOOL GetInflatedPopupRect(RECT *rc)
{
    if (!g_popupWnd || !IsWindowVisible(g_popupWnd)) {
        return FALSE;
    }

    GetWindowRect(g_popupWnd, rc);
    InflateRect(rc, POPUP_EDGE_MARGIN, POPUP_EDGE_MARGIN);
    return TRUE;
}

static BOOL IsMenuActive(void)
{
    GUITHREADINFO info;
    ZeroMemory(&info, sizeof(info));
    info.cbSize = sizeof(info);
    if (GetGUIThreadInfo(0, &info)) {
        if (info.flags & (GUI_INMENUMODE | GUI_POPUPMENUMODE | GUI_SYSTEMMENUMODE)) {
            return TRUE;
        }
    }
    return g_contextMenuOpen;
}

static void ShowContextMenu(HMENU menu, HWND foregroundWnd, POINT pt)
{
    SetForegroundWindow(foregroundWnd);
    g_contextMenuOpen = TRUE;
    TrackPopupMenu(menu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, g_mainWnd, NULL);
    g_contextMenuOpen = FALSE;
}

static void HideHoverPreviewIfCursorAway(void)
{
    POINT cursor;
    RECT popupRect;
    if (!g_popupHoverPreview) {
        KillTimer(g_mainWnd, TIMER_HOVER_PREVIEW);
        return;
    }

    GetCursorPos(&cursor);
    if (!GetInflatedPopupRect(&popupRect)) {
        HidePopup();
        return;
    }

    if (PtInRect(&popupRect, cursor)) {
        return;
    }

    if (cursor.y >= popupRect.bottom && IsPointInTrayIconXRange(cursor)) {
        return;
    }

    HidePopup();
}

static void ShowTrayMenu(void)
{
    POINT pt;
    HMENU menu = CreateTrayMenu();
    g_lastTrayMenuTick = GetTickCount();
    GetCursorPos(&pt);
    ShowContextMenu(menu, g_mainWnd, pt);
    RestorePopupHiddenForTrayMenu();
    DestroyMenu(menu);
}

static void ShowClockMenu(HWND hwnd)
{
    POINT pt;
    HMENU menu = CreatePopupMenuForClock();
    GetCursorPos(&pt);
    ShowContextMenu(menu, hwnd, pt);
    DestroyMenu(menu);
}

static void DrawPinnedIndicator(HDC dc, const RECT *rc)
{
    COLORREF color = IsWindowsDarkMode() ? RGB(185, 235, 255) : RGB(0, 84, 166);
    HPEN pen = CreatePen(PS_SOLID, PINNED_INDICATOR_PEN_WIDTH, color);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    int centerY = (rc->top + rc->bottom) / 2;
    int halfHeight = max(PINNED_INDICATOR_MIN_HALF_HEIGHT, (rc->bottom - rc->top) / 5);
    int leftX = rc->left + PINNED_INDICATOR_X_MARGIN;
    int rightX = rc->right - PINNED_INDICATOR_X_MARGIN - 1;
    int top = centerY - halfHeight;
    int bottom = centerY + halfHeight;

    MoveToEx(dc, leftX, top, NULL);
    LineTo(dc, leftX, bottom);
    SetPixel(dc, leftX, top - PINNED_INDICATOR_DOT_OFFSET, color);
    SetPixel(dc, leftX, bottom + PINNED_INDICATOR_DOT_OFFSET, color);

    MoveToEx(dc, rightX, top, NULL);
    LineTo(dc, rightX, bottom);
    SetPixel(dc, rightX, top - PINNED_INDICATOR_DOT_OFFSET, color);
    SetPixel(dc, rightX, bottom + PINNED_INDICATOR_DOT_OFFSET, color);

    SelectObject(dc, oldPen);
    DeleteObject(pen);
}

static void DrawBorderedTextContent(HDC dc, const RECT *windowRect, const WCHAR *title, const WCHAR *body)
{
    HBRUSH brush;
    HPEN pen;
    HGDIOBJ oldPen;
    HGDIOBJ oldBrush;
    AppTheme theme = GetCurrentTheme();

    brush = CreateSolidBrush(theme.back);
    FillRect(dc, windowRect, brush);
    DeleteObject(brush);

    pen = CreatePen(PS_SOLID, 1, theme.border);
    oldPen = SelectObject(dc, pen);
    oldBrush = SelectObject(dc, GetStockObject(NULL_BRUSH));
    Rectangle(dc, windowRect->left, windowRect->top, windowRect->right, windowRect->bottom);
    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(pen);

    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, theme.text);

    if (body) {
        RECT titleRc = { 12, 10, windowRect->right - 12, 30 };
        RECT bodyRc = { 12, 36, windowRect->right - 12, windowRect->bottom - 10 };
        HFONT oldFont = (HFONT)SelectObject(dc, g_titleFont);
        DrawTextW(dc, title, -1, &titleRc, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS);
        SelectObject(dc, g_messageFont);
        DrawTextW(dc, body, -1, &bodyRc, DT_LEFT | DT_WORDBREAK | DT_END_ELLIPSIS);
        SelectObject(dc, oldFont);
    } else {
        RECT textRc = *windowRect;
        WCHAR timestamp[TIMESTAMP_BUFFER_CCH];
        HFONT oldFont = (HFONT)SelectObject(dc, g_messageFont);
        FormatTimestamp(timestamp, ARRAYSIZE(timestamp));
        DrawTextW(dc, timestamp, -1, &textRc, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
        SelectObject(dc, oldFont);
        if (g_popupPinned && !g_popupHoverPreview) {
            DrawPinnedIndicator(dc, windowRect);
        }
    }
}

static void PaintBorderedTextWindow(HWND hwnd, const WCHAR *title, const WCHAR *body)
{
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(hwnd, &ps);
    RECT rc;
    HDC bufferDc;
    HBITMAP bufferBitmap;
    HGDIOBJ oldBitmap;
    int width;
    int height;

    GetClientRect(hwnd, &rc);
    width = rc.right - rc.left;
    height = rc.bottom - rc.top;

    bufferDc = CreateCompatibleDC(dc);
    bufferBitmap = bufferDc ? CreateCompatibleBitmap(dc, width, height) : NULL;
    if (bufferDc && bufferBitmap) {
        oldBitmap = SelectObject(bufferDc, bufferBitmap);
        DrawBorderedTextContent(bufferDc, &rc, title, body);
        BitBlt(dc, 0, 0, width, height, bufferDc, 0, 0, SRCCOPY);
        SelectObject(bufferDc, oldBitmap);
        DeleteObject(bufferBitmap);
        DeleteDC(bufferDc);
    } else {
        DrawBorderedTextContent(dc, &rc, title, body);
        if (bufferBitmap) DeleteObject(bufferBitmap);
        if (bufferDc) DeleteDC(bufferDc);
    }

    EndPaint(hwnd, &ps);
}

static BOOL RegisterClasses(void)
{
    WNDCLASSW wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.hInstance = g_instance;
    wc.lpfnWndProc = MainWndProc;
    wc.lpszClassName = L"TrayClockTooltipMain";
    wc.hIcon = LoadIconW(g_instance, MAKEINTRESOURCEW(IDI_APP));
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    if (!RegisterClassW(&wc)) return FALSE;

    wc.lpfnWndProc = PopupWndProc;
    wc.lpszClassName = L"TrayClockTooltipPopup";
    if (!RegisterClassW(&wc)) return FALSE;

    wc.lpfnWndProc = NotificationWndProc;
    wc.lpszClassName = L"TrayClockTooltipNotification";
    if (!RegisterClassW(&wc)) return FALSE;

    return TRUE;
}

static BOOL InitFonts(void)
{
    NONCLIENTMETRICSW metrics;
    ZeroMemory(&metrics, sizeof(metrics));
    metrics.cbSize = sizeof(metrics);
    if (!SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0)) {
        return FALSE;
    }
    g_messageFont = CreateFontIndirectW(&metrics.lfMessageFont);
    metrics.lfMessageFont.lfWeight = FW_BOLD;
    g_titleFont = CreateFontIndirectW(&metrics.lfMessageFont);
    g_menuFont = CreateFontIndirectW(&metrics.lfMenuFont);
    return g_messageFont && g_titleFont && g_menuFont;
}

static void Cleanup(void)
{
    HideNotification();
    if (g_popupWnd) DestroyWindow(g_popupWnd);
    RemoveTrayIcon();
    if (g_sessionNotificationRegistered) {
        WTSUnRegisterSessionNotification(g_mainWnd);
    }
    if (g_icon && g_ownsIcon) DestroyIcon(g_icon);
    if (g_messageFont) DeleteObject(g_messageFont);
    if (g_titleFont) DeleteObject(g_titleFont);
    if (g_menuFont) DeleteObject(g_menuFont);
    WSACleanup();
    if (g_oleInitialized) {
        OleUninitialize();
    }
}

static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == g_taskbarCreatedMessage) {
        EnsureTrayIcon();
        return 0;
    }

    switch (msg) {
    case WM_CREATE:
        SetTimer(hwnd, TIMER_APP_INIT, 500, NULL);
        return 0;

    case WM_APP_INIT:
        if (!g_sessionNotificationRegistered &&
            WTSRegisterSessionNotification(hwnd, NOTIFY_FOR_THIS_SESSION)) {
            g_sessionNotificationRegistered = TRUE;
        }
        EnsureTrayIcon();
        ScheduleNextTick();
        StartNtpLoad(FALSE);
        return 0;

    case WM_TIMER:
        if (wParam == TIMER_APP_INIT) {
            KillTimer(hwnd, TIMER_APP_INIT);
            PostMessageW(hwnd, WM_APP_INIT, 0, 0);
        } else if (wParam == TIMER_TRAY_INIT) {
            KillTimer(hwnd, TIMER_TRAY_INIT);
            EnsureTrayIcon();
        } else if (wParam == TIMER_CLOCK) {
            KillTimer(hwnd, TIMER_CLOCK);
            UpdateClockDisplays();
            ScheduleNextTick();
        } else if (wParam == TIMER_HOVER_PREVIEW) {
            if (IsMenuActive()) {
                HidePopup();
                return 0;
            }
            HideHoverPreviewIfCursorAway();
        }
        return 0;

    case WM_TRAYICON:
    {
        UINT trayEvent = LOWORD(lParam);
        if (trayEvent == WM_MOUSEMOVE) {
            POINT cursor;
            GetCursorPos(&cursor);
            g_lastTrayHoverPoint = cursor;
            if (IsMenuActive()) {
                if (g_popupHoverPreview) {
                    HidePopup();
                }
                return 0;
            }
            if (!g_popupPinned) {
                ShowPopupNearCursor(FALSE);
            }
        } else if (trayEvent == NIN_POPUPOPEN || trayEvent == NIN_POPUPCLOSE) {
            ClearTrayTooltip();
        } else if (trayEvent == WM_LBUTTONUP) {
            HideNotification();
            TogglePinnedPopup();
        } else if (trayEvent == WM_RBUTTONUP || trayEvent == WM_CONTEXTMENU) {
            if (GetTickCount() - g_lastTrayMenuTick < 250) {
                return 0;
            }
            HideNotification();
            HidePopupForTrayMenu();
            ShowTrayMenu();
        }
        return 0;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_TRAY_EXIT:
        case ID_POPUP_EXIT:
            PostQuitMessage(0);
            return 0;
        case ID_TRAY_ADJUST:
        case ID_POPUP_ADJUST:
            RunElevatedAdjustment();
            return 0;
        case ID_TRAY_REFRESH_NTP:
            StartNtpLoad(TRUE);
            return 0;
        case ID_POPUP_CLOSE:
            HidePopup();
            return 0;
        }
        break;

    case WM_ENTERMENULOOP:
        g_contextMenuOpen = TRUE;
        if (g_popupHoverPreview) {
            HidePopup();
        }
        return 0;

    case WM_EXITMENULOOP:
        g_contextMenuOpen = FALSE;
        return 0;

    case WM_MEASUREITEM:
        if (MeasureTrayStatusMenuItem((MEASUREITEMSTRUCT *)lParam)) {
            return TRUE;
        }
        break;

    case WM_DRAWITEM:
        if (DrawTrayStatusMenuItem((const DRAWITEMSTRUCT *)lParam)) {
            return TRUE;
        }
        break;

    case WM_NTP_DONE:
        ApplyNtpResult((NtpResult *)lParam);
        if (lParam) HeapFree(GetProcessHeap(), 0, (void *)lParam);
        return 0;

    case WM_WTSSESSION_CHANGE:
        if (wParam == WTS_SESSION_LOGON || wParam == WTS_SESSION_UNLOCK) {
            StartNtpLoad(FALSE);
        }
        return 0;

    case WM_SETTINGCHANGE:
    case WM_THEMECHANGED:
        InvalidatePopup();
        if (g_notificationWnd) InvalidateRect(g_notificationWnd, NULL, TRUE);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static LRESULT CALLBACK PopupWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
        PaintBorderedTextWindow(hwnd, NULL, NULL);
        return 0;

    case WM_LBUTTONDOWN:
        BeginPopupDrag(hwnd, lParam, MK_LBUTTON);
        return 0;

    case WM_RBUTTONDOWN:
        BeginPopupDrag(hwnd, lParam, MK_RBUTTON);
        return 0;

    case WM_MOUSEMOVE:
        if (g_dragging && (wParam & g_dragButtonMask)) {
            POINT pt;
            GetCursorPos(&pt);
            SetWindowPos(hwnd, NULL, pt.x - g_dragOffset.x, pt.y - g_dragOffset.y, 0, 0,
                SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
            if (pt.x != g_dragStartCursor.x || pt.y != g_dragStartCursor.y) {
                g_popupUserMoved = TRUE;
            }
        }
        return 0;

    case WM_LBUTTONUP:
        EndPopupDrag(MK_LBUTTON);
        return 0;

    case WM_RBUTTONUP:
        if (!EndPopupDrag(MK_RBUTTON)) {
            PinHoverPopup();
        }
        ShowClockMenu(hwnd);
        return 0;

    case WM_CONTEXTMENU:
        PinHoverPopup();
        ShowClockMenu(hwnd);
        return 0;

    case WM_CAPTURECHANGED:
        g_dragging = FALSE;
        g_dragButtonMask = 0;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static LRESULT CALLBACK NotificationWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    (void)wParam;
    (void)lParam;
    switch (msg) {
    case WM_PAINT:
        PaintBorderedTextWindow(hwnd, g_notificationTitle, g_notificationBody);
        return 0;
    case WM_TIMER:
        DestroyWindow(hwnd);
        return 0;
    case WM_LBUTTONUP:
        DestroyWindow(hwnd);
        if (g_notificationClickAdjusts) {
            RunElevatedAdjustment();
        }
        return 0;
    case WM_DESTROY:
        KillTimer(hwnd, TIMER_NOTIFICATION);
        if (g_notificationWnd == hwnd) {
            g_notificationWnd = NULL;
        }
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR cmdLine, int nCmdShow)
{
    HANDLE mutex;
    WSADATA wsa;
    MSG msg;
    (void)hPrevInstance;
    (void)nCmdShow;

    if (wcscmp(cmdLine, ADJUST_ARGUMENT) == 0) {
        int exitCode = 1;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) == 0) {
            exitCode = SetLocalTimeFromNtp() ? 0 : 1;
            WSACleanup();
        }
        return exitCode;
    }

    mutex = CreateMutexW(NULL, TRUE, MUTEX_NAME);
    if (!mutex || GetLastError() == ERROR_ALREADY_EXISTS) {
        if (mutex) CloseHandle(mutex);
        return 0;
    }

    g_instance = hInstance;
    SetTrayNtpMenuText(NULL);
    if (SUCCEEDED(OleInitialize(NULL))) {
        g_oleInitialized = TRUE;
    }
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        CloseHandle(mutex);
        return 1;
    }

    g_taskbarCreatedMessage = RegisterWindowMessageW(L"TaskbarCreated");
    if (!InitFonts() || !RegisterClasses()) {
        Cleanup();
        CloseHandle(mutex);
        return 1;
    }

    g_icon = (HICON)LoadImageW(
        hInstance,
        MAKEINTRESOURCEW(IDI_APP),
        IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON),
        LR_DEFAULTCOLOR);
    if (!g_icon) {
        g_icon = LoadIconW(NULL, IDI_APPLICATION);
    } else {
        g_ownsIcon = TRUE;
    }

    g_mainWnd = CreateWindowExW(
        0,
        L"TrayClockTooltipMain",
        APP_NAME,
        WS_OVERLAPPEDWINDOW,
        0, 0, 1, 1,
        NULL, NULL, hInstance, NULL);
    if (!g_mainWnd) {
        Cleanup();
        CloseHandle(mutex);
        return 1;
    }
    ShowWindow(g_mainWnd, SW_HIDE);

    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    Cleanup();
    CloseHandle(mutex);
    return 0;
}
