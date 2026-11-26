// SPDX-SnippetCopyrightText: 2024-2025 SunnyCapturer Project
// SPDX-Author: ffiirree <ice_qi@163.com>
// SPDX-Author: Wengeng Zhang <xmulitech@gmail.com>
#ifndef CAPTURER_EDITING_SUBMENU_H
#define CAPTURER_EDITING_SUBMENU_H

#include "colorpanel.h"
#include "combobox.h"
#include "framelesswindow.h"
#include "widthbutton.h"
#include "canvas/types.h"

#include <QFont>
#include <QPen>

namespace Ui {
    class SubMenuBar;
}

class QCheckBox;

enum SubMenuType
{
    WIDTH_BTN   = 1 <<  0,
    FILL_BTN    = 1 <<  1,

    ARROW       = 1 <<  3,
    BLUR        = 1 <<  4,
    COUNTER     = 1 <<  5,
    FONT_BTNS   = 1 <<  6,
    COLOR_PENAL = 1 <<  7,

    ALL         = 0xFF


    // ARROW_SIMPLE   = 1 <<  2,
    // ARROW_LINE     = 1 <<  3,
    // ARROW_PLOYLINE = 1 <<  4,

    // BLUR_SMOOTH    = 1 <<  5,
    // BLUR_MOSAIC    = 1 <<  6,

    // TEXT_BOLD      = 1 <<  7,
    // TEXT_ITALIC    = 1 <<  8,
    // TEXT_OUTLINE   = 1 <<  9,
    // TEXT_STRIKEOUT = 1 << 10,
    // TEXT_UNDERLINE = 1 << 11,
    // FONT_BTNS      = 1 << 12,

    // COUNTER_LETTER = 1 << 13,
    // COUNTER_NUMBER = 1 << 14,

    // COLOR_PENAL    = 1 << 15,
};

Q_DECLARE_METATYPE(SubMenuType)                     // 可以被 QVariant 类型存储
Q_DECLARE_FLAGS(SubMenuTypes, SubMenuType)          // 枚举 PaintType 生成宏 SubMenuTypes
Q_DECLARE_OPERATORS_FOR_FLAGS(SubMenuTypes)         // 重载宏 SubMenuTypes 的 |() 函数

class EditingSubmenu final : public FramelessWindow
{
    Q_OBJECT
public:


public:
    explicit EditingSubmenu(SubMenuTypes buttons, QWidget *parent = nullptr);
    void autoChangedButtons(const canvas::graphics_t& graph);

    //
    void setPen(const QPen& pen, bool silence = true);

    [[nodiscard]] QPen pen() const { return pen_; }

    //
    void setFont(const QFont& font);

    [[nodiscard]] QFont font() const;
    //
    [[nodiscard]] bool  filled() const;

    void fill(bool);

    int typeId() const;
    TextStyles getTextStyle() const;

signals:
    void penChanged(const QPen&);
    void fontChanged(const QFont&);
    void fillChanged(bool);

    void moved();

private:
    Ui::SubMenuBar  *ui_;
    int             typeId_{ 0 };
    TextStyles      textstyle_{ TextStyleNone };

    QPen pen_{ Qt::red, 2 };
    bool fill_{};
};

#endif //! CAPTURER_EDITING_SUBMENU_H
