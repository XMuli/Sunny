// SPDX-SnippetCopyrightText: 2024-2025 SunnyCapturer Project
// SPDX-Author: ffiirree <ice_qi@163.com>
// SPDX-Author: Wengeng Zhang <xmulitech@gmail.com>

#include "config.h"

#include "helper.h"
#include "json.h"
#include "logging.h"
#include "qaesencryption.h"

#include <probe/system.h>
#include <probe/util.h>
#include <QApplication>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QTextStream>
#include <QCryptographicHash>
#include <QVersionNumber>

#define JSON_GET(V, J, KEY)                                                                                \
    if ((J).contains(KEY)) V = (J)[KEY].get<decltype(V)>()

namespace config
{
    void from_json(const json& j)
    {
        if (j.contains("general")) {
            JSON_GET(general::autorun, j["general"], "autorun");
            JSON_GET(general::language, j["general"], "language");
            JSON_GET(general::theme, j["general"], "theme");
        }

        if (j.contains("extract_text") && j["extract_text"].contains("account")) {
            JSON_GET(extract_text::use_engine, j["extract_text"], "use_engine");
            JSON_GET(extract_text::baseline_range, j["extract_text"], "baseline_range");
            JSON_GET(extract_text::tencent_cloud_id, j["extract_text"]["account"], "tencent_cloud_id");
            JSON_GET(extract_text::tencent_cloud_key, j["extract_text"]["account"], "tencent_cloud_key");
            JSON_GET(extract_text::baidu_cloud_id, j["extract_text"]["account"], "baidu_cloud_id");
            JSON_GET(extract_text::baidu_cloud_key, j["extract_text"]["account"], "baidu_cloud_key");
        }

        if (j.contains("image_translate") && j["image_translate"].contains("account")) {
            JSON_GET(image_translate::use_engine, j["image_translate"], "use_engine");
            JSON_GET(image_translate::tencent_cloud_id, j["image_translate"]["account"], "tencent_cloud_id");
            JSON_GET(image_translate::tencent_cloud_key, j["image_translate"]["account"], "tencent_cloud_key");
            JSON_GET(image_translate::tencent_cloud_souce_language, j["image_translate"]["account"], "tencent_cloud_souce_language");
            JSON_GET(image_translate::tencent_cloud_target_language, j["image_translate"]["account"], "tencent_cloud_target_language");

            JSON_GET(image_translate::baidu_cloud_id, j["image_translate"]["account"], "baidu_cloud_id");
            JSON_GET(image_translate::baidu_cloud_key, j["image_translate"]["account"], "baidu_cloud_key");
            JSON_GET(image_translate::baidu_cloud_souce_language, j["image_translate"]["account"], "baidu_cloud_souce_language");
            JSON_GET(image_translate::baidu_cloud_target_language, j["image_translate"]["account"], "baidu_cloud_target_language");

            JSON_GET(image_translate::youdao_cloud_id, j["image_translate"]["account"], "youdao_cloud_id");
            JSON_GET(image_translate::youdao_cloud_key, j["image_translate"]["account"], "youdao_cloud_key");
            JSON_GET(image_translate::youdao_cloud_souce_language, j["image_translate"]["account"], "youdao_cloud_souce_language");
            JSON_GET(image_translate::youdao_cloud_target_language, j["image_translate"]["account"], "youdao_cloud_target_language");
        }

        if (j.contains("easy_use") && j["easy_use"].contains("ocr_window")) {
            JSON_GET(easy_use::ocr_left, j["easy_use"]["ocr_window"], "ocr_left");
            JSON_GET(easy_use::ocr_top, j["easy_use"]["ocr_window"], "ocr_top");
            JSON_GET(easy_use::ocr_width, j["easy_use"]["ocr_window"], "ocr_width");
            JSON_GET(easy_use::ocr_height, j["easy_use"]["ocr_window"], "ocr_height");
            JSON_GET(easy_use::ocr_split_left, j["easy_use"]["ocr_window"], "ocr_split_left");
            JSON_GET(easy_use::ocr_split_right, j["easy_use"]["ocr_window"], "ocr_split_right");

            JSON_GET(easy_use::time, j["easy_use"], "time");
            JSON_GET(easy_use::show_tip_rect_px, j["easy_use"], "show_tip_rect_px");
            JSON_GET(easy_use::first_run, j["easy_use"], "first_run");
        }

        if (j.contains("output")) {
            JSON_GET(output::image_quality, j["output"], "image_quality");
            JSON_GET(output::last_dir, j["output"], "last_dir");
        }

        if (j.contains("hotkeys")) {
            JSON_GET(hotkeys::screenshot, j["hotkeys"], "screenshot");
            JSON_GET(hotkeys::repeat_last_screenshot, j["hotkeys"], "repeat-last-screenshot");
            JSON_GET(hotkeys::preview, j["hotkeys"], "preview");
            JSON_GET(hotkeys::toggle_previews, j["hotkeys"], "toggle-previews");
            JSON_GET(hotkeys::quick_look, j["hotkeys"], "quick-look");
            JSON_GET(hotkeys::transparent_input, j["hotkeys"], "transparent-input");
        }

        if (j.contains("snip")) {
            if (j["snip"].contains("style")) {
                JSON_GET(snip::style.border_width, j["snip"]["style"], "border-width");
                JSON_GET(snip::style.border_color, j["snip"]["style"], "border-color");
                JSON_GET(snip::style.border_style, j["snip"]["style"], "border-style");
                JSON_GET(snip::style.mask_color, j["snip"]["style"], "mask-color");
            }
        }

        if (j.contains("update")) {
            JSON_GET(update::last_check_time, j["update"], "last_check_time");
            JSON_GET(update::version, j["update"], "version");
        }
    }

    json to_json()
    {
        json j;
        j["general"] = {
            {"autorun", general::autorun},
            {"language", general::language},
            {"theme", general::theme},
        };

        j["extract_text"] = {
            {"use_engine",     extract_text::use_engine},
            {"baseline_range", extract_text::baseline_range},
            {"account", {
                {"tencent_cloud_id",  extract_text::tencent_cloud_id},
                {"tencent_cloud_key", extract_text::tencent_cloud_key},
                {"baidu_cloud_id",    extract_text::baidu_cloud_id},
                {"baidu_cloud_key",   extract_text::baidu_cloud_key},
                },
            }
        };

        j["image_translate"] = {
            {"use_engine", image_translate::use_engine},
            {"account", {
                {"tencent_cloud_id",              image_translate::tencent_cloud_id},
                {"tencent_cloud_key",             image_translate::tencent_cloud_key},
                {"tencent_cloud_souce_language",  image_translate::tencent_cloud_souce_language},
                {"tencent_cloud_target_language", image_translate::tencent_cloud_target_language},

                {"baidu_cloud_id",                image_translate::baidu_cloud_id},
                {"baidu_cloud_key",               image_translate::baidu_cloud_key},
                {"baidu_cloud_souce_language",    image_translate::baidu_cloud_souce_language},
                {"baidu_cloud_target_language",   image_translate::baidu_cloud_target_language},

                {"youdao_cloud_id",               image_translate::youdao_cloud_id},
                {"youdao_cloud_key",              image_translate::youdao_cloud_key},
                {"youdao_cloud_souce_language",   image_translate::youdao_cloud_souce_language},
                {"youdao_cloud_target_language",  image_translate::youdao_cloud_target_language}},
              }
        };

        j["easy_use"] = {
            {"ocr_window", {
                {"ocr_left",  easy_use::ocr_left},
                {"ocr_top",  easy_use::ocr_top},
                {"ocr_width",  easy_use::ocr_width},
                {"ocr_height",  easy_use::ocr_height},
                {"ocr_split_left",  easy_use::ocr_split_left},
                {"ocr_split_right",  easy_use::ocr_split_right}},
            },
            {"time",  easy_use::time},
            {"show_tip_rect_px",  easy_use::show_tip_rect_px},
            {"first_run",  easy_use::first_run},
        };

        j["output"] = {
            {"image_quality", output::image_quality},
            {"last_image_extension", output::last_image_extension},
            {"last_dir", output::last_dir},
            {"manual_save", {
                {"file_name", output::ms_file_name},
                {"remeber_last_image_extension", output::ms_remeber_last_image_extension}}},
            {"quick_save", {
                {"path", output::qs_path},
                {"show_notification", output::qs_show_notification}}},
            {"auto_save", {
                {"enable", output::as_enable},
                {"path", output::as_path}}}
        };

        j["update"] = {
            {"version", update::version},
            {"last_check_time", update::last_check_time},
            {"enable_auto_check", update::enable_auto_check},
            {"day", update::day},
            {"join_inside", update::join_inside},
            {"porxy", {
                {"type", update::type},
                {"server", update::server},
                {"port", update::port}}}
        };

        j["hotkeys"]["screenshot"]             = hotkeys::screenshot;
        j["hotkeys"]["repeat-last-screenshot"] = hotkeys::repeat_last_screenshot;
        j["hotkeys"]["preview"]                = hotkeys::preview;
        j["hotkeys"]["toggle-previews"]        = hotkeys::toggle_previews;
        j["hotkeys"]["quick-look"]             = hotkeys::quick_look;
        j["hotkeys"]["transparent-input"]      = hotkeys::transparent_input;

        j["snip"]["style"]["border-width"] = snip::style.border_width;
        j["snip"]["style"]["border-color"] = snip::style.border_color;
        j["snip"]["style"]["border-style"] = snip::style.border_style;
        j["snip"]["style"]["mask-color"]   = snip::style.mask_color;

        return j;
    }

    std::string encryptString(const QString &input)
    {
        if (input.isEmpty()) return std::string();

        QAESEncryption encryption(QAESEncryption::AES_256, QAESEncryption::CBC);
        QByteArray hashKey = QCryptographicHash::hash(m_key.toLocal8Bit(), QCryptographicHash::Sha256);
        QByteArray hashIV = QCryptographicHash::hash(m_iv.toLocal8Bit(), QCryptographicHash::Md5);
        QByteArray encodeText = encryption.encode(input.toLocal8Bit(), hashKey, hashIV);

        return QString(encodeText.toBase64()).toStdString(); // 保存为 base64 的加密
    }

    QString decryptString(const std::string &input)
    {
        if (input.empty()) return QString();

        QByteArray byte = QByteArray::fromBase64(QString::fromStdString(input).toUtf8());
        QAESEncryption encryption(QAESEncryption::AES_256, QAESEncryption::CBC);
        QByteArray hashKey = QCryptographicHash::hash(m_key.toLocal8Bit(), QCryptographicHash::Sha256);
        QByteArray hashIV = QCryptographicHash::hash(m_iv.toLocal8Bit(), QCryptographicHash::Md5);
        QByteArray decodeText = encryption.decode(byte, hashKey, hashIV);
        QString decodedString = QString(encryption.removePadding(decodeText));

        return decodedString;
    }

} // namespace config

namespace config
{
    void load()
    {
        #if defined(Q_OS_WIN)
            #ifdef BUILD_MICROSOFT_STORE
                QString configPath = QDir::homePath() + QString("/.config/%1").arg(XPROJECT_NAME);
                QDir dir(configPath);

                if (!dir.exists()) dir.mkpath(configPath);
                general::filepath = configPath + "/" + filename;
            #else
                general::filepath = qGuiApp->applicationDirPath() + "/" + filename;
            #endif

        #elif defined(Q_OS_LINUX)


            bool appimage = Helper::isAppImagePackage();
            // if (appimage) directoryPath = qGuiApp->applicationDirPath() + QString("../../opt/%1/translations/").arg(XPROJECT_NAME);
            // general::filepath = QString("/opt/%1/translations/").arg(XPROJECT_NAME);
            qDebug() << "---------->general::filepath:" << general::filepath << "appimage:" << appimage;

        #else
        #endif

        qDebug() << "general::filepath:" << general::filepath;

        // load configure file
        QString text;
        QFile   file(general::filepath);
        if (file.open(QIODevice::ReadWrite | QIODevice::Text)) {
            QTextStream in(&file);
            text = in.readAll();
            file.close();
        }

        if (!text.isEmpty()) {

            // parse json string
            try {
                from_json(json::parse(text.toStdString()));
            }
            catch (json::parse_error& e) {
                logw("JSON parse error: {}", e.what());
            }
        }

        { // output
            if (output::qs_path.isEmpty())
                output::qs_path = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + "/" + output::ms_file_name;

            if (output::as_path.isEmpty())
                output::as_path = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + "/" + output::ms_file_name;

            if (output::last_dir.isEmpty())
                output::last_dir = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
        }

        {
            #if defined(Q_OS_LINUX)
                if (QFile::exists(general::filepath) && !update::version.isEmpty()) {
                    QVersionNumber vJson = QVersionNumber::fromString(update::version);
                    QVersionNumber vSoft = QVersionNumber::fromString(QString(XPROJECT_VERSION));
                    if (vJson < vSoft) { // deb install new version

                        QFile file(general::filepath);
                        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) // Close after emptying the file
                            file.close();
                    }
                }
            #endif

            update::version = XPROJECT_VERSION;
        }
    }

    void save()
    {
        QFile file(general::filepath);

        if (!file.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text)) return;

        QTextStream out(&file);
        out << to_json().dump(4).c_str();

        file.close();
    }

    QString definite_theme()
    {
        if (general::theme == "auto") {
            return probe::to_string(probe::system::theme()).c_str();
        }

        return (general::theme == "dark") ? "dark" : "light";
    }

    void set_theme(const std::string& nt)
    {
        if (general::theme == nt) return;

        general::theme = nt;

        monitor_system_theme(general::theme == "auto");

        on_theme_changed(definite_theme());
    }

    void monitor_system_theme(bool m)
    {
#ifdef _WIN32
        if (m && theme_monitor == nullptr) {
            theme_monitor = std::make_shared<probe::util::registry::RegistryListener>();

            theme_monitor->listen(
                std::pair<HKEY, std::string>{
                    HKEY_CURRENT_USER, R"(Software\Microsoft\Windows\CurrentVersion\Themes\Personalize)" },
                [&](auto) { set_theme(probe::to_string(probe::system::theme())); });
        }

        if (!m) {
            theme_monitor = nullptr;
        }
#elif __linux__
        if (m && theme_monitor == nullptr) {
            if (probe::system::desktop_environment() == probe::system::desktop_environment_t::GNOME ||
                probe::system::desktop_environment() == probe::system::desktop_environment_t::Unity) {

                theme_monitor = std::make_shared<probe::util::PipeListener>();

                auto color_scheme = probe::util::exec_sync(
                    { "gsettings", "get", "org.gnome.desktop.interface", "color-scheme" });
                if (!color_scheme.empty() && color_scheme[0] != "\t") { // TODO
                    theme_monitor->listen(std::vector{ "gsettings", "monitor",
                                                       "org.gnome.desktop.interface", "color-scheme" },
                                          [](auto str) {
                                              auto _theme = probe::system::theme_t::light;
                                              if (std::any_cast<std::string>(str).find("dark") !=
                                                  std::string::npos) {
                                                  _theme = probe::system::theme_t::dark;
                                              }
                                              set_theme(probe::to_string(_theme));
                                          });

                    return;
                }

                auto gtk_theme = probe::util::exec_sync(
                    { "gsettings", "get", "org.gnome.desktop.interface", "gtk-theme" });
                if (!gtk_theme.empty() && gtk_theme[0] != "\t") {
                    theme_monitor->listen(
                        std::vector{ "gsettings", "monitor", "org.gnome.desktop.interface", "gtk-theme" },
                        [](auto str) {
                            auto _theme = probe::system::theme_t::light;
                            if (std::any_cast<std::string>(str).find("dark") != std::string::npos) {
                                _theme = probe::system::theme_t::dark;
                            }

                            set_theme(probe::to_string(_theme));
                        });

                    return;
                }
            }
        }

        if (!m) {
            theme_monitor = nullptr;
        }
#endif
    }

    bool set_autorun(bool state)
    {
#ifdef _WIN32

        QSettings run("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
        QString appPath = QString("\"%1\"").arg(QDir::toNativeSeparators(qGuiApp->applicationFilePath()));
        run.setValue(XPROJECT_NAME, state ? appPath : "");

#elif __linux__
        const std::string desktop_file = "/usr/share/applications/sunnycapturer.desktop";
        const std::string autorun_dir  = std::string{ ::getenv("HOME") } + "/.config/autostart";
        const std::string autorun_file = autorun_dir + "/sunnycapturer.desktop";

        if (!std::filesystem::exists(desktop_file)) {
            logw("failed to set `autorun` since the '{}' does not exists.", desktop_file);
            state = false;
        }

        if (state) {
            if (std::filesystem::exists(desktop_file) && !std::filesystem::exists(autorun_file)) {
                if (!std::filesystem::exists(autorun_dir)) {
                    std::filesystem::create_directories(autorun_dir);
                }
                std::filesystem::create_symlink(desktop_file, autorun_file);
            }
        }
        else {
            std::filesystem::remove(autorun_file);
        }
#endif

        config::general::autorun = state;

        return state;
    }

} // namespace config
