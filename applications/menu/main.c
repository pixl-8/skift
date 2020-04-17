#include <libwidget/Application.h>
#include <libwidget/Panel.h>

int main(int argc, char **argv)
{
    application_initialize(argc, argv);

    Window *window = window_create(NULL, "Panel", 320, 600);

    window_set_border_style(window, WINDOW_BORDER_NONE);

    window_root(window)->layout = (Layout){LAYOUT_VFLOW, 8, 0};

    panel_create(window_root(window));

    return application_run();
}