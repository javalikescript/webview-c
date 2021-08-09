#define WEBVIEW_IMPLEMENTATION
/* don't forget to define WEBVIEW_WINAPI,WEBVIEW_GTK or WEBVIEW_COCAO */
#include "webview.h"

int main(int argc, char *argv[]) {
  /* Open Lua in a 800x600 resizable window */
  /* The URL can be passed in argument, such as https://www.whatsmyua.info/ */
  webview_run("Minimal webview example", argc > 1 ? argv[1] : "https://www.lua.org", 800, 600, 1);
  return 0;
}
