#include <libgraphic/Painter.h>
#include <libmath/MinMax.h>
#include <libsystem/assert.h>
#include <libsystem/io/Stream.h>
#include <libsystem/logger.h>
#include <libwidget/Event.h>
#include <libwidget/Widget.h>
#include <libwidget/Window.h>

static Font *_widget_font = NULL;
Font *widget_font(void)
{
    if (_widget_font == NULL)
    {
        _widget_font = font_create("sans");
    }

    return _widget_font;
}

void widget_initialize(
    Widget *widget,
    const char *classname,
    Widget *parent)
{
    widget->classname = classname;
    widget->childs = list_create();
    widget->bound = RECTANGLE_SIZE(32, 32);

    if (parent != NULL)
    {
        widget->window = parent->window;
        widget_add_child(parent, widget);
    }
}

void widget_destroy(Widget *widget)
{
    logger_info("Destroying %s(%08x)", widget->classname, widget);

    if (widget->destroy)
    {
        widget->destroy(widget);
    }

    Widget *child = NULL;
    while (list_peek(widget->childs, (void **)&child))
    {
        widget_remove_child(widget, child);
        widget_destroy(child);
    }

    list_destroy(widget->childs);

    free(widget);
}

void widget_add_child(Widget *widget, Widget *child)
{
    assert(child->parent == NULL);

    child->parent = widget;
    child->window = widget->window;
    list_pushback(widget->childs, child);

    window_layout(widget->window);

    Event event = {EVENT_CHILD_ADDED, false};
    widget_dispatch_event(widget, &event);
}

void widget_remove_child(Widget *widget, Widget *child)
{
    assert(child->parent == widget);

    child->parent = NULL;
    child->window = NULL;
    list_remove(widget->childs, child);

    window_layout(widget->window);

    Event event = {EVENT_CHILD_REMOVED, false};
    widget_dispatch_event(widget, &event);
}

void widget_dump(Widget *widget, int depth)
{
    for (int i = 0; i < depth; i++)
    {
        printf("\t");
    }

    if (widget == NULL)
    {
        printf("<null>\n");
        return;
    }

    printf("%s(0x%08x) (%d, %d) %dx%d\n",
           widget->classname,
           widget,
           widget->bound.X,
           widget->bound.Y,
           widget->bound.width,
           widget->bound.height);

    list_foreach(Widget, child, widget->childs)
    {
        widget_dump(child, depth + 1);
    }
}

void widget_dispatch_event(Widget *widget, Event *event)
{
    widget_handle_event(widget, event);

    if (!event->accepted && widget->parent)
    {
        widget_dispatch_event(widget->parent, event);
    }
}

void widget_handle_event(Widget *widget, Event *event)
{
    if (widget->event)
    {
        widget->event(widget, event);
    }
}

void widget_paint(Widget *widget, Painter *painter)
{
    painter_push_clip(painter, widget_bound(widget));

    if (widget->paint)
    {
        widget->paint(widget, painter);
    }

    list_foreach(Widget, child, widget->childs)
    {
        widget_paint(child, painter);
    }

    painter_pop_clip(painter);
}

void widget_layout(Widget *widget)
{
    Layout layout = widget->layout;

    switch (layout.type)
    {
    case LAYOUT_STACK:
        list_foreach(Widget, child, widget->childs)
        {
            child->bound = widget_content_bound(widget);
        }
        break;

    case LAYOUT_HGRID:
    {
        int current = widget_content_bound(widget).X;
        int child_width = (widget_content_bound(widget).width - (layout.hspacing * (list_count(widget->childs) - 1))) / list_count(widget->childs);

        list_foreach(Widget, child, widget->childs)
        {
            child->bound = RECTANGLE(current, widget_content_bound(widget).position.Y, child_width, widget_content_bound(widget).height);
            current += child_width + layout.hspacing;
        }
    }
    break;

    case LAYOUT_VGRID:
    {
        int current = widget_content_bound(widget).Y;
        int child_height = (widget_content_bound(widget).height - (layout.vspacing * (list_count(widget->childs) - 1))) / list_count(widget->childs);

        list_foreach(Widget, child, widget->childs)
        {
            child->bound = RECTANGLE(widget_content_bound(widget).position.X, current, widget_content_bound(widget).width, child_height);
            current += child_height + layout.vspacing;
        }
    }
    break;

    case LAYOUT_HFLOW:
    {
        int fixed_child_count = 0;
        int fixed_child_total_width = 0;

        int fill_child_count = 0;

        list_foreach(Widget, child, widget->childs)
        {
            if (child->layout_attributes & LAYOUT_FILL)
            {
                fill_child_count++;
            }
            else
            {
                fixed_child_count++;
                fixed_child_total_width += widget_compute_size(child).X;
            }
        }

        int usable_space =
            widget_content_bound(widget).width -
            layout.hspacing * (list_count(widget->childs) - 1);

        int fill_child_total_width = MAX(0, usable_space - fixed_child_total_width);

        int fill_child_width = (fill_child_total_width) / MAX(1, fill_child_count);

        int current = widget_content_bound(widget).X;

        list_foreach(Widget, child, widget->childs)
        {
            if (child->layout_attributes & LAYOUT_FILL)
            {
                child->bound = RECTANGLE(
                    current,
                    widget_content_bound(widget).position.Y,
                    fill_child_width,
                    widget_content_bound(widget).height);

                current += fill_child_width + layout.hspacing;
            }
            else
            {
                child->bound = RECTANGLE(
                    current,
                    widget_content_bound(widget).position.Y,
                    widget_compute_size(child).X,
                    widget_content_bound(widget).height);

                current += widget_compute_size(child).X + layout.hspacing;
            }
        }
    }
    break;

    case LAYOUT_VFLOW:
    {
        int fixed_child_count = 0;
        int fixed_child_total_height = 0;

        int fill_child_count = 0;

        list_foreach(Widget, child, widget->childs)
        {
            if (child->layout_attributes & LAYOUT_FILL)
            {
                fill_child_count++;
            }
            else
            {
                fixed_child_count++;
                fixed_child_total_height += widget_compute_size(child).Y;
            }
        }

        int usable_space =
            widget_content_bound(widget).height -
            layout.vspacing * (list_count(widget->childs) - 1);

        int fill_child_total_height = MAX(0, usable_space - fixed_child_total_height);

        int fill_child_height = (fill_child_total_height) / MAX(1, fill_child_count);

        int current = widget_content_bound(widget).Y;

        list_foreach(Widget, child, widget->childs)
        {
            if (child->layout_attributes & LAYOUT_FILL)
            {
                child->bound = RECTANGLE(
                    widget_content_bound(widget).position.X,
                    current,
                    widget_content_bound(widget).width,
                    fill_child_height);

                current += fill_child_height + layout.vspacing;
            }
            else
            {
                child->bound = RECTANGLE(
                    widget_content_bound(widget).position.X,
                    current,
                    widget_content_bound(widget).width,
                    widget_compute_size(child).Y);

                current += widget_compute_size(child).Y + layout.vspacing;
            }
        }
    }
    break;

    default:
        break;
    }

    Event event = {EVENT_LAYOUT, true};
    widget_dispatch_event(widget, &event);

    list_foreach(Widget, child, widget->childs)
    {
        widget_layout(child);
    }
}

void widget_focus(Widget *widget)
{
    if (widget->window)
    {
        window_set_focused_widget(widget->window, widget);
    }
}

Point widget_compute_size(Widget *widget)
{
    if (widget->size)
    {
        return widget->size(widget);
    }
    else
    {
        return (Point){widget->bound.width, widget->bound.height};
    }
}

void widget_update(Widget *widget)
{
    window_update(widget->window, widget->bound);
}

Rectangle __widget_bound(Widget *widget)
{
    return widget->bound;
}

Rectangle __widget_content_bound(Widget *widget)
{
    return rectangle_shrink(__widget_bound(widget), widget->insets);
}