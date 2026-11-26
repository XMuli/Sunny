#include "texture-widget.h"

#include <QPainter>

TextureWidget::TextureWidget(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);

    connect(
        this, &TextureWidget::arrived, this, [this] { update(); }, Qt::QueuedConnection);
}

void TextureWidget::present(const QPixmap& pixmap)
{
    std::lock_guard lock(mtx_);
    frame_ = pixmap;

    emit arrived();
}

void TextureWidget::present(const QImage& image)
{
    std::lock_guard lock(mtx_);
    frame_ = QPixmap::fromImage(image);

    emit arrived();
}

void TextureWidget::paintEvent(QPaintEvent *)
{
    if (std::lock_guard lock(mtx_); !frame_.isNull()) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        const auto winrect = rect();

        const auto imgsize = frame_.size().scaled(winrect.width(), winrect.height(), Qt::KeepAspectRatio);
        auto       imgrect = QRect{ { 0, 0 }, imgsize };

        imgrect.moveCenter(winrect.center());

        painter.drawPixmap(imgrect, frame_);
    }
}
