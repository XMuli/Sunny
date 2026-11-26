// SPDX-SnippetCopyrightText: 2024-2025 SunnyCapturer Project
// SPDX-Author: Wengeng Zhang <xmulitech@gmail.com>
#include "imageview.h"
#include "communication.h"
#include "config.h"
#include <QMenu>
#include <QContextMenuEvent>
#include <QStyleOption>
#include <QPainter>
#include <QFileDialog>
#include <QDebug>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QTransform>
#include <QDateTime>
#include <QStandardPaths>
#include <QElapsedTimer>
#include "helper.h"
#include "helper/net_helper.h"
#include "process/ocr/paddle_ocr/paddleocr.h"
using ordered_json = nlohmann::ordered_json;

// 定义输出内存内容的函数
void printMemory(const char *data, size_t size)
{
    QDebug dbg = QDebug(QtDebugMsg);
    dbg << "Memory content in hexadecimal:";

    for (size_t i = 0; i < size; ++i) {
        dbg.noquote() << QString::asprintf("%02x", static_cast<unsigned char>(data[i]));
        if ((i + 1) % 16 == 0) dbg << "";  // 每16字节换行
    }
    dbg << "";  // 最后一行结束后换行
}

// 定义打印argv内容及其内存的函数
void printArgvHex(int argc, char **argv)
{
    for (int i = 0; i < argc; ++i) {
        qDebug() << "argv[" << i << "] content:" << argv[i];
        printMemory(argv[i], std::strlen(argv[i]));
    }

    qDebug() << "\n";
}

ImageView::ImageView(QWidget *parent)
    : QWidget(parent)
    , m_ZoomValue(1.0)
    , m_XPtInterval(0)
    , m_YPtInterval(0)
    , m_OldPos(QPoint())
    , m_Pressed(false)
    , m_showOrigin(false)
    , m_labZoom(new QLabel(this))
    , m_timerLabZoom(new QTimer(this))
{
    initUI();
}

ImageView::~ImageView()
{
}

void ImageView::contextMenuEvent(QContextMenuEvent *event)
{
    QPoint pos = event->pos();
    pos = this->mapToGlobal(pos);
    QMenu *menu = new QMenu(this);

    QAction *loadImage = new QAction(tr("Load Image"));
    QObject::connect(loadImage, &QAction::triggered, this, &ImageView::onLoadImage);
    menu->addAction(loadImage);
    menu->addSeparator();

    QAction *zoomInAction = new QAction(tr("Zoom In"));
    QObject::connect(zoomInAction, &QAction::triggered, this, &ImageView::onZoomInImage);
    menu->addAction(zoomInAction);

    QAction *zoomOutAction = new QAction(tr("Zoom Out"));
    QObject::connect(zoomOutAction, &QAction::triggered, this, &ImageView::onZoomOutImage);
    menu->addAction(zoomOutAction);

    QAction *presetAction = new QAction(tr("Preset"));
    QObject::connect(presetAction, &QAction::triggered, this, &ImageView::onPresetImage);
    menu->addAction(presetAction);

    menu->exec(pos);
}

void ImageView::paintEvent(QPaintEvent *event)
{
    // 绘制样式
    QStyleOption opt;
    // opt.init(this);
    QPainter painter(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);
    // 设置渲染质量
    painter.setRenderHint(QPainter::Antialiasing , true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    if (m_pix.isNull())
        return QWidget::paintEvent(event);

    // 根据窗口计算应该显示的图片的大小
    int width = qMin(m_pix.width(), this->width());
    int height = width * 1.0 / (m_pix.width() * 1.0 / m_pix.height());
    height = qMin(height, this->height());
    width = height * 1.0 * (m_pix.width() * 1.0 / m_pix.height());

    // 平移
    painter.translate(this->width() / 2 + m_XPtInterval, this->height() / 2 + m_YPtInterval);

    // 缩放
    painter.scale(m_ZoomValue, m_ZoomValue);

    // 绘制图像
    QRect picRect(-width / 2, -height / 2, width, height);
    painter.drawPixmap(picRect, m_pix);
}

void ImageView::wheelEvent(QWheelEvent *event)
{
    if (event->angleDelta().y() > 0)
        onZoomInImage();
    else
        onZoomOutImage();

    this->update();
}

void ImageView::mousePressEvent(QMouseEvent *event)
{
    m_OldPos = event->pos();
    m_Pressed = true;
}

void ImageView::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_Pressed)
        return QWidget::mouseMoveEvent(event);

    this->setCursor(Qt::SizeAllCursor);
    QPoint pos = event->pos();
    int xPtInterval = pos.x() - m_OldPos.x();
    int yPtInterval = pos.y() - m_OldPos.y();

    m_XPtInterval += xPtInterval;
    m_YPtInterval += yPtInterval;

    m_OldPos = pos;
    this->update();
}

void ImageView::mouseReleaseEvent(QMouseEvent *event)
{
    m_Pressed = false;
    this->setCursor(Qt::ArrowCursor);
}

void ImageView::dragEnterEvent(QDragEnterEvent *event)
{
    // 只接受文件类型的拖放
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void ImageView::dropEvent(QDropEvent *event)
{
    const QList<QUrl> urls = event->mimeData()->urls();
    if (!urls.isEmpty()) {
        QString filePath = urls.first().toLocalFile();
        QFile file(filePath);
        if (file.exists()) {
            qDebug() << "Dropped file:" << filePath;
            // 你可以在这里调用 onLoadImage() 或者直接加载图片
            loadImageOCR(filePath);
            this->update();
        }
    }
}

void ImageView::onSwitchImage()
{
    m_showOrigin = !m_showOrigin;
    switchPixmap(m_showOrigin);
}

void ImageView::setPixmap(const QPixmap &newPix, const bool extractText)
{
    m_pix = newPix;
    update();

    if (extractText) {
        const QString& path = getCacheFolder(ImgCacheDir::ICD_out_img_extract_text) + "/" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".png";
        bool ok = m_pix.save(path);
        setPixmap(QPixmap(":/images/ocr/extract_text_loading.svg"), false);
        update();
        if (ok) {
            loadImageOCR(path);
        }
    }
}

void ImageView::initUI()
{
    m_labZoom->setParent(this);
    m_labZoom->setStyleSheet("QLabel { background-color: white; border: 1px solid black; }");
    m_labZoom->raise();
    m_labZoom->hide();
    m_labZoom->move(10, 10);
    // creatInputOutputDir();
    getCacheFolder(ImgCacheDir::ICD_all);
    setAcceptDrops(true);  // 启用拖放功能
    setPixmap(QPixmap(":/images/ocr/select_image.svg"), false);
    connect(m_timerLabZoom, &QTimer::timeout, [this]() { m_labZoom->hide();});
}

void ImageView::updateZoomLabel()
{
    m_labZoom->setText(QString(tr("Zoom: %1%")).arg(m_ZoomValue * 100));
    m_labZoom->show();
    m_labZoom->adjustSize();      // 自动调整大小以适应文本
    m_timerLabZoom->stop();       // 取消之前的定时器
    m_timerLabZoom->start(2000);  // 创建一个新的定时器，并在时间到达时隐藏标签
}

void ImageView::analysisExtractText(NetTencent::TYPE type, const QString paddleCmdImgOutput, NetTencent::RECOGNITION recog, QString path)
{
    m_ocrType = type;
    if (type != NetTencent::TYPE::T_paddle_ocr) {
        setPixmap(QPixmap(path), false);
        m_netTencent.requestAPI(path, type, recog);
    } else {

        if(!Helper::isSupportPaddleOCR()) {
            QString text = tr("The crrent version does not support offline OCR, please go to the official website to download the corresponding CPU or GPU version of the DLL library, copy it to the root directory where the program is located, and then restart it to use it.\n- [SITE] %1 [Image recognition engine using native offline]").arg("https://sunnycapturer.xmuli.tech/#/extract_text_ocr");
            emit COMM.sigOcrFinished(text, "");
            return;
        }

        setPixmap(QPixmap(":/icons/ocr/extract_text_loading"), false);
        static PaddleOCR paddleOCR;
#if 0
        ordered_json j = paddleOCR.ocrText(path);
#else

        // 尝试命令行方式调用，可以修改指定文 检测和识别的件夹
        // QString command = "D:/projects/OCR/PaddleOCR-2.8.1/deploy/cpp_infer/build/Release/ppocr.exe --det_model_dir=D:/projects/OCR/PaddleOCR-2.8.1/deploy/cpp_infer/build/Release/infer/ch_PP-OCRv4_det_infer --rec_model_dir=D:/projects/OCR/PaddleOCR-2.8.1/deploy/cpp_infer/build/Release/infer/ch_PP-OCRv4_rec_infer --image_dir=D:/projects/OCR/PaddleOCR-2.8.1/deploy/cpp_infer/build/Release/imgs中文/中文1.jpg";
        // QStringList commandList = command.split(" ");

        QStringList commandList;
        // commandList << "--det_model_dir=D:/projects/OCR/PaddleOCR-2.8.1/deploy/cpp_infer/build/Release/infer/ch_PP-OCRv4_det_infer";
        // commandList << "--rec_model_dir=D:/projects/OCR/PaddleOCR-2.8.1/deploy/cpp_infer/build/Release/infer/ch_PP-OCRv4_rec_infer";

        QFile file(qApp->applicationDirPath() + "/cudnn_cnn64_9.dll");
        const bool& useGpu = file.exists();

        const auto& tDir = qGuiApp->applicationDirPath() + "/infer/";
        commandList << "--det_model_dir=" + tDir + "ch_PP-OCRv4_det_infer";
        commandList << "--rec_model_dir=" + tDir + "ch_PP-OCRv4_rec_infer";
        commandList << "--image_dir=" + path;
        commandList << "--output=" + paddleCmdImgOutput;
        commandList << QString("--use_gpu=%1").arg(useGpu ? "true" : "false");
        commandList << QString("--use_tensorrt=%1").arg(useGpu ? "true" : "false");
        // commandList << "--rec_batch_num=1";
        // commandList << "--cpu_threads=1";


        if (useGpu) {
            QString appDirPath = qApp->applicationDirPath();
            // 拼接文件完整路径
            QString detFilePath = appDirPath + "/trt_det_shape.txt";
            QString recFilePath = appDirPath + "/trt_rec_shape.txt";

                   // 检查并删除 trt_det_shape.txt 文件
            QFile detFile(detFilePath);
            if (detFile.exists()) {
                if (detFile.remove()) {
                    qDebug() << "成功删除文件:" << detFilePath;
                } else {
                    qDebug() << "删除文件失败:" << detFilePath;
                }
            }

                   // 检查并删除 trt_rec_shape.txt 文件
            QFile recFile(recFilePath);
            if (recFile.exists()) {
                if (recFile.remove()) {
                    qDebug() << "成功删除文件:" << recFilePath;
                } else {
                    qDebug() << "删除文件失败:" << recFilePath;
                }
            }
        }

        qDebug() << "OCR commandList:" << commandList;
        std::vector<std::string> stringList;
        std::vector<char*> args;

        for (const QString& str : commandList) {
            std::string ansiStr = str.toLocal8Bit().constData();  // 此处需要传递 ANSI 编码格式 (即toLocal8Bit)
            stringList.push_back(ansiStr);  // 将std::string存入vector
            args.push_back(stringList.back().data());  // 获取std::string的数据指针
        }

        int argc = args.size();
        char** argv = args.data();
        printArgvHex(argc, argv);

        QElapsedTimer timer;
        // 开始计时
        timer.start();
        qDebug() << "Starting OCR...";

        ordered_json j = paddleOCR.ocrTextArgs(argc, argv);
#endif

        // 打印时间
        qint64 elapsedTime = timer.elapsed();  // 以毫秒为单位, 将毫秒转换为秒和毫秒
        qint64 seconds = elapsedTime / 1000;
        qint64 milliseconds = elapsedTime % 1000;
        qDebug() << "OCR process finished in:" << seconds << "s" << milliseconds << "ms";

        setProperty("paddleocr_image_name", QFileInfo(path).fileName());
        setProperty("paddleocr_image_path", path);
        setProperty("paddleocr_cmd_image_output_dir", paddleCmdImgOutput);

        emit sigPaddleOCRFinish(j);
        switchPixmap(j.empty());
        m_showOrigin = j.empty();
    }
}

void ImageView::loadImageOCR(QString path)
{
    if (path.isEmpty()) return;
    qDebug() << "image path:" << path;

    NetTencent::TYPE type = NetTencent::TYPE::T_tencent_cloud_pix_trans;
    QString engine;
    const bool use_extract_text = true;
    if (use_extract_text) { // 走 OCR 提取文字
        engine = config::extract_text::use_engine;
        if (engine == "tencent_cloud") type = NetTencent::TYPE::T_tencent_cloud_pix_trans;
        else if (engine == "baidu_cloud") type = NetTencent::TYPE::T_baidu_cloud_ocr;
        else if (engine == "offline_ocr") type = NetTencent::TYPE::T_paddle_ocr;
        if (engine.isEmpty()) return;

        const QString& local_ocr = QFileInfo(path).absolutePath() + "/local_ocr";
        QDir dir(local_ocr);
        if (!dir.exists()) dir.mkpath(local_ocr); // 创建目录及其父目录

        analysisExtractText(type, local_ocr, NetTencent::RECOGNITION::R_extract_text, path);

    } else {   // 走 图片翻译，获取图片
        // engine = config::image_translate::use_engine;
        // if (engine == "tencent_cloud") type = NetTencent::TYPE::T_tencent_cloud_pix_trans;
        // else if (engine == "baidu_cloud") type = NetTencent::TYPE::T_baidu_cloud_pix_trans;
        // else if (engine == "youdao_cloud") type = NetTencent::TYPE::T_youdao_cloud_pix_trans;
        // if (engine.isEmpty()) return;

        // setPixmap(QPixmap(path), false);
        // m_netTencent.requestAPI(path, type, NetTencent::RECOGNITION::R_img_trans);
    }
}


QString ImageView::replaceLastTwoSlashesSubstring(QString imageFile)
{
    QFileInfo fileInfo(imageFile);
    if (fileInfo.isFile() && fileInfo.exists()) {
        imageFile = fileInfo.absoluteFilePath();

        // 找到最后两个斜杠的位置
        int lastSlashIndex = imageFile.lastIndexOf('/');
        if (lastSlashIndex == -1) {
            // 如果没有斜杠，则返回原始字符串
            return imageFile;
        }

        // 提取斜杠之间的子字符串
        QString beforeLastSlash = imageFile.left(lastSlashIndex + 1); // 包含第二个斜杠的部分

        // 构造新的路径字符串
        QString newPath = beforeLastSlash + "output";
        return newPath;

    } else {

        // 构造新的路径字符串
        QString newPath = imageFile + "/output";
        return newPath;
    }
}

void ImageView::switchPixmap(const bool &showOrigin)
{
    // 在线 OCR 导出路径
    QString image_path = m_netTencent.property("image_path").toString();
    QString image_process_path = m_netTencent.property("image_process_path").toString();
    QString image_translate_path = m_netTencent.property("image_translate_path").toString();

    // 本地 paddle_ocr 导出路径
    if (m_ocrType == NetTencent::TYPE::T_paddle_ocr) {
        image_path = property("paddleocr_image_path").toString();
        image_process_path = property("paddleocr_cmd_image_output_dir").toString() + "/" + property("paddleocr_image_name").toString();
        image_translate_path = "";
    }

    QString path = showOrigin ? image_path : image_process_path;
    QFile file(path);
    if (file.exists()) m_pix.load(path);
    else m_pix.load(image_path);

    update();
}

// void ImageView::creatInputOutputDir()
// {
//     QString temp = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QString("/%1/ocr/").arg(XPROJECT_NAME);
//     QString dirIn = temp + "in";
//     QString dirOutput = temp + "output";

//     QDir qdir(dirIn);
//     if (!qdir.exists()) qdir.mkpath(dirIn); // 创建目录及其父目录

//     qdir.setPath(dirOutput);
//     if (!qdir.exists()) qdir.mkpath(dirOutput); // 创建目录及其父目录
//     qDebug() << "dirIn:" << dirIn << "dirOutput:" << dirOutput;

//     setProperty("cacheDirIn", dirIn);
//     setProperty("cacheDirOutput", dirOutput);
// }

void ImageView::onLoadImage(void)
{
    // QString imageFile = QFileDialog::getOpenFileName(this, "Open Image", "./", tr("Images (*.png *.bpm *.jpg)"));

    // QFile file(imageFile);
    // if (!file.exists())
    //     return;

    QString imageFile = QFileDialog::getExistingDirectory(this, "Open Image");

    loadImageOCR(imageFile);
}

void ImageView::onZoomInImage(void)
{
    m_ZoomValue += 0.1;
    updateZoomLabel();
    this->update();
}

void ImageView::onZoomOutImage(void)
{
    m_ZoomValue -= 0.1;
    m_ZoomValue = qMax(0.1, m_ZoomValue);
    updateZoomLabel();
    this->update();
}

void ImageView::onPresetImage(void)
{
    m_ZoomValue = 1.0;
    m_XPtInterval = 0;
    m_YPtInterval = 0;
    updateZoomLabel();
    this->update();
}

// 图片旋转
void ImageView::onRotateImage(void)
{
    QTransform transform;
    transform.rotate(90);

    // 创建旋转后的 QPixmap
    QPixmap rotatedPixmap = m_pix.transformed(transform);
    m_pix = rotatedPixmap;
    update();
}

void ImageView::onSaveAsImage()
{
    if (m_pix.isNull()) return;

    QString name = property("paddleocr_image_name").toString(); // T_paddle_ocr
    if (m_ocrType != NetTencent::TYPE::T_paddle_ocr) {
        const auto& image_path = m_netTencent.property("image_path").toString();
        QFileInfo info(image_path);
        name = info.fileName();
    }

    const QString& fileter(tr("Image Files(*.png);;Image Files(*.jpg);;Image Files(*.jpeg);;Image Files(*.bmp);;Image Files(*.gif);;All Files(*.*)"));
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Image"), name, fileter);
    if (!fileName.isEmpty()) m_pix.save(fileName);
}

