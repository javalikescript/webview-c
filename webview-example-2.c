#define WEBVIEW_IMPLEMENTATION
/* don't forget to define WEBVIEW_WINAPI,WEBVIEW_GTK or WEBVIEW_COCAO */
#include "webview.h"

static void invoke_cb(struct webview *w, const char *arg) {
  printf("Callback called with '%s'\n", arg);
}

int main(int argc, char *argv[]) {
  struct webview webview;
  int r;
  memset(&webview, 0, sizeof(webview));
  webview.url = "data:text/html,<!DOCTYPE html><html><body>"
    "<p id=\"sentence\">It works !</p>"
    "<button onclick=\"window.external.invoke('Hi')\">Callback</button>"
    "</body></html>";
  webview.title = "Simple webview example";
  webview.width = 800;
  webview.height = 600;
  webview.resizable = 1;
  webview.debug = 1;
  webview.external_invoke_cb = &invoke_cb;
  r = webview_init(&webview);
  do {
    r = webview_loop(&webview, 1);
  } while (r == 0);
  return 0;
}
