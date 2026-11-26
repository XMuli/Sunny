// SPDX-SnippetCopyrightText: 2024-2025 SunnyCapturer Project
// SPDX-Author: Wengeng Zhang <xmulitech@gmail.com>
#include "guideui.h"
#include "ui_guideui.h"
#include <QPainter>
#include <QResizeEvent>

GuideUI::GuideUI(QWidget *parent)
    : QWidget(parent), ui(new Ui::GuideUI)
{
    ui->setupUi(this);
    initUI();
}

GuideUI::~GuideUI() { delete ui; }

void GuideUI::hideAllGuideText()
{
    QGridLayout *layout = ui->gridLayout;
    for (int i = 0; i < layout->rowCount(); ++i) {
        for (int j = 0; j < layout->columnCount(); ++j) {
            QLayoutItem *item = layout->itemAtPosition(i, j);
            if (item) {
                QWidget *widget = item->widget();
                if (widget) {
                    widget->hide();
                }
            }
        }
    }
}
void GuideUI::initUI()
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

    // qApp->installEventFilter(this);
    // QSS 样式
    QString tbTextStyle = R"(
    QToolButton {
        background-color: transparent; /* 背景透明 */
        border: 1px solid #bcbcbc;     /* 边框颜色 */
        border-radius: 0px;            /* 圆角 */
        padding: 0px;                  /* 内边距，确保图标有空间 */
        color: white;                  /* 文字颜色 */
                                       /* font-weight: bold; 字体粗细 */
    }
    QToolButton:hover {
        background-color: rgba(68, 68, 68, 0.5); /* 半透明悬停效果 */
        border-color: #666666;                   /* 悬停时的边框颜色 */
    }
    QToolButton:pressed {
        background-color: rgba(34, 34, 34, 0.7); /* 半透明按下效果 */
        border-color: #777777;                   /* 按下时的边框颜色 */
    }
    )";


    QString tbImageStyle = R"(
    QToolButton {
        background-color: transparent; /* 背景透明 */
        border: 0px solid #bcbcbc;     /* 边框颜色 */
        border-radius: 0px;            /* 圆角 */
        padding: 0px;                  /* 内边距，确保图标有空间 */
        color: white;                  /* 文字颜色 */
                                       /* font-weight: bold; 字体粗细 */
    }
    QToolButton:hover {
        background-color: rgba(68, 68, 68, 0.5); /* 半透明悬停效果 */
        border-color: #666666;                   /* 悬停时的边框颜色 */
    }
    QToolButton:pressed {
        background-color: rgba(34, 34, 34, 0.7); /* 半透明按下效果 */
        border-color: #777777;                   /* 按下时的边框颜色 */
    }
    )";


    const int length = 32;
    for (auto* it : findChildren<QToolButton *>()) {
        if (it) {
            it->setMinimumSize(QSize(length, length));

            if (it->objectName().contains("mouse")) {
                it->setStyleSheet(tbImageStyle);
                it->setIconSize(QSize(length, length));
            } else {
                it->setStyleSheet(tbTextStyle);
            }
        }
    }

    for (auto* it : findChildren<QLabel *>()) {
        if (it) {
            it->setStyleSheet("color: white;");
        }
    }

    hideAllGuideText();

    adjustSize();
}

void GuideUI::autoShowGuide()
{
    hideAllGuideText();
    QString text;
    switch (status_) {
    case SelectorStatus::INVALID: {
        text = "SelectorStatus::INVALID";
        hide();
        break;
    }
    case SelectorStatus::READY: {
        text = "SelectorStatus::READY";
        break;
    }
    case SelectorStatus::PREY_SELECTING: {
        text = "SelectorStatus::PREY_SELECTING";
        ui->dlg_wasd->show();               ui->lab_wasd->show();
        ui->dlg_tab->show();                ui->lab_tab->show();
        ui->dlg_ctrl_a->show();             ui->lab_ctrl_a->show();
        // ui->dlg_wave_plaint->show();        ui->lab_wave_plaint->show();
        break;
    }
    case SelectorStatus::FREE_SELECTING: {
        text = "SelectorStatus::FREE_SELECTING";
        break;
    }
    case SelectorStatus::__RESERVERD__: {
        text = "SelectorStatus::__RESERVERD__";
        break;
    }
    case SelectorStatus::CAPTURED: {
        text = "SelectorStatus::CAPTURED";
        ui->dlg_wasd->show();               ui->lab_wasd->show();
        ui->dlg_direction->show();          ui->lab_direction->show();
        // ui->dlg_tab->show();                ui->lab_tab->show();
        ui->dlg_ctrl_a->show();             ui->lab_ctrl_a->show();
        ui->dlg_shift_direction->show();    ui->lab_shift_direction->show();
        ui->dlg_ctrl_direction->show();     ui->lab_ctrl_direction->show();
        // ui->dlg_wave_plaint->show();        ui->lab_wave_plaint->show();
        // ui->dlg_magnifier_c->show();        ui->lab_magnifier_c->show();
        // ui->dlg_magnifier_shift->show();    ui->lab_magnifier_shift->show();
        break;
    }
    case SelectorStatus::MOVING: {
        text = "SelectorStatus::MOVING";
        break;
    }
    case SelectorStatus::RESIZING: {
        text = "SelectorStatus::RESIZING";
        break;
    }
    case SelectorStatus::LOCKED: {
        text = "SelectorStatus::LOCKED";
        ui->dlg_spacing_lock->show();    ui->lab_spacing_lock->show();
        break;
    }
    default:
        break;
    }

    ui->lab_debug->setText(text);

# ifdef QT_DEBUG
    ui->dlg_debug->show();        ui->lab_debug->show();
#endif

    adjustSize();
}

void GuideUI::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e)
    const QColor wihte(255, 255, 255, 0.4 * 255);
    const QColor black(0, 0, 0, 0.7 * 255);        // 灰色透明

    QPainter pa(this);
    pa.setRenderHint(QPainter::Antialiasing);
    pa.setPen(QPen(wihte, 2));
    pa.setBrush(black);
    pa.drawRect(rect());
}

void GuideUI::setStatus(const SelectorStatus &newStatus)
{
    status_ = newStatus;
    autoShowGuide();
}
