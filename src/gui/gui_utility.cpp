#include "gui_utility.h"

#include "svg_icon_engine/svg_icon_engine.h"

#include <QFile>

namespace gui_utility
{
    QString get_svg_data(const QString& svg_path)
    {
        QFile file(svg_path);
        file.open(QFile::ReadOnly);
        QString string(file.readAll());
        return string;
    }

    void change_svg_color(QString& svg_data, const QColor& from, const QColor& to)
    {
        QRegExp regex(from.name());
        svg_data.replace(regex, to.name().toUtf8());
    }

    void change_all_svg_colors(QString& svg_data, const QColor& to)
    {
        QRegExp regex("#[0-9a-f]{6}", Qt::CaseInsensitive);
        svg_data.replace(regex, to.name().toUtf8());
    }

    QIcon get_icon_from_svg_data(const QString& svg_data)
    {
        return QIcon(new svg_icon_engine(svg_data.toStdString()));
    }

    QIcon get_styled_svg_icon(const QString& from_to_colors, const QString& svg_path)
    {
        QString svg_data = get_svg_data(svg_path);
        if (svg_data.isEmpty())
        {
            //print error
            return QIcon();
        }

        if (!from_to_colors.isEmpty())
        {
            QRegExp color_regex("#[0-9a-f]{6}", Qt::CaseInsensitive);
            QRegExp all_to_color_regex("\\s*all\\s*->\\s*#[0-9a-f]{6}\\s*", Qt::CaseInsensitive);
            QRegExp color_to_color_regex("\\s*(#[0-9a-f]{6}\\s*->\\s*#[0-9a-f]{6}\\s*,\\s*)*#[0-9a-f]{6}\\s*->\\s*#[0-9a-f]{6}\\s*", Qt::CaseInsensitive);
            if (all_to_color_regex.exactMatch(from_to_colors))
            {
                color_regex.indexIn(from_to_colors);
                QString color = color_regex.cap(0);
                svg_data.replace(color_regex, color.toUtf8());
            }
            else if (color_to_color_regex.exactMatch(from_to_colors))
            {
                QString copy = from_to_colors;
                copy         = copy.simplified();
                copy.replace(" ", "");

                QStringList list = copy.split(",");

                for (QString string : list)
                {
                    QString from_color = string.left(7);
                    QString to_color   = string.right(7);

                    QRegExp regex(from_color);
                    svg_data.replace(regex, to_color.toUtf8());
                }
            }
            else
            {
                //print error
                return QIcon();
            }
        }

        return get_icon_from_svg_data(svg_data);
    }

    QColor get_random_color()
    {
        static qreal h = 0.5;

        h += 0.6180339887498948;

        if (h > 1)
            --h;

        QColor c;
        c.setHsvF(h, 0.8, 0.95);    // (MAYBE) GET S AND V FROM STYLESHEET OR CYCLE 3 DIMENSIONAL
        return c;
    }
}    // namespace gui_utility
