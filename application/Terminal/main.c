#include <libsystem/eventloop/EventLoop.h>
#include <libsystem/eventloop/Notifier.h>
#include <libsystem/eventloop/Timer.h>
#include <libsystem/logger.h>
#include <libsystem/process/Launchpad.h>
#include <libwidget/Application.h>

#include "Terminal/ApplicationTerminal.h"
#include "Terminal/FramebufferTerminal.h"
#include "Terminal/TextmodeTerminal.h"

#define TERMINAL_IO_BUFFER_SIZE 4096

Stream *master = NULL, *slave = NULL;

void keyboard_callback(void *target, Stream *keyboard, SelectEvent events)
{
    __unused(target);
    __unused(events);

    char buffer[TERMINAL_IO_BUFFER_SIZE];
    size_t size = stream_read(keyboard, buffer, TERMINAL_IO_BUFFER_SIZE);

    stream_write(master, buffer, size);
}

void master_callback(Terminal *terminal, Stream *master, SelectEvent events)
{
    __unused(events);

    char buffer[TERMINAL_IO_BUFFER_SIZE];
    size_t size = stream_read(master, buffer, TERMINAL_IO_BUFFER_SIZE);

    if (handle_has_error(master))
    {
        handle_printf_error(master, "Terminal: read from master failled");
        return;
    }

    terminal_write(terminal, buffer, size);
}

void repaint_callback(Terminal *terminal)
{
    terminal_repaint(terminal);
}

void cursor_callback(Terminal *terminal)
{
    terminal_blink(terminal);
}

int main(int argc, char **argv)
{
    __unused(argc);
    __unused(argv);

    if (application_initialize(argc, argv) == SUCCESS)
    {
        Window *window = window_create("Terminal", 500, 400);
        window_set_background(window, COLOR(0x0A0E14));

        terminal_widget_create(window_root(window));

        application_dump();

        return application_run();
    }
    else
    {
        Result result = stream_create_term(&master, &slave);

        if (result != SUCCESS)
        {
            logger_error("Failled to create the terminal device: %s", result_to_string(result));
            return -1;
        }

        Stream *keyboard = stream_open("/dev/keyboard", OPEN_READ);

        if (handle_has_error(keyboard))
        {
            logger_error("Failled to open the keyboard device: %s", handle_get_error(keyboard));
            return -1;
        }

        Terminal *terminal = framebuffer_terminal_create();

        if (!terminal)
        {
            terminal = textmode_terminal_create();

            if (!terminal)
            {
                logger_error("Failled to open a device to display the terminal in!");
                return -1;
            }
        }

        eventloop_initialize();

        notifier_create(terminal, HANDLE(keyboard), SELECT_READ, (NotifierCallback)keyboard_callback);
        notifier_create(terminal, HANDLE(master), SELECT_READ, (NotifierCallback)master_callback);

        timer_start(timer_create(terminal, 250, (TimerCallback)cursor_callback));
        timer_start(timer_create(terminal, 16, (TimerCallback)repaint_callback));

        logger_trace("Starting the shell application...");

        Launchpad *shell_launchpad = launchpad_create("sh", "/bin/Shell");
        launchpad_handle(shell_launchpad, HANDLE(slave), 0);
        launchpad_handle(shell_launchpad, HANDLE(slave), 1);
        launchpad_handle(shell_launchpad, HANDLE(slave), 2);
        launchpad_launch(shell_launchpad, NULL);

        logger_trace("Shell application started");

        return eventloop_run();
    }
}
