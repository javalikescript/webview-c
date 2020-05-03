#ifndef WEBVIEW2WIN32_H
#define WEBVIEW2WIN32_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef struct webview2_struct webview2;

typedef void (*WebView2CallbackFn) (webview2 *wv, const char *message, void *context);

typedef webview2 * (*CreateWebView2Fn) (HWND hwnd, const char *url);
typedef void (*ReleaseWebView2Fn) (webview2 *wv);
typedef void (*WebView2SetBoundsFn) (webview2 *wv, RECT bounds);
typedef int (*WebView2EvalFn) (webview2 *wv, const char *js);
typedef void (*WebView2RegisterCallbackFn) (webview2 *wv, WebView2CallbackFn cb, void *context);

typedef struct webview2_win32_struct {
  CreateWebView2Fn create;
  ReleaseWebView2Fn release;
  WebView2SetBoundsFn setBounds;
  WebView2EvalFn eval;
  WebView2RegisterCallbackFn registerCallback;
} webview2_win32;

typedef webview2_win32 * (*GetWebView2Win32Fn) (void);

webview2_win32 * GetWebView2Win32(void);

#endif /* WEBVIEW2WIN32_H */
