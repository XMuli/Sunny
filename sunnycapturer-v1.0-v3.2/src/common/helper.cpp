// SPDX-SnippetCopyrightText: 2024-2025 SunnyCapturer Project
// SPDX-Author: Wengeng Zhang <xmulitech@gmail.com>

#include "helper.h"
#include "logging.h"
#include "config.h"

#include <QDateTime>
#include <QFileInfo>
#include <QGuiApplication>
#include <QDir>
#include <QUrl>

namespace Helper {

    QColor highlightColor(const double& alphaf)
    {
        QColor color = config::snip::style.border_color; // QColor("#0081FF");
        color.setAlphaF(alphaf);
        return color;
    }

    QString formatToFileName(const QString &name)
    {
        auto fileName = name.trimmed();
        auto first = fileName.indexOf("$");
        auto second = fileName.lastIndexOf("$");
        auto nameTime = fileName.mid(first + 1, second - first - 1);
        auto finalyName = fileName.replace(first, second - first + 1, QDateTime::currentDateTime().toString(nameTime));

        return finalyName;
    }

    QFont adjustFontSizeToFitRect(QPainter &pa, const QString &text, const QRect &rt, int maxFontSize)
    {
        QFont font = pa.font();
        int fontSize = 1;

        while (fontSize < maxFontSize) {
            font.setPointSize(fontSize);
            pa.setFont(font);

            QFontMetrics fm(font);
            QRect textRect = fm.boundingRect(rt, Qt::AlignCenter, text);
            const int margin = 0;

            if (textRect.width() >= rt.width() - margin || textRect.height() >= rt.height() - margin) {
                // qDebug() << "textRect:" << textRect << fontSize << font;
                break;
            }

            fontSize++;
        }

        return font;
    }

    bool isSupportPaddleOCR()
    {
        #if defined(Q_OS_WIN)
            QString dllFile = qGuiApp->applicationDirPath() + "/sunny_ocr.dll";
            QFileInfo info(dllFile);
            bool ret1 = info.exists() && info.isFile();
            return ret1;
        #else
            return false;
        #endif
    }

    QScreen* currScrn(const QPoint &pt)
    {
        QScreen *scrn = qGuiApp->screenAt(pt);
        #if defined(Q_OS_MACOS) // In macos, mouse position at the bottom or right edge of the screen will crash
            const auto virRt = qGuiApp->primaryScreen()->virtualGeometry();
            if (!scrn && (pos.x() >= virRt.right() || pos.y() >= virRt.bottom()))
                scrn = qGuiApp->screenAt(virRt.bottomRight() - QPoint(1, 1));
        #endif

        if (!scrn) scrn = qGuiApp->primaryScreen();
        return scrn;
    }

    double dpiScale(const QScreen *scrn)
    {
        double scale = 1;
        if (!scrn) return scale;

        #if defined(Q_OS_WIN) ||  defined(Q_OS_LINUX)
            scale = scrn->logicalDotsPerInch() / 96.0;
                return scale;
        #elif  defined(Q_OS_MAC)
            scale = screen->logicalDotsPerInch() / 72.0;
            return scale;
        #endif
    }

    QString isLanguageAvailable(const QString &language)
    {
        #if defined(Q_OS_WIN)
            QString directoryPath = qGuiApp->applicationDirPath() + "/translations/";
        #elif defined(Q_OS_LINUX)

            bool appimage = Helper::isAppImagePackage();
            QString directoryPath = QString("/opt/%1/translations/").arg(XPROJECT_NAME);

            // "/tmp/.mount_SunnyC1cMm2M/usr/bin/../../opt/SunnyCapturer/translations/"
            if (appimage) directoryPath = qGuiApp->applicationDirPath() + QString("/../../opt/%1/translations/").arg(XPROJECT_NAME);
            qDebug() << "Linux directoryPath:" << directoryPath << "appimage:" << appimage;
        #endif

        QDir dir(directoryPath);
        if (!dir.exists()) {
            const QString& ret = directoryPath + "en_us";
            logi("dir is not exists: {}, retrun: {}", directoryPath.toStdString(), ret.toStdString());
            return ret;
        }

        // get *.qm
        QStringList filters = {"*.qm"};
        QStringList fileList = dir.entryList(filters, QDir::Files);

        QStringList baseNames;
        for (const QString& file : fileList)
            baseNames << QFileInfo(file).baseName();

        // check language contains list
        bool isAvailable = baseNames.contains(language);
        auto path = isAvailable ? directoryPath + language : directoryPath + "en_us";
        logi("isAvailable:{} Language:{} path:{}", isAvailable, language.toStdString(), path.toStdString());
        return path;
    }

    bool isAppImagePackage()
    {
        #if defined(Q_OS_LINUX)
            QString appDirPath = qGuiApp->applicationDirPath();   // AppImage 运行模式,挂在运行
            const bool ret = appDirPath.startsWith("/tmp/.mount_") ? true : false;
            logi("linux is AppImagePackage:{}  appDirPath:{}", ret ? "true" : "false", appDirPath.toStdString());
            return ret;
        #else
            return false;
        #endif
    }

    LONG ApplicationCrashHandler(EXCEPTION_POINTERS *pException)
    {
        const QString& createDir = QString("%1/%2_Dumps")
        .arg(QStandardPaths::standardLocations(QStandardPaths::CacheLocation).first())
            .arg(XPROJECT_NAME);
        QDir dir;
        dir.mkpath(createDir);
        const QString& createPath = QString("%1/dump_%2.dmp").arg(createDir).arg(QDateTime::currentDateTime().toString("yyyy_MM_dd_hh_mm_ss_zzz"));
        qCritical() << "Trigger an exception! createPath:" << createPath;

        std::wstring wlpstr = createPath.toStdWString();
        LPCWSTR lpcwStr = wlpstr.c_str();

        HANDLE hDumpFile = CreateFile(lpcwStr,
                                            GENERIC_WRITE,
                                            0,
                                            NULL,
                                            CREATE_ALWAYS,
                                            FILE_ATTRIBUTE_NORMAL,
                                            NULL);

        if (hDumpFile != INVALID_HANDLE_VALUE){
            //Dump信息
            MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
            dumpInfo.ExceptionPointers = pException;
            dumpInfo.ThreadId = GetCurrentThreadId();
            dumpInfo.ClientPointers = FALSE;
            //写入Dump文件内容
            MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, MiniDumpNormal, &dumpInfo, NULL, NULL);
        }

               // 这里弹出一个错误对话框并退出程序
        EXCEPTION_RECORD* record = pException->ExceptionRecord;
        QString errCode(QString::number(record->ExceptionCode,16)),errAdr(QString::number((UINT)((UINT_PTR)record->ExceptionAddress),16));;
        QString errstr;
        GetExceptionDescription(record->ExceptionCode,errstr);
        if(record->NumberParameters>0){
            if(record->ExceptionInformation[0]==0){
                errstr+="\n访问冲突,线程试图读取不可访问的数据";
            }else if(record->ExceptionInformation[0]==1){
                errstr+="\n访问冲突,线程尝试写入不可访问的地址";
            }
        }

        QString msg = QString("Run Crash! \n\nError code: %1 \nError address: %2 \nDescribe: %3 \n\nDump path: %4")
                          .arg(errCode).arg(errAdr).arg(errstr).arg(createPath);
        QMessageBox::StandardButton ret = QMessageBox::critical(nullptr, QObject::tr("CRASH"), msg, QMessageBox::Open | QMessageBox::Close);

        if (ret == QMessageBox::Open && dir.exists()) QDesktopServices::openUrl(QUrl::fromLocalFile(createDir));
        return EXCEPTION_EXECUTE_HANDLER;
    }

    void GetExceptionDescription(DWORD errCode, QString &err)
    {
        //    errCode = 0xc0000005;
        LPTSTR lpMsgBuf = NULL;
        HMODULE Hand = LoadLibrary(TEXT("ntdll.dll"));
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS/*FORMAT_MESSAGE_FROM_SYSTEM*/| FORMAT_MESSAGE_FROM_HMODULE,
                      Hand,
                      errCode,
                      MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
                      (LPTSTR)&lpMsgBuf,
                      0,NULL);
        err = QString::fromWCharArray( lpMsgBuf );
        qDebug() << "err:" << err;
        LocalFree(lpMsgBuf);
    }

}
