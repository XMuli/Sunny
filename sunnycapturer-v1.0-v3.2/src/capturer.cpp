// SPDX-SnippetCopyrightText: 2024-2025 SunnyCapturer Project
// SPDX-Author: ffiirree <ice_qi@163.com>
// SPDX-Author: Wengeng Zhang <xmulitech@gmail.com>

#include "capturer.h"

#include "clipboard.h"
#include "color-window.h"
#include "config.h"
#include "image-window.h"
#include "logging.h"
#include "settingdialog.h"
#include "communication.h"

#include <probe/system.h>
#include <probe/util.h>
#include <QApplication>
#include <QFileInfo>
#include <QKeyEvent>
#include <QProcess>
#include <QScreen>
#include <QStyleFactory>
#include <QSystemTrayIcon>
#include <QUrl>
#include <QTimer>

#define SET_HOTKEY(X, Y)                                                                                   \
    if (!X->setShortcut(Y, true)) {                                                                        \
        logw("Failed to register hotkey : {}", Y.toString().toStdString());                                \
        error += QObject::tr("Failed to register hotkey : <%1>\n").arg(Y.toString());                               \
    }

Capturer::Capturer(int& argc, char **argv)
    : SingleApplication(argc, argv)
{
    setWindowIcon(QIcon(":/logo/logo.svg"));

    setStyle(QStyleFactory::create("Fusion"));

    snip_hotkey_        = new QHotkey(this);
    repeat_snip_hotkey_ = new QHotkey(this);
    preview_hotkey_     = new QHotkey(this);
    toggle_hotkey_      = new QHotkey(this);
    quicklook_hotkey_   = new QHotkey(this);
    transparent_input_  = new QHotkey(this);

    connect(snip_hotkey_, &QHotkey::activated, this, &Capturer::onCpature);
    connect(repeat_snip_hotkey_, &QHotkey::activated, this, [this]() {
        if (sniper_) sniper_->disconnect();

        if (!sniper_) {
            sniper_ = new ScreenShoter();
            connect(sniper_, &ScreenShoter::pinData, this, &Capturer::PreviewMimeData);
        }
        UpdateScreenshotStyle();
        sniper_->repeat();
    });

    connect(preview_hotkey_, &QHotkey::activated, this, &Capturer::PreviewClipboard);
    connect(toggle_hotkey_, &QHotkey::activated, this, &Capturer::TogglePreviews);
    connect(quicklook_hotkey_, &QHotkey::activated, this, &Capturer::QuickLook);
    connect(transparent_input_, &QHotkey::activated, this, &Capturer::TransparentPreviewInput);


    QTimer::singleShot(1 *  2 * 1000, this, []() { if (COMM.allowSendUserData()) COMM.sendUserData("start"); });
    //  auto check new versions after 1 minute
    QTimer::singleShot(1 * 60 * 1000, this, []() { if (COMM.verUpdate()->userAllowCheckUpdate()) COMM.verUpdate()->checkForUpdate(true); });
}

void Capturer::Init()
{
    clipboard::init();

    SystemTrayInit();

    if (config::easy_use::first_run) {
        emit COMM.sigShowSystemMessagebox(tr("%1 is runing").arg(XPROJECT_NAME), tr("You can control it via the tray icon."));
        config::easy_use::first_run = false;
    }

    UpdateHotkeys();

    UpdateScreenshotStyle();

    SetTheme(config::definite_theme());

    config::on_theme_changed = [this](const QString& theme) { SetTheme(theme); };

    config::set_autorun(config::general::autorun);

    logi("initialized.");
}

void Capturer::SystemTrayInit()
{
    QString suffix = ".svg";
#ifdef Q_OS_LINUX
    suffix = ".png";
#endif

    tray_.reset(new QSystemTrayIcon(QIcon(":/logo/logo" + suffix), this));
    tray_->setToolTip("Capturer Settings");

    tray_menu_.reset(new Menu());
    tray_menu_->setObjectName("tray-menu");

    tray_snip_ = tray_menu_->addAction(tr("Screenshot"), this, &Capturer::onCpature);
    tray_ocr = tray_menu_->addAction(tr("OCR"), &COMM, &Communication::onExtractText);

    tray_menu_->addSeparator();
    tray_settings_ = tray_menu_->addAction(tr("Settings"), this, &Capturer::OpenSettingsDialog);
    tray_menu_->addSeparator();
    tray_restart_ = tray_menu_->addAction(tr("Restart"), this, &Capturer::onRestart);
    // tray_menu_->addSeparator();
    tray_exit_ = tray_menu_->addAction(tr("Quit"), qApp, [](){ config::save(); qGuiApp->exit(); });

    tray_->setContextMenu(tray_menu_.data());
    tray_->show();

    connect(tray_.data(), &QSystemTrayIcon::activated, this, &Capturer::TrayActivated);
    connect(&COMM, &Communication::sigShowSystemMessagebox, this, [this](const QString &title, const QString &msg, const int &msecs = 6000){ tray_->showMessage(title, msg, QIcon(":/logo/logo.svg"), msecs); });
}

void Capturer::PreviewClipboard() { PreviewMimeData(clipboard::back(true)); }

void Capturer::PreviewMimeData(const std::shared_ptr<QMimeData>& mimedata)
{
    if (mimedata != nullptr) {

        FramelessWindow *preview{};

        if (mimedata->hasColor()) {
            preview = new ColorWindow(mimedata);
        }
        else {
            preview = new ImageWindow(mimedata);
        }

        preview->setAttribute(Qt::WA_DeleteOnClose);
        preview->show();
        if (!mimedata->hasFormat(clipboard::MIME_TYPE_POINT)) {
            const auto screen = screenAt(QCursor::pos()) ? screenAt(QCursor::pos()) : primaryScreen();
            const auto pos    = screen->geometry().center() - preview->rect().center();
            preview->move(std::max(screen->geometry().left(), pos.x()),
                          std::max(screen->geometry().top(), pos.y()));
        }
        previews_.emplace_back(preview);

        connect(preview, &FramelessWindow::closed, [this] {
            previews_.erase(std::ranges::remove_if(previews_, [](auto ptr) { return !ptr; }).begin(),
                            previews_.end());
        });
    }
}

void Capturer::OpenSettingsDialog()
{
    if (settings_window_) {
        settings_window_->activateWindow();
        settings_window_->raise();
        return;
    }

    settings_window_ = new SettingWindow();
    settings_window_->show();
    settings_window_->adjustSize();
    settings_window_->move(primaryScreen()->geometry().center() -
                           QPoint{ settings_window_->width() / 2, settings_window_->height() / 2 });
}

void Capturer::onRestart()
{
    config::save();
    const QString& path = qApp->applicationFilePath();
    QProcess::startDetached(path, QStringList());
    QCoreApplication::quit();
}

void Capturer::onCpature()
{
    if (sniper_) sniper_->disconnect();

    if (!sniper_) {
        sniper_ = new ScreenShoter();
        connect(sniper_, &ScreenShoter::pinData, this, &Capturer::PreviewMimeData);
    }
    UpdateScreenshotStyle();
    sniper_->start();
}

void Capturer::QuickLook()
{
#if _WIN32
    const auto active = probe::graphics::active_window();
    if (!active || active->pname != "explorer.exe") return;

    const auto file = (active->classname == "Progman" || active->classname == "WorkerW")
                          ? probe::graphics::desktop_focused()
                          : probe::graphics::explorer_focused(active->handle);

    if (file.empty()) return;

    auto data = std::make_shared<QMimeData>();
    data->setUrls({ QUrl::fromLocalFile(QString::fromUtf8(file.c_str())) });

    PreviewMimeData(data);
#endif
}

void Capturer::TogglePreviews()
{
    if (previews_.empty()) return;

    bool visible = false; // current state
    for (const auto& win : previews_) {
        if (win && win->isVisible()) {
            visible = true;
            break;
        }
    }

    for (const auto& win : previews_) {
        if (win) {
            win->setVisible(!visible);
        }
    }
}

void Capturer::TransparentPreviewInput()
{
    for (const auto& win : previews_) {
        if (win && win->geometry().contains(QCursor::pos())) {
            win->toggleTransparentInput();
#if _WIN32
            if (win->windowOpacity() == 1.0) win->setWindowOpacity(0.99);
#endif
        }
    }
}

void Capturer::UpdateHotkeys()
{
    QString error = "";
    // clang-format off
    SET_HOTKEY(snip_hotkey_,        config::hotkeys::screenshot);
    SET_HOTKEY(repeat_snip_hotkey_, config::hotkeys::repeat_last_screenshot);
    SET_HOTKEY(preview_hotkey_,     config::hotkeys::preview);
    SET_HOTKEY(toggle_hotkey_,      config::hotkeys::toggle_previews);
#if _WIN32
    SET_HOTKEY(quicklook_hotkey_,   config::hotkeys::quick_look);
#endif
    SET_HOTKEY(transparent_input_,  config::hotkeys::transparent_input);
    // clang-format on
    if (!error.isEmpty()) ShowMessage("Capturer", error, QSystemTrayIcon::Critical);
}

void Capturer::TrayActivated(const QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)  onCpature();
}

void Capturer::ShowMessage(const QString& title, const QString& msg,
                           const QSystemTrayIcon::MessageIcon icon, const int msecs)
{
    tray_->showMessage(title, msg, icon, msecs);
}

void Capturer::SetTheme(const QString& theme)
{
    if (theme_ == theme || (theme != "dark" && theme != "light")) return;

    theme_ = theme;

    QIcon::setThemeName(theme);

    std::vector<QString> files{
        ":/stylesheets/capturer",       ":/stylesheets/capturer-" + theme,
        ":/stylesheets/menu",           ":/stylesheets/menu-" + theme,
        ":/stylesheets/settingswindow", ":/stylesheets/settingswindow-" + theme,
        ":/stylesheets/player",
    };

    QString style{};
    for (auto& qss : files) {

        QFile file(qss);
        file.open(QFile::ReadOnly);

        if (file.isOpen()) {
            style += file.readAll();
            file.close();
        }
    }
    setStyleSheet(style);

    // system tray menu icons
    QString color = (config::definite_theme() == "dark") ? "light" : "dark";
    if (probe::util::tolower(probe::system::name()).starts_with("ubuntu")) {
        color = (probe::system::theme() == probe::system::theme_t::dark) ? "light" : "dark";
    }

    tray_snip_->setIcon(QIcon(":/icons/tray_menu/screenshot_" + color));
    tray_ocr->setIcon(QIcon(":/icons/tray_menu/ocr_textrec_" + color));
    tray_settings_->setIcon(QIcon(":/icons/tray_menu/setting_" + color));
    tray_restart_->setIcon(QIcon(":/icons/tray_menu/restart_" + color));
    tray_exit_->setIcon(QIcon(":/icons/tray_menu/quit_" + color));
}

void Capturer::UpdateScreenshotStyle()
{
    if (sniper_) sniper_->setStyle(config::snip::style);
}
