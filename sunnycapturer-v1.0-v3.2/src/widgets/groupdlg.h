// SPDX-SnippetCopyrightText: 2024-2025 SunnyCapturer Project
// SPDX-Author: Wengeng Zhang <xmulitech@gmail.com>

#ifndef GROUPDLG_H
#define GROUPDLG_H

#include <QWidget>
#include <QButtonGroup>

class GroupDlg : public QWidget
{
    Q_OBJECT
public:
    explicit GroupDlg(QWidget *parent = nullptr);
    void setExclusive(bool b = true);

signals:
    void idToggled(int id, bool checked);

private:
    QButtonGroup* m_group;
};

#endif // GROUPDLG_H
