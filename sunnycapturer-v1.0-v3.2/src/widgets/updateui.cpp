#include "updateui.h"

#include "logging.h"
#include "ui_updateui.h"
#include <QRadioButton>
#include <QCheckBox>
#include <QTextDocument>
#include <QDesktopServices>
#include <QScrollBar>
#include "config.h"

UpdateUI::UpdateUI(QWidget *parent)
    : QWidget(parent), ui(new Ui::UpdateUI)
{
    ui->setupUi(this);
    setWindowTitle(XPROJECT_NAME);
    // setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowFlags(Qt::WindowStaysOnTopHint);
}

UpdateUI::~UpdateUI() { delete ui; }

void UpdateUI::setSelectDownVersion(const std::vector<VersionData>& vec)
{
    m_vec = vec;
    auto& layout = ui->layout_gropbox;

    for (const auto& it : ui->groupBox->findChildren<QRadioButton*>()) {
        layout->removeWidget(it);
        disconnect(it);
        it->deleteLater();
    }

    for (auto &it : m_vec) {
        QRadioButton* rb = new QRadioButton(it.name);
        const bool defaultFile = (it.name.contains("exe") && it.name.contains("cpu"));
        if (defaultFile) it.isSelected = true;
        rb->setChecked( defaultFile ? true : false);
        layout->addWidget(rb);

        connect(rb, &QRadioButton::toggled, this, [this, &it](bool checked) {
            if (checked) {
                for (auto& version : m_vec)
                    version.isSelected = (version.name == it.name);
            }
        });

        // logi("name:{} size:{}MB url:{}", it.name.toStdString(), it.size, it.url.toStdString());
    }
}

void UpdateUI::setLatestVersionChanglog(const QString &text)
{
    ui->textEdit->clear();  // 清空内容
    ui->textEdit->setReadOnly(true);

    // 设置 Markdown 内容（确保没有 <br> 之类的特殊 HTML 标签）
    ui->textEdit->document()->setMarkdown(text, QTextDocument::MarkdownDialectGitHub);

    // 创建文本光标并设置为文档的起始位置
    QTextCursor cursor = ui->textEdit->textCursor();
    cursor.movePosition(QTextCursor::Start);
    ui->textEdit->setTextCursor(cursor);
    ui->textEdit->verticalScrollBar()->setValue(ui->textEdit->verticalScrollBar()->minimum());  // 确保滚动条移动到顶部
    adjustSize();
}

void UpdateUI::setLatestVersionLabel(const QString &curVerson, const QString &curVersonTime, const QString &newVerson, const QString &newVersonTime)
{
    ui->lab_cur_version->setText(curVerson);
    ui->lab_cur_version_time->setText(curVersonTime);

    ui->lab_latest_version->setText(newVerson);
    ui->lab_latest_version_time->setText(newVersonTime);

    setLastCheckTime();
}

void UpdateUI::setLastCheckTime(const QString time)
{
    ui->lab_last_check_time->setText(time.isEmpty() ? config::update::last_check_time : time);
}


void UpdateUI::on_btn_down_and_install_released()
{
    // if (bigFile) {
        for (const auto& it : m_vec) {
            if (it.isSelected) {
                QDesktopServices::openUrl(it.url);  // 使用默认浏览器打开网址

                break;
            } else {
                continue;
            }
        }
    // } else {
        // downLatestVersion(url);
    // }

    close();
}


void UpdateUI::on_btn_cancel_released()
{
    close();
}


void UpdateUI::on_btb_recheck_released()
{
    emit sigRecheked();
}

