#pragma once
#include <QDialog>

namespace Ui
{
    class NewArchiveDialog;
}

class NewArchiveDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewArchiveDialog( QWidget* parent = nullptr );
    ~NewArchiveDialog();

    QString getSelectedFilename() const;

private:
    Ui::NewArchiveDialog* m_pUI;
};

