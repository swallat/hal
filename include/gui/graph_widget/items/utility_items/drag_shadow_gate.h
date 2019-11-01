#ifndef DRAG_SHADOW_GATE_H
#define DRAG_SHADOW_GATE_H

#include <QGraphicsObject>

class graphics_gate;

class QParallelAnimationGroup;
class QPropertyAnimation;

class drag_shadow_gate : public QGraphicsObject
{
    Q_OBJECT

public:
    explicit drag_shadow_gate();

    void start(const QPointF& posF, const QSizeF& sizeF);
    void stop();

    qreal width() const;
    qreal height() const;
    QSizeF size() const;

    void set_width(const qreal width);
    void set_height(const qreal height);

    void set_fits(const bool fits);
    bool fits() const;

    static void set_lod(const qreal lod);
    static void load_settings();

protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) Q_DECL_OVERRIDE;
    virtual QRectF boundingRect() const Q_DECL_OVERRIDE;
    virtual QPainterPath shape() const Q_DECL_OVERRIDE;

private:
    // static bool s_delegate_paint;
    static qreal s_lod;
    static QPen s_pen;
    static QColor s_color_pen[];
    static QColor s_color_solid[];
    static QColor s_color_translucent[];

    bool m_fits;

    qreal m_width;
    qreal m_height;
};

#endif // DRAG_SHADOW_GATE_H
