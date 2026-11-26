// SPDX-SnippetCopyrightText: 2024-2025 SunnyCapturer Project
// SPDX-Author: Wengeng Zhang <xmulitech@gmail.com>
#ifndef IMAGEVIEW_H
#define IMAGEVIEW_H

#include <QWidget>
#include <QImage>
#include <QTimer>
#include <QLabel>
#include "../../process/ocr/online_ocr/nettencent.h"
#include "json.h"
using ordered_json = nlohmann::ordered_json;

class ImageView : public QWidget
{
    Q_OBJECT

public:
    ImageView(QWidget *parent = nullptr);
    ~ImageView();
    void setPixmap(const QPixmap &newPix, const bool extractText);

private:
    void initUI();
    void updateZoomLabel();
    void loadImageOCR(QString path);
    QString replaceLastTwoSlashesSubstring(QString imageFile);
    void switchPixmap(const bool& showOrigin);
    void creatInputOutputDir();

signals:
    void sigPaddleOCRFinish(ordered_json j);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

public slots:
    void onSwitchImage(void);
    void onLoadImage(void);
    void onZoomInImage(void);
    void onZoomOutImage(void);
    void onPresetImage(void);
    void onRotateImage(void);
    void onSaveAsImage(void);

private:
    QPixmap          m_pix;
    qreal            m_ZoomValue = 1.0;
    int              m_XPtInterval = 0;
    int              m_YPtInterval = 0;
    QPoint           m_OldPos;
    bool             m_Pressed = false;
    bool             m_showOrigin;    // 识别后绿框和原图切换
    QLabel*          m_labZoom;
    QTimer*          m_timerLabZoom;
    NetTencent       m_netTencent;
    NetTencent::TYPE m_ocrType;       // 本次调用在线还是
    void analysisExtractText(NetTencent::TYPE type, const QString paddleCmdImgOutput, NetTencent::RECOGNITION recog, QString path);
};

#endif // IMAGEVIEW_H
