// SPDX-SnippetCopyrightText: 2024-2025 SunnyCapturer Project
// SPDX-Author: Wengeng Zhang <xmulitech@gmail.com>
#include "tips.h"
#include <QDebug>
#include <QRect>
#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <QString>
#include <QtGlobal>
#include "helper.h"

Tips::Tips(const QString& text, const TipsType& type, QWidget *parent)
    : QWidget(parent)
    , m_text(text)
    , m_type(type)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | windowFlags());

    QFont font = this->font();
    if (m_type == TipsType::TT_picked_rect_tips) {
        font.setPointSize(12);
    } else if (m_type == TipsType::TT_point_changed_tips) {
        font.setPointSize(76);
    } else if (m_type == TipsType::TT_countdown_tips) {
        font.setPointSize(40);
    } else if (m_type == TipsType::TT_keyboard_operation_tips) {
        font.setPointSize(12);
    }

    m_font = font;
}

void Tips::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e)
    QPainter pa(this);
    pa.setFont(m_font);
    pa.setRenderHint(QPainter::Antialiasing);

    pa.save();
    const int penWidth = 1;
    pa.setPen(QPen(Helper::highlightColor(0.6), penWidth));
    pa.setBrush(Qt::NoBrush);
    pa.drawRect(rect());
    QColor white(255, 255, 255, 0.1 * 255);
    QColor black(19, 19, 19, 0.6 * 255);
    pa.setPen(QPen(white, penWidth));
    pa.setBrush(black);
    pa.drawRect(rect().adjusted(1, 1, -1, -1));

    white.setAlphaF(0.8);
    pa.setPen(QPen(white, 100));
    if (m_type == TipsType::TT_picked_rect_tips) {

        QRect rt1 = textRect("px");
        QRect rt2 = textRect(m_text);
        resize(rt1.size().width() + rt2.size().width(), qMax(rt1.size().height(), rt2.size().height()));
        const QPoint& p1 = rect().topRight() - QPoint(rt1.width(), 0);
        const QPoint& p2 = rect().bottomRight() - QPoint(rt1.width(), 0);

        white.setAlphaF(0.4);
        pa.setPen(QPen(white, penWidth));
        pa.setBrush(Qt::NoBrush);
        pa.drawLine(p1, p2);
        white.setAlphaF(0.8);
        pa.setPen(QPen(white, penWidth));
        pa.drawText(QRect(p1, rect().bottomRight()), Qt::AlignCenter, "px");
        pa.drawText(QRect(rect().topLeft(), p2), Qt::AlignCenter, m_text);
    } else if (m_type == TipsType::TT_point_changed_tips) {

        const QRect& rt = textRect(m_text);
        const int& maxWidth = qMax(rt.width(), rt.height());
        resize(maxWidth, maxWidth);
        pa.setBrush(Qt::NoBrush);
        pa.drawText(rect(), Qt::AlignCenter, m_text);
    } else if (m_type == TipsType::TT_countdown_tips) {

        const QRect& rt = textRect(m_text);
        const int& maxWidth = qMax(rt.width(), rt.height());
        resize(maxWidth * 4 / 3, maxWidth);
        pa.setBrush(Qt::NoBrush);
        pa.drawText(rect(), Qt::AlignCenter, m_text);

    } else if (m_type == TipsType::TT_keyboard_operation_tips) {
        pa.drawText(rect().center(), m_text);
    }

    pa.restore();
}

void Tips::setType(TipsType newType)
{
    m_type = newType;
}

QRect Tips::textRect(const QString &text)
{
    QFontMetrics fontMetrics(m_font);
    QRect textRect = fontMetrics.boundingRect(text);
    textRect.moveCenter(rect().center());

    int margin = 10;
    if (m_type == TipsType::TT_picked_rect_tips) {
        margin = 6;
    } else if (m_type == TipsType::TT_point_changed_tips) {
        margin = 10;
    } else if (m_type == TipsType::TT_keyboard_operation_tips) {
        margin = 10;
    }

    const auto& ret = textRect.adjusted(-margin, -margin, margin, margin);
    const int length = qMax<int>(ret.size().width(), ret.size().height());
    setFixedSize(length, length);
    return ret;
}

void Tips::setText(const QString &newText)
{
    m_text = newText;
    if (m_type == TipsType::TT_countdown_tips) m_text += "s";
}

void Tips::setColor(const QColor &newColor)
{
    m_color = newColor;
}
