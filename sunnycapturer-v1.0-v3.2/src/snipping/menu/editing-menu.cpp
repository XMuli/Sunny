#include "editing-menu.h"

#include "probe/graphics.h"
#include "verlinewithspace.h"

#include <map>
#include <QCheckBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QMoveEvent>
#include <QPixmap>
#include <QStandardPaths>
#include <QShortcut>

#ifdef Q_OS_WIN
#include "platforms/window-effect.h"
#endif

EditingMenu::EditingMenu(QWidget *parent, const uint32_t groups)
    : FramelessWindow(parent, Qt::ToolTip | Qt::WindowStaysOnTopHint)
{
    setCursor(Qt::ArrowCursor);
    setAttribute(Qt::WA_ShowWithoutActivating);
    QWidget::setVisible(false);

#ifdef Q_OS_WIN
    windows::dwm::set_window_corner(reinterpret_cast<HWND>(winId()), DWMWCP_DONOTROUND);
#endif

    const auto layout = new QHBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins({});
    layout->setSizeConstraint(QLayout::SetFixedSize);

    //
    pictures_path_ = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);

    //
    group_ = new ButtonGroup(this);
    connect(group_, &ButtonGroup::emptied, [this]() {
        graph_ = canvas::none;
        graphChanged(canvas::none);
    });

    editingSubmenu_ = new EditingSubmenu(SubMenuType::ALL, this);

    if (groups & GRAPH_GROUP) {
        // rectangle
        group_->addButton(new QCheckBox(this), canvas::rectangle);
        group_->button(canvas::rectangle)->setObjectName("rect-btn");
        group_->button(canvas::rectangle)->setToolTip(tr("Rectangle (Ctrl+1)"));

        submenus_[canvas::rectangle] = editingSubmenu_; /*new EditingSubmenu(
            SubMenuType::WIDTH_BTN | SubMenuType::FILL_BTN | SubMenuType::COLOR_PENAL, this);*/

        // ellipse
        group_->addButton(new QCheckBox(this), canvas::ellipse);
        group_->button(canvas::ellipse)->setObjectName("ellipse-btn");
        group_->button(canvas::ellipse)->setToolTip(tr("Ellipse (Ctrl+2)"));
        submenus_[canvas::ellipse] =  editingSubmenu_; /*new EditingSubmenu(
            SubMenuType::WIDTH_BTN | SubMenuType::FILL_BTN | SubMenuType::COLOR_PENAL, this);*/

        // arrow
        group_->addButton(new QCheckBox(this), canvas::arrow);
        group_->button(canvas::arrow)->setObjectName("arrow-btn");
        group_->button(canvas::arrow)->setToolTip(tr("Arrow (Ctrl+3)"));
        submenus_[canvas::arrow] =  editingSubmenu_; //new EditingSubmenu(SubMenuType::WIDTH_BTN | SubMenuType::ARROW | SubMenuType::COLOR_PENAL, this);
        // submenus_[canvas::arrow]->fill(false);

        // // line
        // group_->addButton(new QCheckBox(this), canvas::line);
        // group_->button(canvas::line)->setObjectName("line-btn");
        // group_->button(canvas::line)->setToolTip(tr("Line"));
        // submenus_[canvas::line] = editingSubmenu_;
            // new EditingSubmenu(SubMenuType::WIDTH_BTN | SubMenuType::COLOR_PENAL, this);

        // pencil
        group_->addButton(new QCheckBox(this), canvas::curve);
        group_->button(canvas::curve)->setObjectName("pencil-btn");
        group_->button(canvas::curve)->setToolTip(tr("Pencil (Ctrl+4) (B)"));
        submenus_[canvas::curve] =  editingSubmenu_;
            // new EditingSubmenu(SubMenuType::WIDTH_BTN | SubMenuType::COLOR_PENAL, this);

        // mosaic
        group_->addButton(new QCheckBox(this), canvas::mosaic);
        group_->button(canvas::mosaic)->setObjectName("mosaic-btn");
        group_->button(canvas::mosaic)->setToolTip(tr("Blur/Mosaic (Ctrl+5)"));
        submenus_[canvas::mosaic] =  editingSubmenu_; //new EditingSubmenu(SubMenuType::WIDTH_BTN | SubMenuType::BLUR, this);
        // submenus_[canvas::mosaic]->setPen(QPen{ Qt::transparent, 15 });

        // text
        group_->addButton(new QCheckBox(this), canvas::text);
        group_->button(canvas::text)->setObjectName("text-btn");
        group_->button(canvas::text)->setToolTip(tr("Text (Ctrl+6) (T)"));
        submenus_[canvas::text] =  editingSubmenu_;
            // new EditingSubmenu(SubMenuType::FONT_BTNS | SubMenuType::COLOR_PENAL, this);

        // pixmap
        group_->addButton(new QCheckBox(this), canvas::pixmap);
        group_->button(canvas::pixmap)->setCheckable(false);
        group_->button(canvas::pixmap)->setObjectName("image-btn");
        group_->button(canvas::pixmap)->setToolTip(tr("Image (Ctrl+7)"));

        connect(group_->button(canvas::pixmap), &QCheckBox::clicked, [=, this]() {
            graph_ = canvas::pixmap;
            emit graphChanged(canvas::pixmap);

#ifdef __linux__
            // The QFileDialog can not be raised to the top of Selector on Linux
            hide();
            parent->hide();
#endif
            const auto filename = QFileDialog::getOpenFileName(
                parent, tr("Open Image"), pictures_path_, "Image Files(*.png *.jpg *.jpeg *.bmp *.svg)");
#ifdef __linux__
            parent->show();
            parent->activateWindow();
            show();
#endif

            if (!filename.isEmpty()) {
                pictures_path_ = QFileInfo(filename).absolutePath();

                emit imageArrived(QPixmap(filename));
            }
        });

        // counter
        group_->addButton(new QCheckBox(this), canvas::counter);
        group_->button(canvas::counter)->setObjectName("counter-btn");
        group_->button(canvas::counter)->setToolTip(tr("Counter (Ctrl+8)"));
        submenus_[canvas::counter] =  editingSubmenu_; //new EditingSubmenu(SubMenuType::COUNTER | SubMenuType::COLOR_PENAL, this);

        // eraser
        group_->addButton(new QCheckBox(this), canvas::eraser);
        group_->button(canvas::eraser)->setObjectName("eraser-btn");
        group_->button(canvas::eraser)->setToolTip(tr("Eraser (E)"));
        submenus_[canvas::eraser] =  editingSubmenu_; //new EditingSubmenu(SubMenuType::WIDTH_BTN, this);
        // submenus_[canvas::eraser]->setPen(QPen{ Qt::transparent, 15 });

        //
        for (const auto button : group_->buttons()) {
            auto gtype = static_cast<canvas::graphics_t>(group_->id(button));

            layout->addWidget(button);

            // style changed event
            if (submenus_.contains(gtype)) {
                // clang-format off
                connect(submenus_[gtype], &EditingSubmenu::penChanged,   [gtype, this](auto pen)    { emit penChanged(gtype, pen); });
                connect(submenus_[gtype], &EditingSubmenu::fontChanged,  [gtype, this](auto font)   { emit fontChanged(gtype, font); });
                connect(submenus_[gtype], &EditingSubmenu::fillChanged,  [gtype, this](auto filled) { emit fillChanged(gtype, filled); });
                // clang-format on

                connect(this, &EditingMenu::moved, [=, this]() {
                    if (submenus_[gtype]->isVisible())
                        submenus_[gtype]->move(pos().x(),
                                               pos().y() + (height() + 3) * (sub_menu_show_pos_ ? -1 : 1));
                });
            }
        }

        // toggled
        connect(group_, &ButtonGroup::buttonToggled, [=, this](auto button, auto checked) {

            if (checked) {
                graph_ = static_cast<canvas::graphics_t>(group_->id(button));
                editingSubmenu_->autoChangedButtons(graph_);
                emit graphChanged(graph_);
            }

            if (const auto button_graph = static_cast<canvas::graphics_t>(group_->id(button));
                submenus_.contains(button_graph)) {
                submenus_[button_graph]->setVisible(checked);
                if (checked) {
                    submenus_[button_graph]->move(pos().x(), pos().y() + (height() + 3) *
                                                                             (sub_menu_show_pos_ ? -1 : 1));
                }
            }
        });
    }

    if (groups & PLUS_GROUP) {
        layout->addWidget(new VerLineWithSpace());

        auto translate = new QCheckBox(this);
        translate->setCheckable(false);
        translate->setObjectName("translate-btn");
        translate->setToolTip(tr("Translate"));
        connect(translate, &QCheckBox::clicked, this, &EditingMenu::sigTranslate);
        layout->addWidget(translate);

        auto extract_text = new QCheckBox(this);
        extract_text->setCheckable(false);
        extract_text->setObjectName("extract_text-btn");
        extract_text->setToolTip(tr("Extract text"));
        connect(extract_text, &QCheckBox::clicked, this, &EditingMenu::sigExtractText);
        layout->addWidget(extract_text);
    }
    /////////////////////////////////////////////////////////////////////////////////
    // if (groups & ADVANCED_GROUP) {
    //     layout->addWidget(new VerLineWithSpace());

    //    auto scroll = new QCheckBox(this);
    //    scroll->setCheckable(false);
    //    scroll->setObjectName("scroll-btn");
    //    connect(scroll, &QCheckBox::clicked, this, &EditingMenu::scroll);
    //    layout->addWidget(scroll);
    //}

    /////////////////////////////////////////////////////////////////////////////////
    if (groups & REDO_UNDO_GROUP) {
        layout->addWidget(new VerLineWithSpace());

        undo_btn_ = new QCheckBox(this);
        undo_btn_->setCheckable(false);
        undo_btn_->setObjectName("undo-btn");
        undo_btn_->setToolTip(tr("Undo (Ctrl+Z)"));
        undo_btn_->setDisabled(true);
        connect(undo_btn_, &QCheckBox::clicked, this, &EditingMenu::undo);
        layout->addWidget(undo_btn_);

        redo_btn_ = new QCheckBox(this);
        redo_btn_->setCheckable(false);
        redo_btn_->setObjectName("redo-btn");
        redo_btn_->setToolTip(tr("Redo (Ctrl+Y)"));
        redo_btn_->setDisabled(true);
        connect(redo_btn_, &QCheckBox::clicked, this, &EditingMenu::redo);
        layout->addWidget(redo_btn_);
    }

    /////////////////////////////////////////////////////////////////////////////////
    if (groups & SAVE_EXIT_GROUP) {
        layout->addWidget(new VerLineWithSpace());

        const auto close_btn = new QCheckBox(this);
        close_btn->setCheckable(false);
        close_btn->setObjectName("close-btn");
        close_btn->setToolTip(tr("Cancel Capture (Esc)"));
        connect(close_btn, &QCheckBox::clicked, [this]() {
            emit exit();
            close();
        });
        layout->addWidget(close_btn);

        const auto pin_btn = new QCheckBox(this);
        pin_btn->setCheckable(false);
        pin_btn->setObjectName("pin-btn");
        pin_btn->setToolTip(tr("Pin to screen (P)"));
        connect(pin_btn, &QCheckBox::clicked, [this]() {
            emit pin();
            hide();
        });
        layout->addWidget(pin_btn);

        const auto save_btn = new QCheckBox(this);
        save_btn->setCheckable(false);
        save_btn->setObjectName("save-btn");
        save_btn->setToolTip(tr("Save to file (Ctrl+S)"));
        connect(save_btn, &QCheckBox::clicked, this, [this](bool ){ emit EditingMenu::save(true); });
        connect(save_btn, &QCheckBox::clicked, group_, &ButtonGroup::clear);
        layout->addWidget(save_btn);

        const auto ok_btn = new QCheckBox(this);
        ok_btn->setCheckable(false);
        ok_btn->setObjectName("ok-btn");
        ok_btn->setToolTip(tr("Copy to clipboard (Ctrl+C)"));
        connect(ok_btn, &QCheckBox::clicked, [this]() {
            emit copy();
            close();
        });
        layout->addWidget(ok_btn);

    }
}

void EditingMenu::canUndo(const bool val) { undo_btn_->setDisabled(!val); }

void EditingMenu::canRedo(const bool val) { redo_btn_->setDisabled(!val); }

void EditingMenu::paintGraph(const canvas::graphics_t graph)
{
    graph_ = graph;
    if (group_->button(graph_))
        group_->button(graph_)->setChecked(true);
    else if (graph == canvas::none && group_->checkedButton())
        group_->checkedButton()->setChecked(false);
}

QPen EditingMenu::pen() const
{
    return (submenus_.contains(graph_)) ? submenus_.at(graph_)->pen() : QPen{ Qt::red };
}

void EditingMenu::setPen(const QPen& pen, const bool silence)
{
    if (submenus_.contains(graph_)) submenus_.at(graph_)->setPen(pen, silence);
}

int EditingMenu::getTypeId() const
{
    if (submenus_.contains(graph_)) return submenus_.at(graph_)->typeId();
    else return 0;
}

TextStyles EditingMenu::getTextStyle() const
{
    if (submenus_.contains(graph_)) return submenus_.at(graph_)->getTextStyle();
    else return TextStyleNone;
}

bool EditingMenu::filled() const { return (submenus_.contains(graph_)) && submenus_.at(graph_)->filled(); }

void EditingMenu::fill(const bool v)
{
    if (submenus_.contains(graph_)) submenus_.at(graph_)->fill(v);
}

QFont EditingMenu::font() const
{
    return (submenus_.contains(graph_)) ? submenus_.at(graph_)->font() : QFont{};
}

void EditingMenu::setFont(const QFont& font)
{
    if (submenus_.contains(graph_)) submenus_.at(graph_)->setFont(font);
}

void EditingMenu::reset()
{
    group_->clear();
    redo_btn_->setDisabled(true);
    undo_btn_->setDisabled(true);
}

void EditingMenu::moveEvent(QMoveEvent *) { emit moved(); }

void EditingMenu::closeEvent(QCloseEvent *) { reset(); }

ButtonGroup *EditingMenu::group() const
{
    return group_;
}
