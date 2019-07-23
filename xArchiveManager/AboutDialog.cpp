#include "AboutDialog.h"
#include "ui_AboutDialog.h"

AboutDialog::AboutDialog( QWidget* parent )
    : QDialog( parent )
    , m_pUI( new Ui::AboutDialog )
{
    m_pUI->setupUi(this);
}

AboutDialog::~AboutDialog()
{
    delete m_pUI;
}
