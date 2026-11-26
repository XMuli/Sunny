// SPDX-SnippetCopyrightText: 2024-2025 SunnyCapturer Project
// SPDX-Author: ffiirree <ice_qi@163.com>
// SPDX-Author: Wengeng Zhang <xmulitech@gmail.com>

#ifndef MAGNIFIER_H
#define MAGNIFIER_H

#include <QWidget>
#include <QLabel>

namespace Ui
{
    class Magnifier;
}

class Magnifier final : public QWidget
{
    Q_OBJECT

public:
    enum class ColorFormat : int
    {
        HEX,
        INT,
        FLT,
        AUTO,
    };

public:
    explicit Magnifier(QWidget *parent = nullptr);
    ~Magnifier();

    [[nodiscard]] QColor color() const { return color_; }

    [[nodiscard]] QString colorname(ColorFormat = ColorFormat::AUTO) const;

    void format(ColorFormat format) { cfmt_ = format; }

    [[nodiscard]] ColorFormat format() const { return cfmt_; }

    void toggleFormat()
    {
        cfmt_ = static_cast<ColorFormat>((static_cast<int>(cfmt_) + 1) % 3);
        update();
    }

    //
    void setGrabPixmap(const QPixmap&, const QRect&);
    void adjustedPixmapSize();
    void setLabColorNameMinWidth(const int &n);

protected:
    bool eventFilter(QObject *, QEvent *) override;

    void showEvent(QShowEvent *) override;
    void paintEvent(QPaintEvent *) override;

    void closeEvent(QCloseEvent *) override;

private:
    [[nodiscard]] QRect   grabRect() const;
    [[nodiscard]] QPixmap grab() const;
    [[nodiscard]] QPoint  position() const;

    QPixmap desktop_screenshot_{};
    QRect   desktop_rect_{}; // desktop | virtual screen

    int pix_scale_{ 10 };
    QSize  msize_{ 18, 12 };
    QSize  psize_{ 0, 0 };
    QColor color_;
    double cur_scrn_scale_{1.0};

    ColorFormat cfmt_{ ColorFormat::HEX };
    Ui::Magnifier *ui;
};

#endif // MAGNIFIER_H
