// SPDX-SnippetCopyrightText: 2024-2025 SunnyCapturer Project
// SPDX-Author: ffiirree <ice_qi@163.com>
// SPDX-Author: Wengeng Zhang <xmulitech@gmail.com>
#include "settingdialog.h"
#include "helper/net_helper.h"
#include "sub_page/ui_sub_output.h"
#include "sub_page/ui_sub_update.h"
#include "capturer.h"
#include "colorpanel.h"
#include "combobox.h"
#include "config.h"
#include "logging.h"
#include "path-edit.h"
#include "scrollwidget.h"
#include "version.h"
#include "helper.h"
#include "versionupdater.h"
#include "communication.h"

#include <QCheckBox>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QKeySequenceEdit>
#include <QLineEdit>
#include <QListWidget>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVBoxLayout>

static const std::vector<std::pair<std::underlying_type_t<Qt::PenStyle>, QString>> PENSTYLES = {
    { Qt::SolidLine, "SolidLine" },
    { Qt::DashLine, "DashLine" },
    { Qt::DotLine, "DotLine" },
    { Qt::DashDotLine, "DashDotLine" },
    { Qt::DashDotDotLine, "DashDotDotLine" },
};

SettingWindow::SettingWindow(QWidget *parent)
    : FramelessWindow(parent, Qt::WindowMinMaxButtonsHint)
      , uioutput_(new Ui::SubOutput)
      , uiupdate_(new Ui::SubUpdate)
{
    setContentsMargins({});

    setAttribute(Qt::WA_DeleteOnClose);

    setWindowTitle(tr("Settings"));

    //
    const auto layout = new QHBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins({});
    setLayout(layout);

    {
        const auto menu = new QListWidget();
        menu->setFocusPolicy(Qt::NoFocus);
        menu->addItem(tr("General"));
        menu->addItem(tr("Interface"));
        menu->addItem(tr("OCR"));
        menu->addItem(tr("Image Translate"));
        menu->addItem(tr("Output"));
        menu->addItem(tr("Hotkeys"));
        menu->addItem(tr("Update"));
        menu->addItem(tr("About"));
        menu->setMinimumWidth(250);
        menu->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
        layout->addWidget(menu);

        const auto vlayout = new QVBoxLayout();
        vlayout->setSpacing(0);
        vlayout->setContentsMargins({});
        {
            const auto close_btn = new QCheckBox();
            close_btn->setObjectName("close-btn");
            close_btn->setCheckable(false);
            connect(close_btn, &QCheckBox::clicked, this, &QWidget::close);

            vlayout->addWidget(close_btn, 0, Qt::AlignRight);

            const auto pages = new QStackedWidget();

            pages->addWidget(setupGeneralWidget());
            pages->addWidget(setupInterfaceWidget());
            pages->addWidget(setupExtractTextWidget());
            pages->addWidget(setupImageTranslateWidget());
            pages->addWidget(setupOutputWidget());
            pages->addWidget(setupHotkeyWidget());
            pages->addWidget(setupUpdateWidget());
            pages->addWidget(setupAboutWidget());

            vlayout->addWidget(pages);
            connect(menu, &QListWidget::currentRowChanged, this, [pages, this](int idx) {
                pages->setCurrentIndex(idx);
                this->adjustSize();
            });
        }
        layout->addLayout(vlayout);

        menu->setCurrentRow(0);
    }

    resize(960, 640);

    connect(this, &SettingWindow::closed, [] { config::save(); });
}

static QLabel *LABEL(const QString& text, int width)
{
    const auto label = new QLabel(text);
    label->setMinimumWidth(width);
    return label;
}

QWidget *SettingWindow::setupGeneralWidget()
{
    const auto page = new ScrollWidget();
    {
        const auto form = page->addForm(tr("General"));

        const auto autorun = new QCheckBox();
        connect(autorun, &QCheckBox::toggled, [](bool checked) { config::set_autorun(checked); });
        autorun->setChecked(config::general::autorun);
        form->addRow(LABEL(tr("Run on Startup"), 175), autorun);

        //
        const auto language = new ComboBox({
            { "en_us", "English" },
            { "zh_cn", "简体中文" },
            { "zh_tw", "繁體中文" },
            { "ja_jp", "日本語"},
            { "pt_br", "Português Brasil" },  // 葡萄牙语-巴西
            { "de",    "Deutsch"}             // 德语
        });
        language->setToolTip(tr("Restart to apply after switching languages"));
        language->select(config::general::language);
        language->onselected([](auto value) { config::general::language = value.toString(); });
        form->addRow(tr("Language"), language);

        //
        const auto file = new QLineEdit(config::general::filepath);
        file->setContextMenuPolicy(Qt::NoContextMenu);
        file->setReadOnly(true);
        form->addRow(tr("Settings File"), file);

        //
        const auto theme = new ComboBox({
            { "auto", tr("Auto") },
            { "dark", tr("Dark") },
            { "light", tr("Light") },
        });
        theme->onselected([](auto value) { config::set_theme(value.toString().toStdString()); });
        theme->select(config::general::theme);
        form->addRow(tr("Theme"), theme);
    }

    page->addSpacer();
    return page;
}

QWidget *SettingWindow::setupExtractTextWidget()
{
    using namespace config;

    const auto page = new ScrollWidget();
    {
        auto cbb = new QComboBox();
        const auto list = ListExtractText();
        for (const auto& it : list) {
            QIcon icon(QString(":/icons/setting/image_recognition/%1").arg(it.first));
            cbb->addItem(icon, it.second, it.first);
        }
        qDebug() << "extract_text::use_engine:" << extract_text::use_engine;
        cbb->setCurrentText(findKeyByValue(list, extract_text::use_engine));
        const auto form = page->addForm(tr("Extract text"));
        form->addRow(LABEL(tr("Engine"), 175), cbb);

        auto leID = new QLineEdit();
        auto leKey = new QLineEdit();
        leKey->setEchoMode(QLineEdit::PasswordEchoOnEdit);
        const auto form2 = page->addForm(tr("Private account"));
        form2->addRow(new QLabel(tr("If empty, use the default token.")));
        form2->addRow(LABEL(tr("Secret ID"), 175), leID);
        form2->addRow(tr("Secret Key"), leKey);

        // connect
        connect(cbb, QOverload<const QString &>::of(&QComboBox::currentTextChanged), this, [=](){
            QString id = "";
            QString key = "";
            const QString& data = cbb->currentData().toString();
            extract_text::use_engine = data;
            if (data == "tencent_cloud") {
                id = extract_text::tencent_cloud_id;
                key = extract_text::tencent_cloud_key;
                page->showForm(form2);
            } else if (data == "baidu_cloud") {
                id = extract_text::baidu_cloud_id;
                key = extract_text::baidu_cloud_key;
                page->showForm(form2);
            } else if (data == "offline_ocr") {
                page->hideForm(form2);
            }

            leID->setText(config::decryptString(id.toStdString()));
            leKey->setText(config::decryptString(key.toStdString()));
        });

        emit cbb->currentTextChanged(cbb->currentText());

        connect(leID, &QLineEdit::textChanged, this, [=](const QString &arg1){
            const QString data = cbb->currentData().toString();
            if (data == "tencent_cloud") {
                extract_text::tencent_cloud_id = QString::fromStdString(config::encryptString(arg1));
            } else if (data == "baidu_cloud") {
                extract_text::baidu_cloud_id = QString::fromStdString(config::encryptString(arg1));
            } else if (data == "offline_ocr") { // nothing
            }
        });
        connect(leKey, &QLineEdit::textChanged, this, [=](const QString &arg1){
            const QString data = cbb->currentData().toString();
            if (data == "tencent_cloud") {
                extract_text::tencent_cloud_key = QString::fromStdString(config::encryptString(arg1));
            } else if (data == "baidu_cloud") {
                extract_text::baidu_cloud_key = QString::fromStdString(config::encryptString(arg1));
            } else if (data == "offline_ocr") { // nothing
            }
        });
    }

    page->addSpacer();
    return page;
}

QWidget *SettingWindow::setupImageTranslateWidget()
{
    using namespace config;

    const auto page = new ScrollWidget();
    {
        auto cbb = new QComboBox();
        const auto list = ListImageTranslate();
        for (const auto& it : list) {
            QIcon icon(QString(":/icons/setting/image_recognition/%1").arg(it.first));
            cbb->addItem(icon, it.second, it.first);
        }
        cbb->setCurrentText(findKeyByValue(list, image_translate::use_engine));
        const auto form = page->addForm(tr("Image Translate"));
        form->addRow(LABEL(tr("Engine"), 175), cbb);

        #define NEW_LABEL_SOURCE LABEL(tr("Source language"), 175)
        #define NEW_LABEL_TARGET LABEL(tr("Target language"), 175)

        auto leID = new QLineEdit();
        auto leKey = new QLineEdit();
        leKey->setEchoMode(QLineEdit::PasswordEchoOnEdit);
        const auto form2 = page->addForm(tr("Private Account"));
        form2->addRow(new QLabel(tr("If empty, use the default token.")));
        form2->addRow(LABEL(tr("Secret ID"), 175), leID);
        form2->addRow(tr("Secret Key"), leKey);

        // Tencent
        auto source_tencent = new QComboBox();
        auto target_tencent = new QComboBox();
        const auto transTenentSourceList = transListTenent(true);
        const auto transTenentTargetList = transListTenent(false);
        for (const auto& it : transTenentSourceList) source_tencent->addItem(it.second, it.first);
        for (const auto& it : transTenentTargetList) target_tencent->addItem(it.second, it.first);
        source_tencent->setCurrentText(findKeyByValue(transTenentSourceList, image_translate::tencent_cloud_souce_language));
        target_tencent->setCurrentText(findKeyByValue(transTenentTargetList, image_translate::tencent_cloud_target_language));

        const auto form_tencent = page->addForm(tr("Tencent Cloud Language [%1]").arg(transTenentTargetList.size()));
        form_tencent->addRow(NEW_LABEL_SOURCE, source_tencent);
        form_tencent->addRow(NEW_LABEL_TARGET, target_tencent);

        // BaiDu
        auto source_baidu = new QComboBox();
        auto target_baidu = new QComboBox();
        const auto transBaiduSourceList = transListBaidu(true);
        const auto transBaiduTargetList = transListBaidu(false);
        for (const auto& it : transBaiduSourceList) source_baidu->addItem(it.second, it.first);
        for (const auto& it : transBaiduTargetList) target_baidu->addItem(it.second, it.first);
        source_baidu->setCurrentText(findKeyByValue(transBaiduSourceList, image_translate::baidu_cloud_souce_language));
        target_baidu->setCurrentText(findKeyByValue(transBaiduTargetList, image_translate::baidu_cloud_target_language));

        const auto form_baidu = page->addForm(tr("BaiDu Cloud Language [%1]").arg(transBaiduTargetList.size()));
        form_baidu->addRow(NEW_LABEL_SOURCE, source_baidu);
        form_baidu->addRow(NEW_LABEL_TARGET, target_baidu);

        // YouDao
        auto source_youdao = new QComboBox();
        auto target_youdao = new QComboBox();
        const auto transYoudaoSourceList = transListYoudao(true);
        const auto transYoudaoTargetList = transListYoudao(false);
        for (const auto& it : transYoudaoSourceList) source_youdao->addItem(it.second, it.first);
        for (const auto& it : transYoudaoTargetList) target_youdao->addItem(it.second, it.first);
        source_youdao->setCurrentText(findKeyByValue(transYoudaoSourceList, image_translate::youdao_cloud_souce_language));
        target_youdao->setCurrentText(findKeyByValue(transYoudaoTargetList, image_translate::youdao_cloud_target_language));

        const auto form_youdao = page->addForm(tr("YouDao Cloud Language [%1]").arg(transYoudaoTargetList.size()));
        form_youdao->addRow(NEW_LABEL_SOURCE, source_youdao);
        form_youdao->addRow(NEW_LABEL_TARGET, target_youdao);

        // connect
        connect(source_tencent, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](){ image_translate::tencent_cloud_souce_language  =  source_tencent->currentData().toString(); });
        connect(target_tencent, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](){ image_translate::tencent_cloud_target_language =  target_tencent->currentData().toString(); });
        connect(source_baidu,   QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](){ image_translate::baidu_cloud_souce_language    =  source_baidu->currentData().toString(); });
        connect(target_baidu,   QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](){ image_translate::baidu_cloud_target_language   =  target_baidu->currentData().toString(); });
        connect(source_youdao,  QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](){ image_translate::youdao_cloud_souce_language   =  source_youdao->currentData().toString(); });
        connect(target_youdao,  QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](){ image_translate::youdao_cloud_target_language  =  target_youdao->currentData().toString(); });

        connect(cbb, QOverload<const QString &>::of(&QComboBox::currentTextChanged), this, [=](){
            page->hideForm(form_tencent);
            page->hideForm(form_baidu);
            page->hideForm(form_youdao);

            QString id = "";
            QString key = "";
            const QString& data = cbb->currentData().toString();
            image_translate::use_engine = data;
            if (data == "tencent_cloud") {
                id = image_translate::tencent_cloud_id;
                key = image_translate::tencent_cloud_key;
                page->showForm(form_tencent);
            } else if (data == "baidu_cloud") {
                id = image_translate::baidu_cloud_id;
                key = image_translate::baidu_cloud_key;
                page->showForm(form_baidu);
            } else if (data == "youdao_cloud") {
                id = image_translate::youdao_cloud_id;
                key = image_translate::youdao_cloud_key;
                page->showForm(form_youdao);
            }

            leID->setText(config::decryptString(id.toStdString()));
            leKey->setText(config::decryptString(key.toStdString()));
        });

        emit cbb->currentTextChanged(cbb->currentText());

        connect(leID, &QLineEdit::textChanged, this, [=](const QString &arg1){
            const QString data = cbb->currentData().toString();
            if (data == "tencent_cloud") {
                image_translate::tencent_cloud_id = QString::fromStdString(config::encryptString(arg1));
            } else if (data == "baidu_cloud") {
                image_translate::baidu_cloud_id = QString::fromStdString(config::encryptString(arg1));
            } else if (data == "youdao_cloud") {
                image_translate::youdao_cloud_id = QString::fromStdString(config::encryptString(arg1));
            }
        });
        connect(leKey, &QLineEdit::textChanged, this, [=](const QString &arg1){
            const QString data = cbb->currentData().toString();
            if (data == "tencent_cloud") {
                image_translate::tencent_cloud_key = QString::fromStdString(config::encryptString(arg1));
            } else if (data == "baidu_cloud") {
                image_translate::baidu_cloud_key = QString::fromStdString(config::encryptString(arg1));
            } else if (data == "youdao_cloud") {
                image_translate::youdao_cloud_key = QString::fromStdString(config::encryptString(arg1));
            }
        });
    }

    page->addSpacer();
    return page;
}

QWidget *SettingWindow::setupHotkeyWidget()
{
    using namespace config;

    const auto page = new ScrollWidget();
    {
        const auto form = page->addForm(tr("Hotkeys"));

        const auto updateHotkey = [=](const QString& name, QKeySequence& sequence) {
            const auto edit = new QKeySequenceEdit(sequence);
            edit->setMaximumSequenceLength(1);
            if (edit->findChild<QLineEdit *>())
                edit->findChild<QLineEdit *>()->setContextMenuPolicy(Qt::NoContextMenu);

            connect(edit, &QKeySequenceEdit::keySequenceChanged, [&, edit](const QKeySequence & ks) {
                // fix: QKeySequenceEdit emits the signal twice: pressed and released
                // When the delete key is pressed, calling edit->clear() results in an additional emission of the signal.
                if (ks.isEmpty() || sequence == ks) return;

                if (ks == QKeySequence(Qt::Key_Delete) || ks == QKeySequence(Qt::Key_Backspace)) {
                    sequence = QKeySequence();
                    if (edit) edit->clear();
                } else {
                    sequence = ks;
                }
                App()->UpdateHotkeys();
            });
            form->addRow(LABEL(name, 175), edit);
        };

        updateHotkey(tr("Screenshot"), config::hotkeys::screenshot);
        updateHotkey(tr("Repeat Last Screenshot"), config::hotkeys::repeat_last_screenshot);
        updateHotkey(tr("Preview Clipboard"), config::hotkeys::preview);
        updateHotkey(tr("Toggle Previews"), config::hotkeys::toggle_previews);
#if _WIN32
        // updateHotkey(tr("Quick Look"), config::hotkeys::quick_look);
#endif
        updateHotkey(tr("Transparent Input"), config::hotkeys::transparent_input);
    }

    page->addSpacer();
    return page;
}

QWidget *SettingWindow::setupOutputWidget()
{
    using namespace config;


    const auto page = new ScrollWidget();
    {
        QWidget* w = new QWidget();
        uioutput_->setupUi(w);
        w->hide();

        auto ui = uioutput_;
        ui->sb_image_quality->setValue(output::image_quality);
        ui->le_manual_save_file_name->setText(output::ms_file_name);
        ui->le_manual_save_file_name_preview->setText(Helper::formatToFileName(output::ms_file_name));
        ui->cb_remeber_last_image_extension->setChecked(output::ms_remeber_last_image_extension);

        ui->le_quick_save_path->setText(output::qs_path);
        ui->le_quick_save_path_preview->setText(Helper::formatToFileName(output::qs_path));
        ui->cb_quick_save_show_notification->setChecked(output::qs_show_notification);

        ui->le_auto_save_path->setText(output::as_path);
        ui->le_auto_save_path_preview->setText(Helper::formatToFileName(output::as_path));

        const auto form = page->addForm(tr("Image"));
        form->addRow(ui->dlg_image_quality);

        const auto form1 = page->addForm(tr("Manual Save"));
        form1->addRow(ui->dlg_manual_save);

        const auto form2 = page->addForm(tr("Quick Save"));
        form2->addRow(ui->dlg_quick_save);

        const auto form_tencent = page->addForm(tr("Auto Save"));
        form_tencent->addRow(ui->dlg_auto_save);

        connect(ui->sb_image_quality, QOverload<int>::of(&QSpinBox::valueChanged), this, [](int val){ output::image_quality = val; });
        connect(ui->le_manual_save_file_name, &QLineEdit::textChanged, this, [this](const QString &val){ output::ms_file_name = val;
                    uioutput_->le_manual_save_file_name_preview->setText(Helper::formatToFileName(val)); });
        connect(ui->cb_remeber_last_image_extension, &QCheckBox::toggled, this, [](bool checked){ output::ms_remeber_last_image_extension = checked; });

        connect(ui->le_quick_save_path, &QLineEdit::textChanged, this, [this](const QString &text){     output::qs_path = text;
                    uioutput_->le_quick_save_path_preview->setText(Helper::formatToFileName(text));});
        connect(ui->cb_quick_save_show_notification, &QCheckBox::toggled, this, [](bool checked){ output::qs_show_notification = checked; });
        connect(ui->btn_quick_save_open_folder, &QToolButton::released, this, [](){ QFileInfo fileInfo(output::qs_path);
                    QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.absolutePath())); });
        connect(ui->btn_quick_save_change_folder, &QToolButton::released, this, [this](){ QFileInfo fileInfo(output::qs_path);
                    QString selectedDirectory = QFileDialog::getExistingDirectory(this, tr("Select Quick Save Directory"), fileInfo.absolutePath());
                    if (!selectedDirectory.isEmpty()) {
                        output::qs_path = QString(selectedDirectory + "/" + fileInfo.fileName());
                        emit uioutput_->le_quick_save_path->setText(output::qs_path);
                    }});

        connect(ui->le_auto_save_path, &QLineEdit::textChanged, this, [this](const QString &text){ output::as_path = text;
                    uioutput_->le_auto_save_path_preview->setText(Helper::formatToFileName(text)); });
        connect(ui->cb_auto_save_enable, &QCheckBox::toggled, this, [this](bool checked){ output::as_enable = checked;
                    uioutput_->lab_auto_save_path->setDisabled(!checked);
                    uioutput_->lab_auto_save_path_preview->setDisabled(!checked);
                    uioutput_->le_auto_save_path->setDisabled(!checked);
                    uioutput_->le_auto_save_path_preview->setDisabled(!checked);
                    uioutput_->btn_auto_save_open_folder->setDisabled(!checked);
                    uioutput_->btn_auto_save_change_folder->setDisabled(!checked);
                    });
        connect(ui->btn_auto_save_open_folder, &QToolButton::released, this, [](){ QFileInfo fileInfo(output::as_path);
                    QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.absolutePath())); });
        connect(ui->btn_auto_save_change_folder, &QToolButton::released, this, [this](){ QFileInfo fileInfo(output::as_path);
                    QString selectedDirectory = QFileDialog::getExistingDirectory(this, tr("Select Auto Save Directory"), fileInfo.absolutePath());
                    if (!selectedDirectory.isEmpty()) {
                        output::as_path = QString(selectedDirectory + "/" + fileInfo.fileName());
                        emit uioutput_->le_auto_save_path->setText(output::as_path);
                    }});
    }

    page->addSpacer();
    return page;
}

QWidget *SettingWindow::setupUpdateWidget()
{
    using namespace config;

    const auto page = new ScrollWidget();
    {
        auto ui = uiupdate_;
        QWidget* w = new QWidget();
        uiupdate_->setupUi(w);
        w->hide();

        ui->cbJoinInsiderProgram->setChecked(update::join_inside);
        ui->sbDay->setValue(update::day);

        ui->cbbProxyType->addItem(tr("No Proxy"), QVariant::fromValue(VersionUpdater::NoProxy));
        ui->cbbProxyType->addItem(tr("System Proxy"), QVariant::fromValue(VersionUpdater::SystemProxy));
        ui->cbbProxyType->addItem("HTTP", QVariant::fromValue(VersionUpdater::HttpProxy));
        ui->cbbProxyType->addItem("SOCKS5", QVariant::fromValue(VersionUpdater::Socks5Proxy));
        ui->leIP->setText(update::server);
        ui->sbPort->setValue(update::port);

        const auto form = page->addForm(tr("Check for updates on startup"));
        form->addRow(ui->dlg_check_update);

        const auto form1 = page->addForm(tr("Proxy"));
        form1->addRow(ui->dlg_proxy);

        const auto form2 = page->addForm(tr("Check update"));
        form2->addRow(ui->dlg_check_btn);

        // sub_update.ui
        connect(ui->cbAutoCheckUpdate, &QCheckBox::toggled, this, [this](bool checked){ update::enable_auto_check = checked;
            uiupdate_->cbJoinInsiderProgram->setDisabled(!checked);
            uiupdate_->sbDay->setDisabled(!checked);
        });
        connect(ui->sbDay, QOverload<int>::of(&QSpinBox::valueChanged), this, [](int d){ update::day = d; });
        connect(ui->cbJoinInsiderProgram, &QCheckBox::clicked, this, [](bool checked){ update::join_inside = checked; });

        connect(ui->cbbProxyType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingWindow::setServerIPStatus);
        connect(ui->cbbProxyType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingWindow::onSetProxy);
        connect(ui->leIP, &QLineEdit::editingFinished, this, &SettingWindow::onSetProxy);
        connect(ui->sbPort, QOverload<int>::of(&QSpinBox::valueChanged), this, &SettingWindow::onSetProxy);
        connect(ui->tbDownUrl, &QToolButton::released, this, [](){ QDesktopServices::openUrl(QUrl("https://sunnycapturer.xmuli.tech/")); });
        connect(ui->btnTest, &QPushButton::released, this, [](){
            const auto& url1 = "https://github.com/XMuli/SunnyCapturer/releases";
            // const auto& url2 = "https://baidu.com";
            // const auto& url3 = "https://www.google.com";
            QStringList urlsToTest = {url1/*, url2, url3*/};
            COMM.verUpdate()->testUrlConnectivity(urlsToTest);
        });
        connect(ui->btnCheckUpdate, &QPushButton::released, this, [](){ COMM.verUpdate()->checkForUpdate(false); });

        ui->cbAutoCheckUpdate->setChecked(update::enable_auto_check);
        ui->cbbProxyType->setCurrentIndex(update::type);
    }

    page->addSpacer();
    return page;
}

QWidget *SettingWindow::setupInterfaceWidget()
{
    const auto page = new ScrollWidget();
    {
        const auto form = page->addForm(tr("Appearance"));

        const auto bwidth = new QSpinBox();
        bwidth->setRange(1, 6);
        bwidth->setContextMenuPolicy(Qt::NoContextMenu);
        bwidth->setValue(config::snip::style.border_width);
        connect(bwidth, QOverload<int>::of(&QSpinBox::valueChanged),
                [](int w) { config::snip::style.border_width = w; });
        connect(bwidth, QOverload<int>::of(&QSpinBox::valueChanged),
                [] { App()->UpdateScreenshotStyle(); });
        form->addRow(LABEL(tr("Border Width"), 175), bwidth);

        const auto bcolor = new ColorDialogButton(config::snip::style.border_color);
        connect(bcolor, &ColorDialogButton::changed,
                [](auto&& c) { config::snip::style.border_color = c; });
        connect(bcolor, &ColorDialogButton::changed, [] { App()->UpdateScreenshotStyle(); });
        form->addRow(tr("Border Color"), bcolor);

        const auto bstyle = new ComboBox();
        bstyle->add(PENSTYLES);
        bstyle->onselected([&](auto value) {
            config::snip::style.border_style = static_cast<Qt::PenStyle>(value.toInt());
        });
        connect(bstyle, &ComboBox::selected, [] { App()->UpdateScreenshotStyle(); });
        bstyle->select(static_cast<int>(config::snip::style.border_style));
        form->addRow(tr("Border Style"), bstyle);

        const auto mcolor = new ColorDialogButton(config::snip::style.mask_color);
        connect(mcolor, &ColorDialogButton::changed, [&](auto&& c) { config::snip::style.mask_color = c; });
        connect(mcolor, &ColorDialogButton::changed, [] { App()->UpdateScreenshotStyle(); });
        form->addRow(tr("Mask Color"), mcolor);
    }

    page->addSpacer();
    return page;
}

QWidget *SettingWindow::setupAboutWidget()
{
    const auto page    = new QWidget();
    const auto vlayout = new QVBoxLayout();
    vlayout->setContentsMargins(35, 0, 35, 15);

    {
        vlayout->addStretch(1);

        const auto icon = new QLabel();
        icon->setPixmap(QPixmap(":/logo/logo.svg"));
        icon->setAlignment(Qt::AlignCenter);
        vlayout->addWidget(icon);

        const auto name = new QLabel(XPROJECT_NAME);
        name->setAlignment(Qt::AlignCenter);
        name->setObjectName("about-name");
        vlayout->addWidget(name);

        QString szVersion = tr("Version : %1 (%2)").arg(XPROJECT_VERSION).arg(XBUILD_TIME);
        #ifdef BUILD_MICROSOFT_STORE
            szVersion += "🎉";
        #endif
        auto version = new QLabel(szVersion);
        version->setAlignment(Qt::AlignCenter);
        version->setObjectName("about-version");
        vlayout->addWidget(version);

        const auto feekback = new QLabel();
        feekback->setTextFormat(Qt::RichText);
        feekback->setText(QString("<a href=\"%1\">%2</a>&nbsp;&nbsp;&nbsp;|&nbsp;&nbsp;&nbsp;<a href=\"%3\">%4</a>")
                              .arg("https://sunnycapturer.xmuli.tech/#/translation")
                              .arg(tr("Translations"))
                              .arg("https://github.com/XMuli/SunnyCapturer/issues")
                              .arg(tr("Feekback")));
        feekback->setAlignment(Qt::AlignCenter);
        feekback->setOpenExternalLinks(true);
        feekback->setObjectName("about-feekback");
        vlayout->addWidget(feekback);

        vlayout->addStretch(2);

        const auto copyright = new QLabel(
            "Copyright © 2023-2025 <a href=\"https://sunnycapturer.xmuli.tech\">sunnycapturer.xmuli.tech</a>");
        copyright->setAlignment(Qt::AlignCenter);
        copyright->setOpenExternalLinks(true);
        copyright->setObjectName("about-copyright");
        vlayout->addWidget(copyright);
    }
    page->setLayout(vlayout);

    return page;
}

void SettingWindow::setServerIPStatus()
{
    auto ui = uiupdate_;
    VersionUpdater::ProxyType proxyType = ui->cbbProxyType->currentData().value<VersionUpdater::ProxyType>();
    bool disable = (proxyType == VersionUpdater::NoProxy || proxyType == VersionUpdater::SystemProxy) ? true : false;
    ui->leIP->setDisabled(disable);
    ui->sbPort->setDisabled(disable);
}

// 0-NoProxy  1-SystemProxy  2-HTTPS  3-SOCKET5
void SettingWindow::onSetProxy()
{
    auto ui = uiupdate_;
    VersionUpdater::ProxyType proxyType = ui->cbbProxyType->currentData().value<VersionUpdater::ProxyType>();
    QString ip = ui->leIP->text();
    int port = ui->sbPort->value();

    COMM.verUpdate()->setProxy(proxyType, ip, port);
    setServerIPStatus();
    config::update::type = int(proxyType);
    config::update::server = ip;
    config::update::port = port;
}

void SettingWindow::onExtractTextLineEdit()
{

}

