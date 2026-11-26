// SPDX-SnippetCopyrightText: 2024-2025 SunnyCapturer Project
// SPDX-Author: Wengeng Zhang <xmulitech@gmail.com>
#include "ocrui.h"
#include "communication.h"
#include "ui_ocrui.h"
#include <QShortcut>
#include <QDebug>
#include "config.h"
#include "helper.h"

OcrUI::OcrUI(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::OcrUI)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose, true);

    ui->menubar->hide();
    ui->statusbar->hide();
    ui->actionPrevious->setVisible(false);
    ui->actionNext->setVisible(false);
    ui->actionTranslate->setVisible(false);
    ui->actionExtract_Text->setVisible(false);

    int& left = config::easy_use::ocr_left;
    int& top = config::easy_use::ocr_top;
    int& width = config::easy_use::ocr_width;
    int& height = config::easy_use::ocr_height;

    QScreen* scrn = Helper::currScrn();
    if (!scrn->virtualGeometry().contains(QPoint(left, top)))
        left = top = width = height = 0;

    const auto& rt = scrn->geometry();
    if (width == 0) width = rt.width() * 3 / 4;
    if (height == 0) height = rt.height() * 3 / 4;
    if (left == 0) left = rt.center().x() - width / 2;
    if (top == 0) top = rt.center().y() - height / 2;

    // 设置Splitter的大小比例
    const auto& allWidth = width - ui->horizontalLayout->contentsMargins().left() + ui->horizontalLayout->contentsMargins().right();
    const double& leftRate = config::easy_use::ocr_split_left;
    const double& rightRate = config::easy_use::ocr_split_right;
    const double& allRRate = leftRate + rightRate;
    QList<int> sizes;
    sizes <<  allWidth * double(1.0 * leftRate/allRRate) << allWidth * double(1.0 * rightRate/allRRate);
    ui->splitter->setSizes(sizes);

    // ui->splitter->setStretchFactor(0, 1);
    // new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Plus),  this, [this](){ onZoomIn();});
    // new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Equal),  this, [this](){onZoomIn();});
    // new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Underscore),  this, [this](){onZoomOut();});
    // new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Minus),  this, [this](){onZoomOut();});          // QKeySequence::ZoomIn  ZoomOut
    // new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_N),  this, [this](){onNormalSize();});

    connect(ui->Image, &ImageView::sigPaddleOCRFinish, this, &OcrUI::onPaddleOCRFinish);
    connect(ui->actionOriginal_Image, &QAction::triggered, ui->Image, &ImageView::onSwitchImage);
    connect(ui->actionRotate, &QAction::triggered, ui->Image, &ImageView::onRotateImage);
    connect(ui->actionZoom, &QAction::triggered, ui->Image, &ImageView::onZoomInImage);
    connect(ui->actionShrink, &QAction::triggered, ui->Image, &ImageView::onZoomOutImage);
    connect(ui->actionOriginal_Size, &QAction::triggered, ui->Image, &ImageView::onPresetImage);
    connect(ui->actionEdit, &QAction::triggered, this, &OcrUI::onEditEnable);
    connect(ui->actionSave_as, &QAction::triggered, ui->Image, &ImageView::onSaveAsImage);

    connect(&COMM, &Communication::sigOcrFinished, this, &OcrUI::onOcrFinished);
    resize(width, height);
    move(left, top);
}

OcrUI::~OcrUI()
{
    config::save();
    delete ui;
}


void OcrUI::setLeftPixmap(const QPixmap &pix, const bool extractText)
{
    if (pix.isNull()) return;
    ui->Image->setPixmap(pix, extractText);
}

void OcrUI::setRightText(const QString &text)
{
    if (text.isEmpty()) return;
    ui->textEdit->setText(text);
}

void OcrUI::appendRightText(const QString &text)
{
    // ui->textEdit->append(text);
    ui->textEdit->insertPlainText(text);
}

void OcrUI::on_splitter_splitterMoved(int pos, int index)
{
    Q_UNUSED(pos);
    Q_UNUSED(index);

    const auto& splitter = ui->splitter;
    QList<int> sizes = splitter->sizes();

    int left = 3;
    int right = 2;
    if (sizes.count() == 2) {
        left = sizes[0];
        right = sizes[1];
    }

    config::easy_use::ocr_split_left = left;
    config::easy_use::ocr_split_right = right;
}

void OcrUI::onPaddleOCRFinish(ordered_json j)
{
    setRightText(paddleOCRJsonFormat(j));
}

void OcrUI::onEditEnable(bool enable)
{
    ui->textEdit->setReadOnly(!enable);
}

void OcrUI::onOcrFinished(const QString &text, const QString &path)
{
    setLeftPixmap(path, false);  // fix: true 则解开回死循环无限请求
    ui->textEdit->setText(text);
}

QString OcrUI::paddleOCRJsonFormat(ordered_json &j) {
    QString result;
    int line_threshold = 5; // 误差阈值

    for (const auto& image : j["images"]) {
        // 添加图片的 index 和 path 信息
        int index = image["index"];
        QString path = QString::fromStdString(image["path"]);
        // result.append(QString("Index: %1\nPath: %2\n").arg(index).arg(path));

        std::vector<QString> lines;
        std::vector<int> last_y_positions;

        for (const auto& text_result : image["text_results"]) {
            QString text = QString::fromStdString(text_result["text"]).trimmed();

            // 跳过 text 为空的对象
            if (text.isEmpty())
                continue;

            int y_bottom = text_result["detection_box"][2]["y"]; // 获取检测框的底部 y 坐标

            // 查找是否有位于同一水平线的行
            bool found_line = false;
            for (size_t i = 0; i < last_y_positions.size(); ++i) {
                if (std::abs(y_bottom - last_y_positions[i]) <= line_threshold) {
                    lines[i].append(" " + text); // 同一行添加空格
                    last_y_positions[i] = y_bottom; // 更新 y 位置
                    found_line = true;
                    break;
                }
            }

            // 如果未找到同一水平线上的行，则创建新行
            if (!found_line) {
                lines.push_back(text);
                last_y_positions.push_back(y_bottom);
            }
        }

        // 将所有行合并为最终结果
        for (const auto& line : lines) {
            result.append(line + "\n");
        }

        // 添加分隔线
        result.append("\n--------------------\n");
    }

    return result.trimmed(); // 移除结果末尾的多余换行符
}

void OcrUI::resizeEvent(QResizeEvent *e)
{
    QSize size = e->size();
    config::easy_use::ocr_width = size.width();
    config::easy_use::ocr_height =  size.height();
    config::easy_use::ocr_left = geometry().left();
    config::easy_use::ocr_top = geometry().top();

    QMainWindow::resizeEvent(e);
}

void OcrUI::closeEvent(QCloseEvent *e)
{
    config::save();
    QMainWindow::closeEvent(e);
}

void OcrUI::mouseMoveEvent(QMouseEvent *e)
{
    config::easy_use::ocr_left = geometry().left();
    config::easy_use::ocr_top = geometry().top();
    QMainWindow::mouseMoveEvent(e);
}
