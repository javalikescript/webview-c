#define WEBVIEW_IMPLEMENTATION
//don't forget to define WEBVIEW_WINAPI,WEBVIEW_GTK or WEBVIEW_COCAO
#include "webview.h"

int main() {
  /* Open Lua in a 800x600 resizable window */
  webview("Minimal webview example", "https://www.lua.org", 800, 600, 1);
  return 0;
}
