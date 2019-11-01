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

#ifndef MAIN_SETTINGS_WIDGET_H
#define MAIN_SETTINGS_WIDGET_H

#include "expanding_list/expanding_list_button.h"
#include <QList>
#include <QMap>
#include <QWidget>

class expanding_list_button;
class expanding_list_widget;
class searchbar;
class settings_display;
class settings_widget;

class QFrame;
class QHBoxLayout;
class QPushButton;
class QScrollArea;
class QVBoxLayout;

class QScrollBar;

class main_settings_widget : public QWidget
{
    Q_OBJECT

public:
    explicit main_settings_widget(QWidget* parent = 0);

Q_SIGNALS:
    void close();

public Q_SLOTS:
    void handle_restore_defaults_clicked();
    void handle_cancel_clicked();
    void handle_ok_clicked();
    void handle_button_selected(expanding_list_button* button);
    void handle_text_edited(const QString& text);
    void handle_setting_updated(settings_widget* sender, const QString& key, const QVariant& value);

private:
    void init_widgets();
    void make_section(const QString& label, const QString& name, const QString& icon_path);
    void register_widget(const QString& section_name, settings_widget* widget);
    bool check_conflict(settings_widget* widget, const QVariant& value) const;
    void make_exclusive_group(const QString& name);
    void assign_exclusive_group(const QString& group_name, settings_widget* widget);
    void release_exclusive_group(const QString& group_name, settings_widget* widget);
    void hide_all_settings();
    void show_all_settings();
    void remove_all_highlights();

    QHBoxLayout* m_layout;
    expanding_list_widget* m_expanding_list_widget;
    QVBoxLayout* m_vertical_layout;
    QScrollBar* m_scrollbar;

    QFrame* m_searchbar_container;
    QHBoxLayout* m_searchbar_layout;
    searchbar* m_searchbar;

    settings_display* m_settings_display;
    QScrollArea* m_scroll_area;
    QFrame* m_content;
    QHBoxLayout* m_content_layout;
    QFrame* m_settings_container;
    QVBoxLayout* m_container_layout;

    QHBoxLayout* m_button_layout;

    QPushButton* m_restore_defaults;
    QPushButton* m_cancel;
    QPushButton* m_ok;

    QMap<QString, QList<settings_widget*>*> m_map;
    QMap<QString, expanding_list_button*> m_sections;
    QString m_active_section;

    QList<settings_widget*> m_all_settings;
    
    QList<QString> m_exclusive_groups;
    QMap<settings_widget*, QString> m_exclusive_w2g;
    QMap<QString, QList<settings_widget*>*> m_exclusive_g2w;
};

#endif    // MAIN_SETTINGS_WIDGET_H
