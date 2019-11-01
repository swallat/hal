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

#ifndef KEYBIND_SETTINGS_H
#define KEYBIND_SETTINGS_H

#include "settings_widget.h"

#include "gui/keybind_edit/keybind_edit.h"

#include <QCheckBox>
#include <QStringList>

class keybind_setting : public settings_widget
{
    Q_OBJECT

public:
    keybind_setting(const QString& key, const QString& title, const QString& description, QWidget *parent = nullptr);

    virtual void load(const QVariant& value) Q_DECL_OVERRIDE;
    virtual QVariant value() Q_DECL_OVERRIDE;
    //virtual void rollback() Q_DECL_OVERRIDE;

private:
    keybind_edit* m_keybind_edit;
    void on_keybind_changed();
    void on_keybind_edit_rejected();

};

#endif //KEYBIND_SETTINGS_H

