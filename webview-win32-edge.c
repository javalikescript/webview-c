#include "webview2.h"

#include <stddef.h>
//#include <stdio.h>

#define WEBVIEW2_WIN32_API extern

typedef void (*WebView2CallbackFn) (webview2 *wv, const char *message, void *context);

typedef HRESULT (*CreateWebView2EnvironmentWithDetailsFnType) (PCWSTR browserExecutableFolder, PCWSTR userDataFolder, PCWSTR additionalBrowserArguments, IWebView2CreateWebView2EnvironmentCompletedHandler* environment_created_handler);
typedef HRESULT (*GetWebView2BrowserVersionInfoFnType) (PCWSTR browserExecutableFolder, LPWSTR* versionInfo);

typedef struct webview2_struct {
  HWND hwnd;
  const char *url;
  WebView2CallbackFn cb;
  void *context;
  IWebView2WebView *webview;
  IWebView2CreateWebView2EnvironmentCompletedHandler env_created_handler;
  IWebView2CreateWebView2EnvironmentCompletedHandlerVtbl env_created_handler_vtbl;
  IWebView2CreateWebViewCompletedHandler webview_created_handler;
  IWebView2CreateWebViewCompletedHandlerVtbl webview_created_handler_vtbl;
  IWebView2WebMessageReceivedEventHandlerVtbl message_received_event_handler_vtbl;
  IWebView2WebMessageReceivedEventHandler message_received_event_handler;
} webview2;

#define WEBVIEW2_PTR_FROM(_cp, _field) \
	((webview2 *) ((char *) (_cp) - offsetof(webview2, _field)))

static CreateWebView2EnvironmentWithDetailsFnType CreateWebView2EnvironmentWithDetailsFn = NULL;

static HRESULT WebView2WebMessageReceivedEventHandleInvoke(IWebView2WebMessageReceivedEventHandler * This, IWebView2WebView *webView, IWebView2WebMessageReceivedEventArgs *args) {
  LPWSTR webMessage;
  HRESULT hRes;
  webview2 *pwv2 = WEBVIEW2_PTR_FROM(This, message_received_event_handler);
  hRes = args->lpVtbl->get_WebMessageAsString(args, &webMessage);
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

static HRESULT CreateWebView2_Invoke(IWebView2CreateWebViewCompletedHandler * This, HRESULT result, IWebView2WebView *webView) {
  webview2 *pwv2 = WEBVIEW2_PTR_FROM(This, webview_created_handler);
  webView->lpVtbl->AddRef(webView);
  pwv2->webview = webView;
  RECT bounds;
  GetClientRect(pwv2->hwnd, &bounds);
  webView->lpVtbl->put_Bounds(webView, bounds);
  webView->lpVtbl->AddScriptToExecuteOnDocumentCreated(webView, L"window.external={invoke:s=>window.chrome.webview.postMessage(s)}", NULL);

  EventRegistrationToken token;
  webView->lpVtbl->add_WebMessageReceived(webView, &pwv2->message_received_event_handler, &token);

  if ((pwv2->url != NULL) && (strlen(pwv2->url) > 0)) {
    wchar_t *wurl = webview_to_utf16(pwv2->url);
    if (wurl != NULL) {
      webView->lpVtbl->Navigate(webView, wurl);
      GlobalFree(wurl);
    }
  }
  return S_OK;
}

static HRESULT CreateWebView2Environment_Invoke(IWebView2CreateWebView2EnvironmentCompletedHandler * This, HRESULT result, IWebView2Environment * webViewEnvironment) {
  webview2 *pwv2 = WEBVIEW2_PTR_FROM(This, env_created_handler);
  webViewEnvironment->lpVtbl->CreateWebView(webViewEnvironment, pwv2->hwnd, &pwv2->webview_created_handler);
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
  pwv2->env_created_handler_vtbl.QueryInterface = (HRESULT (*)(IWebView2CreateWebView2EnvironmentCompletedHandler * This, REFIID riid, void **ppvObject)) &NoOpQueryInterface;
  pwv2->env_created_handler_vtbl.AddRef = (ULONG (*)(IWebView2CreateWebView2EnvironmentCompletedHandler * This)) &NoOpAddRef;
  pwv2->env_created_handler_vtbl.Release = (ULONG (*)(IWebView2CreateWebView2EnvironmentCompletedHandler * This)) &NoOpRelease;
  pwv2->env_created_handler_vtbl.Invoke = &CreateWebView2Environment_Invoke;
  pwv2->env_created_handler.lpVtbl = &pwv2->env_created_handler_vtbl;
  pwv2->webview_created_handler_vtbl.QueryInterface = (HRESULT (*)(IWebView2CreateWebViewCompletedHandler * This, REFIID riid, void **ppvObject)) &NoOpQueryInterface;
  pwv2->webview_created_handler_vtbl.AddRef = (ULONG (*)(IWebView2CreateWebViewCompletedHandler * This)) &NoOpAddRef;
  pwv2->webview_created_handler_vtbl.Release = (ULONG (*)(IWebView2CreateWebViewCompletedHandler * This)) &NoOpRelease;
  pwv2->webview_created_handler_vtbl.Invoke = &CreateWebView2_Invoke;
  pwv2->webview_created_handler.lpVtbl = &pwv2->webview_created_handler_vtbl;
  pwv2->message_received_event_handler_vtbl.QueryInterface = (HRESULT (*)(IWebView2WebMessageReceivedEventHandler * This, REFIID riid, void **ppvObject)) &NoOpQueryInterface;
  pwv2->message_received_event_handler_vtbl.AddRef = (ULONG (*)(IWebView2WebMessageReceivedEventHandler * This)) &NoOpAddRef;
  pwv2->message_received_event_handler_vtbl.Release = (ULONG (*)(IWebView2WebMessageReceivedEventHandler * This)) &NoOpRelease;
  pwv2->message_received_event_handler_vtbl.Invoke = &WebView2WebMessageReceivedEventHandleInvoke;
  pwv2->message_received_event_handler.lpVtbl = &pwv2->message_received_event_handler_vtbl;
}

WEBVIEW2_WIN32_API webview2 * CreateWebView2(HWND hwnd, const char *url) {
  webview2 *pwv2 = (webview2 *)GlobalAlloc(GMEM_FIXED, sizeof(webview2));
  if (pwv2 != NULL) {
    InitWebView2(pwv2);
    pwv2->webview = NULL;
    pwv2->hwnd = hwnd;
    pwv2->url = url;
    HRESULT res = CreateWebView2EnvironmentWithDetailsFn(NULL, NULL, NULL, &pwv2->env_created_handler);
    if (res != S_OK) {
      GlobalFree(pwv2);
      pwv2 = NULL;
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
  if ((pwv2 != NULL) && (pwv2->webview != NULL)) {
    pwv2->webview->lpVtbl->put_Bounds(pwv2->webview, bounds);
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
        IWebView2ExecuteScriptCompletedHandler *handler = NULL;
        pwv2->webview->lpVtbl->ExecuteScript(pwv2->webview, wjs, handler);
        GlobalFree(wjs);
        return 0;
      }
    }
  }
  return 1;
}

static int WebView2Enable() {
  TCHAR modulePath[MAX_PATH + 22];
  char * webView2Win32Path = getenv("WEBVIEW2_WIN32_PATH");
  if ((webView2Win32Path != NULL) && (strlen(webView2Win32Path) > MAX_PATH)) {
    webView2Win32Path = NULL;
  }
  if (webView2Win32Path == NULL) {
    strcpy(modulePath, "WebView2Loader.dll");
  } else {
    sprintf(modulePath, "%s\\WebView2Loader.dll", webView2Win32Path);
  }
  HMODULE hWebView2LoaderModule = LoadLibraryA(modulePath);
  if (hWebView2LoaderModule != NULL) {
    GetWebView2BrowserVersionInfoFnType GetWebView2BrowserVersionInfoFn = (GetWebView2BrowserVersionInfoFnType)GetProcAddress(hWebView2LoaderModule, "GetWebView2BrowserVersionInfo");
    CreateWebView2EnvironmentWithDetailsFn = (CreateWebView2EnvironmentWithDetailsFnType)GetProcAddress(hWebView2LoaderModule, "CreateWebView2EnvironmentWithDetails");
    if ((CreateWebView2EnvironmentWithDetailsFn != NULL) && (GetWebView2BrowserVersionInfoFn != NULL)) {
      LPWSTR versionInfo = NULL;
      HRESULT res = GetWebView2BrowserVersionInfoFn(NULL, &versionInfo);
      if ((res == S_OK) && (versionInfo != NULL)) {
        webview_print_log("WebView2 enabled");
        return 1;
      }
    }
  } else {
    webview_print_log(modulePath);
  }
  CreateWebView2EnvironmentWithDetailsFn = NULL;
  webview_print_log("WebView2Loader not available");
  return 0;
}
