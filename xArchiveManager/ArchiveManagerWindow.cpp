#include "ArchiveManagerWindow.h"
#include "ui_archivemanagerwindow.h"
#include "NewArchiveDialog.h"
#include "AboutDialog.h"
#include <QFileDialog>

ArchiveManagerWindow::ArchiveManagerWindow( QWidget* parent )
    : QMainWindow( parent )
    , m_pUI( new Ui::ArchiveManagerWindow )
    , m_Manager( this )
{
    m_pUI->setupUi( this );
    m_WindowTitleBase = this->windowTitle();
}

ArchiveManagerWindow::~ArchiveManagerWindow()
{
    delete m_pUI;
}

void ArchiveManagerWindow::openArchive()
{
    QFileDialog dialog( this );

    if( dialog.exec() == QDialog::Accepted )
    {
        QString filename = dialog.selectedFiles().first();

        this->statusBar()->showMessage( "Opening " + filename + "..." );

        m_Manager.openArchive( filename );

        this->statusBar()->showMessage( "Opened " + filename, 3000 );
        this->setWindowTitle( m_WindowTitleBase + " - " + filename );
    }
}

void ArchiveManagerWindow::createNewArchive()
{
    NewArchiveDialog dialog( this );

    if( dialog.exec() == QDialog::Accepted )
    {
        QString filename = dialog.getSelectedFilename();

        m_Manager.createNewArchive( dialog.getSelectedFilename() );

        this->statusBar()->showMessage( "Created " + filename, 3000 );
        this->setWindowTitle( m_WindowTitleBase + " - " + filename );
    }
}

void ArchiveManagerWindow::showAbout()
{
    AboutDialog( this ).exec();
}
