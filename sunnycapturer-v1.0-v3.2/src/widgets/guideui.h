// SPDX-SnippetCopyrightText: 2024-2025 SunnyCapturer Project
// SPDX-Author: Wengeng Zhang <xmulitech@gmail.com>
#ifndef GUIDEUI_H
#define GUIDEUI_H

#include <QWidget>
#include "selector.h"

namespace Ui
{
    class GuideUI;
}

class GuideUI : public QWidget
{
    Q_OBJECT

public:
    explicit GuideUI(QWidget *parent = nullptr);
    ~GuideUI();

    void setStatus(const SelectorStatus &newStatus);

private:
    void hideAllGuideText();
    void initUI();
    void autoShowGuide();

protected:
    void paintEvent(QPaintEvent *e) override;

private:
    Ui::GuideUI *ui;
    SelectorStatus status_{ SelectorStatus::INVALID };
};

#endif // GUIDEUI_H
