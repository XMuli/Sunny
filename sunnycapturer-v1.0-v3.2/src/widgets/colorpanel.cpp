#include "colorpanel.h"

#include <QColorDialog>
#include <QGridLayout>
#include <QPainter>
#include <QStyleOptionFrame>
#include <QWheelEvent>

ColorButton::ColorButton(QWidget *parent)
    : ColorButton(Qt::blue, parent)
{}

ColorButton::ColorButton(const QColor& c, QWidget *parent)
    : QPushButton(parent), color_(c)
{
    connect(this, &QPushButton::clicked, [this]() { emit clicked(color_); });
}

void ColorButton::setColor(const QColor& c, bool silence)
{
    if (!c.isValid() || color_ == c) return;

    color_ = c;
    update();

    if (!silence) emit changed(color_);
}

void ColorButton::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QColor wihte = QColor(255, 255, 255, 1 * 255);
    const QColor black = QColor(0, 0, 0, 0.9 * 255);

    if (type_ == Type::T_color_pick) {

        int margin = 2;
        if (underMouse()) {
            // If the mouse hovers over the button, draw a raised effect.
            painter.setPen(QPen(black, 1));
            painter.setBrush(color_);
            margin -= 1;
            painter.drawRect(rect().adjusted(margin, margin, -margin, -margin));
        } else {
            // Draw a 1px black line on the border.
            painter.setPen(QPen(black, 1));
            painter.setBrush(color_);
            painter.drawRect(rect().adjusted(margin, margin, -margin, -margin));
        }

    } else if (type_ == Type::T_color_magnifier) {
        int margin = 0;
        painter.setPen(QPen(wihte, 2));
        painter.setBrush(color_);
        painter.drawRect(rect().adjusted(margin, margin, -margin, -margin));
    }

}

void ColorButton::setType(Type newType)
{
    type_ = newType;
}

///////////////////////////////////////////////////////////////////////////
ColorDialogButton::ColorDialogButton(QWidget *parent)
    : ColorButton(Qt::blue, parent)
{}

ColorDialogButton::ColorDialogButton(const QColor& color, QWidget *parent)
    : ColorButton(color, parent)
{
    // FIXME(Linux): can't stay on top
    connect(this, &ColorDialogButton::clicked, [this]() {
        constexpr auto options = QColorDialog::DontUseNativeDialog | QColorDialog::ShowAlphaChannel;
        setColor(QColorDialog::getColor(color_, this, tr("Select Color"), options), false);
    });
}

void ColorDialogButton::wheelEvent(QWheelEvent *event)
{
    QColor tmp(color_);
    tmp.setAlpha(std::clamp<int>(color_.alpha() + event->angleDelta().y() / 60, 0, 255));
    setColor(tmp, false);
}

///////////////////////////////////////////////////////////////////////////
ColorPanel::ColorPanel(QWidget *parent)
    : QWidget(parent)
{
    auto layout = new QGridLayout();
    layout->setContentsMargins({ 2, 0, 5, 0 });
    layout->setSpacing(0);

    color_dialog_btn_ = new ColorDialogButton(Qt::red);
    color_dialog_btn_->setObjectName("selected-color");
    layout->addWidget(color_dialog_btn_, 0, 0, 2, 2);

#define ADD_COLOR(COLOR, X, Y)                                                                             \
    do {                                                                                                   \
        auto cbtn = new ColorButton(COLOR);                                                                \
        layout->addWidget(cbtn, X, Y);                                                                     \
        connect(cbtn, &ColorButton::clicked, [this](auto c) { setColor(c, false); });                      \
    } while (0)

    QStringList colors;
    colors << "#000000" << "#7F7F7F" << "#880015" << "#ED1C24" << "#FF7F27"
           << "#FFF200" << "#22B14C" << "#00A2E8" << "#3F48CC" << "#A349A4"
           << "#FFFFFF" << "#C3C3C3" << "#B97A57" << "#FFAEC9" << "#FFC90E"
           << "#EFE4B0" << "#B5E61D" << "#99D9EA" << "#7092BE" << "#C8BFE7";

    const int nCount = colors.count() / 2;
    for (int i = 0; i < nCount; ++i) ADD_COLOR(QColor(colors.at(i)), 0,  2 + i);
    for (int i = 0; i < nCount; ++i) ADD_COLOR(QColor(colors.at(nCount + i)), 1,  2 + i);
#undef ADD_COLOR

    setLayout(layout);

    connect(color_dialog_btn_, &ColorDialogButton::changed, this, &ColorPanel::changed);
}
