// SPDX-SnippetCopyrightText: 2024-2025 SunnyCapturer Project
// SPDX-Author: ffiirree <ice_qi@163.com>
// SPDX-Author: Wengeng Zhang <xmulitech@gmail.com>

#include "screenshoter.h"

#include "circlecursor.h"
#include "clipboard.h"
#include "config.h"
#include "logging.h"
#include "communication.h"
#include "helper.h"
#include "helper/net_helper.h"

#include <probe/system.h>
#include <QApplication>
#include <QDateTime>
#include <QFileDialog>
#include <QMouseEvent>
#include <QTimer>
#include <QScreen>
#include <QShortcut>

#ifdef Q_OS_LINUX
#include <QDBusInterface>
#include <QDBusReply>
#endif

EditingMenu *ScreenShoter::menu_ = nullptr;
std::vector<hunter::prey_t> ScreenShoter::history_{};

ScreenShoter::ScreenShoter(QWidget *parent)
    : QGraphicsView(parent)
      , point_tips_(new Tips("", TipsType::TT_point_changed_tips, this))
      , timer_point_tips_(new QTimer(this))
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::BypassWindowManagerHint);

#ifdef NDEBUG
    setWindowFlag(Qt::WindowStaysOnTopHint, true);
#endif

    setAttribute(Qt::WA_TranslucentBackground); // FIXME: otherwise, the screen will be black when full
                                                // screen on Linux

    setFrameStyle(QGraphicsView::NoFrame);
    setContentsMargins({});
    setViewportMargins({});

    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);      // Fix: 绘画多个手绘三个圆轨迹，后面会覆盖前面的

    selector_   = new Selector(this);
    if (!menu_) menu_       = new EditingMenu(nullptr);
    magnifier_  = new Magnifier(this);
    guideui_    = new GuideUI(this);
    undo_stack_ = new QUndoStack(this);

    guideui_->setStatus(SelectorStatus::READY);
    guideui_->hide();

    timer_point_tips_->setInterval(5000);
    point_tips_->raise();
    point_tips_->hide();

    scene_ = new canvas::Canvas(this);
    setScene(scene_);

    setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);

    connect(scene_, &canvas::Canvas::focusItemChanged, this, [this](auto item, auto, auto reason) {
        if (item != nullptr && reason == Qt::MouseFocusReason) {
            scene()->clearSelection(); // clear selection of text items

            const auto wrapper = dynamic_cast<GraphicsItemWrapper *>(item);
            menu_->paintGraph((wrapper->graph() == canvas::pixmap) ? canvas::none : wrapper->graph());

            menu_->fill(wrapper->filled());
            menu_->setPen(wrapper->pen());
            menu_->setFont(wrapper->font());
        }
    });

    connect(menu_, &EditingMenu::sigTranslate, this, &ScreenShoter::imageTranslate);
    connect(menu_, &EditingMenu::sigExtractText, this, &ScreenShoter::extractText);

    connect(menu_, &EditingMenu::save, this, &ScreenShoter::onSave);
    connect(menu_, &EditingMenu::copy, this, &ScreenShoter::copy);
    connect(menu_, &EditingMenu::pin, this, &ScreenShoter::pin);
    connect(menu_, &EditingMenu::exit, this, &ScreenShoter::exit);

    connect(menu_, &EditingMenu::undo, undo_stack_, &QUndoStack::undo);
    connect(menu_, &EditingMenu::redo, undo_stack_, &QUndoStack::redo);

    connect(menu_, &EditingMenu::graphChanged, [this](auto graph) {
        if (graph != canvas::none) {
            selector_->status(SelectorStatus::LOCKED);
        }

        updateCursor(ResizerLocation::DEFAULT);
    });

    connect(menu_, &EditingMenu::penChanged, [this](auto graph, auto pen) {
        if (auto item = scene_->focusItem(); item && item->graph() == graph && item->pen() != pen) {
            undo_stack_->push(new PenChangedCommand(item, item->pen(), pen));
        }
    });

    connect(menu_, &EditingMenu::fontChanged, [this](auto graph, auto font) {
        if (auto item = scene_->focusOrFirstSelectedItem();
            item && item->graph() == graph && item->font() != font) {
            showPointTips(QString::number(font.pointSize()));
            undo_stack_->push(new FontChangedCommand(item, item->font(), font));
        }
    });

    connect(menu_, &EditingMenu::fillChanged, [this](auto graph, auto filled) {
        if (auto item = scene_->focusItem(); item && item->graph() == graph && item->filled() != filled) {
            undo_stack_->push(new FillChangedCommand(item, filled));
        }
    });

    connect(menu_, &EditingMenu::imageArrived, [this](auto pixmap) {
        GraphicsItemWrapper *item = new GraphicsPixmapItem(pixmap, selector_->selected(true).center());

        undo_stack_->push(new CreatedCommand(scene(), dynamic_cast<QGraphicsItem *>(item)));

        item->onhovered([this](auto rl) { updateCursor(rl); });
        item->onmoved([=, this](auto opos) {
            undo_stack_->push(new MoveCommand(dynamic_cast<QGraphicsItem *>(item), opos));
        });
        item->onrotated([=, this](auto angle) { undo_stack_->push(new RotateCommand(item, angle)); });
        item->onresized([=, this](auto g, auto l) { undo_stack_->push(new ResizeCommand(item, g, l)); });
    });

    // hide/show menu
    connect(selector_, &Selector::selecting, menu_, &QWidget::hide);
    connect(selector_, &Selector::captured, [this]() {
        menu_->show();
        moveMenu();
    });

    // move menu
    connect(selector_, &Selector::moved, this, &ScreenShoter::moveMenu);
    connect(selector_, &Selector::resized, this, &ScreenShoter::moveMenu);
    connect(selector_, &Selector::sigResizeCpaturedRect, this, [](){ if (menu_ && menu_->isVisible()) menu_->raise(); });
    connect(selector_, &Selector::statusChanged, [this](auto status) {

        if ((status > SelectorStatus::READY && status < SelectorStatus::CAPTURED) ||
            status == SelectorStatus::RESIZING) {
#ifdef Q_OS_LINUX
            magnifier_->hide(); // FIXME: Ubuntu
#endif

            magnifier_->adjustedPixmapSize();
            magnifier_->show();
        }
        else
            magnifier_->hide();

        if (guideui_) guideui_->setStatus(status);
    });

    // stop
    connect(selector_, &Selector::stopped, this, &ScreenShoter::exit);

    selector_->installEventFilter(this);

    //
    connect(undo_stack_, &QUndoStack::canRedoChanged, menu_, &EditingMenu::canRedo);
    connect(undo_stack_, &QUndoStack::canUndoChanged, menu_, &EditingMenu::canUndo);

    //
    connect(timer_point_tips_, &QTimer::timeout, this, [this](){
        if (point_tips_) point_tips_->hide();
        timer_point_tips_->stop();
    });

    //
    connect(&COMM, &Communication::sigImgTransFinshed, this, &ScreenShoter::onImgTransFinshed);

    // shortcuts
    registerShortcuts();
}

ScreenShoter::~ScreenShoter() 
{
    /* menu_ 为静态对象，且父对象是 nullptr,故不被影响 → ScreenShoter 对象被  setAttribute(Qt::WA_DeleteOnClose, true) 释放其内存时候
    *  1. menu_  不会被释放内存，且静态变量保留在下次截图时的，保留上次的颜色等设置
    *  2. 必须断开已销毁对象的所有槽函数，否则 CRASH
    */
    if (menu_) menu_->disconnect(); // 断开所有连接
}

void ScreenShoter::start()
{
    if (isVisible()) return;

    const auto virtual_geometry = probe::graphics::virtual_screen_geometry();

    // geometry
    setGeometry(QRect{ virtual_geometry });  // window geometry == virtual geometry, absolute coordinate
    scene()->setSceneRect(rect());           // relative coordinate, start at (0, 0)
    selector_->setGeometry(rect());          // relative coordinate, start at (0, 0)
    selector_->coordinate(virtual_geometry); // painter coordinate, absolute coordinate

    if (screenshot(virtual_geometry)) {
        //
        selector_->start(probe::graphics::window_filter_t::visible |
                         probe::graphics::window_filter_t::children);


        selector_->show();
        show();

        // Qt::BypassWindowManagerHint: no keyboard input unless call QWidget::activateWindow()
        activateWindow();
    }

    history_idx_ = history_.size();

    if (COMM.allowSendUserData())
        COMM.sendUserData("Capture");
}

void ScreenShoter::repeat()
{
    start();

    if (!history_.empty()) {
        selector_->select(history_.back());
        selector_->status(SelectorStatus::CAPTURED);
        moveMenu();
    }
}

void ScreenShoter::imageTranslate()
{
    // if (menu_) menu_->hide();
    // if (magnifier_) magnifier_->hide();
    // update();
    // QApplication::processEvents(); // "Process all pending events, including the effects of `hide()` and `update()`."

    const QString& path = getCacheFolder(ImgCacheDir::ICD_out_img_translate) + "/" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".png";
    // auto [pixmap, topLeft] = snip();
    const auto rect = selector_->selected(); // absolute coordinate
    // selector_->close();
    // scene()->clearFocus();
    // scene()->clearSelection();

    const QPixmap& pixmap = grab(rect.translated(-geometry().topLeft()));
    bool ok = pixmap.save(path, nullptr, config::output::image_quality);
    if (!ok) {
        loge("failed to save the captured image.");
        return;
    }

    // Send a request for image translation
    NetTencent::TYPE type = NetTencent::TYPE::T_baidu_cloud_ocr;
    QString engine = config::image_translate::use_engine;
    if (engine == "tencent_cloud") type = NetTencent::TYPE::T_tencent_cloud_pix_trans;
    else if (engine == "baidu_cloud") type = NetTencent::TYPE::T_baidu_cloud_pix_trans;
    else if (engine == "youdao_cloud") type = NetTencent::TYPE::T_youdao_cloud_pix_trans;
    if (engine.isEmpty()) return;

    netmgn_.requestAPI(path, type, NetTencent::RECOGNITION::R_img_trans);
}

void ScreenShoter::extractText()
{
    if (menu_) menu_->hide();
    if (magnifier_) magnifier_->hide();
    update();
    QApplication::processEvents();

    auto [pixmap, _] = snip();
    COMM.onExtractText();
    QTimer::singleShot(10, [pixmap](){ COMM.extractTextLoadImage(pixmap); });  // Fix: 避免直接加载窗口卡住变白色
    exit();
}

void ScreenShoter::onImgTransFinshed(const QString &path)
{
    if (path.isEmpty()) return;
    emit menu_->imageArrived(QPixmap(path));
}

#ifdef Q_OS_LINUX
void ScreenShoter::DbusScreenshotArrived(uint response, const QVariantMap& results)
{
    if (!response && results.contains(QLatin1String("uri"))) {
        auto       uri  = results.value(QLatin1String("uri")).toString();
        const auto path = uri.remove(QLatin1String("file://"));

        QPixmap background(path);

        setBackground(background, probe::geometry_t{ 0, 0, static_cast<uint32_t>(background.width()),
                                                     static_cast<uint32_t>(background.height()) });
    }
}
#endif

bool ScreenShoter::screenshot(const probe::geometry_t& geometry)
{
#ifdef Q_OS_LINUX
    if (probe::system::windowing_system() == probe::system::windowing_system_t::Wayland) {
        QDBusInterface interface("org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop",
                                 "org.freedesktop.portal.Screenshot");

        QDBusReply<QDBusObjectPath> reply = interface.call(
            "Screenshot", "", QVariantMap{ { "interactive", false }, { "handle_token", "123456" } });
        if (!reply.isValid()) {
            loge("[WAYLOAD] org.freedesktop.portal.Screenshot");
            return false;
        }
        QDBusConnection::sessionBus().connect(QString(), reply.value().path(),
                                              "org.freedesktop.portal.Request", "Response", this,
                                              SLOT(DbusScreenshotArrived(uint, QVariantMap)));
    }
    else {
#endif
        // TODO: 180ms (4K + 2K two monitors), speed up
        const auto background = QGuiApplication::primaryScreen()->grabWindow(
            probe::graphics::virtual_screen().handle, geometry.x, geometry.y,
            static_cast<int>(geometry.width), static_cast<int>(geometry.height));

        setBackground(background, geometry);

#ifdef Q_OS_LINUX
    }
#endif

    return true;
}

void ScreenShoter::setBackground(const QPixmap& background, const probe::geometry_t& geometry)
{
    setBackgroundBrush(background);
    setCacheMode(QGraphicsView::CacheBackground);

    magnifier_->setGrabPixmap(background, geometry);
}

QBrush ScreenShoter::mosaicBrush()
{
    auto     pixmap = QPixmap::fromImage(backgroundBrush().textureImage());
    QPainter painter(&pixmap);

    scene()->render(&painter, pixmap.rect());

    return pixmap.scaled(pixmap.size() / 9, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
        .scaled(pixmap.size());
}

void ScreenShoter::updateCursor(const ResizerLocation location)
{
    if (menu_->graph() & canvas::eraser) {
        setCursor(
            QCursor{ cursor::circle(menu_->pen().width(), { QColor{ 136, 136, 136 }, 3 }, Qt::NoBrush) });
    }
    else if (menu_->graph() & canvas::mosaic) {
        setCursor(QCursor{ cursor::circle(menu_->pen().width(), { QColor{ 136, 136, 136 }, 3 }) });
    }
    else {
        setCursor(getCursorByLocation(location, Qt::CrossCursor));
    }
}

bool ScreenShoter::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == selector_ && event->type() == QEvent::MouseButtonDblClick) {
        mouseDoubleClickEvent(dynamic_cast<QMouseEvent *>(event));
        return true;
    }
    else if (obj == selector_ /*&& event->type() == QEvent::MouseMove*/) {

        QScreen* scrn = Helper::currScrn();
        const QRect& geometry = scrn->geometry();
        guideui_->move(mapFromGlobal(geometry.bottomLeft()) + QPoint(20, -guideui_->rect().height() - 20));

        QRect rt(guideui_->mapToParent(QPoint(0, 0)), guideui_->rect().size());
        guideui_->setVisible(!( (rt.contains(mapFromGlobal(QCursor::pos())) && selector_->status() == SelectorStatus::PREY_SELECTING)
                               || (rt.intersects(selector_->selected(true)) && selector_->status() != SelectorStatus::PREY_SELECTING)) );
    }

    return QGraphicsView::eventFilter(obj, event);
}

void ScreenShoter::mousePressEvent(QMouseEvent *event)
{
    const auto ffs_item = scene_->focusOrFirstSelectedItem();

    // focus or blur
    QGraphicsView::mousePressEvent(event);

    //
    if (event->isAccepted() || event->button() != Qt::LeftButton) return;

    // do not create text items if a text item has focus
    if ((creating_item_ && (creating_item_->graph() & canvas::text)) ||
        (ffs_item && (ffs_item->graph() & canvas::text))) {
        if (menu_->graph() & canvas::text) return;
    }

    createItem(event->pos());

     // fix: debug ok, release failed → menu_ When clicked, it will be covered after release mode.
    if (menu_ && menu_->isVisible()) menu_->raise();
}

void ScreenShoter::mouseMoveEvent(QMouseEvent *event)
{
    if (creating_item_) {
        creating_item_->push(event->pos());
    }

    if (menu_ && menu_->isVisible()) menu_->raise();
    QGraphicsView::mouseMoveEvent(event);
}

void ScreenShoter::mouseReleaseEvent(QMouseEvent *event)
{
    if (creating_item_) {
        // handle the creating text item on the second click
        if (!((creating_item_->graph() & canvas::text) && creating_item_ == scene_->focusItem())) {
            finishItem();
        }
    }

    if (menu_ && menu_->isVisible()) menu_->raise();
    QGraphicsView::mouseReleaseEvent(event);
}

void ScreenShoter::createItem(const QPointF& pos)
{
    if (menu_->graph() == canvas::none) return;
    if (creating_item_) finishItem();

    switch (menu_->graph()) {
    case canvas::rectangle: creating_item_ = new GraphicsRectItem(pos, pos); break;
    case canvas::ellipse:   creating_item_ = new GraphicsEllipseleItem(pos, pos); break;
    case canvas::arrow:     creating_item_ = new GraphicsArrowItem(pos, pos); break;
    case canvas::line:      creating_item_ = new GraphicsLineItem(pos, pos); break;
    case canvas::curve:     creating_item_ = new GraphicsCurveItem(pos, sceneRect().size()); break;
    case canvas::counter:   creating_item_ = new GraphicsCounterItem(pos, ++counter_); break;
    case canvas::text:      {
        creating_item_ = new GraphicsTextItem(pos);
        auto ptr = static_cast<GraphicsTextItem*>(creating_item_);
        ptr->setTextstyle(menu_->getTextStyle());
        qDebug() << menu_->getTextStyle();
        break;
    }
    case canvas::eraser:
        creating_item_ = new GraphicsEraserItem(pos, sceneRect().size(), backgroundBrush());
        break;
    case canvas::mosaic:
        creating_item_ = new GraphicsMosaicItem(pos, sceneRect().size(), mosaicBrush());
        break;
    default: return;
    }

    creating_item_->setPen(menu_->pen());
    creating_item_->setFont(menu_->font());
    creating_item_->fill(menu_->filled());
    creating_item_->setTypeId(menu_->getTypeId());
    // qDebug() << "creating_item_:" << menu_->graph() << menu_->getTypeId();

    creating_item_->onhovered([this](auto location) { updateCursor(location); });
    creating_item_->onmoved([item = creating_item_, this](auto opos) {
        undo_stack_->push(new MoveCommand(dynamic_cast<QGraphicsItem *>(item), opos));
    });
    creating_item_->onresized([item = creating_item_, this](auto g, auto l) {
        undo_stack_->push(new ResizeCommand(item, g, l));
        if (item->graph() & canvas::text) menu_->setFont(item->font());
    });

    creating_item_->onrotated(
        [item = creating_item_, this](auto angle) { undo_stack_->push(new RotateCommand(item, angle)); });

    scene_->add(creating_item_);
    dynamic_cast<QGraphicsItem *>(creating_item_)->setFocus();
}

void ScreenShoter::finishItem()
{
    if (!creating_item_) return;

    if (creating_item_->invalid()) {
        scene_->remove(creating_item_);
        delete creating_item_;
        creating_item_ = nullptr;
    }
    else {
        creating_item_->end();
        undo_stack_->push(new CreatedCommand(scene(), dynamic_cast<QGraphicsItem *>(creating_item_)));
        creating_item_ = nullptr;
    }
}

void ScreenShoter::wheelEvent(QWheelEvent *event)
{
    if (menu_->graph() != canvas::none) {
        const auto delta = event->angleDelta().y() / 120;
        auto       pen   = menu_->pen();

        if ((event->modifiers() & Qt::CTRL) && canvas::has_color(menu_->graph())) {
            auto color = pen.color();
            color.setAlpha(std::clamp<int>(color.alpha() + delta * 5, 0, 255));
            pen.setColor(color);

            menu_->setPen(pen, false);
            updateCursor(ResizerLocation::DEFAULT);
        }
        else if (canvas::has_width(menu_->graph())) {
            pen.setWidth(std::clamp(menu_->pen().width() + delta, 1, 71));

            menu_->setPen(pen, false);
            updateCursor(ResizerLocation::DEFAULT);
            showPointTips(QString::number(pen.width()));
        }
        else if (menu_->graph() == canvas::text) {
            if (!creating_item_) return;
            const int& stepY = delta > 0 ? 1 : -1;
            auto font = menu_->font();
            font.setPointSize(font.pointSize() + stepY);
            menu_->setFont(font);
            // creating_item_->setFont(menu_->font());
        }
    }

    QGraphicsView::wheelEvent(event);
}

void ScreenShoter::keyPressEvent(QKeyEvent *event)
{
    // hotkey 'Space': move the selector while editing.
    if (selector_->status() == SelectorStatus::LOCKED && event->key() == Qt::Key_Space &&
        !event->isAutoRepeat()) {
        selector_->status(SelectorStatus::CAPTURED);
    }
    else if (event->key() == Qt::Key_Alt && !event->isAutoRepeat()) {
        selector_->showCrossHair(true);
    }

    if (menu_ && menu_->isVisible()) menu_->raise();
    QGraphicsView::keyPressEvent(event);
}

void ScreenShoter::keyReleaseEvent(QKeyEvent *event)
{
    // stop moving the selector while editing
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        selector_->status(SelectorStatus::LOCKED);
    }
    else if (event->key() == Qt::Key_Alt && !event->isAutoRepeat()) {
        selector_->showCrossHair(false);
    }
    else if (event->key() == Qt::Key_Tab) {
        if (selector_->status() == SelectorStatus::PREY_SELECTING) {
            const auto& prey = selector_->prey();
            if (prey.type == hunter::prey_type_t::desktop) {
                const QPointF& pos = QCursor::pos();
                QMouseEvent *mouseEvent = new QMouseEvent(QEvent::MouseMove, pos, pos, Qt::NoButton, Qt::NoButton, Qt::NoModifier);
                QApplication::postEvent(selector_, mouseEvent);
            }
            else {
                selector_->select(hunter::contains(prey));
            }
        }
    }
    else if (event->key() == Qt::Key_Shift) {
        if (magnifier_->isVisible()) {

            magnifier_->toggleFormat();
            magnifier_->setLabColorNameMinWidth(magnifier_->format() == Magnifier::ColorFormat::HEX ? 60 : 90);
        }
    }

    if (menu_ && menu_->isVisible()) menu_->raise();
    QGraphicsView::keyReleaseEvent(event);
}

void ScreenShoter::mouseDoubleClickEvent(QMouseEvent *event)
{
    QGraphicsView::mouseDoubleClickEvent(event);

    if (!event->isAccepted() && event->button() == Qt::LeftButton &&
        selector_->status() >= SelectorStatus::CAPTURED) {
        copy();
    }
}

void ScreenShoter::moveMenu()
{
    const auto area  = selector_->selected();
    auto       right = area.right() - menu_->width() - 2;
    if (right < geometry().left()) right = geometry().left();

    if (area.bottom() + (menu_->height() * 2 + 6 /*margin*/) < geometry().bottom()) {
        menu_->move(right, area.bottom() + 6);
        menu_->setSubMenuShowAbove(false);
    }
    else if (area.top() - (menu_->height() * 2 + 6) > 0) {
        menu_->move(right, area.top() - menu_->height() - 6);
        menu_->setSubMenuShowAbove(true);
    }
    else {
        menu_->move(right, area.bottom() - menu_->height() - 6);
        menu_->setSubMenuShowAbove(true);
    }
}

std::pair<QPixmap, QPoint> ScreenShoter::snip()
{
    if (history_.empty() || history_.back() != selector_->prey()) {
        history_.push_back(selector_->prey());
        history_idx_ = history_.size();
    }

    const auto rect = selector_->selected(); // absolute coordinate
    selector_->close();
    scene()->clearFocus();
    scene()->clearSelection();

    return { grab(rect.translated(-geometry().topLeft())), rect.topLeft() };
}

void ScreenShoter::onSave(bool bManualSave)
{
    QString default_filename =
        XPROJECT_NAME + QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss") + "." + (config::output::ms_remeber_last_image_extension ? config::output::last_image_extension : "png");

    QString filename;
    if (bManualSave) {

        #ifdef Q_OS_LINUX  // fix: snip after save fileDialog not OnTop show
        setWindowFlag(Qt::WindowStaysOnTopHint, false);
        magnifier_->hide();
        menu_->hide();
        selector_->hide();
        #endif

        filename = QFileDialog::getSaveFileName(this, tr("Save Image"), config::output::last_dir + QDir::separator() + default_filename, "PNG(*.png);;JPEG(*.jpg *.jpeg);;BMP(*.bmp)");
        if (!filename.isEmpty())
            config::output::last_dir = QFileInfo(filename).absolutePath();

        #ifdef Q_OS_LINUX
        setWindowFlag(Qt::WindowStaysOnTopHint, true);
        activateWindow();
        #endif

    }
    else {
        filename = Helper::formatToFileName(config::output::qs_path);
    }

    auto [pixmap, _] = snip();
    bool ok = pixmap.save(filename, nullptr, config::output::image_quality);
    if (!ok) {
        loge("failed to save the captured image.");
    }

    if (!bManualSave && config::output::qs_show_notification) {
        COMM.sigShowSystemMessagebox(ok ? tr("Success") : tr("Failed"), tr("Image save to %1").arg(filename));
    }

    config::output::last_dir = QFileInfo{ filename }.absoluteDir().path();
    if (config::output::ms_remeber_last_image_extension) config::output::last_image_extension = QFileInfo(filename).suffix();
    if (config::output::as_enable)
        pixmap.save(Helper::formatToFileName(config::output::as_path), nullptr, config::output::image_quality);

    emit saved(filename);
    exit();
}

void ScreenShoter::copy()
{
    clipboard::push(snip().first);

    exit();
}

void ScreenShoter::pin()
{
    auto [pixmap, pos] = snip();

    emit pinData(clipboard::push(pixmap, pos));

    exit();
}

void ScreenShoter::exit()
{
    magnifier_->close();

    menu_->close();

    selector_->close();

    //! 1.
    undo_stack_->clear();

    //! 2.
    if (creating_item_ && !dynamic_cast<QGraphicsItem *>(creating_item_)->scene()) delete creating_item_;
    creating_item_ = {};

    //! 3.
    scene_->clear();

    counter_ = 0;

    setBackgroundBrush(Qt::NoBrush);

    QWidget::close();
}

void ScreenShoter::setStyle(const SelectorStyle& style)
{
    selector_->setBorderStyle(QPen{
        style.border_color,
        static_cast<qreal>(style.border_width),
        style.border_style,
    });

    selector_->setMaskStyle(style.mask_color);
}

size_t ScreenShoter::history_index()
{
    return std::clamp<size_t>(history_idx_, 0, std::max<size_t>(0, history_.size() - 1));
}

void ScreenShoter::showPointTips(const QString &text)
{
    QScreen* scrn = Helper::currScrn();
    const QRect& geometry = scrn->geometry();
    guideui_->move(mapFromGlobal(geometry.bottomLeft()) + QPoint(20, -guideui_->rect().height() - 20));

    point_tips_->setText(text);
    point_tips_->move(mapFromGlobal(geometry.topLeft()));
    point_tips_->raise();
    point_tips_->show();

    timer_point_tips_->setSingleShot(true);
    timer_point_tips_->stop();
    timer_point_tips_->start();
    point_tips_->update();
}

void ScreenShoter::registerShortcuts()
{
    connect(new QShortcut(Qt::CTRL | Qt::Key_S, this), &QShortcut::activated, [this]() {
        if (any(selector_->status() & SelectorStatus::CAPTURED) ||
            any(selector_->status() & SelectorStatus::LOCKED)) {
            onSave(true);
        }
    });

    connect(new QShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_S, this), &QShortcut::activated, [this]() {
        if (any(selector_->status() & SelectorStatus::CAPTURED) ||
            any(selector_->status() & SelectorStatus::LOCKED)) {
            onSave(false);
        }
    });

    connect(new QShortcut(Qt::Key_P, this), &QShortcut::activated, [this] {
        if (selector_->status() == SelectorStatus::CAPTURED ||
            selector_->status() == SelectorStatus::LOCKED) {
            pin();
        }
    });

    connect(new QShortcut(Qt::Key_F5, this), &QShortcut::activated, [this] {
        const auto menuv = menu_->isVisible();
        const auto magnv = magnifier_->isVisible();
        if (menuv) menu_->hide();
        if (magnv) magnifier_->hide();

        hide();

        screenshot(probe::graphics::virtual_screen_geometry());

        show();

        if (menuv) menu_->show();
        if (magnv) magnifier_->show();

        activateWindow();
    });

    // clang-format off
    connect(new QShortcut(Qt::Key_Escape,        this), &QShortcut::activated, [this] { exit(); });
    for (const QKeySequence &keySequence : {QKeySequence(Qt::Key_Return), QKeySequence(Qt::Key_Enter), QKeySequence(Qt::CTRL | Qt::Key_C)})
        connect(new QShortcut(keySequence, this), &QShortcut::activated, this, [this] { copy(); exit(); });

    connect(new QShortcut(Qt::CTRL | Qt::Key_Z,             this), &QShortcut::activated, undo_stack_, &QUndoStack::undo);
    for (const QKeySequence &keySequence : {QKeySequence(Qt::CTRL | Qt::Key_Y), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Z)})
        connect(new QShortcut(keySequence, this), &QShortcut::activated, undo_stack_, &QUndoStack::redo);
    // clang-format on

    connect(new QShortcut(QKeySequence::Delete, this), &QShortcut::activated, [this] {
        if (!scene()->selectedItems().isEmpty() || scene()->focusItem()) {
            undo_stack_->push(new DeleteCommand(scene()));
        }
    });

    connect(new QShortcut(Qt::Key_PageUp, this), &QShortcut::activated, [=, this] {
        if (history_.empty()) return;

        history_idx_ = history_idx_ > 0 ? history_idx_ - 1 : 0;
        selector_->select(history_[history_index()]);
        selector_->status(SelectorStatus::CAPTURED);
        moveMenu();
    });

    connect(new QShortcut(Qt::Key_PageDown, this), &QShortcut::activated, [=, this] {
        if (history_.empty()) return;

        history_idx_ = history_idx_ < history_.size() - 1 ? history_idx_ + 1 : history_.size() - 1;
        selector_->select(history_[history_index()]);
        selector_->status(SelectorStatus::CAPTURED);
        moveMenu();
    });

    connect(new QShortcut(Qt::Key_C, this), &QShortcut::activated, [=, this] {
        if (selector_->status() < SelectorStatus::CAPTURED && magnifier_->isVisible()) {
            clipboard::push(magnifier_->color(), magnifier_->colorname());
            exit();
        }
    });


    //
    #define CREATOR_QSHORTCUT(_keySequence, _tb) \
    new QShortcut(QKeySequence(_keySequence), this, []() { \
            static bool checked = false; \
            auto tb = _tb; \
            if (tb->isCheckable()) { \
                checked = !tb->isChecked(); \
                tb->setChecked(checked); \
        } \
            tb->clicked(); \
    });

    CREATOR_QSHORTCUT(Qt::CTRL | Qt::Key_1, menu_->group()->button(canvas::rectangle));
    CREATOR_QSHORTCUT(Qt::CTRL | Qt::Key_2, menu_->group()->button(canvas::ellipse));
    CREATOR_QSHORTCUT(Qt::CTRL | Qt::Key_3, menu_->group()->button(canvas::arrow));
    CREATOR_QSHORTCUT(Qt::CTRL | Qt::Key_4, menu_->group()->button(canvas::curve));
    CREATOR_QSHORTCUT(Qt::CTRL | Qt::Key_5, menu_->group()->button(canvas::mosaic));
    CREATOR_QSHORTCUT(Qt::CTRL | Qt::Key_6, menu_->group()->button(canvas::text));
    CREATOR_QSHORTCUT(Qt::CTRL | Qt::Key_7, menu_->group()->button(canvas::pixmap));
    CREATOR_QSHORTCUT(Qt::CTRL | Qt::Key_8, menu_->group()->button(canvas::counter));

    CREATOR_QSHORTCUT(Qt::Key_B, menu_->group()->button(canvas::curve));
    CREATOR_QSHORTCUT(Qt::Key_T, menu_->group()->button(canvas::text));
    CREATOR_QSHORTCUT(Qt::Key_I, menu_->group()->button(canvas::pixmap));
    CREATOR_QSHORTCUT(Qt::Key_E, menu_->group()->button(canvas::eraser));
}
