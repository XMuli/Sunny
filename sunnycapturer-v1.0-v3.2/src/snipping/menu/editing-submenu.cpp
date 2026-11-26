// SPDX-SnippetCopyrightText: 2024-2025 SunnyCapturer Project
// SPDX-Author: ffiirree <ice_qi@163.com>
// SPDX-Author: Wengeng Zhang <xmulitech@gmail.com>
#include "editing-submenu.h"
#include "ui_submenubar.h"
#include "verlinewithspace.h"
#include <QButtonGroup>
#include <QCheckBox>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLineEdit>

#ifdef Q_OS_WIN
#include "platforms/window-effect.h"
#endif

EditingSubmenu::EditingSubmenu(SubMenuTypes buttons, QWidget *parent)
    : FramelessWindow(parent, Qt::ToolTip | Qt::WindowStaysOnTopHint)
      , ui_(new Ui::SubMenuBar)
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
    QWidget* temp = new QWidget(nullptr);
    ui_->setupUi(temp);
    temp->hide();

    for (auto it : temp->findChildren<GroupDlg*>()) {
        bool statue = true;
        if (it == ui_->text_dlg) statue = false;
        it->setExclusive(statue);
    }

    //
    const auto group = new QButtonGroup(this);

    if (buttons & WIDTH_BTN) {
        // pen: width
        ui_->width_btn->setCheckable(true);
        ui_->width_btn->setChecked(true);
        connect(ui_->width_btn, &WidthButton::changed, [this](int w) {
            pen_.setWidth(w);
            emit penChanged(pen_);
        });
        layout->addWidget(ui_->width_btn);
        group->addButton(ui_->width_btn);
    }

    // brush
    if (buttons & FILL_BTN) {
        connect(ui_->fill_btn, &QCheckBox::toggled, [this](bool checked) { emit fillChanged(checked); });

        layout->addWidget(ui_->fill_btn);
        group->addButton(ui_->fill_btn);
    }

    // arrow
    if (buttons & ARROW) {
        if (buttons & (FILL_BTN | WIDTH_BTN)) ui_->hLayout_arrow_dlg->insertWidget(0, new VerLineWithSpace());
        connect(ui_->arrow_dlg, &GroupDlg::idToggled, [this](int id) { typeId_= id; });
        layout->addWidget(ui_->arrow_dlg);
    }

    // blur
    if (buttons & BLUR) {
        ui_->hLayout_blur_dlg->insertWidget(0, new VerLineWithSpace());
        layout->addWidget(ui_->blur_dlg);
    }

    // counter
    if (buttons & COUNTER) {
        layout->addWidget(ui_->counter_dlg);
    }


    // font
    if (buttons & FONT_BTNS) {
        layout->addWidget(ui_->text_dlg);
        connect(ui_->text_dlg, &GroupDlg::idToggled, [this](int id, bool checked) {
            static TextStyles styles;
            if (id == 0) checked ? styles |= TextStyleBold      : styles &= ~TextStyleBold;
            if (id == 1) checked ? styles |= TextStyleItalic    : styles &= ~TextStyleItalic;
            if (id == 2) checked ? styles |= TextStyleOutline   : styles &= ~TextStyleOutline;
            if (id == 3) checked ? styles |= TextStyleUnderline : styles &= ~TextStyleUnderline;
            if (id == 4) checked ? styles |= TextStyleStrikeout : styles &= ~TextStyleStrikeout;
            textstyle_ = styles;
        });

        layout->setSpacing(0);
        layout->setContentsMargins({});
        ui_->hLayout_text_dlg->insertWidget(5, new VerLineWithSpace());

        // font family
        ui_->font_family->addItems(QFontDatabase::families());
        connect(ui_->font_family, &QComboBox::currentTextChanged, [this](const QString& family) {
            ui_->font_style->clear();
            ui_->font_style->addItems(QFontDatabase::styles(family));
            emit fontChanged(font());
        });

        // font style
        ui_->font_style->hide();
        connect(ui_->font_style, &QComboBox::currentTextChanged, [this] { emit fontChanged(font()); });

        // font size
        ui_->font_size->setEditable(true);
        foreach(const int& s, QFontDatabase::standardSizes()) {
            ui_->font_size->addItem(QString::number(s));
        }
        connect(ui_->font_size, &QComboBox::currentTextChanged, [this] { emit fontChanged(font()); });

        // default font
        setFont(QFontDatabase::font("微软雅黑", "Regular", 16)); // or application's default font
    }

    // pen: color
    if (buttons & COLOR_PENAL) {
        ui_->hLayout_color_dlg->insertWidget(0, new VerLineWithSpace());
        connect(ui_->color_panel, &ColorPanel::changed, [this](const QColor& c) {
            pen_.setColor(c);
            emit penChanged(pen_);
        });
        layout->addWidget(ui_->color_Dlg);
    }
}

void EditingSubmenu::autoChangedButtons(const canvas::graphics_t& graph)
{
    for (auto it : findChildren<GroupDlg*>()) it->hide();

    ui_->width_btn->hide();
    ui_->fill_btn->hide();
    ui_->color_Dlg->show();

    if (graph != canvas::graphics_t::rectangle && graph != canvas::graphics_t::ellipse)
        fill(false);

    if (graph == canvas::graphics_t::rectangle || graph == canvas::graphics_t::ellipse) {
        ui_->width_btn->show();
        ui_->fill_btn->show();
    } else if (graph == canvas::graphics_t::arrow) {
        ui_->width_btn->show();
        ui_->arrow_dlg->show();
    } else if (graph == canvas::graphics_t::curve) {
        ui_->width_btn->show();
    } else if (graph == canvas::graphics_t::mosaic) {

        ui_->width_btn->show();
        // ui_->blur_dlg->show();
        ui_->color_Dlg->hide();
    } else if (graph == canvas::graphics_t::text) {
        ui_->text_dlg->show();
    } else if (graph == canvas::graphics_t::pixmap) {
    } else if (graph == canvas::graphics_t::counter) {
        ui_->width_btn->show();
        // ui_->counter_dlg->show();
    } else if (graph == canvas::graphics_t::eraser) {
        ui_->width_btn->show();
        ui_->color_Dlg->hide();
    } else {

    }

    adjustSize();
    update();
}

void EditingSubmenu::setPen(const QPen& pen, bool silence)
{
    if (ui_->color_panel) {
        ui_->color_panel->setColor(pen.color(), silence);
        pen_.setColor(ui_->color_panel->color());
    }

    if (ui_->width_btn) {
        ui_->width_btn->setValue(pen.width(), silence);
        pen_.setWidth(ui_->width_btn->value());
    }
}

void EditingSubmenu::setFont(const QFont& font)
{
    auto* uisubbar = ui_;
    if (uisubbar->font_family) uisubbar->font_family->setCurrentText(font.family());
    if (uisubbar->font_size) uisubbar->font_size->setCurrentText(QString::number(font.pointSize()));

    if (uisubbar->font_style) uisubbar->font_style->setCurrentText(font.styleName());
}

QFont EditingSubmenu::font() const
{
    auto* uisubbar = ui_;
    if (!uisubbar->font_family || !uisubbar->font_style || !uisubbar->font_size) return QFont{};

    auto font = QFontDatabase::font(uisubbar->font_family->currentText(), uisubbar->font_style->currentText(), 16);
    const auto& size = uisubbar->font_size->currentText().toFloat();
    if (size > 0) font.setPointSizeF(size);
    return font;
}

bool EditingSubmenu::filled() const { return ui_->fill_btn != nullptr && ui_->fill_btn->isChecked(); }

void EditingSubmenu::fill(bool s)
{
    fill_ = s;

    if (ui_->fill_btn) ui_->fill_btn->setChecked(fill_);
    if (ui_->width_btn) ui_->width_btn->setChecked(!fill_);
}

int EditingSubmenu::typeId() const
{
    return typeId_;
}

TextStyles EditingSubmenu::getTextStyle() const
{
    return textstyle_;
}

