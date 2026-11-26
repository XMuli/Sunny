// SPDX-SnippetCopyrightText: 2024-2025 SunnyCapturer Project
// SPDX-Author: Wengeng Zhang <xmulitech@gmail.com>
#ifndef OCRUI_H
#define OCRUI_H

#include <QMainWindow>
#include <QResizeEvent>
#include <QtPdf/QPdfDocument>
#include <QtPdf/QPdfDocumentRenderOptions>
#include <QImage>
#include <QDebug>
#include "imageview.h"
#include "json.h"
using ordered_json = nlohmann::ordered_json;

namespace Ui {
class OcrUI;
}

class OcrUI : public QMainWindow
{
    Q_OBJECT

public:
    explicit OcrUI(QWidget *parent = nullptr);
    ~OcrUI();

    // PDF
    // 将 pdfFile 路径下的 PDF 文件转换为多个 QImage，并保存到 outputDir 文件夹中
    void convertPdfToImages(const QString &pdfFile, const QString &outputDir)
    {
        QPdfDocument pdfDoc;
        // 在 Qt 6.8.3 中，load() 依然返回 Error 枚举
        if (pdfDoc.load(pdfFile) != QPdfDocument::Error::None) {
            qWarning() << "无法加载 PDF 文件:" << pdfFile;
            return;
        }

        const int pageCount = pdfDoc.pageCount();
        QPdfDocumentRenderOptions renderOpts;
        renderOpts.setRenderFlags(QPdfDocumentRenderOptions::RenderFlag::Annotations);

        const qreal scale = 2.0; // 缩放因子，可根据需求调整
        for (int i = 0; i < pageCount; ++i) {
            // 获取页面大小（单位：点）
            const QSizeF pageSize = pdfDoc.pagePointSize(i);
            const QSize imageSize(pageSize.width() * scale,
                                   pageSize.height() * scale);

                   // Qt 6.8.3 中直接用 QPdfDocument::render()
            const QImage image = pdfDoc.render(i, imageSize, renderOpts);
            if (image.isNull()) {
                qWarning() << "渲染第" << i+1 << "页失败";
                continue;
            }

            const QString outPath = outputDir + QString("/page_%1.png").arg(i + 1);
            if (!image.save(outPath)) {
                qWarning() << "保存图像失败:" << outPath;
            } else {
                qDebug() << "保存成功:" << outPath;
            }
        }
    }

    void setLeftPixmap(const QPixmap& pix, const bool extractText);
    void setRightText(const QString& text);
    void appendRightText(const QString& text);

private slots:
    void on_splitter_splitterMoved(int pos, int index);
    void onPaddleOCRFinish(ordered_json j);
    void onEditEnable(bool enable);

    void onOcrFinished(const QString& text, const QString& path); // OCR Text 保存成功

private:
    QString paddleOCRJsonFormat(ordered_json& j);

protected:
    void resizeEvent(QResizeEvent *e) override;
    void closeEvent(QCloseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;

private:
    Ui::OcrUI *ui;
};

#endif // OCRUI_H
