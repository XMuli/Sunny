// SPDX-SnippetCopyrightText: 2024-2025 SunnyCapturer Project
// SPDX-Author: Wengeng Zhang <xmulitech@gmail.com>

#ifndef COMMUNICATION_H
#define COMMUNICATION_H
#include <QObject>
#include <QString>
#include <QPointer>
#include "ui/ocr/ocrui.h"
#include "versionupdater.h"
#include "process/data_analytics/dbanalytics.h"

#define COMM Communication::instance()

class Communication : public QObject
{
    Q_OBJECT
public:
    QString toLocaleNameByKey(const QString& language);
    QString toLocaleNameByVal(const QString& locale);
    void trialVersion();
    static Communication &instance(); // signle
    QPointer<VersionUpdater> verUpdate() const;
    void extractTextLoadImage(const QPixmap& pix);  // OCRUI 导入具体识别图片

    bool allowSendUserData();
    void sendUserData(const QString& str);

public slots:
    void onExtractText();

signals:
    void sigShowSystemMessagebox(const QString &title, const QString &msg, const int& msecs = 10*1000); // 系统消息通知

    void sigOcrFinished(const QString& text, const QString& path);   // 提取文本保存成功 OCR
    void sigImgTransFinshed(const QString& path);                    // 图片翻译为图片保存成功

private:
    void init();

private:
    explicit Communication(QObject *parent = nullptr);
    virtual ~Communication();
    Communication(const Communication&) = delete;
    Communication& operator=(const Communication&) = delete;
    Communication(Communication&&) = delete;
    Communication& operator=(Communication&&) = delete;

private:
    QPointer<VersionUpdater>  m_verUpdate;
    QPointer<OcrUI>           m_ocrUI;     // tray 和 工具栏都需要，故放此
};

std::map<QString, QString> languageMap();

#endif // COMMUNICATION_H
