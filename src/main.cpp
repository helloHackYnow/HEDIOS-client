#include "App.h"



int main() {
    App app;
    app.init();
    app.main_loop();
    app.cleanup();
}