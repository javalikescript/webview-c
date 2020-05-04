## Overview

A tiny cross-platform webview C library to build modern cross-platform GUIs.

It supports two-way JavaScript bindings (to call JavaScript from C and to call C from JavaScript).

It uses Cocoa/WebKit on macOS, gtk-webkit2 on Linux and MSHTML (IE10/11) or Edge (Chromium) on Windows.

This library is a fork of the [webview](https://github.com/zserge/webview/tree/9c1b0a888aa40039d501c1ea9f60b22a076a25ea) library.

A [Lua binding](https://github.com/javalikescript/lua-webview) is available.

## Example

Look at the example `webview-example.c`.

```c
#define WEBVIEW_IMPLEMENTATION
//don't forget to define WEBVIEW_WINAPI,WEBVIEW_GTK or WEBVIEW_COCAO
#include "webview.h"

int main() {
  /* Open Lua in a 800x600 resizable window */
  webview("Minimal webview example", "https://www.lua.org", 800, 600, 1);
  return 0;
}
```

Build it using the following command line.

```bash
# Linux
gcc webview-example.c -DWEBVIEW_GTK=1 `pkg-config --cflags --libs gtk+-3.0 webkit2gtk-4.0` -o webview-example

# MacOS
gcc webview-example.c -DWEBVIEW_COCOA=1 -framework WebKit -o webview-example

# Windows (mingw)
gcc webview-example.c -DWEBVIEW_WINAPI=1 -Ims.webview2.0.8.355/include -lole32 -lcomctl32 -loleaut32 -luuid -lgdi32 -o webview-example.exe
```

## Edge

Microsoft Edge (Chromium) shall be installed with the same architecture, x64 or x86.

The Edge (Chromium) implementation requires the extra library `WebView2Loader.dll`
part of the [Microsoft Edge WebView2](https://docs.microsoft.com/en-gb/microsoft-edge/hosting/webview2) SDK.
The environment variable `WEBVIEW2_WIN32_PATH` can be used to pass the folder containing the extra library.

Note that this implementation creates a folder for the user data.
