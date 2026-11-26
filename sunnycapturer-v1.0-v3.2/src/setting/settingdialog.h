// SPDX-SnippetCopyrightText: 2024-2025 SunnyCapturer Project
// SPDX-Author: ffiirree <ice_qi@163.com>
// SPDX-Author: Wengeng Zhang <xmulitech@gmail.com>
#ifndef CAPTURER_SETTING_DIALOG_H
#define CAPTURER_SETTING_DIALOG_H

#include "framelesswindow.h"

namespace Ui {
    class SubOutput;
    class SubUpdate;
}

class SettingWindow final : public FramelessWindow
{
    Q_OBJECT

public:
    explicit SettingWindow(QWidget * = nullptr);

private:
    QWidget *setupGeneralWidget();
    QWidget *setupExtractTextWidget();
    QWidget *setupImageTranslateWidget();
    QWidget *setupInterfaceWidget();
    QWidget *setupHotkeyWidget();
    QWidget *setupOutputWidget();
    QWidget *setupUpdateWidget();
    QWidget *setupAboutWidget();

    void setServerIPStatus();

private slots:
    void onSetProxy();
    void onExtractTextLineEdit();

private:
    Ui::SubOutput *uioutput_;
    Ui::SubUpdate *uiupdate_;
};

#endif //! CAPTURER_SETTING_DIALOG_H
