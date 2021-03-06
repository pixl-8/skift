#pragma once

#include <libutils/Callback.h>
#include <libwidget/Panel.h>
#include <libwidget/model/TextModel.h>

namespace panel
{

class SearchBar : public Widget::Panel
{
public:
    SearchBar(Component *parent, RefPtr<Widget::TextModel> model);
};

} // namespace panel
