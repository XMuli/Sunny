// SPDX-SnippetCopyrightText: 2024-2025 SunnyCapturer Project
// SPDX-Author: Wengeng Zhang <xmulitech@gmail.com>

#ifndef HELPER_H
#define HELPER_H

#include <QFont>
#include <QRect>
#include <QColor>
#include <QString>
#include <QPainter>
#include <QScreen>
#include <QCursor>

#include <QTranslator>
#include <Windows.h>
#include <windef.h>
#include <DbgHelp.h>
#include <winuser.h>
#include <dwmapi.h>
#include <QStandardPaths>
#include <QDir>
#include <QMessageBox>
#include <QDesktopServices>
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "dbghelp.lib")

namespace Helper {

    QString isLanguageAvailable(const QString &language);
    bool isAppImagePackage();

    QColor highlightColor(const double& alphaf = 1.0);
    QString formatToFileName(const QString& name);
    QFont adjustFontSizeToFitRect(QPainter& pa, const QString& text, const QRect& rt, int maxFontSize = 100);

    QScreen *currScrn(const QPoint &pt = QCursor::pos());
    double dpiScale(const QScreen* scrn = qGuiApp->primaryScreen());

    // PaddleOCR
    bool isSupportPaddleOCR();



    // 程式异常捕获-保存数据代码-创建 Dump 文件
    void GetExceptionDescription(DWORD errCode, QString &err);
    LONG ApplicationCrashHandler(EXCEPTION_POINTERS *pException);
}
#endif // HELPER_H
