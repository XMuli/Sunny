#ifndef UPDATEUI_H
#define UPDATEUI_H

#include <QWidget>

namespace Ui
{
    class UpdateUI;
}

struct VersionData
{
    // 默认构造函数
    VersionData()
        : name(""),
          url(""),
          time(""),
          size(0),
          isPre(false),
          isSelected(false)
    {}

           // 带参数的构造函数
    VersionData(const QString& name,
                const QString& url,
                const QString& time,
                int64_t size,
                bool isPre,
                bool isSelected)
        : name(name),
          url(url),
          time(time),
          size(size),
          isPre(isPre),
          isSelected(isSelected)
    {}

           // 成员变量
    QString name;
    QString url;
    QString time;
    int     size;   // MB
    bool    isPre;  // true: beta; false: release
    bool    isSelected;
};


class UpdateUI : public QWidget
{
    Q_OBJECT

public:
    explicit UpdateUI(QWidget *parent = nullptr);
    ~UpdateUI();

    void setSelectDownVersion(const std::vector<VersionData> &vec);
    void setLatestVersionChanglog(const QString& text);
    void setLatestVersionLabel(const QString& curVerson, const QString& curVersonTime, const QString& newVerson, const QString& newVersonTime);
    void setLastCheckTime(const QString time = "");

signals:
    void sigRecheked();

private slots:
    void on_btn_down_and_install_released();
    void on_btn_cancel_released();

    void on_btb_recheck_released();

private:
    Ui::UpdateUI *ui;
    std::vector<VersionData> m_vec;
};

#endif // UPDATEUI_H
