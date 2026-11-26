// SPDX-SnippetCopyrightText: 2024-2025 SunnyCapturer Project
// SPDX-Author: Wengeng Zhang <xmulitech@gmail.com>
#ifndef NETTENCENT_H
#define NETTENCENT_H

#include <QObject>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QEventLoop>

class NetTencent : public QObject
{
    Q_OBJECT
public:

    enum class TYPE {
        // T_tencent_cloud_ocr,
        T_tencent_cloud_pix_trans,             // Tencent Machine Translation 机器翻译(图片翻译)，这里当作 OCR + 已经翻译好的翻译
        T_baidu_cloud_ocr,         // Baidu OCR
        T_baidu_cloud_pix_trans,   // Baidu Picture translation
        T_youdao_cloud_pix_trans,  // Youdao Picture translation
        T_paddle_ocr
    };
    Q_ENUM(TYPE)

    enum class RECOGNITION {
        R_extract_text,         // 提取文字 OCR
        R_img_trans            // 机器翻译(图片翻译)
        // R_only_trans            // 仅存粹的语言翻译
    };
    Q_ENUM(RECOGNITION)

    explicit NetTencent(QObject *parent = nullptr);
    virtual ~NetTencent() = default;

    void requestAPI(const QString& path, const TYPE& type, const RECOGNITION& recognition = RECOGNITION::R_img_trans);      // send pix Translate Request

private:
    QNetworkReply* reqTencentCloudTMT(const QString &path, const RECOGNITION& recognition);                 // send pix Translate Request
    void onReqTencentTmt(QNetworkReply *reply);

    QNetworkReply* reqBaiduCloudOcr(const QString& path, const RECOGNITION& recognition);
    void onReqBaiduCloudOcr(QNetworkReply *reply);

    QNetworkReply* reqBaiduCloudPixTrans(const QString& path, const RECOGNITION& recognition);
    void onReqBaiduCloudPixTrans(QNetworkReply *reply);

    QNetworkReply* reqYoudaoCloudPixTrans(const QString& path);
    void onReqYoudaoCloudPixTrans(QNetworkReply *reply);

private:
    // base
    const QStringList rawHeader(const QNetworkReply* reply) const;                                 // 解析头部
    const QString replyErrorShowText(const QStringList& dataHead);
    QString base64FromFileContent(const QString& path, bool urlencoded = false);     // T_tencent_cloud_pix_trans + T_baidu_cloud_ocr
    void generateErrorImage(const QString &errorMessage);

    // baidu
    void getBaiduTokens(const RECOGNITION& recognition);
    const bool validityBaiDuKey(const QString &id, const QString &secret, const RECOGNITION &recognition);     // 校验 BaiDu key 的有效性

private slots:
    void onRequestFinished(QNetworkReply* reply);

private:
    QPointer<QNetworkAccessManager>      m_netMgn;
};

#endif // NETTENCENT_H
