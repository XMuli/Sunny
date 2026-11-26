// SPDX-SnippetCopyrightText: 2024-2025 SunnyCapturer Project
// SPDX-Author: ffiirree <ice_qi@163.com>
// SPDX-Author: Wengeng Zhang <xmulitech@gmail.com>

#include "magnifier.h"
#include "ui_magnifier.h"
#include "probe/graphics.h"

#include <fmt/format.h>
#include <QApplication>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>

#include "colorpanel.h"
#include "helper.h"

Magnifier::Magnifier(QWidget *parent)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::ToolTip)
    , ui(new Ui::Magnifier)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);

    QWidget::setVisible(false);

    // pixmap size
    psize_ = msize_ * pix_scale_;
    ui->lab_pixmap->resize(psize_);
    setLabColorNameMinWidth(60);

    // label size
    ui->lab_coordinate->setObjectName("magnifier-color-label");
    ui->lab_color_name->setObjectName("magnifier-color-name");

    const int& btnFixHeight = ui->lab_color_name->height() * 2 / 3;
    ui->btn_color->setFixedSize(btnFixHeight, btnFixHeight);
    ui->btn_color->setType(ColorButton::Type::T_color_magnifier);

    // TODO: listen system level mouse move event, not just this APP
    qApp->installEventFilter(this);
}

Magnifier::~Magnifier() { delete ui; }

QRect Magnifier::grabRect() const
{
    auto mouse_pos = QCursor::pos();

    if (!desktop_screenshot_.isNull()) {
        mouse_pos -= desktop_rect_.topLeft();
    }

    return {
        mouse_pos.x() - msize_.width() / 2,
        mouse_pos.y() - msize_.height() / 2,
        msize_.width(),
        msize_.height(),
    };
}

QString Magnifier::colorname(const ColorFormat format) const
{
    // clang-format off
    switch ((format == ColorFormat::AUTO) ? cfmt_ : format) {
    case ColorFormat::INT:  return QString::fromStdString(fmt::format("{:3d}, {:3d}, {:3d}",        color_.red(),  color_.green(),  color_.blue()));
    case ColorFormat::FLT:  return QString::fromStdString(fmt::format("{:4.2f}, {:4.2f}, {:4.2f}",  color_.redF(), color_.greenF(), color_.blueF()));
    default:                return QString::fromStdString(fmt::format("#{:02X}{:02X}{:02X}",        color_.red(),  color_.green(),  color_.blue()));
    }
    // clang-format on
}

QPoint Magnifier::position() const
{
    const auto cx = QCursor::pos().x();
    const auto cy = QCursor::pos().y();

    int mx = (desktop_rect_.right() - cx > width()) ? cx + 16 : cx - width() - 16;
    int my = (desktop_rect_.bottom() - cy > height()) ? cy + 16 : cy - height() - 16;
    return { mx, my };
}

void Magnifier::adjustedPixmapSize()
{
    QScreen* scrn = Helper::currScrn();
    if (scrn) {
        cur_scrn_scale_ = Helper::dpiScale(scrn);
        msize_ = QSize(18, 12) * cur_scrn_scale_;
    }

    // pixmap size: It must be done in this order, otherwise there will be UI issues or warning messages!!!
    // and fix: Magnifier show QWindowsWindow::setGeometry: Unable to set geometry
    psize_ = msize_ * pix_scale_;
    ui->lab_pixmap->setMinimumSize(psize_);
    setMaximumWidth(psize_.width() + layout()->contentsMargins().left() + layout()->contentsMargins().right());
    // auto t1 = ui->lab_coordinate->sizeHint().height();
    // auto t2 = ui->color_name_dlg->sizeHint().height();
    // const auto& vlayout = ui->verticalLayout_2->contentsMargins();
    // auto t3 = vlayout.top() + vlayout.bottom() + ui->verticalLayout_2->spacing();
    // const auto& vLayout1 = ui->verticalLayout->contentsMargins();
    // auto t4 = vLayout1.top() + vLayout1.bottom() + ui->verticalLayout->spacing();
    // setMaximumHeight(psize_.height() + t1 + t2 + t3 + t4);
    adjustSize();
}

void Magnifier::setLabColorNameMinWidth(const int &n)
{
    ui->lab_color_name->setMinimumWidth(n * cur_scrn_scale_);
}

bool Magnifier::eventFilter(QObject *obj, QEvent *event)
{
    // FIXME: grab the system-wide mouse event
    if (event->type() == QEvent::MouseMove && isVisible()) {
        static QScreen *scrn = nullptr;
        QScreen *newScrn = QGuiApplication::screenAt(QCursor::pos());
        if (scrn != newScrn) {
            scrn = newScrn;
            adjustedPixmapSize();
        }

        update();
        move(position());
    }
    return qApp->eventFilter(obj, event);
}

void Magnifier::showEvent(QShowEvent *event)
{
    move(position());
    QWidget::showEvent(event);
}

void Magnifier::setGrabPixmap(const QPixmap& pixmap, const QRect& rect)
{
    desktop_screenshot_ = pixmap;
    desktop_rect_       = rect;
}

QPixmap Magnifier::grab() const
{
    const auto rect = grabRect();
    if (desktop_screenshot_.isNull()) {
        return QGuiApplication::primaryScreen()->grabWindow(
            probe::graphics::virtual_screen().handle, rect.x(), rect.y(), rect.width(), rect.height());
    }

    return desktop_screenshot_.copy(rect);
}

void Magnifier::paintEvent(QPaintEvent *)
{
    QPainter pa_bg;
    pa_bg.begin(this);
    pa_bg.setPen(Qt::NoPen);
    pa_bg.setBrush(Qt::NoBrush);
    pa_bg.fillRect(rect(), QColor(0, 0, 0, 255 * 0.65));

    auto& lab_pixmap = ui->lab_pixmap;
    const QRect& pixRt = lab_pixmap->rect();
    const QRect& rt = QRect(lab_pixmap->mapToParent(pixRt.topLeft()), pixRt.size());
    int margin = 1;
    pa_bg.setPen(QPen(Qt::white, 1));
    pa_bg.drawRect(rt.adjusted(-margin, -margin, margin-1, margin-1));
    margin = 2;
    pa_bg.setPen(QPen(Qt::black, 1));
    pa_bg.drawRect(rect());
    pa_bg.drawRect(rt.adjusted(-margin, -margin, margin-1, margin-1));
    pa_bg.end();

    // *******************************************************
    // draw pixmap
    QPixmap draw = grab().scaled(psize_, Qt::KeepAspectRatioByExpanding);
    QPainter pa(&draw);

    const bool& is_integer_scale = std::floor(cur_scrn_scale_) == cur_scrn_scale_; // The current screen has integer scaling, such as 1.0 or 2.0, but not 1.5.
    const int offset = pix_scale_ / 2;
    int centerSize = pix_scale_;
    int centerY = draw.height() / 2 - centerSize / 2;
    int centerX = draw.width() / 2 - centerSize / 2;
    QRect rect = QRect(centerX + (is_integer_scale ? offset : 0), centerY + offset, centerSize, centerSize).adjusted(-1, -1, 0, 0);
    QImage image = draw.toImage();
    color_ = QColor(draw.toImage().pixelColor(rect.center()));

    // cross curve
    pa.setBrush(Qt::NoBrush);
    pa.setPen(QPen(Helper::highlightColor(0.2), pix_scale_, Qt::SolidLine));
    pa.drawLine(0, draw.height() / 2 + offset, draw.width(), draw.height() / 2+ offset);
    pa.drawLine(draw.width() / 2 + (is_integer_scale ? offset : 0), 0, draw.width() / 2  + (is_integer_scale ? offset : 0), draw.height());

    // Draw a black border square with a center point
    pa.setPen(QPen(Qt::black, 1, Qt::SolidLine));
    pa.drawRect(rect.adjusted(-1, -1, 1, 1));
    pa.setPen(QPen(Qt::white, 1, Qt::SolidLine));
    pa.setBrush(QBrush(color_));
    pa.drawRect(rect);

    lab_pixmap->setPixmap(draw);

    const auto text = QString("(%1, %2)").arg(QCursor::pos().x()).arg(QCursor::pos().y());
    ui->lab_coordinate->setText(text);
    ui->lab_color_name->setText(colorname());
    ui->btn_color->setColor(color_, false);
}

void Magnifier::closeEvent(QCloseEvent *event)
{
    desktop_screenshot_ = {};
    QWidget::closeEvent(event);
}
