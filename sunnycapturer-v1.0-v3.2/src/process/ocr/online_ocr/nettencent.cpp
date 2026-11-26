// SPDX-SnippetCopyrightText: 2024-2025 SunnyCapturer Project
// SPDX-Author: Wengeng Zhang <xmulitech@gmail.com>
#include "nettencent.h"
#include <QFile>
#include <QCoreApplication>
#include <QDateTime>
#include <QMessageAuthenticationCode>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCryptographicHash>
#include <QByteArray>
#include <QProcess>
#include <QDebug>
#include <QTextStream>
#include <QString>
#include <QUrl>
#include <QUrlQuery>
#include <QMetaEnum>
#include <QHttpPart>
#include <QFileInfo>
#include <QImage>
#include <QDir>
#include <QStandardPaths>
#include <QPainter>
#include <QMessageBox>
#include "communication.h"
#include "config.h"
#include "helper.h"
#include "helper/net_helper.h"
#include "helper/authv3util.h"
#include "json.h"
using json = nlohmann::json;

QString get_data(qint64 timestamp)
{
    QDateTime dateTime = QDateTime::fromSecsSinceEpoch(timestamp, Qt::UTC);
    return dateTime.toString("yyyy-MM-dd");
}

QByteArray sha256Hex(const QString &str)
{
    QByteArray hash = QCryptographicHash::hash(str.toUtf8(), QCryptographicHash::Sha256).toHex();
    // qDebug() << "hash:" << hash;
    return hash;
}

QByteArray hmacSha256(const QByteArray &key, const QByteArray &input)
{
    QByteArray sign = QMessageAuthenticationCode::hash(input, key, QCryptographicHash::Sha256);
    // qDebug() << "sign:" << sign << "sign toHex:" << sign.toHex();
    return sign;
}

NetTencent::NetTencent(QObject *parent)
    : QObject{parent}
    , m_netMgn(new QNetworkAccessManager(this))
{
    connect(m_netMgn, &QNetworkAccessManager::finished, this, &NetTencent::onRequestFinished);

    // request(":/images/test/20240813_161730.png", TYPE::T_baidu_cloud_ocr);
    // request(":/images/test/20240813_161730.png", TYPE::T_baidu_cloud_pix_trans);
    // request(":/images/test/20240813_161730.png", TYPE::T_tencent_cloud_pix_trans);
}

void NetTencent::requestAPI(const QString &path, const TYPE &type, const RECOGNITION& recognition)
{
    qDebug() << "[requestAPI] path" << path << "type:" << type << "recognition:" << recognition;
    setProperty("image_path", path);

    QNetworkReply* reply = nullptr;
    QString text;
    if (recognition == RECOGNITION::R_extract_text) {
        if (type == TYPE::T_tencent_cloud_pix_trans) {
            reply = reqTencentCloudTMT(path, RECOGNITION::R_extract_text);
            text = tr("Tencent cloud extract text");
        } else if (type == TYPE::T_baidu_cloud_ocr) {
            reply = reqBaiduCloudOcr(path, recognition);
            text = tr("BaiDu cloud extract text");
        } else if (type == TYPE::T_paddle_ocr) {
            return;
        }
    } else if (recognition == RECOGNITION::R_img_trans) {
        if (type == TYPE::T_tencent_cloud_pix_trans) {
            reply = reqTencentCloudTMT(path, RECOGNITION::R_img_trans);
            text = tr("Tencent cloud image translate");
        } else if (type == TYPE::T_baidu_cloud_pix_trans) {
            reply = reqBaiduCloudPixTrans(path, recognition);
            text = tr("BaiDu cloud image translate");
        } else if (type == TYPE::T_youdao_cloud_pix_trans) {
            reply = reqYoudaoCloudPixTrans(path);
            text = tr("YouDao cloud image translate");
        }
    }

    connect(reply, &QNetworkReply::errorOccurred, this, [reply, text](QNetworkReply::NetworkError code) {
        if (!reply) return;
        const QMetaObject &metaObject = QNetworkReply::staticMetaObject;
        QMetaEnum metaEnum = metaObject.enumerator(metaObject.indexOfEnumerator("NetworkError"));
        QString errorMessage = text + QString(" failed!\n\nError code: %1\nReason: %2\ndescribe: %3").arg(static_cast<int>(code)).arg(reply->errorString()).arg(QString::fromLocal8Bit(metaEnum.valueToKey(code)));
        // qDebug() << "errorMessage:" << errorMessage << "" << reply->error();
        // emit COMM.sigShowSystemMessagebox(tr("NETWORK ERROR"), tr("%1").arg(errorMessage));
        QMessageBox::critical(nullptr, tr("NETWORK ERROR"), errorMessage);
    });
}

QNetworkReply* NetTencent::reqTencentCloudTMT(const QString &path, const RECOGNITION &recognition)
{
    // Provider credentials are loaded from encrypted local configuration.

    json para;
    std::string id;
    std::string key;
    using namespace config;

    if (recognition == RECOGNITION::R_extract_text) {
        id = extract_text::tencent_cloud_id.toStdString();
        key = extract_text::tencent_cloud_key.toStdString();
    } else if (recognition == RECOGNITION::R_img_trans) {
        id = image_translate::tencent_cloud_id.toStdString();
        key = image_translate::tencent_cloud_key.toStdString();
    }

    // // 使用默认的密码
    if (id.empty()) id = "AZpvFcuhyPcvRGOFaaJFfaBj7JTClK5QZ95GqhhoMbTOevpgn9uua4joqH7nFbp3";
    if (key.empty()) key = "BeIJZvl71mXqt9fc+B3oSt7zdQv5DMa6UOQbSxM+6NY=";

    QString secret_id = config::decryptString(id);
    QString secret_key = config::decryptString(key);

    QString service = "tmt";
    QString host = "tmt.tencentcloudapi.com";
    QString region = "ap-guangzhou";
    QString version = "2018-03-21";
    QString action = "ImageTranslate";
    qint64 timestamp = QDateTime::currentSecsSinceEpoch(); //1551113065;
    QString date = get_data(timestamp);
    // qDebug() << "timestamp:" << timestamp << "date" << date;

    // 步骤 1：拼接规范请求串
    QString httpRequestMethod = "POST";
    QString canonicalUri = "/";
    QString canonicalQueryString = "";
    QString lower = action.toLower();
    QString canonicalHeaders = QString("content-type:application/json; charset=utf-8\n") +
                               "host:" + host + "\n" +
                               "x-tc-action:" + lower + "\n";
    // qDebug().noquote() << "canonicalHeaders:" << canonicalHeaders;
    QString signedHeaders = "content-type;host;x-tc-action";
    // QString payload = "{\"SessionUuid\":\"session-00001\",\"Scene\":\"doc\",\"Data\":\"iVBORw0KGgoAkJggg==\",\"Source\":\"auto\",\"Target\":\"zh\",\"ProjectId\":0}";

    QString base64DataUtf8 = base64FromFileContent(path);
    // qDebug().noquote() << "base64DataUtf8:" << base64DataUtf8;
    json j = {
        {"SessionUuid", "session-00001"},
        {"Scene", "doc"}, // doc:文档扫描
        {"Data", base64DataUtf8.toStdString()},
        {"Source", image_translate::tencent_cloud_souce_language.toStdString()},
        {"Target", image_translate::tencent_cloud_target_language.toStdString()},
        {"ProjectId", 0}
    };

    QString payload = QString::fromStdString(j.dump());  // 将 JSON 对象序列化为字符串
    QString hashedRequestPayload = sha256Hex(payload);
    QString canonicalRequest = httpRequestMethod + "\n" +
                               canonicalUri + "\n" +
                               canonicalQueryString + "\n" +
                               canonicalHeaders + "\n" +
                               signedHeaders + "\n" +
                               hashedRequestPayload;
    // qDebug().noquote() << "canonicalRequest:" << canonicalRequest;

    // 步骤 2：拼接待签名字符串
    QString algorithm = "TC3-HMAC-SHA256";
    QString requestTimestamp = QString::number(timestamp);
    QString credentialScope = date + "/" + service + "/tc3_request";
    QByteArray hashedCanonicalRequest = sha256Hex(canonicalRequest);
    QString stringToSign = algorithm + "\n" + requestTimestamp + "\n" + credentialScope + "\n" + hashedCanonicalRequest;
    // qDebug().noquote() << "[stringToSign]:" << stringToSign;

    // 步骤 3：计算签名
    QString kKey = "TC3" + secret_key;
    QByteArray kDate = hmacSha256(kKey.toUtf8(), date.toUtf8());
    QByteArray kService = hmacSha256(kDate, service.toUtf8());
    QByteArray kSigning = hmacSha256(kService, QByteArray("tc3_request"));
    QByteArray signature = hmacSha256(kSigning, stringToSign.toUtf8()).toHex(); // 最后一个要转为一个十六进制

    // QByteArray kDate2 = hmacSha256(QByteArray("TC3lIgJ2nPq57wXO2RghCYmqy2u7EIAvbGn"), QByteArray("2019-02-25"));
    // qDebug() << "kDate2:" << kDate2.toHex();
    // qDebug() << "[kDate]:" << kDate.toHex();
    // qDebug() << "[kService]:" << kService.toHex();
    // qDebug() << "[kSigning]:" << kSigning.toHex();
    // qDebug() << "[Signature]:" << signature;

    // 步骤 4：拼接 Authorization
    QString authorization = algorithm + " Credential=" + secret_id + "/" + credentialScope + ", " + "SignedHeaders=" + signedHeaders + ", Signature=" + QString::fromUtf8(signature);
    // qDebug() << "[Authorization]:" << authorization;

    QNetworkRequest request(QUrl("https://" + host));
    request.setRawHeader("Authorization", authorization.toUtf8());
    request.setRawHeader("Content-Type", "application/json; charset=utf-8");
    request.setRawHeader("Host", host.toUtf8());
    request.setRawHeader("X-TC-Action", action.toUtf8());
    request.setRawHeader("X-TC-Timestamp", requestTimestamp.toUtf8());
    request.setRawHeader("X-TC-Version", version.toUtf8());
    request.setRawHeader("X-TC-Region", region.toUtf8());
    request.setAttribute(QNetworkRequest::User, QVariant::fromValue(TYPE::T_tencent_cloud_pix_trans));
    request.setAttribute(static_cast<QNetworkRequest::Attribute>(QNetworkRequest::User + 1), QVariant::fromValue(recognition));

    return m_netMgn->post(request, payload.toUtf8());
}

void NetTencent::onReqTencentTmt(QNetworkReply *reply)
{
    const QStringList& dataHead = rawHeader(reply);
    RECOGNITION recog = reply->request().attribute(static_cast<QNetworkRequest::Attribute>(QNetworkRequest::User + 1)).value<RECOGNITION>();
    QByteArray response = reply->readAll();
    // qDebug().noquote() << "onReqTencentTmt reply -> Content:" << response;
    if (response.isEmpty()) {
        qWarning() << "reply->readAll() is empty!  retrn;!!!";
        return;
    }

    if (dataHead.at(0) == "200") {
        json j;
        try { j = json::parse(response.toStdString());
        } catch (const std::exception& e) {
            qDebug() << "Failed to parse JSON:" << e.what(); return;
        }

        const QString& text = QString::fromStdString(j.dump());    // qDebug().noquote() << "------>text:" << text;
        const bool& bValid = !j.empty() && j.contains("Response") && j["Response"].contains("ImageRecord")
                             && j["Response"]["ImageRecord"].contains("Value") && j["Response"]["ImageRecord"]["Value"].size() > 0;

        if (!bValid) {

            if (recog == RECOGNITION::R_extract_text) emit COMM.sigOcrFinished(text, "");
            else if (recog == RECOGNITION::R_img_trans) generateErrorImage(text);
            return;
        } else {

            // Load the original image
            QString imgPath = property("image_path").toString();
            QImage imageA(imgPath);  // 原始图片
            QImage imageB = imageA.copy(); // 复制一份作为 imageB

            // Prepare to draw rectangles and text
            QPainter painterA(&imageA);
            QPainter painterB(&imageB);
            painterA.setPen(QPen(Qt::green, 2));
            painterB.setPen(QPen(Qt::black, 2));

            QFont font = painterB.font();
            font.setPointSize(12); // Adjust font size as needed
            painterB.setFont(font);
            painterB.setBrush(Qt::white); // No filling for rectangle

            QString ret;
            const int range = config::extract_text::baseline_range;
            int lastBottom = -1;

            for (const auto& it : j["Response"]["ImageRecord"]["Value"]) {
                std::string sourceText = it["SourceText"];
                std::string targetText = it["TargetText"];
                QString textSource = QString::fromStdString(sourceText);
                QString textTarget = QString::fromStdString(targetText);

                // Coordinates for rectangle
                int left = it["X"];
                int top = it["Y"];
                int right = left + it["W"].get<int>();
                int bottom = top + it["H"].get<int>();

                // Draw green rectangle on image A
                QRect rect(left, top, right - left, bottom - top);
                // qDebug() << "rect:" << rect;

                painterA.drawRect(rect);

                // Draw green rectangle and target text on image B
                painterB.setPen(Qt::NoPen);
                painterB.setBrush(Qt::white);
                painterB.drawRect(rect);
                painterB.setPen(QPen(Qt::black, 2));
                painterB.setBrush(Qt::NoBrush);

                // 调整字体大小以适应矩形
                QFont adjustedFont = Helper::adjustFontSizeToFitRect(painterB, textTarget, rect);
                painterB.setFont(adjustedFont);
                painterB.drawText(rect, Qt::AlignCenter, textTarget);

                // Handling text arrangement (ret for debugging/logging purposes)
                if (qAbs(bottom - lastBottom) <= range) {
                    ret += (textSource + "\t");
                } else {
                    ret += ("\n" + textSource + "\t");
                }

                lastBottom = bottom;
            }

            painterA.end();
            painterB.end();

            // Save the images
            QString image_process_path = imgPath.left(imgPath.lastIndexOf(".")) + "_process.png";
            QString image_translate_path = imgPath.left(imgPath.lastIndexOf(".")) + "_translate.png";
            qDebug() << "image_process_path:" << image_process_path << "image_translate_path:" << image_translate_path;
            imageA.save(image_process_path);
            imageB.save(image_translate_path);

            setProperty("image_process_path", image_process_path);
            setProperty("image_translate_path", image_translate_path);

            if (recog == RECOGNITION::R_extract_text)
                emit COMM.sigOcrFinished(ret, image_process_path);
            else if (recog == RECOGNITION::R_img_trans)
                emit COMM.sigImgTransFinshed(image_translate_path);

        }
    } else {
        // qWarning() << replyErrorShowText(dataHead);
        generateErrorImage(replyErrorShowText(dataHead));
    }
}

void NetTencent::getBaiduTokens(const RECOGNITION &recognition)
{
    // Baidu 账号 → 加密后
    // [credential sample redacted]
    // [credential sample redacted]

    std::string id;
    std::string key;

    using namespace config;

    if (recognition == RECOGNITION::R_extract_text) {
        id = extract_text::baidu_cloud_id.toStdString();
        key = extract_text::baidu_cloud_key.toStdString();

    } else if (recognition == RECOGNITION::R_img_trans) {
        id = image_translate::baidu_cloud_id.toStdString();
        key = image_translate::baidu_cloud_key.toStdString();
    }

    // 使用默认的密码
    if (id.empty()) id = "XAhwYsZUiblM77X1f6vXSuE3vmF1PJEKenWh1IbzG+g=";
    if (key.empty()) key = "nZCNORegGDl8D9xAq9oVjDTP2VHGmiNCwm+6b2ym2XQ=";

    QString client_id = decryptString(id);
    QString client_secret = decryptString(key);

    if (!validityBaiDuKey(client_id, client_secret, recognition)) return;

    QNetworkRequest request(QUrl("https://openapi.baidu.com/oauth/2.0/token"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QUrlQuery params;
    params.addQueryItem("client_id", client_id);
    params.addQueryItem("client_secret", client_secret);
    params.addQueryItem("grant_type", "client_credentials");

    // 使用 QNetworkAccessManager 进行网络请求
    QNetworkAccessManager manager;
    QNetworkReply *reply = manager.post(request, params.query().toUtf8());

    connect(reply, &QNetworkReply::errorOccurred, this, [reply](QNetworkReply::NetworkError code) {
        const QMetaObject &metaObject = QNetworkReply::staticMetaObject;
        QMetaEnum metaEnum = metaObject.enumerator(metaObject.indexOfEnumerator("NetworkError"));
        QString errorMessage = QString("get Baidu token failed!\n\nError code: %1\nReason: %2\ndescribe: %3").arg(static_cast<int>(code)).arg(reply->errorString()).arg(QString::fromLocal8Bit(metaEnum.valueToKey(code)));
        QMessageBox::critical(nullptr, tr("NETWORK ERROR"), errorMessage);
    });

    // 创建事件循环，等待网络响应完成
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec(); // 等待直到请求完成

    const QStringList& dataHead = rawHeader(reply);
    QByteArray response = reply->readAll();
    //    qDebug().noquote() << " dealBaiDuAccessToken reply -> Content:" << response;
    if (response.isEmpty()) {
        qWarning() << "reply->readAll() is empty!  retrn;!!!";
        return;
    }

    if (dataHead.at(0) == "200") {
        json j;
        try {
            j = json::parse(response.toStdString());
        } catch (const std::exception& e) {
            qDebug() << "Failed to parse JSON:" << e.what();
            return;
        }

        if (!j.empty() && j.contains("access_token")) {
            QString baiDuToken = QString::fromStdString(j["access_token"]);
            setProperty("baidu_get_token", baiDuToken);
            // qDebug() << "baidu_get_token:" << baiDuToken;
        }
    } else {
        // qWarning() << replyErrorShowText(dataHead);
        if (recognition == RECOGNITION::R_extract_text) emit COMM.sigOcrFinished(replyErrorShowText(dataHead) + + "\n\n" + QString(response), "");
        else if (recognition == RECOGNITION::R_img_trans) generateErrorImage(replyErrorShowText(dataHead) + "\n\n" + QString(response));
    }

    reply->deleteLater();
}

QNetworkReply* NetTencent::reqBaiduCloudOcr(const QString &path, const RECOGNITION& recognition)
{
    if (property("baidu_get_token").toString().isEmpty()) getBaiduTokens(recognition); // 若是为空，则执行一次
    const QString token = property("baidu_get_token").toString();
    if (token.isEmpty()) return nullptr;

    // 从文件中读取图像数据并进行base64编码
    BaiduOcrChannel channel = property("baidu_ocr_ohannel").value<BaiduOcrChannel>(); // 没有数值存储进去，恰好缺省数值是 0; // const QVariant valueDef = QVariant::fromValue<BaiduOcrChannel>(BaiduOcrChannel::BOC_high_precision_location);
    QString para1 = "accurate";
    if (channel == BaiduOcrChannel::BOC_high_precision_location) para1 = "accurate";
    else if (channel == BaiduOcrChannel::BOC_standard_location) para1 = "general";
    else if (channel == BaiduOcrChannel::BOC_high_precision) para1 = "accurate_basic";
    else if (channel == BaiduOcrChannel::BOC_standard) para1 = "general_basic";

    QUrl url("https://aip.baidubce.com/rest/2.0/ocr/v1/" + para1 + "?access_token=" + token);
    QNetworkRequest request(url);
    QByteArray postData = "image=" + QUrl::toPercentEncoding(base64FromFileContent(path)) + "&detect_direction=false&vertexes_location=false&paragraph=false&probability=false";
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setRawHeader("Accept", "application/json");
    request.setAttribute(QNetworkRequest::User, QVariant::fromValue(TYPE::T_baidu_cloud_ocr));
    request.setAttribute(static_cast<QNetworkRequest::Attribute>(QNetworkRequest::User + 1), QVariant::fromValue<BaiduOcrChannel>(channel));   // 当前 发送的请求线路

    QNetworkReply *reply = m_netMgn->post(request, postData);
    QEventLoop loop;   // fix: 局部的事件循环,不卡主界面; 且修复了短时间连续请求 ocr，导致第二个无任何响应
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    return reply;
}

void NetTencent::onReqBaiduCloudOcr(QNetworkReply *reply)
{
    const QStringList& dataHead = rawHeader(reply);
    BaiduOcrChannel channel = reply->request().attribute(static_cast<QNetworkRequest::Attribute>(QNetworkRequest::User + 1)).value<BaiduOcrChannel>();
    qDebug() << "onReqBaiduCloudOcr channel type:" <<  (int)channel;
    QByteArray response = reply->readAll();
    // qDebug().noquote() << "onReqBaiduCloudOcr reply -> Content:" << response;
    if (response.isEmpty()) {
        qWarning() << "reply->readAll() is empty!  retrn;!!!";
        return;
    }

    if (dataHead.at(0) == "200") {
        json j;
        try { j = json::parse(response.toStdString());
        } catch (const std::exception& e) {
            qDebug() << "Failed to parse JSON:" << e.what(); return;
        }

        const QString& text = QString::fromStdString(j.dump());
        // qDebug().noquote() << "------>text:" << text;
        const bool& bValid = !j.empty() && j.contains("words_result") && j["words_result"].size() > 0;

        if (bValid) {
            setProperty("baidu_ocr_ohannel", QVariant::fromValue<BaiduOcrChannel>(channel));
            // 遍历 "words_result" 数组并将 "words" 按照 "location" 字段的坐标插入
            QString ret;
            // Load the original image
            const QString& imgPath = property("image_path").toString();
            QImage imageA(imgPath);
            QPainter painterA(&imageA);
            painterA.setPen(QPen(Qt::green, 2));

            for (const auto& item : j["words_result"]) {
                std::string words = item["words"];
                QString text = QString::fromStdString(words);

                const bool& withLocation = (channel == BaiduOcrChannel::BOC_high_precision_location || channel == BaiduOcrChannel::BOC_standard_location);
                if (withLocation) {
                    json location = item["location"];
                    int left = location["left"];
                    int top = location["top"];
                    int right = left + location["width"].get<int>();
                    int bottom = top + location["height"].get<int>();
                    static int lastBottom = bottom;
                    const int range = 5; //CJ_GET("advanced.customize_ui_parameters.ocr_bottom_align_rang").get<int>();
                    if (qAbs(bottom - lastBottom) <= range) ret += (text + "\t");
                    else ret += ("\n" + text + "\t");

                    lastBottom = bottom;
                    painterA.drawRect(QRect(left, top, right - left, bottom - top));
                } else {
                    ret += (text + "\n");
                }
            }

            QString image_process_path = imgPath.left(imgPath.lastIndexOf(".")) + "_process.png";
            imageA.save(image_process_path);
            qDebug() << "image_process_path:" << image_process_path;
            setProperty("image_process_path", image_process_path);
            emit COMM.sigOcrFinished(ret, image_process_path);

        } else {

            if (!j.empty() && j.contains("error_code")) {
                const int& error_code = j["error_code"];

                if (error_code == 17) {   // 17 每天请求量超限额
                    static int nCount = 0;
                    if (nCount < 4) {  // 所有的线路全部失效的时候结束，不再尝试
                        channel++;
                        setProperty("baidu_ocr_ohannel", QVariant::fromValue<BaiduOcrChannel>(channel));
                        const QString& path = property("image_path").toString();
                        if (!path.isEmpty()) reqBaiduCloudOcr(path, RECOGNITION::R_extract_text);
                        nCount++;
                    } else {
                        nCount = 0;
                        qDebug() << "All baidu ocr channel is failed! Open api daily request limit reached.";
                    }

                    return;
                } else {
                    emit COMM.sigOcrFinished(text, "");
                    return;
                }
            }
        }

        // emit COMM.sigOCRTextGenerateFinsh(response, channel);
    } else {
        qWarning() << replyErrorShowText(dataHead);
    }
}

QNetworkReply* NetTencent::reqBaiduCloudPixTrans(const QString &path, const RECOGNITION &recognition)
{
    if (property("baidu_get_token").toString().isEmpty()) getBaiduTokens(recognition); // 若是为空，则执行一次
    const QString token = property("baidu_get_token").toString();
    if (token.isEmpty()) return nullptr;

    QUrl url("https://aip.baidubce.com/file/2.0/mt/pictrans/v1");
    QUrlQuery query;
    query.addQueryItem("access_token", token);
    url.setQuery(query);

    QHttpMultiPart* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QHttpPart imagePart;
    imagePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/png"));
    imagePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"image\"; filename=\"" + QFileInfo(path).fileName() + "\";"));

    QFile *file = new QFile(path);
    file->open(QIODevice::ReadOnly);
    imagePart.setBodyDevice(file);
    file->setParent(multiPart);
    multiPart->append(imagePart);

    // Convert query parameters to QHttpPart and append to multiPart
    QList<QPair<QString, QString>> queryParams = {
        {"from", config::image_translate::baidu_cloud_souce_language},
        {"to", config::image_translate::baidu_cloud_target_language},
        {"v", "3"},
        {"paste", "1"}
    };

    foreach (auto param, queryParams) {
        QHttpPart paramPart;
        paramPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"" + param.first + "\""));
        paramPart.setBody(param.second.toUtf8());
        multiPart->append(paramPart);
    }

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::User, QVariant::fromValue(TYPE::T_baidu_cloud_pix_trans));

    return m_netMgn->post(request, multiPart);
}

// Ref: https://ai.baidu.com/ai-doc/MT/mki483xpu
void NetTencent::onReqBaiduCloudPixTrans(QNetworkReply *reply)
{
    const QStringList& dataHead = rawHeader(reply);
    QByteArray response = reply->readAll();
    // qDebug().noquote() << " dealBaiDuImgTranslateRequest reply -> Content:" << response;
    if (response.isEmpty()) {
        qWarning()<<"reply->readAll() is empty!  retrn;!!!";
        return;
    }

    // 解析 JSON 数据
    json j;
    try {
        j = json::parse(response.toStdString());
    } catch (const std::exception& e) {
        qDebug() << "Failed to parse JSON:" << e.what();
        return;
    }

    if (dataHead.at(0) == "200") {
        if (j.contains("error_code") && j["error_code"] == "0" && j.contains("data") && j.contains("data") && j["data"].contains("pasteImg")) {
            QString render_image = QString::fromStdString(j["data"]["pasteImg"].dump());
            QByteArray byteArray = QByteArray::fromBase64(render_image.toUtf8());

            // 将图像数据加载到QImage
            QImage image;
            if (image.loadFromData(byteArray)) {
                const QString dir = QStandardPaths::standardLocations(QStandardPaths::CacheLocation).first() + "/ocr/";
                QDir directory(dir);
                if (!directory.exists() && !directory.mkpath(dir)) {
                    qDebug() << "Failed to create directory: " << dir;
                }

                const auto& path = dir + QString("%1_baidu_%2.png").arg(XPROJECT_NAME).arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
                image.save(path);

                emit COMM.sigImgTransFinshed(path);
                qDebug() << "OCR image size:" << image.size() << "path:" << path;
            } else {
                qDebug() << "Failed to load image.";
            }

            //            // 反向 URL 解码数据以查看 +
            //            QString decodedData = QUrl::fromPercentEncoding(responseData);
            //            qDebug() << "decodedData:" << decodedData;

        } else {
            // emit COMM.sigImgTransFinshed(QString::fromStdString(j.dump()));   // 返回具体的错误码
            generateErrorImage(QString::fromStdString(j.dump()));
        }

    } else {
        // qDebug() << replyErrorShowText(dataHead);
        generateErrorImage(replyErrorShowText(dataHead));
    }
}

QNetworkReply *NetTencent::reqYoudaoCloudPixTrans(const QString &path)
{
    using namespace config;
    // 构造POST请求
    QNetworkRequest request(QUrl("https://openapi.youdao.com/ocrtransapi"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setAttribute(QNetworkRequest::User, QVariant::fromValue(TYPE::T_youdao_cloud_pix_trans));

    QUrlQuery postData;
    QString base64DataUtf8 = base64FromFileContent(path);
    postData.addQueryItem("q", QUrl::toPercentEncoding(base64DataUtf8)); // fix 【重要】: 是 Qt 框架中用于将字符串转换为百分号编码（percent encoding）格式的函数。这个编码方式也称为 URL 编码或 URI 编码。它将字符串中的特殊字符以 % 符号后跟两位十六进制数字的形式进行编码，
    postData.addQueryItem("from", image_translate::youdao_cloud_souce_language);
    postData.addQueryItem("to", image_translate::youdao_cloud_target_language);
    postData.addQueryItem("render", "1");  // 是否需要服务端返回渲染的图片，0-否； 1-是
    postData.addQueryItem("type", "1");

    // Youdao 账号 → 加密后
    // [credential sample redacted]
    // [credential sample redacted]

    std::string id = image_translate::youdao_cloud_id.toStdString();
    std::string key = image_translate::youdao_cloud_key.toStdString();

    // 使用默认的密码
    if (id.empty()) id = "GiryxLDmyrq2Xlmi/yfyGA==";
    if (key.empty()) key = "Q+MTCKjz+I9Smg/BJtBI720FkaxIQ4ZRIK1VaQleP14=";

    QString APP_KEY = decryptString(id);
    QString APP_SECRET = decryptString(key);

    // 创建 QMap 并添加参数
    QMap<QString, QString> params;
    params["appKey"] = APP_KEY;
    params["salt"] = QUuid::createUuid().toString();
    params["curtime"] = QString::number(QDateTime::currentSecsSinceEpoch());
    params["signType"] = "v3";
    params["q"] = base64DataUtf8;

    // 添加身份验证参数
    AuthV3Util::addAuthParams(APP_KEY, APP_SECRET, params);

    for (const QString &key : params.keys()) {
        if (key != "q")
            postData.addQueryItem(key, params[key]);

        // static int i = 1;
        // qDebug().noquote() << "i:" << i++ << "key:" << key << params[key];
    }

    QNetworkReply *reply = m_netMgn->post(request, postData.toString(QUrl::FullyEncoded).toUtf8());
    // QEventLoop loop;
    // connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    // loop.exec();
    // qDebug() << "【发送时候的 c】 sendYouDaoImgTranslateRequest reply:" << reply;
    return reply;
}

void NetTencent::onReqYoudaoCloudPixTrans(QNetworkReply *reply)
{
    const QStringList& dataHead = rawHeader(reply);
    QByteArray response = reply->readAll();
    //    qDebug().noquote() << " dealYouDaoImgTranslateRequest reply -> Content:" << response;
    if (response.isEmpty()) {
        qWarning()<<"reply->readAll() is empty!  retrn;!!!";
        return;
    }

    // 解析 JSON 数据
    json j;
    try {
        j = json::parse(response.toStdString());
    } catch (const std::exception& e) {
        qDebug() << "Failed to parse JSON:" << e.what();
        return;
    }

    if (dataHead.at(0) == "200") {
        if (j.contains("errorCode") && j["errorCode"] == "0" && j.contains("render_image") && j.contains("image_size")) {
            QString render_image = QString::fromStdString(j["render_image"].dump());
            QString image_size = QString::fromStdString(j["image_size"].dump());
            QString tImageSize = image_size.mid(1, image_size.size() - 2);
            const auto& lists = tImageSize.split(',');

            QSize size(-1, -1);
            if (lists.count() == 2) {
                size.setWidth(lists.at(1).toInt());
                size.setHeight(lists.at(0).toInt());
            }

            QByteArray byteArray = QByteArray::fromBase64(render_image.toUtf8());

            // 将图像数据加载到QImage
            QImage image;
            if (image.loadFromData(byteArray)) {
                const QString dir = QStandardPaths::standardLocations(QStandardPaths::CacheLocation).first() + "/ocr/";
                QDir directory(dir);
                if (!directory.exists() && !directory.mkpath(dir)) {
                    qDebug() << "Failed to create directory: " << dir;
                }

                const auto& path = dir + QString("%1_youdao_%2.png").arg(XPROJECT_NAME).arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
                image.save(path);

                emit COMM.sigImgTransFinshed(path);
                qDebug() << "path:" << path;
            } else {
                qDebug() << "Failed to load image.";
            }

            //            // 反向 URL 解码数据以查看 +
            //            QString decodedData = QUrl::fromPercentEncoding(responseData);
            //            qDebug() << "decodedData:" << decodedData;

        } else {
            qDebug() << "QString::fromStdString(j.dump()):" << QString::fromStdString(j.dump());
            // emit COMM.sigOCRImageGenerateFinsh(QSize(), QString::fromStdString(j.dump()));   // 返回具体的错误码
            generateErrorImage(QString::fromStdString(j.dump()));
        }

    } else {
        // qWarning() << replyErrorShowText(dataHead);
        // emit COMM.sigImgTransFinshed(replyErrorShowText(dataHead));
        generateErrorImage(replyErrorShowText(dataHead));
    }
}

const QStringList NetTencent::rawHeader(const QNetworkReply *reply) const
{
    QStringList list;
    if (!reply)
        return list;

    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QString reasonPhrase = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();

    list << QString::number(statusCode)  // Status Code
         << reasonPhrase;                // Reason Phrase

#if 0
    qDebug() << "Raw Header****:\n" << reply->rawHeaderList() << "\n";

    QList<QByteArray> headerList = reply->rawHeaderList();
    for (const QByteArray& header : headerList)
        qDebug() << header << ": " << reply->rawHeader(header);

#endif

    return list;
}

const QString NetTencent::replyErrorShowText(const QStringList &dataHead)
{
    return tr("Status Code: ") + dataHead.at(0) + tr("\nReason Phrase: ") + dataHead.at(1);
}

QString NetTencent::base64FromFileContent(const QString &path, bool urlencoded)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return "";

    QByteArray fileData = file.readAll();
    file.close();
    QString base64Content = fileData.toBase64();

    //    qDebug().noquote() << "fileData:" << fileData << Qt::endl << Qt::endl;
    //    qDebug().noquote() << "base64Content:" << base64Content;

    if (urlencoded)
        base64Content = QUrl(base64Content).toEncoded();

    return base64Content;
}

void NetTencent::generateErrorImage(const QString &errorMessage)
{
    // 创建一个 QImage
    QString imgPath = property("image_path").toString();
    QImage image(imgPath);  // 原始图片
    image.fill(Qt::white); // 设置背景色为白色

    // 创建 QPainter 来绘制文本
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::red); // 设置文本颜色为红色

    // 创建文本选项，允许自动换行
    QTextOption textOption;
    textOption.setWrapMode(QTextOption::WordWrap); // 启用自动换行
    textOption.setAlignment(Qt::AlignCenter); // 设置文本居中

    // 计算文本的绘制区域
    QRect textRect = image.rect(); // 使用整个图像区域
    painter.drawText(textRect, errorMessage, textOption);

    // 保存图像到指定路径
    const QString dir = QStandardPaths::standardLocations(QStandardPaths::CacheLocation).first() + "/ocr/";
    QDir directory(dir);
    if (!directory.exists() && !directory.mkpath(dir)) {
        qDebug() << "Failed to create directory: " << dir;
        return;
    }

    const QString path = dir + QString("error_%1.png").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
    if (image.save(path)) {
        emit COMM.sigImgTransFinshed(path);
    } else {
        qDebug() << "Failed to save error image.";
    }
}



const bool NetTencent::validityBaiDuKey(const QString &id, const QString &secret, const RECOGNITION &recognition)
{
    if (id.isEmpty() || secret.isEmpty()) {
        const QString title(tr("Warning"));
        const QString msg(tr("Your Baidu Yun API Secret [ID Or Key] is empty, please check and re-enter it!"));

        // qDebug() << title + msg;
        // QMessageBox::warning(nullptr, title, msg); // 暂时屏蔽
        if (recognition == RECOGNITION::R_extract_text) emit COMM.sigOcrFinished(title + msg, property("image_path").toString());
        else if (recognition == RECOGNITION::R_img_trans) generateErrorImage(title + msg);

        return false;
    } else {
        return true;
    }
}

void NetTencent::onRequestFinished(QNetworkReply *reply)
{
    const auto& head = rawHeader(reply);
    // if (head.count() >= 2) qDebug() << "head:" << head << "  head.at(0):" << head.at(0) << "  head.at(1):" << head.at(1);

    if (reply->error() == QNetworkReply::NoError) {
        const TYPE& type = reply->request().attribute(QNetworkRequest::User).value<TYPE>();
        qDebug() << "onRequestFinished type:" << QMetaEnum::fromType<NetTencent::TYPE>().valueToKey(int(type)) << (int)type;

        if (type == TYPE::T_tencent_cloud_pix_trans) {
            onReqTencentTmt(reply);
        } else if (type == TYPE::T_baidu_cloud_ocr) {
            onReqBaiduCloudOcr(reply);
        } else if (type == TYPE::T_baidu_cloud_pix_trans) {
            onReqBaiduCloudPixTrans(reply);
        } else if (type == TYPE::T_youdao_cloud_pix_trans) {
            onReqYoudaoCloudPixTrans(reply);
        } else {
            qDebug() << "***************** TYPE::RT_empty not match resp type! If you forget to set request->setAttribute(QNetworkRequest::User, nType);, it will look like this*****************";
        }

    } else {
        QStringList list;
        list << head.at(0) << reply->errorString();  // reply->errorString() 比 head.at(1) 更详细
    }

    if (reply)
        reply->deleteLater();  // 此处统一释放
}
