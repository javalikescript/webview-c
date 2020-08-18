#include <stddef.h>
//#include <stdio.h>

#define WEBVIEW2_WIN32_API extern

typedef void (*WebView2CallbackFn) (webview2 *wv, const char *message, void *context);

typedef HRESULT (*GetWebView2BrowserVersionInfoFnType) (PCWSTR browserExecutableFolder, LPWSTR* versionInfo);
typedef HRESULT (*CreateCoreWebView2EnvironmentWithDetailsFnType) (PCWSTR browserExecutableFolder, PCWSTR userDataFolder, PCWSTR additionalBrowserArguments, ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler* environment_created_handler);

typedef struct webview2_struct {
  HWND hwnd;
  const char *url;
  WebView2CallbackFn cb;
  void *context;
  ICoreWebView2 *webview;
  ICoreWebView2Host *host;
  ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler env_created_handler;
  ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandlerVtbl env_created_handler_vtbl;
  ICoreWebView2CreateCoreWebView2HostCompletedHandler host_created_handler;
  ICoreWebView2CreateCoreWebView2HostCompletedHandlerVtbl host_created_handler_vtbl;
  ICoreWebView2WebMessageReceivedEventHandler message_received_event_handler;
  ICoreWebView2WebMessageReceivedEventHandlerVtbl message_received_event_handler_vtbl;
} webview2;

#define WEBVIEW2_PTR_FROM(_cp, _field) \
	((webview2 *) ((char *) (_cp) - offsetof(webview2, _field)))

static CreateCoreWebView2EnvironmentWithDetailsFnType CreateCoreWebView2EnvironmentFn = NULL;

static HRESULT WebView2WebMessageReceivedEventHandleInvoke(ICoreWebView2WebMessageReceivedEventHandler * This, ICoreWebView2 *webView, ICoreWebView2WebMessageReceivedEventArgs *args) {
  LPWSTR webMessage;
  HRESULT hRes;
  webview2 *pwv2 = WEBVIEW2_PTR_FROM(This, message_received_event_handler);
  hRes = args->lpVtbl->TryGetWebMessageAsString(args, &webMessage);
  if (hRes == E_INVALIDARG) {
    hRes = args->lpVtbl->get_WebMessageAsJson(args, &webMessage);
  }
  if (pwv2->cb != NULL) {
    char *message = webview_from_utf16(webMessage);
    if (message != NULL) {
      pwv2->cb(pwv2, message, pwv2->context);
      GlobalFree(message);
    }
  }
  return S_OK;
}

static HRESULT CreateWebView2Controller_Invoke(ICoreWebView2CreateCoreWebView2HostCompletedHandler * This, HRESULT result, ICoreWebView2Host *webViewHost) {
  webview2 *pwv2 = WEBVIEW2_PTR_FROM(This, host_created_handler);
  ICoreWebView2 *webview;
  webViewHost->lpVtbl->AddRef(webViewHost);
  pwv2->host = webViewHost;

  webViewHost->lpVtbl->get_CoreWebView2(webViewHost, &webview);
  pwv2->webview = webview;

  RECT bounds;
  GetClientRect(pwv2->hwnd, &bounds);
  webViewHost->lpVtbl->put_Bounds(webViewHost, bounds);

  webview->lpVtbl->AddScriptToExecuteOnDocumentCreated(webview, L"window.external={invoke:s=>window.chrome.webview.postMessage(s)}", NULL);

  EventRegistrationToken token;
  webview->lpVtbl->add_WebMessageReceived(webview, &pwv2->message_received_event_handler, &token);

  if ((pwv2->url != NULL) && (strlen(pwv2->url) > 0)) {
    wchar_t *wurl = webview_to_utf16(pwv2->url);
    if (wurl != NULL) {
      webview->lpVtbl->Navigate(webview, wurl);
      GlobalFree(wurl);
    }
  }
  return S_OK;
}

static HRESULT CreateWebView2Environment_Invoke(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler * This, HRESULT result, ICoreWebView2Environment * webViewEnvironment) {
  webview2 *pwv2 = WEBVIEW2_PTR_FROM(This, env_created_handler);
  webViewEnvironment->lpVtbl->CreateCoreWebView2Host(webViewEnvironment, pwv2->hwnd, &pwv2->host_created_handler);
  return S_OK;
}

static HRESULT NoOpQueryInterface(void * This, REFIID riid, void **ppvObject) {
  return S_OK;
}
static ULONG NoOpAddRef(void * This) {
  return 1;
}
static ULONG NoOpRelease(void * This) {
  return 1;
}

static void InitWebView2(webview2 *pwv2) {
  pwv2->env_created_handler_vtbl.QueryInterface = (HRESULT (*)(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler * This, REFIID riid, void **ppvObject)) &NoOpQueryInterface;
  pwv2->env_created_handler_vtbl.AddRef = (ULONG (*)(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler * This)) &NoOpAddRef;
  pwv2->env_created_handler_vtbl.Release = (ULONG (*)(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler * This)) &NoOpRelease;
  pwv2->env_created_handler_vtbl.Invoke = &CreateWebView2Environment_Invoke;
  pwv2->env_created_handler.lpVtbl = &pwv2->env_created_handler_vtbl;
  pwv2->host_created_handler_vtbl.QueryInterface = (HRESULT (*)(ICoreWebView2CreateCoreWebView2HostCompletedHandler * This, REFIID riid, void **ppvObject)) &NoOpQueryInterface;
  pwv2->host_created_handler_vtbl.AddRef = (ULONG (*)(ICoreWebView2CreateCoreWebView2HostCompletedHandler * This)) &NoOpAddRef;
  pwv2->host_created_handler_vtbl.Release = (ULONG (*)(ICoreWebView2CreateCoreWebView2HostCompletedHandler * This)) &NoOpRelease;
  pwv2->host_created_handler_vtbl.Invoke = &CreateWebView2Controller_Invoke;
  pwv2->host_created_handler.lpVtbl = &pwv2->host_created_handler_vtbl;
  pwv2->message_received_event_handler_vtbl.QueryInterface = (HRESULT (*)(ICoreWebView2WebMessageReceivedEventHandler * This, REFIID riid, void **ppvObject)) &NoOpQueryInterface;
  pwv2->message_received_event_handler_vtbl.AddRef = (ULONG (*)(ICoreWebView2WebMessageReceivedEventHandler * This)) &NoOpAddRef;
  pwv2->message_received_event_handler_vtbl.Release = (ULONG (*)(ICoreWebView2WebMessageReceivedEventHandler * This)) &NoOpRelease;
  pwv2->message_received_event_handler_vtbl.Invoke = &WebView2WebMessageReceivedEventHandleInvoke;
  pwv2->message_received_event_handler.lpVtbl = &pwv2->message_received_event_handler_vtbl;
}

static WCHAR *getUserData(WCHAR *buffer, size_t sizeOfBuffer) {
  WCHAR filename[MAX_PATH];
  WCHAR *appData = _wgetenv(L"APPDATA");
  char *noAppData = getenv("WEBVIEW2_WIN32_USER_DATA_NO_APPDATA");
  if ((getenv("WEBVIEW2_USER_DATA_FOLDER") == NULL) && (appData != NULL) && (noAppData == NULL)) {
    GetModuleFileNameW(NULL, filename, MAX_PATH);
    WCHAR *executableName = wcsrchr(filename, L'\\');
    if (executableName == NULL) {
      executableName = filename;
    }
    swprintf(buffer, sizeOfBuffer, L"%s\\%s.WebView2", appData, executableName);
    return buffer;
  }
  return NULL;
}

WEBVIEW2_WIN32_API webview2 * CreateWebView2(HWND hwnd, const char *url) {
  WCHAR dirname[MAX_PATH];
  webview2 *pwv2 = NULL;
  if (CreateCoreWebView2EnvironmentFn != NULL) {
    pwv2 = (webview2 *)GlobalAlloc(GMEM_FIXED, sizeof(webview2));
    if (pwv2 != NULL) {
      InitWebView2(pwv2);
      pwv2->webview = NULL;
      pwv2->hwnd = hwnd;
      pwv2->url = url;
      PCWSTR userDataFolder = getUserData(dirname, MAX_PATH);
      HRESULT res = CreateCoreWebView2EnvironmentFn(NULL, userDataFolder, NULL, &pwv2->env_created_handler);
      if (res != S_OK) {
        GlobalFree(pwv2);
        pwv2 = NULL;
      }
    }
  }
  return pwv2;
}

WEBVIEW2_WIN32_API void ReleaseWebView2(webview2 *pwv2) {
  if (pwv2 != NULL) {
    GlobalFree(pwv2);
  }
}

WEBVIEW2_WIN32_API void WebView2SetBounds(webview2 *pwv2, RECT bounds) {
  if ((pwv2 != NULL) && (pwv2->host != NULL) && (pwv2->webview != NULL)) {
    pwv2->host->lpVtbl->put_Bounds(pwv2->host, bounds);
  }
}

WEBVIEW2_WIN32_API void WebView2RegisterCallback(webview2 *pwv2, WebView2CallbackFn cb, void *context) {
  if (pwv2 != NULL) {
    pwv2->cb = cb;
    pwv2->context = context;
  }
}

static char * formatJavaScriptForEval(const char *js) {
  static const char *prologue = "(function(){";
  static const char *epilogue = ";})();";
  int n = strlen(prologue) + strlen(epilogue) + strlen(js) + 1;
  char *fjs = (char *)malloc(n);
  if (fjs != NULL) {
    snprintf(fjs, n, "%s%s%s", prologue, js, epilogue);
  }
  return fjs;
}

WEBVIEW2_WIN32_API int WebView2Eval(webview2 *pwv2, const char *js) {
  if (pwv2->webview != NULL) {
    char *fjs = formatJavaScriptForEval(js);
    if (fjs != NULL) {
      wchar_t *wjs = webview_to_utf16(fjs);
      free(fjs);
      if (wjs != NULL) {
        ICoreWebView2ExecuteScriptCompletedHandler *handler = NULL;
        pwv2->webview->lpVtbl->ExecuteScript(pwv2->webview, wjs, handler);
        GlobalFree(wjs);
        return 0;
      }
    }
  }
  return 1;
}

#define WEBVIEW2_BROWSER_EXECUTABLE_FOLDER "WEBVIEW2_BROWSER_EXECUTABLE_FOLDER"
#define WEBVIEW2_DISABLE_AUTO_DETECT "WEBVIEW2_DISABLE_AUTO_DETECT"
#define WEBVIEW2_WIN32_PATH "WEBVIEW2_WIN32_PATH"

#define KEY_MS_EDGEUPDATE_CLIENTSTATE "SOFTWARE\\WOW6432Node\\Microsoft\\EdgeUpdate\\ClientState\\{56EB18F8-B008-4CBD-B6D2-8C97FE7E9062}"
#define KEY_EBWEBVIEW "EBWebView"

static int directoryExists(LPCSTR lpFileName) {
    DWORD attributes = GetFileAttributes(lpFileName);
    return (attributes != INVALID_FILE_ATTRIBUTES) && (attributes & FILE_ATTRIBUTE_DIRECTORY);
}

static int fileExists(LPCSTR lpFileName) {
    DWORD attributes = GetFileAttributes(lpFileName);
    return (attributes != INVALID_FILE_ATTRIBUTES) && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
}

static void findWebView2BrowserExecutableFolder() {
  char data[MAX_PATH];
  char path[MAX_PATH];
  char *name;
  DWORD size;
  WIN32_FIND_DATA findFileData;
  HANDLE findFileHandle;
  if ((getenv(WEBVIEW2_BROWSER_EXECUTABLE_FOLDER) != NULL) || (getenv(WEBVIEW2_DISABLE_AUTO_DETECT) != NULL)) {
    return;
  }
  size = sizeof(data);
  if (RegGetValueA(HKEY_LOCAL_MACHINE, KEY_MS_EDGEUPDATE_CLIENTSTATE, KEY_EBWEBVIEW, RRF_RT_REG_SZ, NULL, &data, &size) == ERROR_SUCCESS) {
    if (!directoryExists(data)) {
      name = strrchr(data, '\\');
      if (name != NULL) {
        *name = '\0';
        name++;
        path[MAX_PATH];
        sprintf(path, "%s\\*", data);
        findFileHandle = FindFirstFile(path, &findFileData) ;
        if (findFileHandle != INVALID_HANDLE_VALUE) {
          do {
            if ((findFileData.cFileName[0] != '.') && (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
              sprintf(path, "%s\\%s\\msedge.exe", data, findFileData.cFileName);
              if (fileExists(path)) {
                sprintf(path, "%s\\%s", data, findFileData.cFileName);
                SetEnvironmentVariable(WEBVIEW2_BROWSER_EXECUTABLE_FOLDER, path);
                webview_print_log("found potential WebView2 executable folder, use " WEBVIEW2_DISABLE_AUTO_DETECT " to disable");
                webview_print_log(path);
                break;
              }
            }
          } while (FindNextFileA(findFileHandle, &findFileData));
          FindClose(findFileHandle);
        }
      }
    }
  }
}

static TCHAR *getWebView2LoaderFileName(TCHAR *modulePath) {
  char * webView2Win32Path = getenv(WEBVIEW2_WIN32_PATH);
  if ((webView2Win32Path != NULL) && (strlen(webView2Win32Path) > MAX_PATH)) {
    webView2Win32Path = NULL;
  }
  if (webView2Win32Path == NULL) {
    strcpy(modulePath, "WebView2Loader.dll");
  } else {
    sprintf(modulePath, "%s\\WebView2Loader.dll", webView2Win32Path);
  }
  return modulePath;
}

static int WebView2Enable() {
  TCHAR modulePath[MAX_PATH + 22];
  webview_print_log("Loading WebView2Loader (0.9.430)");
  findWebView2BrowserExecutableFolder();
  getWebView2LoaderFileName(modulePath);
  webview_print_log(modulePath);
  HMODULE hWebView2LoaderModule = LoadLibraryA(modulePath);
  if (hWebView2LoaderModule != NULL) {
    GetWebView2BrowserVersionInfoFnType GetWebView2BrowserVersionInfoFn = (GetWebView2BrowserVersionInfoFnType)GetProcAddress(hWebView2LoaderModule, "GetCoreWebView2BrowserVersionInfo");
    CreateCoreWebView2EnvironmentFn = (CreateCoreWebView2EnvironmentWithDetailsFnType)GetProcAddress(hWebView2LoaderModule, "CreateCoreWebView2EnvironmentWithDetails");
    if ((CreateCoreWebView2EnvironmentFn != NULL) && (GetWebView2BrowserVersionInfoFn != NULL)) {
      LPWSTR versionInfo = NULL;
      HRESULT res = GetWebView2BrowserVersionInfoFn(NULL, &versionInfo);
      if ((res == S_OK) && (versionInfo != NULL)) {
        webview_print_log("WebView2 enabled");
        OutputDebugStringW(versionInfo);
        return 1;
      } else {
        webview_print_log("Unable to get version");
      }
    } else {
      webview_print_log("Uncompatible");
    }
  } else {
    webview_print_log("Unable to load, you could set " WEBVIEW2_WIN32_PATH);
  }
  CreateCoreWebView2EnvironmentFn = NULL;
  webview_print_log("Not available");
  return 0;
}
