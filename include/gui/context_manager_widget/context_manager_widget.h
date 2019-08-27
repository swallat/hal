//  MIT License
//
//  Copyright (c) 2019 Ruhr-University Bochum, Germany, Chair for Embedded Security. All Rights reserved.
//  Copyright (c) 2019 Marc Fyrbiak, Sebastian Wallat, Max Hoffmann ("ORIGINAL AUTHORS"). All rights reserved.
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in all
//  copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//  SOFTWARE.

#ifndef CONTEXT_MANAGER_WIDGET
#define CONTEXT_MANAGER_WIDGET

#include "content_widget/content_widget.h"

#include "def.h"

#include "graph_widget/contexts/dynamic_context.h"

#include <QListView>
#include <QListWidget>
#include <QMap>
#include <QPoint>

class graph_context;
class graph_tab_widget;

class context_manager_widget : public content_widget
{
    Q_OBJECT

public:
    context_manager_widget(graph_tab_widget* tab_view, QWidget* parent = nullptr);
    void resizeEvent(QResizeEvent* event);

public Q_SLOTS:
    void handle_context_created(dynamic_context* context);
    void handle_context_renamed(dynamic_context* context);
    void handle_context_removed(dynamic_context* context);

private:
    graph_tab_widget* m_tab_view;

    QListWidget* m_list_widget;

    QModelIndex m_clicked_index;

    void handle_context_menu_request(const QPoint& point);

    void handle_create_context_clicked();
    void handle_open_context_clicked();
    void handle_rename_context_clicked();
    void handle_delete_context_clicked();
    void handle_item_double_clicked(QListWidgetItem*);

    void show_context_menu(const QPoint& point);

    u32 m_context_counter = 0;
};

#endif    // CONTEXT_MANAGER_WIDGET
