#include "AboutDialog.h"
#include "ui_AboutDialog.h"
#include <QtGlobal>

#define STRINGIFY_(X) #X
#define STRINGIFY(X) STRINGIFY_( X )

AboutDialog::AboutDialog( QWidget* parent )
    : QDialog( parent )
    , m_pUI( new Ui::AboutDialog )
{
    m_pUI->setupUi(this);

    QString version =
            QString( "v" )
            + STRINGIFY( XARCHIVEMANAGER_VERSION );

    m_pUI->versionLabel->setText( version );

    QString qtVersion =
            QString( "This program uses Qt version " )
            + STRINGIFY( QT_VERSION_MAJOR ) + '.'
            + STRINGIFY( QT_VERSION_MINOR ) + '.'
            + STRINGIFY( QT_VERSION_PATCH );

    m_pUI->qtVersionLabel->setText( qtVersion );
}

AboutDialog::~AboutDialog()
{
    delete m_pUI;
}
