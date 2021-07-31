#include <stddef.h>
//#include <stdio.h>

#define WEBVIEW2_WIN32_DISABLE_AUTO_DETECT "WEBVIEW2_WIN32_DISABLE_AUTO_DETECT"
#define WEBVIEW2_WIN32_PATH "WEBVIEW2_WIN32_PATH"
#define WEBVIEW2_WIN32_USER_DATA_NO_APPDATA "WEBVIEW2_WIN32_USER_DATA_NO_APPDATA"

#define WEBVIEW2_BROWSER_EXECUTABLE_FOLDER "WEBVIEW2_BROWSER_EXECUTABLE_FOLDER"
#define WEBVIEW2_USER_DATA_FOLDER "WEBVIEW2_USER_DATA_FOLDER"

#define WEBVIEW2_RT_UUID "{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}"
#define EDGE_STABLE_UUID "{56EB18F8-B008-4CBD-B6D2-8C97FE7E9062}"

#define KEY_CLIENTS_LOCATION "location"
#define KEY_CLIENTS_PV "pv"
#define KEY_MS_EDGEUPDATE_CLIENTSTATE "SOFTWARE\\WOW6432Node\\Microsoft\\EdgeUpdate\\ClientState"
#define KEY_MS_EDGEUPDATE_CLIENTS "SOFTWARE\\WOW6432Node\\Microsoft\\EdgeUpdate\\Clients"

#define WEBVIEW2_LOADER_DLL "WebView2Loader.dll"

static int fileExists(LPCSTR lpFileName) {
  return GetFileAttributes(lpFileName) != INVALID_FILE_ATTRIBUTES;
}
static int isDirectory(LPCSTR lpFileName) {
  DWORD attributes = GetFileAttributes(lpFileName);
  return (attributes != INVALID_FILE_ATTRIBUTES) && (attributes & FILE_ATTRIBUTE_DIRECTORY);
}
static int isFile(LPCSTR lpFileName) {
  DWORD attributes = GetFileAttributes(lpFileName);
  return (attributes != INVALID_FILE_ATTRIBUTES) && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
}

static void findWebView2BrowserExecutableFolder() {
  char data[MAX_PATH];
  char version[64];
  char path[MAX_PATH];
  char *bs;
  DWORD size;
  if ((getenv(WEBVIEW2_BROWSER_EXECUTABLE_FOLDER) != NULL) || (getenv(WEBVIEW2_WIN32_DISABLE_AUTO_DETECT) != NULL)) {
    return;
  }
  /*
  see https://docs.microsoft.com/en-gb/deployedge/microsoft-edge-webview-policies
  see https://docs.microsoft.com/en-gb/microsoft-edge/webview2/concepts/distribution
  see https://docs.microsoft.com/en-gb/microsoft-edge/webview2/reference/win32/webview2-idl?view=webview2-0.9.622#createcorewebview2environmentwithoptions
  To use a fixed version of the WebView2 Runtime, pass the relative path of the folder
  that contains the fixed version of the WebView2 Runtime to browserExecutableFolder.
  The path of fixed version of the WebView2 Runtime should not contain \Edge\Application\.
  When such a path is used, the API will fail with ERROR_NOT_SUPPORTED.
  */
  size = sizeof(data);
  if (RegGetValueA(HKEY_LOCAL_MACHINE, KEY_MS_EDGEUPDATE_CLIENTS "\\" EDGE_STABLE_UUID, KEY_CLIENTS_LOCATION, RRF_RT_REG_SZ, NULL, &data, &size) == ERROR_SUCCESS) {
    size = sizeof(version);
    if (isDirectory(data) && (RegGetValueA(HKEY_LOCAL_MACHINE, KEY_MS_EDGEUPDATE_CLIENTS "\\" EDGE_STABLE_UUID, KEY_CLIENTS_PV, RRF_RT_REG_SZ, NULL, &version, &size) == ERROR_SUCCESS)) {
      bs = strrchr(data, '\\');
      if (bs != NULL) {
        *bs = '-';
        sprintf(path, "%s\\%s\\msedge.dll", data, version);
        if (fileExists(path)) {
          sprintf(path, "%s\\%s", data, version);
          webview_print_log("found Edge alternative folder, use " WEBVIEW2_WIN32_DISABLE_AUTO_DETECT " to disable");
          webview_print_log(path);
          SetEnvironmentVariable(WEBVIEW2_BROWSER_EXECUTABLE_FOLDER, path);
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
    strcpy(modulePath, WEBVIEW2_LOADER_DLL);
  } else {
    sprintf(modulePath, "%s\\" WEBVIEW2_LOADER_DLL, webView2Win32Path);
  }
  return modulePath;
}

static WCHAR *getUserData(WCHAR *buffer, size_t sizeOfBuffer) {
  WCHAR filename[MAX_PATH];
  WCHAR *appData = _wgetenv(L"APPDATA");
  char *noAppData = getenv(WEBVIEW2_WIN32_USER_DATA_NO_APPDATA);
  if ((getenv(WEBVIEW2_USER_DATA_FOLDER) == NULL) && (appData != NULL) && (noAppData == NULL)) {
    GetModuleFileNameW(NULL, filename, MAX_PATH);
    WCHAR *executableName = wcsrchr(filename, L'\\');
    if (executableName != NULL) {
      executableName++;
    } else {
      executableName = filename;
    }
    swprintf(buffer, sizeOfBuffer, L"%s\\%s.WebView2", appData, executableName);
    webview_print_log("getUserData()");
    OutputDebugStringW(buffer);
    return buffer;
  }
  return NULL;
}


#define WEBVIEW2_WIN32_API extern

typedef void (*WebView2CallbackFn) (webview2 *wv, const char *message, void *context);

#define GET_AVAILABLE_COREWEBVIEW2_BROWSER_VERSION_FN_NAME "GetAvailableCoreWebView2BrowserVersionString"
#define CREATE_COREWEBVIEW2_ENVIRONMENTWITHOPTIONS_FN_NAME "CreateCoreWebView2EnvironmentWithOptions"

typedef HRESULT (*GetWebView2BrowserVersionInfoFnType) (PCWSTR browserExecutableFolder, LPWSTR* versionInfo);
typedef HRESULT (*CreateCoreWebView2EnvironmentWithOptionsFnType) (PCWSTR browserExecutableFolder, PCWSTR userDataFolder, ICoreWebView2EnvironmentOptions* environmentOptions, ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler* environment_created_handler);

typedef struct webview2_struct {
  HWND hwnd;
  const char *url;
  int debug;
  WebView2CallbackFn cb;
  void *context;
  ICoreWebView2 *webview;
  unsigned char ready;
  char *jsToEval;
  ICoreWebView2Controller *controller;
  ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler env_created_handler;
  ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandlerVtbl env_created_handler_vtbl;
  ICoreWebView2CreateCoreWebView2ControllerCompletedHandler controller_created_handler;
  ICoreWebView2CreateCoreWebView2ControllerCompletedHandlerVtbl controller_created_handler_vtbl;
  ICoreWebView2WebMessageReceivedEventHandler message_received_event_handler;
  ICoreWebView2WebMessageReceivedEventHandlerVtbl message_received_event_handler_vtbl;
  ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler add_script_on_document_created_handler;
  ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandlerVtbl add_script_on_document_created_handler_vtbl;
} webview2;

#define WEBVIEW2_PTR_FROM(_cp, _field) \
	((webview2 *) ((char *) (_cp) - offsetof(webview2, _field)))

static CreateCoreWebView2EnvironmentWithOptionsFnType CreateCoreWebView2EnvironmentFn = NULL;

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

static HRESULT AddScriptToExecuteOnDocumentCreated_Invoke(ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler * This, HRESULT errorCode, LPCWSTR id) {
  webview2 *pwv2 = WEBVIEW2_PTR_FROM(This, add_script_on_document_created_handler);
  webview_print_log("AddScriptToExecuteOnDocumentCreated");
  pwv2->ready = 1;
  PostMessageW(pwv2->hwnd, WM_WEBVIEW_READY, 0, 0);
  return S_OK;
}

static HRESULT CreateWebView2Controller_Invoke(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler * This, HRESULT result, ICoreWebView2Controller *webViewController) {
  webview_print_log("CreateWebView2Controller_Invoke()");
  ICoreWebView2 *webview;
  if (FAILED(result) || (webViewController == NULL)) {
    char buffer[256];
    sprintf(buffer, "CreateWebView2Controller_Invoke(%p, %08X, %p) => %08X (fail)", This, result, webViewController, E_FAIL);
    webview_print_log(buffer);
    return E_FAIL;
  }
  webViewController->lpVtbl->AddRef(webViewController);

  webview2 *pwv2 = WEBVIEW2_PTR_FROM(This, controller_created_handler);
  pwv2->controller = webViewController;

  webViewController->lpVtbl->get_CoreWebView2(webViewController, &webview);
  pwv2->webview = webview;

  ICoreWebView2Settings* settings;
  webview->lpVtbl->get_Settings(webview, &settings);
  if (settings->lpVtbl != NULL) {
    settings->lpVtbl->put_AreDevToolsEnabled(settings, pwv2->debug);
    settings->lpVtbl->put_AreDefaultContextMenusEnabled(settings, pwv2->debug);
  }

  RECT bounds;
  GetClientRect(pwv2->hwnd, &bounds);
  webViewController->lpVtbl->put_Bounds(webViewController, bounds);

  webview->lpVtbl->AddScriptToExecuteOnDocumentCreated(webview, L"window.external={invoke:s=>window.chrome.webview.postMessage(s)}", &pwv2->add_script_on_document_created_handler);

  EventRegistrationToken token;
  webview->lpVtbl->add_WebMessageReceived(webview, &pwv2->message_received_event_handler, &token);

  if ((pwv2->url != NULL) && (strlen(pwv2->url) > 0)) {
    wchar_t *wurl = webview_to_utf16(pwv2->url);
    if (wurl != NULL) {
      webview_print_log("CreateWebView2Controller_Invoke() Navigate");
      webview->lpVtbl->Navigate(webview, wurl);
      GlobalFree(wurl);
    }
  }
  return S_OK;
}

static HRESULT CreateWebView2Environment_Invoke(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler * This, HRESULT result, ICoreWebView2Environment * webViewEnvironment) {
  webview_print_log("CreateWebView2Environment_Invoke()");
  if (FAILED(result) || (webViewEnvironment == NULL)) {
    char buffer[256];
    sprintf(buffer, "CreateWebView2Environment_Invoke(%p, %08X, %p) => %08X (fail)", This, result, webViewEnvironment, E_FAIL);
    webview_print_log(buffer);
    return E_FAIL;
  }
  webViewEnvironment->lpVtbl->AddRef(webViewEnvironment);
  webview2 *pwv2 = WEBVIEW2_PTR_FROM(This, env_created_handler);
  webview_print_log("CreateWebView2Environment_Invoke() CreateCoreWebView2Controller");
  HRESULT hResult = webViewEnvironment->lpVtbl->CreateCoreWebView2Controller(webViewEnvironment, pwv2->hwnd, &pwv2->controller_created_handler);
  webview_print_log("CreateWebView2Environment_Invoke() => ok");
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
  pwv2->controller_created_handler_vtbl.QueryInterface = (HRESULT (*)(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler * This, REFIID riid, void **ppvObject)) &NoOpQueryInterface;
  pwv2->controller_created_handler_vtbl.AddRef = (ULONG (*)(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler * This)) &NoOpAddRef;
  pwv2->controller_created_handler_vtbl.Release = (ULONG (*)(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler * This)) &NoOpRelease;
  pwv2->controller_created_handler_vtbl.Invoke = &CreateWebView2Controller_Invoke;
  pwv2->controller_created_handler.lpVtbl = &pwv2->controller_created_handler_vtbl;
  pwv2->message_received_event_handler_vtbl.QueryInterface = (HRESULT (*)(ICoreWebView2WebMessageReceivedEventHandler * This, REFIID riid, void **ppvObject)) &NoOpQueryInterface;
  pwv2->message_received_event_handler_vtbl.AddRef = (ULONG (*)(ICoreWebView2WebMessageReceivedEventHandler * This)) &NoOpAddRef;
  pwv2->message_received_event_handler_vtbl.Release = (ULONG (*)(ICoreWebView2WebMessageReceivedEventHandler * This)) &NoOpRelease;
  pwv2->message_received_event_handler_vtbl.Invoke = &WebView2WebMessageReceivedEventHandleInvoke;
  pwv2->message_received_event_handler.lpVtbl = &pwv2->message_received_event_handler_vtbl;
  pwv2->add_script_on_document_created_handler_vtbl.QueryInterface = (HRESULT (*)(ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler * This, REFIID riid, void **ppvObject)) &NoOpQueryInterface;
  pwv2->add_script_on_document_created_handler_vtbl.AddRef = (ULONG (*)(ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler * This)) &NoOpAddRef;
  pwv2->add_script_on_document_created_handler_vtbl.Release = (ULONG (*)(ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler * This)) &NoOpRelease;
  pwv2->add_script_on_document_created_handler_vtbl.Invoke = &AddScriptToExecuteOnDocumentCreated_Invoke;
  pwv2->add_script_on_document_created_handler.lpVtbl = &pwv2->add_script_on_document_created_handler_vtbl;
}

WEBVIEW2_WIN32_API webview2 * CreateWebView2(HWND hwnd, const char *url, int debug) {
  WCHAR dirname[MAX_PATH];
  webview2 *pwv2 = NULL;
  if (CreateCoreWebView2EnvironmentFn != NULL) {
    pwv2 = (webview2 *)GlobalAlloc(GMEM_FIXED, sizeof(webview2));
    if (pwv2 != NULL) {
      InitWebView2(pwv2);
      pwv2->ready = 0;
      pwv2->jsToEval = NULL;
      pwv2->webview = NULL;
      pwv2->hwnd = hwnd;
      pwv2->url = url;
      pwv2->debug = debug;
      PCWSTR userDataFolder = getUserData(dirname, MAX_PATH);
      HRESULT hr = CreateCoreWebView2EnvironmentFn(NULL, userDataFolder, NULL, &pwv2->env_created_handler);
      if (FAILED(hr)) {
        char buffer[256];
        sprintf(buffer, CREATE_COREWEBVIEW2_ENVIRONMENTWITHOPTIONS_FN_NAME "(%p, %p, %p) => %08X (fail)", NULL, userDataFolder, NULL, hr);
        webview_print_log(buffer);
        GlobalFree(pwv2);
        pwv2 = NULL;
      }
    }
  } else {
    webview_print_log("CreateWebView2 error WebView2 is not enabled");
  }
  return pwv2;
}

WEBVIEW2_WIN32_API void ReleaseWebView2(webview2 *pwv2) {
  if (pwv2 != NULL) {
    GlobalFree(pwv2);
  }
}

WEBVIEW2_WIN32_API void WebView2SetBounds(webview2 *pwv2, RECT bounds) {
  if ((pwv2 != NULL) && (pwv2->controller != NULL) && (pwv2->webview != NULL)) {
    pwv2->controller->lpVtbl->put_Bounds(pwv2->controller, bounds);
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
  webview_print_log("WebView2Eval() not available yet");
  return 1;
}

static int WebView2Enable() {
  TCHAR modulePath[MAX_PATH + 22];
  webview_print_log("Loading WebView2Loader (1.0.818)");
  findWebView2BrowserExecutableFolder();
  getWebView2LoaderFileName(modulePath);
  webview_print_log(modulePath);
  HMODULE hWebView2LoaderModule = LoadLibraryA(modulePath);
  if (hWebView2LoaderModule != NULL) {
    GetWebView2BrowserVersionInfoFnType GetWebView2BrowserVersionInfoFn = (GetWebView2BrowserVersionInfoFnType)GetProcAddress(hWebView2LoaderModule, GET_AVAILABLE_COREWEBVIEW2_BROWSER_VERSION_FN_NAME);
    CreateCoreWebView2EnvironmentFn = (CreateCoreWebView2EnvironmentWithOptionsFnType)GetProcAddress(hWebView2LoaderModule, CREATE_COREWEBVIEW2_ENVIRONMENTWITHOPTIONS_FN_NAME);
    if ((CreateCoreWebView2EnvironmentFn != NULL) && (GetWebView2BrowserVersionInfoFn != NULL)) {
      LPWSTR versionInfo = NULL;
      HRESULT hr = GetWebView2BrowserVersionInfoFn(NULL, &versionInfo);
      if ((hr == S_OK) && (versionInfo != NULL)) {
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
