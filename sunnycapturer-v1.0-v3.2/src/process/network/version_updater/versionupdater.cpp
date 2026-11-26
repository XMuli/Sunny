// SPDX-SnippetCopyrightText: 2024-2025 SunnyCapturer Project
// SPDX-Author: Wengeng Zhang <xmulitech@gmail.com>
#include "versionupdater.h"
#include <QNetworkRequest>
#include <QMessageBox>
#include <QRadioButton>
#include <QUrl>
#include <QFileInfo>
#include <QMetaEnum>
#include <QDir>
#include <QProcess>
#include <QThread>
#include <QDesktopServices>
#include <QCoreApplication>
#include "logging.h"
#include "config.h"
#include "communication.h"
#include "logging.h"

VersionUpdater::VersionUpdater(const QString &localVersion, QObject *parent)
    : QObject(parent)
    , m_localVersion(localVersion)
    , m_manager(new QNetworkAccessManager(this))
    , m_redirect(true)
    , m_updateUI(new UpdateUI(nullptr))
{
    m_updateUI->hide();
    connect(m_updateUI, &UpdateUI::sigRecheked, this, [this](){ checkForUpdate(false); });
    connect(m_manager, &QNetworkAccessManager::finished, this, &VersionUpdater::onFinished);
}

VersionUpdater::~VersionUpdater()
{
    if (m_updateUI)
        m_updateUI->deleteLater();
}

void VersionUpdater::checkForUpdate(const bool noUpdateNotShow)
{
    QUrl url("https://api.github.com/repos/XMuli/SunnyCapturer/releases");

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::User, int(RESP_TYPE::RT_check_update));
    request.setAttribute(static_cast<QNetworkRequest::Attribute>(QNetworkRequest::User + 1), noUpdateNotShow);
    QNetworkReply* reply = m_manager->get(request);
    config::update::last_check_time = QDateTime::currentDateTime().toString(Qt::ISODate);

    connect(reply, &QNetworkReply::errorOccurred, this, [reply](QNetworkReply::NetworkError code) {
        QString errorMessage = QString("Error code: %1, Reason: %2").arg(static_cast<int>(code)).arg(reply->errorString());
        emit COMM.sigShowSystemMessagebox(tr("Check For Update"), tr("Network error occurred: %1").arg(errorMessage));
        if (reply) reply->deleteLater();
    });
}

void VersionUpdater::testUrlConnectivity(const QStringList &urls)
{
    foreach (const QString &url, urls) {
        QNetworkRequest request((QUrl(url)));
        request.setAttribute(QNetworkRequest::User, int(RESP_TYPE::RT_test_url));
        QNetworkReply *reply = m_manager->get(request);
        connect(reply, &QNetworkReply::errorOccurred, this, [reply](QNetworkReply::NetworkError code) {
            QString errorMessage = QString("Error code: %1, Reason: %2").arg(static_cast<int>(code)).arg(reply->errorString());
            emit COMM.sigShowSystemMessagebox(tr("Network URL Test"), tr("Network error occurred: %1").arg(errorMessage));
            if (reply) reply->deleteLater();
        });
    }
}

void VersionUpdater::downLatestVersion(QString url)
{
    if (url.isEmpty()) {
        logi("Download URL is empty.");
        return;
    }

    QUrl qurl(url);
    QNetworkRequest request(qurl);
    request.setAttribute(QNetworkRequest::User, int(RESP_TYPE::RT_download_latest));
    setProperty("down_file_name", QFileInfo(qurl.path()).fileName());
    // logd("down_file_name: {}", url.toStdString());

    if (m_redirect) { // 是 GitHub 会重定向的链接

        QNetworkReply* reply = m_manager->get(request);
        connect(reply, &QNetworkReply::finished, this, [this, reply](){
            QUrl redirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();

            if (!redirectUrl.isEmpty()) {
                m_redirect = false;
                QNetworkRequest newRequest(redirectUrl);
                newRequest.setAttribute(QNetworkRequest::User, int(RESP_TYPE::RT_download_latest));
                QNetworkReply* newReply = m_manager->get(newRequest);

                m_tempFile.open();
                connect(newReply, &QNetworkReply::downloadProgress, this, &VersionUpdater::sigDownloadProgress);
                connect(newReply, &QNetworkReply::readyRead, this, [this, newReply]() {
                    if (m_tempFile.isOpen()) m_tempFile.write(newReply->readAll());});   // 逐块读取数据并写入临时文件
            }
        });

    }
}

void VersionUpdater::setProxy(ProxyType proxyType, const QString &host, int port)
{
    QNetworkProxy proxy;
    switch (proxyType) {
    case NoProxy:
        proxy.setType(QNetworkProxy::NoProxy);
        break;
    case SystemProxy:
        proxy.setType(QNetworkProxy::DefaultProxy);
        break;
    case HttpProxy:
        proxy.setType(QNetworkProxy::HttpProxy);
        proxy.setHostName(host);
        proxy.setPort(port);
        break;
    case Socks5Proxy:
        proxy.setType(QNetworkProxy::Socks5Proxy);
        proxy.setHostName(host);
        proxy.setPort(port);
        break;
    }
    m_manager->setProxy(proxy);
}


void VersionUpdater::dealCheckForUpdate(QNetworkReply *reply)
{
    json j;
    try {
        j = json::parse(reply->readAll().toStdString());
    } catch (json::parse_error& e) {
        loge("JSON Parsing failure: {}", e.what());
        return;
    }
    if (j.empty()) {
        return;
    } else if (!j.at(0).contains("prerelease")) {
        QMessageBox::information(nullptr, tr("Request Limit"), QString::fromStdString(j.dump(4)));
        return;
    }

    // logi("dealCheckForUpdate json:{}", j.dump(4));

    QString latestVersion;
    QString latestVersionTime;
    json retJ;
    bool isPrerelease = false;
    for (const auto &it : j) {

        QString currentTagName = QString::fromStdString(it["tag_name"].get<std::string>());               // 获取当前版本的 tag_name
        if (currentTagName.startsWith("v", Qt::CaseInsensitive)) currentTagName = currentTagName.mid(1);  // 去掉开头的 'v' 字符并去除两边的空格
        currentTagName = currentTagName.trimmed();

        isPrerelease = it["prerelease"].get<bool>();
        if (!config::update::join_inside && isPrerelease)                                 // 检查是否跳过 prerelease
            continue;

        // 如果 latestTagName 为空，或者当前的 tag_name 大于 latestTagName，则更新 latestTagName
        if (latestVersion.isEmpty() || QVersionNumber::fromString(currentTagName) > QVersionNumber::fromString(latestVersion)) {
            currentTagName += (isPrerelease ?  "-beta" : "");
            latestVersion = currentTagName;
            latestVersionTime = QString::fromStdString(it["published_at"].dump(4)).mid(1).split('T')[0].replace("-", ".");
            retJ = it["assets"];
            break;
        }
    }

    std::vector<VersionData> vec;
    for (const auto &it : retJ) {
        VersionData data;
        data.name = QString::fromStdString(it["name"].get<std::string>());
        data.size = it["size"].get<int64_t>() / (1024 * 1024);  // MB
        data.time = QString::fromStdString(it["created_at"].get<std::string>());
        data.isPre = isPrerelease;

    #if defined(Q_OS_WIN)

        if (data.name.contains(".exe")/* || data.name.contains(".7z") || data.name.contains(".zip")*/) {
            data.url = QString::fromStdString(it["browser_download_url"].get<std::string>());
            vec.push_back(data);
        } else {
            continue;
        }

    #elif defined(Q_OS_MAC)
        if (data.name.contains(".dmg")) {
            data.url = QString::fromStdString(it["browser_download_url"].get<std::string>());
            vec.push_back(data);
        } else {
            continue;
        }

    #elif defined(Q_OS_LINUX)
        if (data.name.contains(".deb") || data.name.contains(".AppImage")) {
            data.url = QString::fromStdString(it["browser_download_url"].get<std::string>());
            vec.push_back(data);
        } else {
            continue;
        }

    #else

    #endif

    }

    m_updateUI->setSelectDownVersion(vec);

    if (j.at(0).contains("body")) {
        const QString& text = QString::fromStdString(j[0]["body"].get<std::string>());
        m_updateUI->setLatestVersionChanglog(text);
    }


    if (QVersionNumber::fromString(m_localVersion) < QVersionNumber::fromString(latestVersion)) {
        m_updateUI->setLatestVersionLabel(m_localVersion, XBUILD_TIME, latestVersion, latestVersionTime);
        m_updateUI->show();
        m_updateUI->adjustSize();
    } else {

        bool noUpdateNotShow = reply->request().attribute(static_cast<QNetworkRequest::Attribute>(QNetworkRequest::User + 1)).toBool();
        if (!noUpdateNotShow)
            emit COMM.sigShowSystemMessagebox(tr("No Update"), tr("You are already on the latest version."), 10 * 1000);
    }
}

void VersionUpdater::dealDownLatestVersion(QNetworkReply *reply)
{
    m_redirect = true;
    if (reply->error() != QNetworkReply::NoError) {
        logw("Download error: {}", reply->errorString().toStdString());
        m_tempFile.remove(); // 删除错误的文件
        m_tempFile.close(); // 关闭文件
        return;
    } else {
        m_tempFile.flush(); // 确保所有数据都写入文件
        m_tempFile.close(); // 关闭文件
    }

    QString tempFilePath = m_tempFile.fileName();  // 获取临时文件路径
    QFileInfo fileInfo(tempFilePath);
    QString directoryPath = fileInfo.path();  // 获取文件所在目录路径
    const QString name = property("down_file_name").toString();
    // QString finalFilePath = QDir::homePath() + QDir::separator() + name;
    QString finalFilePath = directoryPath + QDir::separator() + name;
    logi("Download completed. Temporary file saved to: {}\nFinal file path: {}", reply->errorString().toStdString(), tempFilePath.toStdString());

    QFile::remove(finalFilePath);  // 删除目标文件（如果存在）
    if (QFile::copy(tempFilePath, finalFilePath)) {
        logd("File copied successfully to: {}", finalFilePath.toStdString());
        m_tempFile.remove();  // 删除原来的临时文件

        // 弹出确认对话框
        QMessageBox::StandardButton reply = QMessageBox::question(nullptr, tr("Install new version"), tr("A new version [%1] is available. Do you want to install it now?").arg(name),
                                                                  QMessageBox::Yes | QMessageBox::No,
                                                                  QMessageBox::Yes);

        if (reply == QMessageBox::Yes) {
            if (QProcess::startDetached(finalFilePath))
                QCoreApplication::exit(0);
        }


    } else {
        logw("Failed to copy file to: {}", finalFilePath.toStdString());
    }
}

void VersionUpdater::dealTestUrlConnectivity(QNetworkReply *reply)
{
    bool success = (reply->error() == QNetworkReply::NoError);
    emit COMM.sigShowSystemMessagebox(tr("GitHub Network Test"), tr("Down url results: %1\n"
                                                             "(url: %2)")
                                                              .arg(success ? "success" : "failed")
                                                              .arg(reply->url().toString()));
}

void VersionUpdater::handleRedirect(QNetworkReply *reply)
{
    QUrl redirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
    logd("redirectUrl: {}", redirectUrl.toString().toStdString());
    if (!redirectUrl.isEmpty()) {
        QNetworkRequest newRequest(redirectUrl);
        newRequest.setAttribute(QNetworkRequest::User, int(RESP_TYPE::RT_download_latest));
        m_manager->get(newRequest);
    }
}

bool VersionUpdater::userAllowCheckUpdate()
{
    bool bAutoCheck = config::update::enable_auto_check;
    if (!bAutoCheck) return false;

    QString lastCheckTimeStr = config::update::last_check_time;
    if (lastCheckTimeStr.isEmpty()) return true;

    // 将 lastCheckTime 转换为 QDateTime 对象
    QDateTime lastCheckTime = QDateTime::fromString(lastCheckTimeStr, Qt::ISODate);
    if (!lastCheckTime.isValid()) return false;  // 确保上次检查时间是有效的
    QDateTime currentTime = QDateTime::currentDateTime();

    // 计算当前时间与上次检查时间的差值，判断是否大于等于设置的天数
    if (lastCheckTime.daysTo(currentTime) >= config::update::day)
        return true;

    return false;
}

void VersionUpdater::onFinished(QNetworkReply *reply)
{
    const RESP_TYPE& type = static_cast<RESP_TYPE>(reply->request().attribute(QNetworkRequest::User, int(RESP_TYPE::RT_empty)).toInt());
    const bool nonTest = static_cast<RESP_TYPE>(type) != RESP_TYPE::RT_test_url;
    if (reply->error() != QNetworkReply::NoError && !nonTest) return;

    QMetaEnum metaEnum = QMetaEnum::fromType<VersionUpdater::RESP_TYPE>();
    const auto& szType = metaEnum.valueToKey(int(type));            //! enum转string
    logd("type: {} {}", szType, (int)type);

    if (static_cast<RESP_TYPE>(type) == RESP_TYPE::RT_test_url) {
        dealTestUrlConnectivity(reply);
    } else if (static_cast<RESP_TYPE>(type) == RESP_TYPE::RT_check_update) {
        dealCheckForUpdate(reply);
    } else if (static_cast<RESP_TYPE>(type) == RESP_TYPE::RT_download_latest || static_cast<RESP_TYPE>(type) == RESP_TYPE::RT_download_insider) {

        if (!m_redirect)
            dealDownLatestVersion(reply);
    }

    // static int i = 1;
    // logd("deleteLater reply: {} {} {}", static_cast<void*>(reply), szType, i++);
    if (reply) reply->deleteLater();
}



