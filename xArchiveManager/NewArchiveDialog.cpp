#include "NewArchiveDialog.h"
#include "ui_NewArchiveDialog.h"

NewArchiveDialog::NewArchiveDialog( QWidget* parent )
    : QDialog( parent )
    , m_pUI( new Ui::NewArchiveDialog )
{
    m_pUI->setupUi( this );
}

NewArchiveDialog::~NewArchiveDialog()
{
    delete m_pUI;
}

QString NewArchiveDialog::getSelectedFilename() const
{
    return m_pUI->filenameLineEdit->text();
}
