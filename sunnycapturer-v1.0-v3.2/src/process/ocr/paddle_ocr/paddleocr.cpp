// SPDX-SnippetCopyrightText: 2024-2025 SunnyCapturer Project
// SPDX-Author: Wengeng Zhang <xmulitech@gmail.com>
#include "paddleocr.h"
#include <QDebug>
#include <QFileInfo>

PaddleOCR::PaddleOCR(QObject *parent)
    : QObject{parent}
    , m_hDll(qApp->applicationDirPath() + "/sunny_ocr.dll")
    , m_ocrFun(nullptr)
    , m_freePtrFun(nullptr)
{
    loadDll();
}

PaddleOCR::~PaddleOCR()
{
    freeDll();
}

const ordered_json PaddleOCR::ocrText(const QString &dir)
{
    ordered_json j;
    if (!m_ocrFun || !m_freePtrFun) return j;

#if Q_DEBUG
    qDebug() << dir << dir.data() << dir.toUtf8().constData() << dir.toLatin1().constData() << dir.toLocal8Bit().constData();
    QByteArray toLocal8BitData = dir.toLocal8Bit();
    printMemory(toLocal8BitData.constData(), toLocal8BitData.size());
    qDebug() << "toLocal8BitData and size:" << toLocal8BitData.constData() << toLocal8BitData.size();
#endif

    const char* result  = m_ocrFun(dir.toLocal8Bit().constData());
    if (result) {
        j = ordered_json::parse(result);

#if Q_DEBUG
    // qDebug().noquote() << "PaddleOCR::ocrText（） j = ordered_json::parse(result): "<< QString::fromStdString(j.dump());
#endif
        m_freePtrFun(result);
    } else {
        qWarning() << "Failed to get JSON result.";
    }

    return j;
}

const ordered_json PaddleOCR::ocrTextArgs(int argc, char **argv)
{
    ordered_json j;
    if (!m_ocrFun || !m_freePtrFun) return j;

    const char* result  = m_ocrFunArgs(argc, argv);
    if (result) {
        j = ordered_json::parse(result);

#if Q_DEBUG
        qDebug().noquote() << "j = ordered_json::parse(result): "<< QString::fromStdString(j.dump(4));
#endif

        m_freePtrFun(result);
    } else {
        qWarning() << "Failed to get JSON result.";
    }

    return j;
}

bool PaddleOCR::loadDll()
{
    if (!m_hDll.load()) {
        qDebug() << "Load DLL failed:" << m_hDll.errorString();
        return false;
    }

    m_ocrFun = reinterpret_cast<ImageOCR>(m_hDll.resolve("ImageOcrProcess"));
    m_ocrFunArgs = reinterpret_cast<ImageOCRArgs>(m_hDll.resolve("ImageOcrProcessWithArgs"));
    m_freePtrFun = reinterpret_cast<FreePtr>(m_hDll.resolve("FreeReturnPtr"));

    if (!m_ocrFun) {
        qDebug() << "Failed to get function address, m_ocrFun:" << m_ocrFun;
        return false;
    } else if (!m_ocrFunArgs) {
        qDebug() << "Failed to get function address, m_freePtrFun:" << m_ocrFunArgs;
        return false;
    } else if (!m_freePtrFun) {
        qDebug() << "Failed to get function address, m_freePtrFun:" << m_freePtrFun;
        return false;
    } else {
        return true;
    }
}

void PaddleOCR::freeDll()
{
    m_hDll.unload();   // stall CRASH!!! Fuck.
}
