// SPDX-SnippetCopyrightText: 2024-2025 SunnyCapturer Project
// SPDX-Author: Wengeng Zhang <xmulitech@gmail.com>

#include "communication.h"
#include <cstdlib> // 包含 exit 函数的头文件
#include <QLocale>
#include <QTranslator>
#include <QGuiApplication>
#include <QDebug>
#include <QMessageBox>
#include <QDate>
#include <QString>
#include "config.h"
// #include "../ui/tray/tray.h"

Communication &Communication::instance()
{
    static Communication ins;
    return ins;
}

QString Communication::toLocaleNameByKey(const QString &language)
{
    const auto& map = languageMap();
    auto it = map.find(language);
    return it != map.cend() ? it->second : "en_us";
}

QString Communication::toLocaleNameByVal(const QString &locale)
{
    const auto& map = languageMap();
    // 遍历 map 查找 value 对应的 key
    for (const auto& pair : map)
    {
        if (pair.second == locale)
            return pair.first; // 返回对应的 key
    }

    return "English"; // 如果没有找到对应的 value，返回空字符串
}

void Communication::trialVersion()
{
    // 设定检查的日期
    QDate checkDate(2025, 9, 30);
    QDate currentDate = QDate::currentDate();

    if (currentDate >= checkDate) {
        QString message = QObject::tr(
            "Your current version is outdated. Please download the latest version from "
            "<a href='https://sunnycapturer.xmuli.tech/'>https://sunnycapturer.xmuli.tech/</a> or contact developer at "
            "<a href='mailto:xmulitech@gmail.com'>xmulitech@gmail.com</a> for the download link."
            );

        QMessageBox msgBox;
        msgBox.setWindowTitle(QObject::tr("Version Update"));
        msgBox.setText(message);
        msgBox.setTextFormat(Qt::RichText);  // 使用富文本格式
        msgBox.setTextInteractionFlags(Qt::TextBrowserInteraction);  // 允许交互
        msgBox.setStandardButtons(QMessageBox::Ok);

        // 执行消息框，并在用户点击 OK 后退出程序
        if (msgBox.exec() == QMessageBox::Ok) {
            std::exit(0);
        }
    }
}

Communication::Communication(QObject *parent)
    : QObject(parent)
    // , m_hkCapture(new QHotkey(CJ_GET_QSTR("shortcuts.capture"), true, qApp))              //The hotkey will be automatically registered
    // , m_hkDelayCapture(new QHotkey(CJ_GET_QSTR("shortcuts.delay_capture"), true, qApp))
    // , m_hkScrollCapture(new QHotkey(CJ_GET_QSTR("shortcuts.scroll_capture"), true, qApp))
    // , m_hkImageTranslateCapture(new QHotkey(CJ_GET_QSTR("shortcuts.image_translate_capture"), true, qApp))
    // , m_hkExtractTextCapture(new QHotkey(CJ_GET_QSTR("shortcuts.extract_text_capture"), true, qApp))
    // , m_hkOpenExtractText(new QHotkey(CJ_GET_QSTR("shortcuts.open_extract_text"), true, qApp))
    , m_verUpdate(new VersionUpdater(QString(XPROJECT_VERSION), this))
    , m_ocrUI(nullptr)
{
    init();
}

Communication::~Communication()
{
    if (m_ocrUI) m_ocrUI->deleteLater();
}

QPointer<VersionUpdater> Communication::verUpdate() const
{
    return m_verUpdate;
}

void Communication::extractTextLoadImage(const QPixmap &pix)
{
    if (pix.isNull() || !m_ocrUI) return;

    m_ocrUI->setLeftPixmap(pix, true);
    m_ocrUI->setRightText("");
}

// 是否满足发送时间： 时间至少属于第二天就行
bool Communication::allowSendUserData()
{
    QString format = "yyyy-MM-dd HH:mm:ss";
    QString lastTimeStr = config::easy_use::time;
    if (lastTimeStr.isEmpty())  // 如果 lastTimeStr 为空，直接返回 true
        return true;

    // 获取当前时间的QDateTime对象
    QDateTime currentDateTime = QDateTime::currentDateTime();
    QDateTime lastDateTime = QDateTime::fromString(lastTimeStr, format);

    // 检查解析是否成功
    if (!lastDateTime.isValid()) {
        qWarning() << "Invalid date format.";
        return false;
    }

    // 计算日期差异
    QDate lastDate = lastDateTime.date();
    QDate currentDate = currentDateTime.date();

    // 检查当前时间是否已经跨过了第二天的00:00
    if (currentDate > lastDate)
        return true;

    return false;
}

void Communication::sendUserData(const QString &str)
{
#ifndef QT_DEBUG
    static DbAnalytics dbAnalytics;
    dbAnalytics.sendData(str);
#endif
}

void Communication::onExtractText()
{
    if (!m_ocrUI) m_ocrUI = new OcrUI();
    m_ocrUI->show();
    m_ocrUI->activateWindow();
}


void Communication::init()
{
    // connect(m_hkOpenExtractText, &QHotkey::activated, this, &Communication::onExtractText);
}


std::map<QString, QString> languageMap()
{
    static  std::map<QString, QString> map = {
          {"English", "en_us"}
        , {"简体中文", "zh_cn"}
        , {"繁体中文", "zh_tw"}
        , {"日本語",   "ja_jp"}
        , {"Português Brasil", "pt_br"}     // Portuguese-Brazil 葡萄牙语-巴西
        , {"Deutsch", "de"}                 // 德语
    };
    return map;
}

