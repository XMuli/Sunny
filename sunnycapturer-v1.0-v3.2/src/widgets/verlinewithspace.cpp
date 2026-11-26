// SPDX-SnippetCopyrightText: 2024-2025 SunnyCapturer Project
// SPDX-Author: Wengeng Zhang <xmulitech@gmail.com>
#include "verlinewithspace.h"
#include <QHBoxLayout>
#include <qguiapplication.h>
#include "verspacerline.h"

VerLineWithSpace::VerLineWithSpace(QWidget *parent)
    : QWidget{parent}
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    const int space = 4;
    layout->addSpacing(space);
    layout->addWidget(new VerSpacerLine(20));
    layout->addSpacing(space);
}
