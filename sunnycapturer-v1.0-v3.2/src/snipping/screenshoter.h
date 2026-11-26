// SPDX-SnippetCopyrightText: 2024-2025 SunnyCapturer Project
// SPDX-Author: ffiirree <ice_qi@163.com>
// SPDX-Author: Wengeng Zhang <xmulitech@gmail.com>

#ifndef CAPTURER_SCREEN_SHOTER_H
#define CAPTURER_SCREEN_SHOTER_H

#include "canvas/canvas.h"
#include "canvas/command.h"
#include "hunter.h"
#include "magnifier.h"
#include "menu/editing-menu.h"
#include "resizer.h"
#include "selector.h"
#include "guideui.h"
#include "tips.h"
#include "nettencent.h"
#include <QGraphicsView>
#include <QPixmap>

class ScreenShoter final : public QGraphicsView
{
    Q_OBJECT

public:
    explicit ScreenShoter(QWidget *parent = nullptr);
    virtual ~ScreenShoter();

signals:
    void pinData(const std::shared_ptr<QMimeData>&);

    void saved(const QString& path);

public slots:
    void start();
    void exit();

    void repeat();

    void imageTranslate();
    void extractText();
    void onImgTransFinshed(const QString& path);

    void                       onSave(bool bManualSave);
    void                       copy();
    void                       pin();
    std::pair<QPixmap, QPoint> snip();

    void setStyle(const SelectorStyle& style);

    void moveMenu();

    void updateCursor(ResizerLocation);

    bool screenshot(const probe::geometry_t&);
    void setBackground(const QPixmap&, const probe::geometry_t&);

    void createItem(const QPointF& pos);
    void finishItem();

#ifdef Q_OS_LINUX
    void DbusScreenshotArrived(uint, const QVariantMap&);
#endif

protected:
    bool eventFilter(QObject *, QEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    void wheelEvent(QWheelEvent *) override;

    void keyPressEvent(QKeyEvent *) override;
    void keyReleaseEvent(QKeyEvent *) override;
    void mouseDoubleClickEvent(QMouseEvent *) override;

private:
    void                 registerShortcuts();
    QBrush               mosaicBrush();
    [[nodiscard]] size_t history_index();
    void                 showPointTips(const QString &text);

    Selector       *selector_{}; // Layer 1
    canvas::Canvas *scene_{};

    GraphicsItemWrapper *creating_item_{};
    int                  counter_{ 0 };

    static EditingMenu        *menu_;           // editing menu
    Magnifier          *magnifier_{};      // magnifier
    QPointer<GuideUI>  guideui_{};
    QPointer<Tips>     point_tips_{};      // top left pen width tips
    QPointer<QTimer>   timer_point_tips_;

    // history
    static std::vector<hunter::prey_t> history_;
    size_t                      history_idx_{ 0 };
    double                      cur_scrn_scale_{ 1.0 };

    QUndoStack *undo_stack_{};

    // image Translate and ocr
    NetTencent                  netmgn_;
};

#endif //! CAPTURER_SCREEN_SHOTER_H
