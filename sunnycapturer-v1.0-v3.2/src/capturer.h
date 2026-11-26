// SPDX-SnippetCopyrightText: 2024-2025 SunnyCapturer Project
// SPDX-Author: ffiirree <ice_qi@163.com>
// SPDX-Author: Wengeng Zhang <xmulitech@gmail.com>

#ifndef CAPTURER_H
#define CAPTURER_H

#include "framelesswindow.h"
#include "menu.h"
#include "screenshoter.h"
#include "settingdialog.h"
#include "singleapplication.h"

#include <memory>
#include <QApplication>
#include <qhotkey.h>
#include <QMimeData>
#include <QPointer>
#include <QScopedPointer>
#include <QSystemTrayIcon>

class Capturer final : public SingleApplication
{
    Q_OBJECT

public:
    Capturer(int& argc, char **argv);

    void Init();

public slots:
    void onCpature();

    void QuickLook();

    void PreviewClipboard();
    void PreviewMimeData(const std::shared_ptr<QMimeData>& mimedata);
    void TogglePreviews();
    void TransparentPreviewInput();

    void OpenSettingsDialog();
    void onRestart();

    void UpdateHotkeys();

    void TrayActivated(QSystemTrayIcon::ActivationReason reason);
    void ShowMessage(const QString& title, const QString& msg,
                     QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::Information, int msecs = 10000);

    void SetTheme(const QString& theme);

    void UpdateScreenshotStyle();

private:
    void SystemTrayInit();

    QString theme_{};

    QScopedPointer<QSystemTrayIcon> tray_{};
    QScopedPointer<Menu>            tray_menu_{};
    QPointer<QAction>               tray_snip_{};
    QPointer<QAction>               tray_ocr{};
    QPointer<QAction>               tray_settings_{};
    QPointer<QAction>               tray_restart_{};
    QPointer<QAction>               tray_exit_{};

    QPointer<SettingWindow> settings_window_{};

    // hotkey
    QPointer<QHotkey> snip_hotkey_{};       // screenshot
    QPointer<QHotkey> repeat_snip_hotkey_{}; // screenshot
    QPointer<QHotkey> preview_hotkey_{};    // preview
    QPointer<QHotkey> quicklook_hotkey_{};  // Explorer window, Windows only
    QPointer<QHotkey> transparent_input_{}; // for preview window
    QPointer<QHotkey> toggle_hotkey_{};     // toggle previews

    QPointer<ScreenShoter> sniper_{ nullptr };

    std::list<QPointer<FramelessWindow>> previews_{};
};

inline Capturer *App() { return dynamic_cast<Capturer *>(qApp); }

#endif // CAPTURER_H
