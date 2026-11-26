// SPDX-SnippetCopyrightText: 2024-2025 SunnyCapturer Project
// SPDX-Author: ffiirree <ice_qi@163.com>
// SPDX-Author: Wengeng Zhang <xmulitech@gmail.com>
#ifndef CAPTURER_CONFIG_H
#define CAPTURER_CONFIG_H

#include "selector.h"

#include <functional>
#include <QLocale>

namespace config
{
    namespace general
    {
        inline bool        autorun{ true };
        inline QString     language{ QLocale::system().name().toLower() };
        inline std::string theme{ "auto" }; // auto, dark, light
        inline QString     filepath{};
    }

    namespace extract_text
    {
        inline QString     use_engine{ "tencent_cloud" };
        inline int         baseline_range{ 5 };
        // account
        inline QString     tencent_cloud_id{ "" };
        inline QString     tencent_cloud_key{ "" };
        inline QString     baidu_cloud_id{ "" };
        inline QString     baidu_cloud_key{ "" };
    }

    namespace image_translate
    {
        inline QString     use_engine{ "tencent_cloud" };
        // account
        inline QString     tencent_cloud_id{ "" };
        inline QString     tencent_cloud_key{ "" };
        inline QString     tencent_cloud_souce_language{ "auto" };
        inline QString     tencent_cloud_target_language{ "zh" };

        inline QString     baidu_cloud_id{ "" };
        inline QString     baidu_cloud_key{ "" };
        inline QString     baidu_cloud_souce_language{ "auto" };
        inline QString     baidu_cloud_target_language{ "zh" };

        inline QString     youdao_cloud_id{ "" };
        inline QString     youdao_cloud_key{ "" };
        inline QString     youdao_cloud_souce_language{ "auto" };
        inline QString     youdao_cloud_target_language{ "zh-CHS" };
    }

    namespace easy_use
    {
        // ocr_window info last_open and ocr_split
        inline int     ocr_left{ 0 };
        inline int     ocr_top{ 0 };
        inline int     ocr_width{ 0 };
        inline int     ocr_height{ 0 };
        inline double  ocr_split_left{ 775 };
        inline double  ocr_split_right{ 425 };
        inline QString time{}; // send user data last time

        inline bool    show_tip_rect_px{ true };

        inline bool     first_run{ true };
    }

    namespace output
    {
        inline int     image_quality{ -1 };
        inline QString last_image_extension{ "png" };
        inline QString last_dir{ "" };

        // manual save
        inline QString ms_file_name{ "SunnyCaptruer_$yyyy-MM-dd_HH-mm-ss$.png" };
        inline bool    ms_remeber_last_image_extension{ true };
        // quick save
        inline QString qs_path{ "" };
        inline bool    qs_show_notification{ true };
        // auto save
        inline bool    as_enable{ false };
        inline QString as_path{ "" };
    }; // namespace output

    namespace hotkeys
    {
        inline QKeySequence screenshot{ "F1" };
        inline QKeySequence repeat_last_screenshot{ "Ctrl+PgUp" };
        inline QKeySequence preview{ "Shift+F3" };
        inline QKeySequence toggle_previews{ "Alt+F3" };
        inline QKeySequence quick_look{ "Shift+F6" };
        inline QKeySequence transparent_input{ "Ctrl+T" };
    }; // namespace hotkeys

    namespace snip
    {
        inline SelectorStyle style{ 2, "#409EFF", Qt::SolidLine, "#88000000" };
    } // namespace snip

    namespace update
    {
        inline QString version{ "" };         // linux check version update replace
        inline QString last_check_time{ "" };
        inline bool    enable_auto_check{ true };
        inline int     day{ 1 };
        inline bool    join_inside{ true };
        // porxy
        inline int     type{ 1 };
        inline QString server{ "" };
        inline int     port{ 0 };
        inline QString url_down{ "https://github.com/XMuli/SunnyCapturer/releases" };
    } // namespace update

    void from_json(const json& j);
    void to_json(json& j);

    // using API-Key
    std::string encryptString(const QString& input);
    QString decryptString(const std::string& input);

    static QString      m_key{ "f%AvG3Wp3jBKsLub" };
    static QString      m_iv{ "34Hc2&TXf%FzTZGY" };
} // namespace config

namespace config
{
    inline constexpr auto                      filename = "sunny_capturer.json";
    inline std::shared_ptr<probe::Listener>    theme_monitor{};
    inline std::function<void(const QString&)> on_theme_changed = [](auto) {};

    void load();
    void save();

    QString definite_theme();

    bool set_autorun(bool autorun);

    void set_theme(const std::string& theme);

    void monitor_system_theme(bool en);
} // namespace config

#endif //! CAPTURER_CONFIG_H
