## Overview

A tiny cross-platform webview C library to build modern cross-platform GUIs.

It supports two-way JavaScript bindings, to call JavaScript from C and to call C from JavaScript.

It uses Cocoa/WebKit on macOS, gtk-webkit2 on Linux and MSHTML (IE10/11) or Edge (Chromium) on Windows.

This library is a fork of the [webview](https://github.com/zserge/webview/tree/9c1b0a888aa40039d501c1ea9f60b22a076a25ea) library.
The goal of this fork is mainly to retain the support of MinGW, the C language and the IE fallback on Windows.

A [Lua binding](https://github.com/javalikescript/lua-webview) is available.

## Limitations

Opening multiple webviews is not supported.

## Prerequisites

You need the `gcc` compiler.

On Linux you need the GTK and WebKit2GTK libraries.

```bash
sudo apt-get install libgtk-3-dev libwebkit2gtk-4.0-dev
```

## Example

Look at the example `webview-example.c`.

```c
#define WEBVIEW_IMPLEMENTATION
/* don't forget to define WEBVIEW_WINAPI,WEBVIEW_GTK or WEBVIEW_COCAO */
#include "webview.h"

int main() {
  /* Open Lua in a 800x600 resizable window */
  webview_run("Minimal webview example", "https://www.lua.org", 800, 600, 1);
  return 0;
}
```

Build it using the following command line.

```bash
# Linux
gcc webview-example.c -DWEBVIEW_GTK=1 `pkg-config --cflags --libs gtk+-3.0 webkit2gtk-4.0` -o webview-example

# MacOS
gcc -ObjC -DOBJC_OLD_DISPATCH_PROTOTYPES=1 webview-example.c -DWEBVIEW_COCOA=1 -framework WebKit -o webview-example

# Windows (MinGW)
gcc webview-example.c -DWEBVIEW_WINAPI=1 -Ims.webview2/include -lole32 -lcomctl32 -loleaut32 -luuid -lgdi32 -o webview-example.exe -w
```

## Edge

The Edge (Chromium) implementation requires the extra library `WebView2Loader.dll`
part of the [Microsoft Edge WebView2](https://docs.microsoft.com/en-gb/microsoft-edge/hosting/webview2) SDK.
The environment variable `WEBVIEW2_WIN32_PATH` can be used to pass the folder containing the extra library.

The [WebView2 Runtime](https://docs.microsoft.com/en-gb/microsoft-edge/webview2/concepts/distribution#understanding-the-webview2-runtime) shall be installed with the same architecture, x64 or x86. The Evergreen WebView2 Runtime is included as part of the Windows 11 operating system.

The WebView2 SDK may fail to auto detect the WebView2 Runtime installation path to use,
you could indicate the correct installation path by using the environment variable `WEBVIEW2_BROWSER_EXECUTABLE_FOLDER`.

[_The Microsoft Edge Stable channel is unavailable for WebView2 usage to prevent apps from taking a dependency on the browser in production._](https://docs.microsoft.com/en-gb/microsoft-edge/webview2/reference/win32/webview2-idl?view=webview2-0.9.622#createcorewebview2environmentwithoptions)
If you still want to use an Microsoft Edge Stable channel for WebView2 you may create a symbolic link named `Edge-Application` using the following command as administrator.

```cmd
cd "\Program Files (x86)\Microsoft"
mklink /D Edge-Application Edge\Application
```

Note that this implementation creates a folder for the user data,
you could specify a user data folder by using the environment variable `WEBVIEW2_USER_DATA_FOLDER`.

## WebKitGTK

The WebKitGTK implementation is sensitive to the environment variables.
Starting a webview from a VS Code terminal may require to disable environment variables inheritance.

```bash
for name in `env | grep _VSCODE_SNAP_ORIG | sed 's/_VSCODE_SNAP_ORIG.*$//'`; do fullname=${name}_VSCODE_SNAP_ORIG; test -z "${!fullname}" && unset ${name} || export ${name}=${!fullname}; unset ${fullname}; done
```
