// -*- coding: UTF-8 -*-

#include <napi.h>
#include <windows.h>
#include <vector>
#include <iostream>
#include <regex>
#include <chrono>

std::vector<HWND> chromeWindows;
int screenWidth, screenHeight;
HWND master;
std::vector<HWND> slaves;
HWND masterDocument;
std::vector<HWND> slaveDocuments;
HHOOK g_hMouseHook;

struct Params
{
  std::wregex titlePattern;
  std::wregex classPattern;
  std::vector<HWND> &windows;
};

BOOL CALLBACK FindWindowsByRegexCallBack(HWND hwnd, LPARAM lParam)
{
  const int SIZE = 1024;
  WCHAR windowTitle[SIZE];
  GetWindowTextW(hwnd, windowTitle, SIZE);

  WCHAR className[SIZE];
  GetClassNameW(hwnd, className, sizeof(className) / sizeof(WCHAR));

  Params *params = reinterpret_cast<Params *>(lParam);
  bool visble = IsWindowVisible(hwnd);
  bool titleMatch = std::regex_match(windowTitle, params->titlePattern);
  bool classNameMatch = std::regex_match(className, params->classPattern);
  if (!visble || !titleMatch || !classNameMatch)
  {
    return TRUE;
  }
  params->windows.push_back(hwnd);
  return TRUE;
}

void debounce(std::function<void()> func, int interval)
{
  static std::chrono::steady_clock::time_point last_call = std::chrono::steady_clock::now();
  auto now = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_call);
  if (duration.count() > interval)
  {
    func();
    last_call = now;
  }
}
std::wstring GetWindowTitle(HWND hwnd)
{
  const int SIZE = 1024;
  WCHAR windowTitle[SIZE];
  GetWindowTextW(hwnd, windowTitle, SIZE);
  return windowTitle;
}

std::wstring GetWindowClassName(HWND hwnd)
{
  const int SIZE = 1024;
  WCHAR className[SIZE];
  GetClassNameW(hwnd, className, sizeof(className) / sizeof(WCHAR));
  return className;
}

std::vector<HWND> FindWindowsByRegex(const std::wstring &titleRegex = L".*", const std::wstring &classRegex = L".*", HWND parentHwnd = NULL)
{
  std::vector<HWND> windows;
  Params params = {std::wregex(titleRegex), std::wregex(classRegex), windows};

  if (parentHwnd != NULL)
  {
    EnumChildWindows(parentHwnd, FindWindowsByRegexCallBack, reinterpret_cast<LPARAM>(&params));
  }
  else
  {
    EnumWindows(FindWindowsByRegexCallBack, reinterpret_cast<LPARAM>(&params));
  }

  return windows;
}

void TileWindows()
{

  chromeWindows.clear();

  screenWidth = GetSystemMetrics(SM_CXSCREEN);
  screenHeight = GetSystemMetrics(SM_CYSCREEN);

  chromeWindows = FindWindowsByRegex(L".*By ChromePower.*");

  int numWindows = chromeWindows.size();

  if (numWindows == 0)
  {
    return;
  }

  if (numWindows == 1)
  {

    ShowWindow(chromeWindows[0], SW_MAXIMIZE);
    SetForegroundWindow(chromeWindows[0]);
    return;
  }

  int rows = ceil(sqrt(numWindows));
  int cols = ceil((double)numWindows / rows);

  int windowWidth = screenWidth / cols;
  int windowHeight = screenHeight / rows;

  for (size_t i = 0; i < chromeWindows.size(); i++)
  {

    int col = i % cols;
    int row = i / cols;

    MoveWindow(chromeWindows[i], col * windowWidth, row * windowHeight, windowWidth, windowHeight, TRUE);
    SetForegroundWindow(chromeWindows[i]);
  };
}
void ReplayEventInSlaveWindows(HWND masterWindow, std::vector<HWND> slaveWindows, MSLLHOOKSTRUCT *mouseInfo, WPARAM event)
{
  POINT pt = mouseInfo->pt;
  ScreenToClient(masterWindow, &pt);
  for (HWND slave : slaveWindows)
  {
    if (event == WM_LBUTTONDOWN)
    {
      SendMessage(slave, WM_LBUTTONDOWN, 0, MAKELPARAM(pt.x, pt.y));
    }
    else if (event == WM_LBUTTONUP)
    {
      SendMessage(slave, WM_LBUTTONUP, 0, MAKELPARAM(pt.x, pt.y));
    }
    else if (event == WM_RBUTTONDOWN)
    {
      SendMessage(slave, WM_RBUTTONDOWN, 0, MAKELPARAM(pt.x, pt.y));
    }
    else if (event == WM_RBUTTONUP)
    {
      SendMessage(slave, WM_RBUTTONUP, 0, MAKELPARAM(pt.x, pt.y));
    }
    else if (event == WM_MOUSEMOVE)
    {
      debounce([slave, pt]()
               { SendMessage(slave, WM_MOUSEMOVE, 0, MAKELPARAM(pt.x, pt.y)); },
               100);
    }
    else if (event == WM_MOUSEWHEEL)
    {
      int wheelDelta = GET_WHEEL_DELTA_WPARAM(mouseInfo->mouseData);
      std::wstring currentTitle = GetWindowTitle(slave);
      std::wcout << currentTitle << std::endl;
      std::cout << wheelDelta << std::endl;
      debounce([slave, wheelDelta, pt]()
               {
                bool result = SendMessage(slave, WM_MOUSEWHEEL, MAKEWPARAM(0, wheelDelta), MAKELPARAM(pt.x, pt.y));
                std::wcout << result << std::endl; },
               100);
    }
  }
}
LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
  if (nCode >= 0)
  {
    if (wParam == WM_LBUTTONDOWN ||
        wParam == WM_RBUTTONDOWN ||
        wParam == WM_LBUTTONUP ||
        wParam == WM_RBUTTONUP ||
        wParam == WM_MOUSEMOVE ||
        wParam == WM_MOUSEWHEEL)
    {
      //! 获取鼠标事件信息
      MSLLHOOKSTRUCT *mouseInfo = reinterpret_cast<MSLLHOOKSTRUCT *>(lParam);
      if (mouseInfo != NULL)
      {
        HWND hwnd = WindowFromPoint(mouseInfo->pt);
        //! 如果是主窗口，则将事件转发到从窗口
        if (hwnd == master)
        {
          ReplayEventInSlaveWindows(master, slaves, mouseInfo, wParam);
        }
        std::wstring currentWindowClassName = GetWindowClassName(hwnd);
        bool isMasterChild = GetParent(hwnd) == master;
        if (currentWindowClassName == L"Chrome_RenderWidgetHostHWND" && isMasterChild)
        {
          // ! 获取主文档窗口
          masterDocument = hwnd;
          slaveDocuments.clear();
          for (size_t i = 0; i < slaves.size(); i++)
          {
            HWND slave = slaves[i];
            std::vector<HWND> slaveWindows = FindWindowsByRegex(L".*", L"Chrome_RenderWidgetHostHWND", slave);
            if (slaveWindows.size() > 0)
            {
              slaveDocuments.push_back(slaveWindows[0]);
            }
          }
          ReplayEventInSlaveWindows(masterDocument, slaveDocuments, mouseInfo, wParam);
        }
      }
    }
  }
  return CallNextHookEx(NULL, nCode, wParam, lParam);
}
void UninstallHook()
{
  if (g_hMouseHook != NULL)
  {
    UnhookWindowsHookEx(g_hMouseHook);
    g_hMouseHook = NULL;
  }
}

void StartGroupControl()
{
  chromeWindows.clear();
  chromeWindows = FindWindowsByRegex(L".*By ChromePower.*", L".*Chrome_WidgetWin_1.*", NULL);
  int numWindows = chromeWindows.size();
  if (numWindows == 0)
  {
    return;
  }
  if (numWindows == 1)
  {
    SetForegroundWindow(chromeWindows[0]);
    return;
  }

  master = chromeWindows[0];

  slaves.clear();
  for (size_t i = 1; i < chromeWindows.size(); i++)
  {
    slaves.push_back(chromeWindows[i]);
  }

  g_hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, NULL, 0);
}

Napi::Object TileChromeWindows(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();
  Napi::Object result = Napi::Object::New(env);
  try
  {
    TileWindows();
    result.Set("success", true);
    result.Set("message", "Windows tiled successfully");
    return result;
  }
  catch (const std::exception &e)
  {

    result.Set("success", false);
    result.Set("message", e.what());
    return result;
  }
}

Napi::Object startGroupControl(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();
  Napi::Object result = Napi::Object::New(env);
  try
  {
    StartGroupControl();
    result.Set("success", true);
    result.Set("message", "Group control started successfully");
    return result;
  }
  catch (const std::exception &e)
  {

    result.Set("success", false);
    result.Set("message", e.what());
    return result;
  }
}

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
  exports.Set("tileChromeWindows", Napi::Function::New(env, TileChromeWindows));
  exports.Set("startGroupControl", Napi::Function::New(env, startGroupControl));
  return exports;
}

NODE_API_MODULE(windowAddon, Init)
