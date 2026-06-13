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
#define MAIN_WINDOW_CLASS L"TrayClockTooltipMain"
#define ADJUST_ARGUMENT L"--set-local-time"
#define WAIT_PARENT_ARGUMENT L"--wait-for-parent"
#define INSTALL_PROMPT_ARGUMENT L"--install-prompt"
#define OPEN_LOG_FOLDER_ARGUMENT L"--open-log-folder"
#define NOTIFY_THRESHOLD_ARGUMENT_PREFIX L"--notify-threshold="

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
#define ID_TRAY_INSTALL_USER 1005
#define ID_TRAY_STARTUP_CURRENT 1006
#define ID_TRAY_STARTUP_REMOVE 1007
#define ID_OPEN_LOG_FOLDER 1008
#define ID_TRAY_THRESHOLD_1 1009
#define ID_TRAY_THRESHOLD_3 1010
#define ID_TRAY_THRESHOLD_5 1011
#define ID_TRAY_THRESHOLD_7 1012
#define ID_TRAY_THRESHOLD_10 1013
#define ID_TRAY_THRESHOLD_OFF 1014
#define ID_POPUP_CLOSE 2001
#define ID_POPUP_EXIT 2002
#define ID_POPUP_ADJUST 2003

#define STARTUP_RUN_KEY L"Software\\Microsoft\\Windows\\CurrentVersion\\Run"
#define EXE_FILE_NAME L"TrayClockTooltip.exe"
#define LOG_FILE_NAME L"TrayClockTooltip.log"
#define LOG_MAX_BYTES 262144
#define NTP_PORT "123"
#define NTP_PACKET_LENGTH 48
#define NTP_TIMEOUT_MS 3000
#define NTP_TO_FILETIME_SECONDS 9435484800ULL
#define HNS_PER_SECOND 10000000LL
#define NOTIFICATION_TIMEOUT_MS 5000
#define TRAY_HOVER_X_MARGIN 4
#define TRAY_HOVER_FALLBACK_HALF_WIDTH 24
#define POPUP_EDGE_MARGIN 2
#define POPUP_ICON_GAP 8
#define POPUP_CURSOR_GAP 12
#define HOVER_PREVIEW_POLL_MS 250
#define INSTALL_CLOSE_TIMEOUT_MS 5000
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

typedef enum NtpEvent {
    NTP_EVENT_STARTUP = 0,
    NTP_EVENT_REFRESH,
    NTP_EVENT_LOGON,
    NTP_EVENT_UNLOCK
} NtpEvent;

typedef struct NtpResult {
    BOOL success;
    NtpEvent event;
    int64_t offsetHns;
    WCHAR source[256];
    WCHAR message[320];
} NtpResult;

typedef enum InstallResult {
    INSTALL_RESULT_FAILED = 0,
    INSTALL_RESULT_INSTALLED,
    INSTALL_RESULT_INSTALLED_AND_LAUNCHED
} InstallResult;

typedef struct NotifyThreshold {
    BOOL off;
    int seconds;
} NotifyThreshold;

typedef enum NotificationTextId {
    NOTIFY_TEXT_REFRESH_OFF = 0,
    NOTIFY_TEXT_REFRESH_WITHIN_THRESHOLD,
    NOTIFY_TEXT_ADJUST_START_FAILED,
    NOTIFY_TEXT_ADJUST_OK,
    NOTIFY_TEXT_ADJUST_FAILED,
    NOTIFY_TEXT_THRESHOLD_UPDATE_FAILED,
    NOTIFY_TEXT_INSTALL_FAILED,
    NOTIFY_TEXT_INSTALL_OK,
    NOTIFY_TEXT_STARTUP_REGISTER_FAILED,
    NOTIFY_TEXT_STARTUP_REGISTER_OK,
    NOTIFY_TEXT_STARTUP_REMOVE_FAILED,
    NOTIFY_TEXT_STARTUP_REMOVE_OK,
    NOTIFY_TEXT_NTP_FAILED,
    NOTIFY_TEXT_OPEN_LOG_FAILED,
    NOTIFY_TEXT_PORTABLE_STARTUP_OK
} NotificationTextId;

typedef struct CloseInstanceContext {
    const WCHAR *path;
    BOOL failed;
} CloseInstanceContext;

typedef struct NtpQueryContext {
    NtpEvent event;
} NtpQueryContext;

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
static BOOL g_ntpTimeAvailable;
static BOOL g_adjustmentAvailable;
static BOOL g_ntpQueryInProgress;
static BOOL g_notifyNtpSuccessIfNoDrift;
static BOOL g_notifyPortableStartupIfNoDrift;
static BOOL g_notifyInstallSuccessOnStartup;
static BOOL g_ownsIcon;
static BOOL g_oleInitialized;
static BOOL g_sessionNotificationRegistered;
static WCHAR g_statusText[320];
static WCHAR g_clockSource[256];
static WCHAR g_notificationTitle[128];
static WCHAR g_notificationBody[320];
static NotifyThreshold g_notifyThreshold = { FALSE, 1 };
static BOOL g_pendingThresholdDriftNotification;
static int64_t g_pendingThresholdDriftOffsetHns;
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
static void ShowNotification(const WCHAR *title, const WCHAR *body, BOOL clickAdjusts);
static void LogSimpleEvent(const WCHAR *event, const WCHAR *result, const WCHAR *details);
static void LogNtpResult(NtpEvent event, const NtpResult *result);
static void LogInstallEvent(const WCHAR *result, const WCHAR *stage);
static BOOL FileExists(const WCHAR *path);
static void StripFileName(WCHAR *path);
static BOOL GetDirectoryFromPath(const WCHAR *path, WCHAR *dir, DWORD cch);

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

static BOOL IsSupportedNotifyThreshold(int seconds)
{
    return seconds == 1 || seconds == 3 || seconds == 5 || seconds == 7 || seconds == 10;
}

static BOOL ParseNotifyThresholdValue(const WCHAR *value, NotifyThreshold *threshold)
{
    WCHAR token[16];
    size_t i = 0;

    while (value[i] && value[i] != L' ' && value[i] != L'\t' && i + 1 < ARRAYSIZE(token)) {
        token[i] = value[i];
        i++;
    }
    token[i] = L'\0';

    if (_wcsicmp(token, L"off") == 0) {
        threshold->off = TRUE;
        threshold->seconds = 0;
        return TRUE;
    }
    if (token[0] >= L'0' && token[0] <= L'9') {
        int seconds = _wtoi(token);
        if (IsSupportedNotifyThreshold(seconds)) {
            threshold->off = FALSE;
            threshold->seconds = seconds;
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL FindNotifyThresholdArgument(const WCHAR *text, NotifyThreshold *threshold)
{
    const WCHAR *cursor = text;
    size_t prefixLen = wcslen(NOTIFY_THRESHOLD_ARGUMENT_PREFIX);

    while ((cursor = wcsstr(cursor, NOTIFY_THRESHOLD_ARGUMENT_PREFIX)) != NULL) {
        if (cursor == text || cursor[-1] == L' ' || cursor[-1] == L'\t') {
            return ParseNotifyThresholdValue(cursor + prefixLen, threshold);
        }
        cursor += prefixLen;
    }
    return FALSE;
}

static BOOL NotifyThresholdEquals(const NotifyThreshold *left, const NotifyThreshold *right)
{
    return left->off == right->off && left->seconds == right->seconds;
}

static BOOL IsDefaultNotifyThreshold(const NotifyThreshold *threshold)
{
    return !threshold->off && threshold->seconds == 1;
}

static BOOL IsOffsetAtNotifyThreshold(int64_t offsetHns)
{
    if (g_notifyThreshold.off) {
        return FALSE;
    }
    return llabs(offsetHns) >= ((int64_t)g_notifyThreshold.seconds * HNS_PER_SECOND);
}

static void BuildNtpStatusText(int64_t offsetHns, const WCHAR *source, WCHAR *buffer, DWORD cch)
{
    double seconds = (double)offsetHns / (double)HNS_PER_SECOND;
    swprintf(buffer, cch, L"Time: %+.3fs (%ls)", seconds, source && source[0] ? source : L"-");
}

static void ReevaluateAdjustmentAvailability(void)
{
    if (g_ntpQueryInProgress) {
        SetTrayNtpMenuText(NULL);
        g_adjustmentAvailable = FALSE;
        return;
    }
    if (g_ntpTimeAvailable && IsOffsetAtNotifyThreshold(g_clockOffsetHns)) {
        WCHAR status[320];
        BuildNtpStatusText(g_clockOffsetHns, g_clockSource, status, ARRAYSIZE(status));
        SetTrayNtpMenuText(status);
        g_adjustmentAvailable = TRUE;
    } else {
        SetTrayNtpMenuText(NULL);
        g_adjustmentAvailable = FALSE;
    }
}

static BOOL CanAdjustFromNormalMenu(void)
{
    return g_adjustmentAvailable || (g_notifyThreshold.off && g_ntpTimeAvailable && !g_ntpQueryInProgress);
}

static BOOL IsJapaneseUiLanguage(void)
{
    return PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_JAPANESE;
}

static const WCHAR *NotificationText(NotificationTextId id)
{
    BOOL ja = IsJapaneseUiLanguage();
    switch (id) {
    case NOTIFY_TEXT_REFRESH_OFF:
        return ja ? L"NTP時刻を更新しました。自動のずれ通知はオフです。"
                  : L"NTP time was refreshed. Automatic drift notifications are off.";
    case NOTIFY_TEXT_REFRESH_WITHIN_THRESHOLD:
        return ja ? L"NTP時刻を更新しました。Windows時計は通知しきい値内です。"
                  : L"NTP time was refreshed. Windows time is within the notification threshold.";
    case NOTIFY_TEXT_ADJUST_START_FAILED:
        return ja ? L"時刻調整を開始できませんでした。"
                  : L"Could not start time adjustment.";
    case NOTIFY_TEXT_ADJUST_OK:
        return ja ? L"Windows時刻を調整しました。"
                  : L"Windows time was adjusted.";
    case NOTIFY_TEXT_ADJUST_FAILED:
        return ja ? L"Windows時刻を調整できませんでした。"
                  : L"Could not adjust Windows time.";
    case NOTIFY_TEXT_THRESHOLD_UPDATE_FAILED:
        return ja ? L"スタートアップの通知しきい値を更新できませんでした。"
                  : L"Could not update startup notification threshold.";
    case NOTIFY_TEXT_INSTALL_FAILED:
        return ja ? L"このユーザー向けにインストールできませんでした。"
                  : L"Could not install for this user.";
    case NOTIFY_TEXT_INSTALL_OK:
        return ja ? L"このユーザー向けにインストールし、スタートアップに登録しました。"
                  : L"Installed for this user and registered for startup.";
    case NOTIFY_TEXT_STARTUP_REGISTER_FAILED:
        return ja ? L"スタートアップに登録できませんでした。"
                  : L"Could not register startup.";
    case NOTIFY_TEXT_STARTUP_REGISTER_OK:
        return ja ? L"このEXEをスタートアップに登録しました。"
                  : L"This EXE was registered for startup.";
    case NOTIFY_TEXT_STARTUP_REMOVE_FAILED:
        return ja ? L"スタートアップ登録を削除できませんでした。"
                  : L"Could not remove startup registration.";
    case NOTIFY_TEXT_STARTUP_REMOVE_OK:
        return ja ? L"スタートアップ登録を削除しました。"
                  : L"Startup registration was removed.";
    case NOTIFY_TEXT_NTP_FAILED:
        return ja ? L"NTP時刻を取得できませんでした。アプリの時計はWindowsのシステム時計を使用しています。"
                  : L"Could not obtain NTP time. The app clock is using the Windows system clock.";
    case NOTIFY_TEXT_OPEN_LOG_FAILED:
        return ja ? L"ログフォルダを開けませんでした。"
                  : L"Could not open log folder.";
    case NOTIFY_TEXT_PORTABLE_STARTUP_OK:
        return ja ? L"TrayClockTooltipを起動しました。トレイアイコンにマウスを合わせると時計を表示します。"
                  : L"TrayClockTooltip is running. Hover the tray icon to show the clock.";
    default:
        return APP_NAME;
    }
}

static void ShowManualRefreshSuccessNotification(void)
{
    if (g_notifyThreshold.off) {
        ShowNotification(APP_NAME, NotificationText(NOTIFY_TEXT_REFRESH_OFF), FALSE);
    } else {
        ShowNotification(APP_NAME, NotificationText(NOTIFY_TEXT_REFRESH_WITHIN_THRESHOLD), FALSE);
    }
}

static void ShowDriftNotification(int64_t offsetHns)
{
    WCHAR body[320];
    double seconds = (double)llabs(offsetHns) / (double)HNS_PER_SECOND;
    if (IsJapaneseUiLanguage()) {
        swprintf(body, ARRAYSIZE(body),
            L"Windows時計は%.3f秒ずれています。アプリの時計はNTP時刻を使用しています。",
            seconds);
    } else {
        swprintf(body, ARRAYSIZE(body),
            L"Windows time differs by %.3f seconds. The app clock is using NTP time.",
            seconds);
    }
    ShowNotification(APP_NAME, body, TRUE);
}

static void QueueDriftNotificationAfterMenu(int64_t offsetHns)
{
    g_pendingThresholdDriftNotification = TRUE;
    g_pendingThresholdDriftOffsetHns = offsetHns;
}

static void ShowPendingThresholdDriftNotification(void)
{
    if (!g_pendingThresholdDriftNotification) {
        return;
    }
    g_pendingThresholdDriftNotification = FALSE;
    ShowDriftNotification(g_pendingThresholdDriftOffsetHns);
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

static BOOL GetCurrentExePath(WCHAR *path, DWORD cch)
{
    DWORD len = GetModuleFileNameW(NULL, path, cch);
    return len > 0 && len < cch;
}

static void RunElevatedAdjustment(void)
{
    WCHAR exePath[MAX_PATH];
    SHELLEXECUTEINFOW sei;
    DWORD exitCode = 1;

    if (!GetCurrentExePath(exePath, ARRAYSIZE(exePath))) {
        LogSimpleEvent(L"adjust", L"FAILED", L"error=exe_path");
        ShowNotification(APP_NAME, NotificationText(NOTIFY_TEXT_ADJUST_START_FAILED), FALSE);
        return;
    }

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
            g_ntpTimeAvailable = FALSE;
            g_clockSource[0] = L'\0';
            g_adjustmentAvailable = FALSE;
            SetTrayNtpMenuText(NULL);
            HideNotification();
            UpdateClockDisplays();
            ShowNotification(APP_NAME, NotificationText(NOTIFY_TEXT_ADJUST_OK), FALSE);
        } else {
            ShowNotification(APP_NAME, NotificationText(NOTIFY_TEXT_ADJUST_FAILED), FALSE);
        }
    } else if (GetLastError() == ERROR_CANCELLED) {
        LogSimpleEvent(L"adjust", L"CANCEL", L"error=uac");
    } else {
        LogSimpleEvent(L"adjust", L"FAILED", L"error=launch");
        ShowNotification(APP_NAME, NotificationText(NOTIFY_TEXT_ADJUST_START_FAILED), FALSE);
    }
}

/* Self-placement and startup registration stay in HKCU, so they do not need UAC. */
static BOOL AppendPathPart(WCHAR *path, DWORD cch, const WCHAR *part)
{
    size_t len = wcslen(path);
    size_t partLen = wcslen(part);
    BOOL needsSlash = len > 0 && path[len - 1] != L'\\' && path[len - 1] != L'/';

    if (len + (needsSlash ? 1 : 0) + partLen + 1 > cch) {
        return FALSE;
    }
    if (needsSlash) {
        path[len++] = L'\\';
        path[len] = L'\0';
    }
    wcscat_s(path, cch, part);
    return TRUE;
}

static BOOL EnsureInstallDirectory(WCHAR *installDir, DWORD cch)
{
    WCHAR localAppData[MAX_PATH];
    DWORD used = GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, ARRAYSIZE(localAppData));

    if (used == 0 || used >= ARRAYSIZE(localAppData) || used >= cch) {
        return FALSE;
    }

    wcscpy_s(installDir, cch, localAppData);
    if (!AppendPathPart(installDir, cch, L"Programs")) {
        return FALSE;
    }
    if (!CreateDirectoryW(installDir, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
        return FALSE;
    }
    if (!AppendPathPart(installDir, cch, APP_NAME)) {
        return FALSE;
    }
    if (!CreateDirectoryW(installDir, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
        return FALSE;
    }
    return TRUE;
}

static BOOL BuildInstalledExePath(WCHAR *path, DWORD cch)
{
    DWORD used = GetEnvironmentVariableW(L"LOCALAPPDATA", path, cch);
    if (used == 0 || used >= cch) {
        return FALSE;
    }
    return AppendPathPart(path, cch, L"Programs") &&
        AppendPathPart(path, cch, APP_NAME) &&
        AppendPathPart(path, cch, EXE_FILE_NAME);
}

static BOOL BuildStartupCommand(const WCHAR *exePath, const NotifyThreshold *threshold, WCHAR *command, DWORD cch)
{
    if (IsDefaultNotifyThreshold(threshold)) {
        return swprintf(command, cch, L"\"%ls\"", exePath) >= 0;
    }
    if (threshold->off) {
        return swprintf(command, cch, L"\"%ls\" %lsoff", exePath, NOTIFY_THRESHOLD_ARGUMENT_PREFIX) >= 0;
    }
    return swprintf(command, cch, L"\"%ls\" %ls%d", exePath, NOTIFY_THRESHOLD_ARGUMENT_PREFIX, threshold->seconds) >= 0;
}

static BOOL SetStartupRegistrationWithThreshold(const WCHAR *exePath, const NotifyThreshold *threshold)
{
    HKEY key;
    WCHAR command[MAX_PATH + 64];
    LSTATUS status;

    if (!BuildStartupCommand(exePath, threshold, command, ARRAYSIZE(command))) {
        return FALSE;
    }

    status = RegCreateKeyExW(HKEY_CURRENT_USER, STARTUP_RUN_KEY, 0, NULL, 0, KEY_SET_VALUE, NULL, &key, NULL);
    if (status != ERROR_SUCCESS) {
        return FALSE;
    }

    status = RegSetValueExW(key, APP_NAME, 0, REG_SZ, (const BYTE *)command,
        (DWORD)((wcslen(command) + 1) * sizeof(WCHAR)));
    RegCloseKey(key);
    return status == ERROR_SUCCESS;
}

static BOOL SetStartupRegistration(const WCHAR *exePath)
{
    return SetStartupRegistrationWithThreshold(exePath, &g_notifyThreshold);
}

static BOOL ExtractCommandPath(const WCHAR *command, WCHAR *path, DWORD cch)
{
    WCHAR *end;
    size_t len;

    while (*command == L' ' || *command == L'\t') {
        command++;
    }

    if (command[0] == L'"') {
        end = wcschr(command + 1, L'"');
        if (end) {
            len = (size_t)(end - (command + 1));
            if (len + 1 > cch) {
                return FALSE;
            }
            memcpy(path, command + 1, len * sizeof(WCHAR));
            path[len] = L'\0';
            return path[0] != L'\0';
        }
    }

    end = wcspbrk(command, L" \t");
    len = end ? (size_t)(end - command) : wcslen(command);
    if (len + 1 > cch) {
        return FALSE;
    }
    memcpy(path, command, len * sizeof(WCHAR));
    path[len] = L'\0';
    return path[0] != L'\0';
}

static BOOL GetStartupRegistrationCommand(WCHAR *command, DWORD cch)
{
    HKEY key;
    DWORD type = 0;
    DWORD bytes = cch * sizeof(WCHAR);
    LSTATUS status = RegOpenKeyExW(HKEY_CURRENT_USER, STARTUP_RUN_KEY, 0, KEY_QUERY_VALUE, &key);
    if (status != ERROR_SUCCESS) {
        return FALSE;
    }

    status = RegQueryValueExW(key, APP_NAME, NULL, &type, (BYTE *)command, &bytes);
    RegCloseKey(key);
    if (status != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ) || bytes < sizeof(WCHAR)) {
        return FALSE;
    }

    command[cch - 1] = L'\0';
    return command[0] != L'\0';
}

static BOOL GetStartupRegistration(WCHAR *path, DWORD cch)
{
    WCHAR command[MAX_PATH + 128];
    if (!GetStartupRegistrationCommand(command, ARRAYSIZE(command))) {
        return FALSE;
    }
    return ExtractCommandPath(command, path, cch);
}

static BOOL GetStartupRegistrationThreshold(NotifyThreshold *threshold)
{
    WCHAR command[MAX_PATH + 128];
    if (!GetStartupRegistrationCommand(command, ARRAYSIZE(command))) {
        return FALSE;
    }
    return FindNotifyThresholdArgument(command, threshold);
}

static BOOL UpdateStartupRegistrationThreshold(void)
{
    WCHAR command[MAX_PATH + 128];
    WCHAR path[MAX_PATH];

    if (!GetStartupRegistrationCommand(command, ARRAYSIZE(command))) {
        return TRUE;
    }
    if (!ExtractCommandPath(command, path, ARRAYSIZE(path))) {
        return FALSE;
    }
    return SetStartupRegistrationWithThreshold(path, &g_notifyThreshold);
}

static void SetNotifyThreshold(NotifyThreshold threshold)
{
    BOOL wasAtThreshold;

    if (NotifyThresholdEquals(&g_notifyThreshold, &threshold)) {
        return;
    }
    wasAtThreshold = g_ntpTimeAvailable && !g_ntpQueryInProgress && IsOffsetAtNotifyThreshold(g_clockOffsetHns);
    g_notifyThreshold = threshold;
    ReevaluateAdjustmentAvailability();
    if (!wasAtThreshold && g_adjustmentAvailable) {
        if (IsMenuActive()) {
            QueueDriftNotificationAfterMenu(g_clockOffsetHns);
        } else {
            ShowDriftNotification(g_clockOffsetHns);
        }
    }
    if (!UpdateStartupRegistrationThreshold()) {
        ShowNotification(APP_NAME, NotificationText(NOTIFY_TEXT_THRESHOLD_UPDATE_FAILED), FALSE);
    }
}

static void InitializeNotifyThreshold(PWSTR cmdLine)
{
    NotifyThreshold threshold;

    if (FindNotifyThresholdArgument(cmdLine, &threshold) ||
        GetStartupRegistrationThreshold(&threshold)) {
        g_notifyThreshold = threshold;
    }
}

static BOOL CopyReleaseReadmeToInstallDir(const WCHAR *currentPath, const WCHAR *installDir, const WCHAR *fileName)
{
    WCHAR source[MAX_PATH];
    WCHAR target[MAX_PATH];

    if (!GetDirectoryFromPath(currentPath, source, ARRAYSIZE(source)) ||
        !AppendPathPart(source, ARRAYSIZE(source), fileName)) {
        return FALSE;
    }
    if (!FileExists(source)) {
        return TRUE;
    }
    wcscpy_s(target, ARRAYSIZE(target), installDir);
    if (!AppendPathPart(target, ARRAYSIZE(target), fileName)) {
        return FALSE;
    }
    return CopyFileW(source, target, FALSE);
}

static void CopyReleaseReadmesToInstallDir(const WCHAR *currentPath, const WCHAR *installedPath)
{
    WCHAR installDir[MAX_PATH];

    wcscpy_s(installDir, ARRAYSIZE(installDir), installedPath);
    StripFileName(installDir);
    LogInstallEvent(CopyReleaseReadmeToInstallDir(currentPath, installDir, L"README.txt") ? L"OK" : L"FAILED", L"readme-en");
    LogInstallEvent(CopyReleaseReadmeToInstallDir(currentPath, installDir, L"README.ja.txt") ? L"OK" : L"FAILED", L"readme-ja");
}

static BOOL RemoveStartupRegistration(void)
{
    HKEY key;
    LSTATUS status = RegOpenKeyExW(HKEY_CURRENT_USER, STARTUP_RUN_KEY, 0, KEY_SET_VALUE, &key);
    if (status == ERROR_FILE_NOT_FOUND) {
        return TRUE;
    }
    if (status != ERROR_SUCCESS) {
        return FALSE;
    }

    status = RegDeleteValueW(key, APP_NAME);
    RegCloseKey(key);
    return status == ERROR_SUCCESS || status == ERROR_FILE_NOT_FOUND;
}

static BOOL FileExists(const WCHAR *path)
{
    DWORD attrs = GetFileAttributesW(path);
    return attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}

static BOOL BuffersAreEqual(const BYTE *left, const BYTE *right, DWORD bytes)
{
    DWORD i;
    for (i = 0; i < bytes; i++) {
        if (left[i] != right[i]) {
            return FALSE;
        }
    }
    return TRUE;
}

static BOOL FilesAreIdentical(const WCHAR *leftPath, const WCHAR *rightPath)
{
    HANDLE left;
    HANDLE right;
    LARGE_INTEGER leftSize;
    LARGE_INTEGER rightSize;
    BYTE leftBuffer[4096];
    BYTE rightBuffer[4096];
    DWORD leftRead;
    DWORD rightRead;
    BOOL identical = FALSE;

    left = CreateFileW(leftPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (left == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    right = CreateFileW(rightPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (right == INVALID_HANDLE_VALUE) {
        CloseHandle(left);
        return FALSE;
    }

    if (GetFileSizeEx(left, &leftSize) && GetFileSizeEx(right, &rightSize) &&
        leftSize.QuadPart == rightSize.QuadPart) {
        identical = TRUE;
        for (;;) {
            if (!ReadFile(left, leftBuffer, sizeof(leftBuffer), &leftRead, NULL) ||
                !ReadFile(right, rightBuffer, sizeof(rightBuffer), &rightRead, NULL) ||
                leftRead != rightRead ||
                !BuffersAreEqual(leftBuffer, rightBuffer, leftRead)) {
                identical = FALSE;
                break;
            }
            if (leftRead == 0) {
                break;
            }
        }
    }

    CloseHandle(right);
    CloseHandle(left);
    return identical;
}

static void StripFileName(WCHAR *path)
{
    WCHAR *slash = wcsrchr(path, L'\\');
    WCHAR *altSlash = wcsrchr(path, L'/');
    if (!slash || (altSlash && altSlash > slash)) {
        slash = altSlash;
    }
    if (slash) {
        *slash = L'\0';
    }
}

static BOOL GetDirectoryFromPath(const WCHAR *path, WCHAR *dir, DWORD cch)
{
    if (wcscpy_s(dir, cch, path) != 0) {
        return FALSE;
    }
    StripFileName(dir);
    return dir[0] != L'\0';
}

static BOOL BuildUserLogPath(WCHAR *path, DWORD cch)
{
    DWORD used = GetEnvironmentVariableW(L"LOCALAPPDATA", path, cch);
    if (used == 0 || used >= cch) {
        return FALSE;
    }
    if (!AppendPathPart(path, cch, APP_NAME)) {
        return FALSE;
    }
    if (!CreateDirectoryW(path, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
        return FALSE;
    }
    return AppendPathPart(path, cch, LOG_FILE_NAME);
}

static BOOL BuildLogPath(WCHAR *path, DWORD cch)
{
    WCHAR currentPath[MAX_PATH];
    WCHAR installedPath[MAX_PATH];

    if (!GetCurrentExePath(currentPath, ARRAYSIZE(currentPath))) {
        return FALSE;
    }

    if (BuildInstalledExePath(installedPath, ARRAYSIZE(installedPath)) &&
        _wcsicmp(currentPath, installedPath) == 0) {
        return BuildUserLogPath(path, cch);
    }

    wcscpy_s(path, cch, currentPath);
    StripFileName(path);
    return AppendPathPart(path, cch, LOG_FILE_NAME);
}

static void RotateLogIfNeeded(const WCHAR *path)
{
    WIN32_FILE_ATTRIBUTE_DATA data;
    LARGE_INTEGER size;
    WCHAR backup[MAX_PATH + 3];

    if (!GetFileAttributesExW(path, GetFileExInfoStandard, &data)) {
        return;
    }
    size.HighPart = data.nFileSizeHigh;
    size.LowPart = data.nFileSizeLow;
    if (size.QuadPart <= LOG_MAX_BYTES) {
        return;
    }
    if (swprintf(backup, ARRAYSIZE(backup), L"%ls.1", path) < 0) {
        return;
    }
    MoveFileExW(path, backup, MOVEFILE_REPLACE_EXISTING);
}

static void WriteLogLineToPath(const WCHAR *path, const WCHAR *line)
{
    WCHAR withBreak[1024];
    char utf8[3072];
    HANDLE file;
    int bytes;
    DWORD written;

    RotateLogIfNeeded(path);
    if (swprintf(withBreak, ARRAYSIZE(withBreak), L"%ls\r\n", line) < 0) {
        return;
    }
    bytes = WideCharToMultiByte(CP_UTF8, 0, withBreak, -1, utf8, sizeof(utf8), NULL, NULL);
    if (bytes <= 1) {
        return;
    }

    file = CreateFileW(path, FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        return;
    }
    WriteFile(file, utf8, (DWORD)(bytes - 1), &written, NULL);
    CloseHandle(file);
}

static void WriteLogLine(const WCHAR *line)
{
    WCHAR path[MAX_PATH];
    if (BuildLogPath(path, ARRAYSIZE(path))) {
        WriteLogLineToPath(path, line);
    }
}

static void WriteUserLogLine(const WCHAR *line)
{
    WCHAR path[MAX_PATH];
    if (BuildUserLogPath(path, ARRAYSIZE(path))) {
        WriteLogLineToPath(path, line);
    }
}

static void FormatLogTimestamp(WCHAR *buffer, DWORD cch)
{
    SYSTEMTIME st;
    GetLocalTime(&st);
    swprintf(buffer, cch, L"%04u-%02u-%02u %02u:%02u:%02u.%03u",
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
}

static void FormatOffsetValue(int64_t offsetHns, WCHAR *buffer, DWORD cch)
{
    WCHAR sign = offsetHns < 0 ? L'-' : L'+';
    int64_t absHns = offsetHns < 0 ? -offsetHns : offsetHns;
    int64_t totalMs = (absHns + 5000) / 10000;
    int64_t seconds = totalMs / 1000;
    int64_t millis = totalMs % 1000;

    if (seconds < 100) {
        swprintf(buffer, cch, L"%c%02lld.%03llds", sign, (long long)seconds, (long long)millis);
    } else {
        swprintf(buffer, cch, L"%c%lld.%03llds", sign, (long long)seconds, (long long)millis);
    }
}

static const WCHAR *NtpEventName(NtpEvent event)
{
    switch (event) {
    case NTP_EVENT_REFRESH:
        return L"refresh";
    case NTP_EVENT_LOGON:
        return L"logon";
    case NTP_EVENT_UNLOCK:
        return L"unlock";
    case NTP_EVENT_STARTUP:
    default:
        return L"startup";
    }
}

static void FormatSimpleLogLine(WCHAR *line, DWORD cch, const WCHAR *event, const WCHAR *result, const WCHAR *details)
{
    WCHAR timestamp[32];
    FormatLogTimestamp(timestamp, ARRAYSIZE(timestamp));
    if (details && details[0]) {
        swprintf(line, cch, L"%ls  %-8ls %-7ls %ls", timestamp, event, result, details);
    } else {
        swprintf(line, cch, L"%ls  %-8ls %-7ls", timestamp, event, result);
    }
}

static void LogSimpleEvent(const WCHAR *event, const WCHAR *result, const WCHAR *details)
{
    WCHAR line[1024];
    FormatSimpleLogLine(line, ARRAYSIZE(line), event, result, details);
    WriteLogLine(line);
}

static void LogSimpleUserEvent(const WCHAR *event, const WCHAR *result, const WCHAR *details)
{
    WCHAR line[1024];
    FormatSimpleLogLine(line, ARRAYSIZE(line), event, result, details);
    WriteUserLogLine(line);
}

static void LogInstallEvent(const WCHAR *result, const WCHAR *stage)
{
    WCHAR details[128];
    swprintf(details, ARRAYSIZE(details), L"stage=%ls", stage);
    LogSimpleUserEvent(L"install", result, details);
}

static void LogNtpResult(NtpEvent event, const NtpResult *result)
{
    WCHAR details[512];
    WCHAR offset[32];
    const WCHAR *source = (result && result->source[0]) ? result->source : L"-";

    if (result && result->success) {
        FormatOffsetValue(result->offsetHns, offset, ARRAYSIZE(offset));
        swprintf(details, ARRAYSIZE(details), L"offset=%ls  source=%ls", offset, source);
        LogSimpleEvent(NtpEventName(event), L"OK", details);
    } else {
        swprintf(details, ARRAYSIZE(details), L"error=ntp  source=%ls", source);
        LogSimpleEvent(NtpEventName(event), L"FAILED", details);
    }
}

static BOOL IsStartupRegistered(void)
{
    WCHAR registeredPath[MAX_PATH];
    return GetStartupRegistration(registeredPath, ARRAYSIZE(registeredPath));
}

static BOOL CanRegisterCurrentExeForStartup(void)
{
    WCHAR currentPath[MAX_PATH];
    WCHAR registeredPath[MAX_PATH];

    if (!GetCurrentExePath(currentPath, ARRAYSIZE(currentPath))) {
        return FALSE;
    }
    if (!GetStartupRegistration(registeredPath, ARRAYSIZE(registeredPath))) {
        return TRUE;
    }
    if (_wcsicmp(currentPath, registeredPath) == 0) {
        return FALSE;
    }
    if (!FileExists(registeredPath)) {
        return TRUE;
    }
    return !FilesAreIdentical(currentPath, registeredPath);
}

static BOOL CanInstallForCurrentUser(void)
{
    WCHAR currentPath[MAX_PATH];
    WCHAR installedPath[MAX_PATH];
    WCHAR registeredPath[MAX_PATH];

    if (!GetCurrentExePath(currentPath, ARRAYSIZE(currentPath)) ||
        !BuildInstalledExePath(installedPath, ARRAYSIZE(installedPath))) {
        return FALSE;
    }
    if (!FileExists(installedPath) || !FilesAreIdentical(currentPath, installedPath)) {
        return TRUE;
    }
    if (_wcsicmp(currentPath, installedPath) == 0) {
        return FALSE;
    }
    if (!GetStartupRegistration(registeredPath, ARRAYSIZE(registeredPath))) {
        return TRUE;
    }
    if (_wcsicmp(registeredPath, installedPath) != 0) {
        return TRUE;
    }
    return FALSE;
}

static BOOL ShouldPromptForInstall(void)
{
    WCHAR currentPath[MAX_PATH];
    WCHAR installedPath[MAX_PATH];

    return GetCurrentExePath(currentPath, ARRAYSIZE(currentPath)) &&
        BuildInstalledExePath(installedPath, ARRAYSIZE(installedPath)) &&
        FileExists(installedPath) &&
        _wcsicmp(currentPath, installedPath) != 0;
}

static BOOL ShouldNotifyPortableStartup(void)
{
    WCHAR currentPath[MAX_PATH];
    WCHAR installedPath[MAX_PATH];

    if (!GetCurrentExePath(currentPath, ARRAYSIZE(currentPath)) ||
        !BuildInstalledExePath(installedPath, ARRAYSIZE(installedPath))) {
        return FALSE;
    }
    if (IsStartupRegistered() || FileExists(installedPath)) {
        return FALSE;
    }
    return _wcsicmp(currentPath, installedPath) != 0;
}

static BOOL IsWindowProcessPath(HWND hwnd, const WCHAR *path, HANDLE *process)
{
    DWORD pid = 0;
    WCHAR processPath[MAX_PATH];
    DWORD cch = ARRAYSIZE(processPath);

    *process = NULL;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == 0 || pid == GetCurrentProcessId()) {
        return FALSE;
    }

    *process = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!*process) {
        return FALSE;
    }

    if (!QueryFullProcessImageNameW(*process, 0, processPath, &cch) ||
        _wcsicmp(processPath, path) != 0) {
        CloseHandle(*process);
        *process = NULL;
        return FALSE;
    }

    return TRUE;
}

static BOOL IsMainWindowClass(HWND hwnd)
{
    WCHAR className[64];
    return GetClassNameW(hwnd, className, ARRAYSIZE(className)) > 0 &&
        wcscmp(className, MAIN_WINDOW_CLASS) == 0;
}

static BOOL CloseInstanceWindow(HWND hwnd, const WCHAR *path)
{
    HANDLE process;

    if (!IsMainWindowClass(hwnd)) {
        return TRUE;
    }
    if (!IsWindowProcessPath(hwnd, path, &process)) {
        return TRUE;
    }

    PostMessageW(hwnd, WM_CLOSE, 0, 0);
    if (WaitForSingleObject(process, INSTALL_CLOSE_TIMEOUT_MS) != WAIT_OBJECT_0) {
        CloseHandle(process);
        return FALSE;
    }

    CloseHandle(process);
    return TRUE;
}

static BOOL CALLBACK CloseInstanceEnumProc(HWND hwnd, LPARAM lParam)
{
    CloseInstanceContext *context = (CloseInstanceContext *)lParam;
    if (!CloseInstanceWindow(hwnd, context->path)) {
        context->failed = TRUE;
        return FALSE;
    }
    return TRUE;
}

static BOOL CloseInstancesForPath(const WCHAR *path)
{
    CloseInstanceContext context;
    context.path = path;
    context.failed = FALSE;
    EnumWindows(CloseInstanceEnumProc, (LPARAM)&context);
    return !context.failed;
}

static void RequestAppExit(void)
{
    if (g_mainWnd) {
        PostMessageW(g_mainWnd, WM_CLOSE, 0, 0);
    } else {
        PostQuitMessage(0);
    }
}

static BOOL LaunchInstalledExeAfterExit(const WCHAR *installedPath)
{
    WCHAR commandLine[MAX_PATH + 64];
    WCHAR currentDirectory[MAX_PATH];
    STARTUPINFOW startupInfo;
    PROCESS_INFORMATION processInfo;

    if (swprintf(commandLine, ARRAYSIZE(commandLine), L"\"%ls\" %ls %lu",
        installedPath, WAIT_PARENT_ARGUMENT, GetCurrentProcessId()) < 0) {
        return FALSE;
    }
    if (!GetDirectoryFromPath(installedPath, currentDirectory, ARRAYSIZE(currentDirectory))) {
        return FALSE;
    }

    ZeroMemory(&startupInfo, sizeof(startupInfo));
    ZeroMemory(&processInfo, sizeof(processInfo));
    startupInfo.cb = sizeof(startupInfo);
    if (!CreateProcessW(installedPath, commandLine, NULL, NULL, FALSE, 0, NULL, currentDirectory,
        &startupInfo, &processInfo)) {
        return FALSE;
    }
    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);
    return TRUE;
}

static InstallResult InstallForCurrentUserCore(void)
{
    WCHAR currentPath[MAX_PATH];
    WCHAR installedPath[MAX_PATH];

    LogInstallEvent(L"START", L"begin");
    if (!GetCurrentExePath(currentPath, ARRAYSIZE(currentPath)) ||
        !EnsureInstallDirectory(installedPath, ARRAYSIZE(installedPath)) ||
        !AppendPathPart(installedPath, ARRAYSIZE(installedPath), EXE_FILE_NAME)) {
        LogInstallEvent(L"FAILED", L"path");
        return INSTALL_RESULT_FAILED;
    }

    if (_wcsicmp(currentPath, installedPath) != 0) {
        if (!CloseInstancesForPath(installedPath) || !CloseInstancesForPath(currentPath)) {
            LogInstallEvent(L"FAILED", L"close");
            return INSTALL_RESULT_FAILED;
        }
        if (!CopyFileW(currentPath, installedPath, FALSE)) {
            LogInstallEvent(L"FAILED", L"copy");
            return INSTALL_RESULT_FAILED;
        }
        LogInstallEvent(L"OK", L"copy");
    }

    if (!FilesAreIdentical(currentPath, installedPath)) {
        LogInstallEvent(L"FAILED", L"verify");
        return INSTALL_RESULT_FAILED;
    }
    LogInstallEvent(L"OK", L"verify");
    CopyReleaseReadmesToInstallDir(currentPath, installedPath);

    if (!SetStartupRegistration(installedPath)) {
        LogInstallEvent(L"FAILED", L"startup");
        return INSTALL_RESULT_FAILED;
    }
    LogInstallEvent(L"OK", L"startup");

    if (_wcsicmp(currentPath, installedPath) == 0) {
        LogInstallEvent(L"OK", L"installed");
        return INSTALL_RESULT_INSTALLED;
    }

    if (!LaunchInstalledExeAfterExit(installedPath)) {
        LogInstallEvent(L"FAILED", L"launch");
        return INSTALL_RESULT_FAILED;
    }
    LogInstallEvent(L"OK", L"launch");

    return INSTALL_RESULT_INSTALLED_AND_LAUNCHED;
}

static void InstallForCurrentUser(void)
{
    InstallResult result = InstallForCurrentUserCore();

    if (result == INSTALL_RESULT_FAILED) {
        ShowNotification(APP_NAME, NotificationText(NOTIFY_TEXT_INSTALL_FAILED), FALSE);
        return;
    }
    if (result == INSTALL_RESULT_INSTALLED) {
        ShowNotification(APP_NAME, NotificationText(NOTIFY_TEXT_INSTALL_OK), FALSE);
        return;
    }

    LogInstallEvent(L"OK", L"exit-request");
    RequestAppExit();
}

static void RegisterCurrentExeForStartup(void)
{
    WCHAR currentPath[MAX_PATH];
    if (!GetCurrentExePath(currentPath, ARRAYSIZE(currentPath)) ||
        !SetStartupRegistration(currentPath)) {
        ShowNotification(APP_NAME, NotificationText(NOTIFY_TEXT_STARTUP_REGISTER_FAILED), FALSE);
        return;
    }
    ShowNotification(APP_NAME, NotificationText(NOTIFY_TEXT_STARTUP_REGISTER_OK), FALSE);
}

static void UnregisterStartup(void)
{
    if (!RemoveStartupRegistration()) {
        ShowNotification(APP_NAME, NotificationText(NOTIFY_TEXT_STARTUP_REMOVE_FAILED), FALSE);
        return;
    }
    ShowNotification(APP_NAME, NotificationText(NOTIFY_TEXT_STARTUP_REMOVE_OK), FALSE);
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

static NtpResult GetWindowsTimeOffset(NtpEvent event)
{
    NtpResult result;
    ZeroMemory(&result, sizeof(result));
    result.event = event;
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
    NtpQueryContext *context = (NtpQueryContext *)param;
    NtpResult *result = (NtpResult *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(NtpResult));
    if (result) {
        *result = GetWindowsTimeOffset(context ? context->event : NTP_EVENT_STARTUP);
    }
    if (context) {
        HeapFree(GetProcessHeap(), 0, context);
    }
    if (!PostMessageW(g_mainWnd, WM_NTP_DONE, 0, (LPARAM)result) && result) {
        HeapFree(GetProcessHeap(), 0, result);
    }
    return 0;
}

static BOOL SetLocalTimeFromNtp(void)
{
    NtpResult result = GetWindowsTimeOffset(NTP_EVENT_REFRESH);
    FILETIME utcFt;
    FILETIME localFt;
    SYSTEMTIME localSt;
    WCHAR offset[32];
    WCHAR details[512];
    const WCHAR *source = result.source[0] ? result.source : L"-";
    if (!result.success) {
        swprintf(details, ARRAYSIZE(details), L"error=ntp  source=%ls", source);
        LogSimpleEvent(L"adjust", L"FAILED", details);
        return FALSE;
    }
    utcFt = HnsToFileTime(GetUtcNowHns() + result.offsetHns);
    FileTimeToLocalFileTime(&utcFt, &localFt);
    FileTimeToSystemTime(&localFt, &localSt);
    FormatOffsetValue(result.offsetHns, offset, ARRAYSIZE(offset));
    if (SetLocalTime(&localSt)) {
        swprintf(details, ARRAYSIZE(details), L"offset=%ls  source=%ls", offset, source);
        LogSimpleEvent(L"adjust", L"OK", details);
        return TRUE;
    }
    swprintf(details, ARRAYSIZE(details), L"offset=%ls  error=set_time  source=%ls", offset, source);
    LogSimpleEvent(L"adjust", L"FAILED", details);
    return FALSE;
}

static void ApplyNtpResult(const NtpResult *result)
{
    BOOL notifySuccessIfNoDrift = g_notifyNtpSuccessIfNoDrift;
    g_notifyNtpSuccessIfNoDrift = FALSE;
    g_ntpQueryInProgress = FALSE;
    LogNtpResult(result ? result->event : NTP_EVENT_STARTUP, result);

    if (!result || !result->success) {
        g_clockOffsetHns = 0;
        g_ntpTimeAvailable = FALSE;
        g_clockSource[0] = L'\0';
        SetTrayNtpMenuText(NULL);
        g_adjustmentAvailable = FALSE;
        g_notifyPortableStartupIfNoDrift = FALSE;
        UpdateClockDisplays();
        ShowNotification(APP_NAME, NotificationText(NOTIFY_TEXT_NTP_FAILED), FALSE);
        return;
    }

    g_clockOffsetHns = result->offsetHns;
    g_ntpTimeAvailable = TRUE;
    wcscpy_s(g_clockSource, ARRAYSIZE(g_clockSource), result->source);
    UpdateClockDisplays();

    if (IsOffsetAtNotifyThreshold(result->offsetHns)) {
        SetTrayNtpMenuText(result->message);
        g_adjustmentAvailable = TRUE;
        g_notifyPortableStartupIfNoDrift = FALSE;
        ShowDriftNotification(result->offsetHns);
    } else {
        SetTrayNtpMenuText(NULL);
        g_adjustmentAvailable = FALSE;
        if (notifySuccessIfNoDrift) {
            ShowManualRefreshSuccessNotification();
        } else if (result->event == NTP_EVENT_STARTUP && g_notifyPortableStartupIfNoDrift) {
            g_notifyPortableStartupIfNoDrift = FALSE;
            ShowNotification(APP_NAME, NotificationText(NOTIFY_TEXT_PORTABLE_STARTUP_OK), FALSE);
        }
    }
}

static void StartNtpLoad(NtpEvent event, BOOL notifySuccessIfNoDrift)
{
    HANDLE thread;
    NtpQueryContext *context;
    if (g_ntpQueryInProgress) {
        return;
    }

    g_ntpQueryInProgress = TRUE;
    g_notifyNtpSuccessIfNoDrift = notifySuccessIfNoDrift;
    g_adjustmentAvailable = FALSE;
    SetTrayNtpMenuText(NULL);
    context = (NtpQueryContext *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(NtpQueryContext));
    if (!context) {
        g_ntpQueryInProgress = FALSE;
        g_notifyNtpSuccessIfNoDrift = FALSE;
        SetTrayNtpMenuText(NULL);
        LogSimpleEvent(NtpEventName(event), L"FAILED", L"error=memory");
        return;
    }
    context->event = event;
    thread = CreateThread(NULL, 0, NtpThreadProc, context, 0, NULL);
    if (thread) {
        CloseHandle(thread);
    } else {
        HeapFree(GetProcessHeap(), 0, context);
        g_ntpQueryInProgress = FALSE;
        g_notifyNtpSuccessIfNoDrift = FALSE;
        SetTrayNtpMenuText(NULL);
        LogSimpleEvent(NtpEventName(event), L"FAILED", L"error=thread");
    }
}

static BOOL IsShiftPressed(void)
{
    return (GetKeyState(VK_SHIFT) & 0x8000) != 0;
}

static BOOL IsAdvancedMenuRequested(void)
{
    return IsShiftPressed();
}

static BOOL CanForceAdjustmentFromMenu(BOOL advancedMenuRequested)
{
    return advancedMenuRequested && !g_ntpQueryInProgress;
}

static BOOL CanShowAdjustmentMenu(BOOL advancedMenuRequested)
{
    return CanAdjustFromNormalMenu() || CanForceAdjustmentFromMenu(advancedMenuRequested);
}

static BOOL OpenLogFolderWithShell(void)
{
    WCHAR path[MAX_PATH];

    if (!BuildLogPath(path, ARRAYSIZE(path))) {
        return FALSE;
    }
    StripFileName(path);
    return (INT_PTR)ShellExecuteW(NULL, L"open", path, NULL, NULL, SW_SHOWNORMAL) > 32;
}

static BOOL RunOpenLogFolderHelper(void)
{
    WCHAR exePath[MAX_PATH];
    WCHAR currentDirectory[MAX_PATH];
    WCHAR commandLine[MAX_PATH + 64];
    STARTUPINFOW startupInfo;
    PROCESS_INFORMATION processInfo;
    DWORD exitCode = 1;

    if (!GetCurrentExePath(exePath, ARRAYSIZE(exePath)) ||
        !GetDirectoryFromPath(exePath, currentDirectory, ARRAYSIZE(currentDirectory)) ||
        swprintf(commandLine, ARRAYSIZE(commandLine), L"\"%ls\" %ls", exePath, OPEN_LOG_FOLDER_ARGUMENT) < 0) {
        return FALSE;
    }

    ZeroMemory(&startupInfo, sizeof(startupInfo));
    ZeroMemory(&processInfo, sizeof(processInfo));
    startupInfo.cb = sizeof(startupInfo);
    if (!CreateProcessW(exePath, commandLine, NULL, NULL, FALSE, 0, NULL, currentDirectory,
        &startupInfo, &processInfo)) {
        return FALSE;
    }

    WaitForSingleObject(processInfo.hProcess, INFINITE);
    GetExitCodeProcess(processInfo.hProcess, &exitCode);
    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);
    return exitCode == 0;
}

static void OpenLogFolder(void)
{
    if (!RunOpenLogFolderHelper()) {
        ShowNotification(APP_NAME, NotificationText(NOTIFY_TEXT_OPEN_LOG_FAILED), FALSE);
    }
}

static void AppendTrayRefreshMenuItem(HMENU menu)
{
    UINT flags = g_ntpQueryInProgress ? MF_STRING | MF_DISABLED : MF_STRING;
    AppendMenuW(menu, flags, ID_TRAY_REFRESH_NTP, g_statusText);
}

static void AppendThresholdItem(HMENU menu, UINT id, int seconds, const WCHAR *text)
{
    NotifyThreshold threshold = { FALSE, seconds };
    UINT flags = MF_STRING;
    if (NotifyThresholdEquals(&g_notifyThreshold, &threshold)) {
        flags |= MF_CHECKED;
    }
    AppendMenuW(menu, flags, id, text);
}

static void AppendNotifyThresholdMenu(HMENU menu)
{
    HMENU thresholdMenu = CreatePopupMenu();
    UINT offFlags = MF_STRING;

    if (!thresholdMenu) {
        return;
    }
    AppendThresholdItem(thresholdMenu, ID_TRAY_THRESHOLD_1, 1, L"1 sec");
    AppendThresholdItem(thresholdMenu, ID_TRAY_THRESHOLD_3, 3, L"3 sec");
    AppendThresholdItem(thresholdMenu, ID_TRAY_THRESHOLD_5, 5, L"5 sec");
    AppendThresholdItem(thresholdMenu, ID_TRAY_THRESHOLD_7, 7, L"7 sec");
    AppendThresholdItem(thresholdMenu, ID_TRAY_THRESHOLD_10, 10, L"10 sec");
    AppendMenuW(thresholdMenu, MF_SEPARATOR, 0, NULL);
    if (g_notifyThreshold.off) {
        offFlags |= MF_CHECKED;
    }
    AppendMenuW(thresholdMenu, offFlags, ID_TRAY_THRESHOLD_OFF, L"Off");
    AppendMenuW(menu, MF_POPUP, (UINT_PTR)thresholdMenu, L"Notify threshold");
}

static BOOL NotifyThresholdFromCommandId(UINT id, NotifyThreshold *threshold)
{
    threshold->off = FALSE;
    switch (id) {
    case ID_TRAY_THRESHOLD_1:
        threshold->seconds = 1;
        return TRUE;
    case ID_TRAY_THRESHOLD_3:
        threshold->seconds = 3;
        return TRUE;
    case ID_TRAY_THRESHOLD_5:
        threshold->seconds = 5;
        return TRUE;
    case ID_TRAY_THRESHOLD_7:
        threshold->seconds = 7;
        return TRUE;
    case ID_TRAY_THRESHOLD_10:
        threshold->seconds = 10;
        return TRUE;
    case ID_TRAY_THRESHOLD_OFF:
        threshold->off = TRUE;
        threshold->seconds = 0;
        return TRUE;
    default:
        threshold->seconds = 0;
        return FALSE;
    }
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

static HMENU CreateTrayMenu(BOOL advancedMenuRequested)
{
    HMENU menu = CreatePopupMenu();
    HMENU startupMenu = CreatePopupMenu();
    BOOL startupRegistered = IsStartupRegistered();
    BOOL canRegisterCurrentExe = CanRegisterCurrentExeForStartup();
    BOOL canInstallForCurrentUser = CanInstallForCurrentUser();
    BOOL adjustmentMenuAvailable = CanShowAdjustmentMenu(advancedMenuRequested);
    if (g_adjustmentAvailable) {
        AppendMenuW(menu, MF_OWNERDRAW | MF_DISABLED, ID_TRAY_STATUS, NULL);
        AppendMenuW(menu, MF_STRING, ID_TRAY_ADJUST, L"Adjust Windows time (admin)");
        if (advancedMenuRequested) {
            AppendMenuW(menu, MF_STRING, ID_OPEN_LOG_FOLDER, L"Open log folder");
        }
        AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    } else {
        AppendTrayRefreshMenuItem(menu);
        if (adjustmentMenuAvailable) {
            AppendMenuW(menu, MF_STRING, ID_TRAY_ADJUST, L"Adjust Windows time (admin)");
        }
        if (advancedMenuRequested) {
            AppendMenuW(menu, MF_STRING, ID_OPEN_LOG_FOLDER, L"Open log folder");
        }
        AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    }
    AppendNotifyThresholdMenu(menu);
    AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    if (startupMenu) {
        AppendMenuW(startupMenu, canInstallForCurrentUser ? MF_STRING : MF_STRING | MF_GRAYED,
            ID_TRAY_INSTALL_USER, L"Install for this user");
        AppendMenuW(startupMenu, canRegisterCurrentExe ? MF_STRING : MF_STRING | MF_GRAYED,
            ID_TRAY_STARTUP_CURRENT, L"Add this EXE to startup");
        AppendMenuW(startupMenu, startupRegistered ? MF_STRING : MF_STRING | MF_GRAYED,
            ID_TRAY_STARTUP_REMOVE, L"Remove startup registration");
        AppendMenuW(menu, MF_POPUP, (UINT_PTR)startupMenu, L"Startup");
        AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    }
    AppendMenuW(menu, MF_STRING, ID_TRAY_EXIT, L"Exit");
    return menu;
}

static HMENU CreatePopupMenuForClock(BOOL advancedMenuRequested)
{
    HMENU menu = CreatePopupMenu();
    BOOL adjustmentMenuAvailable = CanShowAdjustmentMenu(advancedMenuRequested);
    if (adjustmentMenuAvailable) {
        AppendMenuW(menu, MF_STRING, ID_POPUP_ADJUST, L"Adjust Windows time (admin)");
    }
    if (advancedMenuRequested) {
        AppendMenuW(menu, MF_STRING, ID_OPEN_LOG_FOLDER, L"Open log folder");
    }
    if (adjustmentMenuAvailable || advancedMenuRequested) {
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
    HMENU menu = CreateTrayMenu(IsAdvancedMenuRequested());
    g_lastTrayMenuTick = GetTickCount();
    GetCursorPos(&pt);
    ShowContextMenu(menu, g_mainWnd, pt);
    RestorePopupHiddenForTrayMenu();
    DestroyMenu(menu);
}

static void ShowClockMenu(HWND hwnd)
{
    POINT pt;
    HMENU menu = CreatePopupMenuForClock(IsAdvancedMenuRequested());
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
    wc.lpszClassName = MAIN_WINDOW_CLASS;
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
        if (g_notifyInstallSuccessOnStartup) {
            g_notifyInstallSuccessOnStartup = FALSE;
            ShowNotification(APP_NAME, NotificationText(NOTIFY_TEXT_INSTALL_OK), FALSE);
        }
        ScheduleNextTick();
        StartNtpLoad(NTP_EVENT_STARTUP, FALSE);
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
    {
        UINT commandId = LOWORD(wParam);
        NotifyThreshold threshold;
        if (NotifyThresholdFromCommandId(commandId, &threshold)) {
            SetNotifyThreshold(threshold);
            return 0;
        }
        switch (commandId) {
        case ID_TRAY_EXIT:
        case ID_POPUP_EXIT:
            RequestAppExit();
            return 0;
        case ID_TRAY_ADJUST:
        case ID_POPUP_ADJUST:
            RunElevatedAdjustment();
            return 0;
        case ID_TRAY_REFRESH_NTP:
            StartNtpLoad(NTP_EVENT_REFRESH, TRUE);
            return 0;
        case ID_TRAY_INSTALL_USER:
            InstallForCurrentUser();
            return 0;
        case ID_TRAY_STARTUP_CURRENT:
            RegisterCurrentExeForStartup();
            return 0;
        case ID_TRAY_STARTUP_REMOVE:
            UnregisterStartup();
            return 0;
        case ID_OPEN_LOG_FOLDER:
            OpenLogFolder();
            return 0;
        case ID_POPUP_CLOSE:
            HidePopup();
            return 0;
        }
        break;
    }

    case WM_ENTERMENULOOP:
        g_contextMenuOpen = TRUE;
        if (g_popupHoverPreview) {
            HidePopup();
        }
        return 0;

    case WM_EXITMENULOOP:
        g_contextMenuOpen = FALSE;
        ShowPendingThresholdDriftNotification();
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
        if (wParam == WTS_SESSION_LOGON) {
            StartNtpLoad(NTP_EVENT_LOGON, FALSE);
        } else if (wParam == WTS_SESSION_UNLOCK) {
            StartNtpLoad(NTP_EVENT_UNLOCK, FALSE);
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

static BOOL CommandLineHasOnlyArgument(PWSTR cmdLine, const WCHAR *argument)
{
    size_t len = wcslen(argument);
    while (*cmdLine == L' ' || *cmdLine == L'\t') {
        cmdLine++;
    }
    if (wcsncmp(cmdLine, argument, len) != 0) {
        return FALSE;
    }
    cmdLine += len;
    while (*cmdLine == L' ' || *cmdLine == L'\t') {
        cmdLine++;
    }
    return *cmdLine == L'\0';
}

static void WaitForParentIfRequested(PWSTR cmdLine)
{
    DWORD parentPid = 0;
    const WCHAR *cursor;
    HANDLE parent;
    size_t prefixLen = wcslen(WAIT_PARENT_ARGUMENT);

    if (wcsncmp(cmdLine, WAIT_PARENT_ARGUMENT, prefixLen) != 0 ||
        (cmdLine[prefixLen] != L'\0' && cmdLine[prefixLen] != L' ' && cmdLine[prefixLen] != L'\t')) {
        return;
    }

    cursor = cmdLine + prefixLen;
    while (*cursor == L' ' || *cursor == L'\t') {
        cursor++;
    }
    while (*cursor >= L'0' && *cursor <= L'9') {
        parentPid = parentPid * 10 + (DWORD)(*cursor - L'0');
        cursor++;
    }
    if (parentPid == 0) {
        return;
    }

    parent = OpenProcess(SYNCHRONIZE, FALSE, parentPid);
    if (parent) {
        WaitForSingleObject(parent, INFINITE);
        CloseHandle(parent);
        WCHAR currentPath[MAX_PATH];
        WCHAR installedPath[MAX_PATH];
        g_notifyInstallSuccessOnStartup =
            GetCurrentExePath(currentPath, ARRAYSIZE(currentPath)) &&
            BuildInstalledExePath(installedPath, ARRAYSIZE(installedPath)) &&
            _wcsicmp(currentPath, installedPath) == 0;
    }
}

static BOOL CloseExistingInstancesForPortableRun(void)
{
    WCHAR currentPath[MAX_PATH];
    WCHAR installedPath[MAX_PATH];
    if (!GetCurrentExePath(currentPath, ARRAYSIZE(currentPath)) ||
        !BuildInstalledExePath(installedPath, ARRAYSIZE(installedPath))) {
        return FALSE;
    }
    return CloseInstancesForPath(installedPath) && CloseInstancesForPath(currentPath);
}

static BOOL HandleInstallPromptIfRequested(PWSTR cmdLine)
{
    int choice;
    InstallResult result;

    if (!CommandLineHasOnlyArgument(cmdLine, INSTALL_PROMPT_ARGUMENT) || !ShouldPromptForInstall()) {
        return TRUE;
    }

    choice = MessageBoxW(NULL,
        L"An installed TrayClockTooltip EXE already exists.\n\n"
        L"Yes: install this EXE and restart from the installed location\n"
        L"No: close existing app instances and run this EXE as portable\n"
        L"Cancel: do not start",
        APP_NAME,
        MB_ICONQUESTION | MB_YESNOCANCEL | MB_DEFBUTTON1 | MB_SETFOREGROUND | MB_TOPMOST);

    if (choice == IDNO) {
        if (!CloseExistingInstancesForPortableRun()) {
            MessageBoxW(NULL, L"Could not close the running app.", APP_NAME, MB_ICONERROR | MB_OK);
            return FALSE;
        }
        return TRUE;
    }
    if (choice != IDYES) {
        return FALSE;
    }

    result = InstallForCurrentUserCore();
    if (result == INSTALL_RESULT_INSTALLED_AND_LAUNCHED) {
        return FALSE;
    }
    if (result == INSTALL_RESULT_INSTALLED) {
        return TRUE;
    }

    MessageBoxW(NULL, L"Could not install for this user.", APP_NAME, MB_ICONERROR | MB_OK);
    return FALSE;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR cmdLine, int nCmdShow)
{
    HANDLE mutex;
    WSADATA wsa;
    MSG msg;
    (void)hPrevInstance;
    (void)nCmdShow;

    if (CommandLineHasOnlyArgument(cmdLine, ADJUST_ARGUMENT)) {
        int exitCode = 1;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) == 0) {
            exitCode = SetLocalTimeFromNtp() ? 0 : 1;
            WSACleanup();
        }
        return exitCode;
    }
    if (CommandLineHasOnlyArgument(cmdLine, OPEN_LOG_FOLDER_ARGUMENT)) {
        return OpenLogFolderWithShell() ? 0 : 1;
    }

    InitializeNotifyThreshold(cmdLine);
    WaitForParentIfRequested(cmdLine);
    if (!HandleInstallPromptIfRequested(cmdLine)) {
        return 0;
    }
    g_notifyPortableStartupIfNoDrift = ShouldNotifyPortableStartup();

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
