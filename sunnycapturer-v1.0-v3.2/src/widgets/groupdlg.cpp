// SPDX-SnippetCopyrightText: 2024-2025 SunnyCapturer Project
// SPDX-Author: Wengeng Zhang <xmulitech@gmail.com>

#include "groupdlg.h"
#include <QList>
#include <QDebug>
#include <QCheckBox>
#include <QAbstractButton>

GroupDlg::GroupDlg(QWidget *parent)
    : QWidget{parent}
    , m_group(new QButtonGroup(this))
{
}

void GroupDlg::setExclusive(bool b)
{
    m_group->setExclusive(b);
    bool isFirst = b ? true : false;
    int idx = 0;
    for (auto& it : findChildren<QCheckBox*>()) {
        m_group->addButton(it, idx++);
        it->setCheckable(true);
        it->setChecked(isFirst);
        isFirst = false;
    }

    connect(m_group, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonReleased), this, [this](QAbstractButton* button){
        emit idToggled(m_group->id(button), button->isChecked());
    });
}
