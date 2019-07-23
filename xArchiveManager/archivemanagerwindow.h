#pragma once
#include "ArchiveManager.h"
#include <QMainWindow>

namespace Ui
{
    class ArchiveManagerWindow;
}

class ArchiveManagerWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ArchiveManagerWindow( QWidget* parent = nullptr );
    ~ArchiveManagerWindow();

public slots:
    void openArchive();
    void createNewArchive();
    void showAbout();

private:
    Ui::ArchiveManagerWindow* m_pUI;
    ArchiveManager m_Manager;

    QString m_WindowTitleBase;
};
