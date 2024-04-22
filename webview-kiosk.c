#define WEBVIEW_IMPLEMENTATION

#include "webview.h"

int main(int argc, char *argv[]) {
  struct webview webview;
  int r;
  memset(&webview, 0, sizeof(webview));
  webview.url = argc > 1 ? argv[1] : "https://javalikescript.github.io/time.html";
  webview.width = 320;
  webview.height = 240;
  webview.resizable = 1;
  r = webview_init(&webview);
  webview_set_fullscreen(&webview, 1);
  do {
    r = webview_loop(&webview, 1);
  } while (r == 0);
  return 0;
}
