#include "ArchiveManagerWindow.h"
#include <QApplication>

int main( int argc, char *argv[] )
{
    QApplication a( argc, argv );

    ArchiveManagerWindow w;
    w.show();

    return a.exec();
}
