// SPDX-SnippetCopyrightText: 2024-2025 SunnyCapturer Project
// SPDX-Author: Wengeng Zhang <xmulitech@gmail.com>
#ifndef PADDLEOCR_H
#define PADDLEOCR_H
#include <QApplication>
#include <QObject>
#include <QLibrary>
#include "json.h"
using ordered_json = nlohmann::ordered_json;

typedef const char* (*ImageOCR)(const char* image_dir);
typedef const char* (*ImageOCRArgs)(int argc, char** argv);
typedef void (*FreePtr)(const char* p);

class PaddleOCR : public QObject
{
    Q_OBJECT
public:
    explicit PaddleOCR(QObject *parent = nullptr);
    virtual ~PaddleOCR();

    const ordered_json ocrText(const QString& dir = qApp->applicationDirPath() + "/imgs");
    const ordered_json ocrTextArgs(int argc, char** argv);

private:
    bool loadDll();
    void freeDll();

private:
    QLibrary      m_hDll;
    ImageOCR      m_ocrFun;
    ImageOCRArgs  m_ocrFunArgs;
    FreePtr       m_freePtrFun;
};

#endif // PADDLEOCR_H
