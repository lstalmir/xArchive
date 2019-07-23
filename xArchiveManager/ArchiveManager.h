#pragma once
#include "xArchive.h"
#include <QObject>
#include <QSharedPointer>

class ArchiveManager : public QObject
{
    Q_OBJECT

public:
    explicit ArchiveManager( QObject* parent = nullptr );

    void createNewArchive( QString filename );
    void openArchive( QString filename );

signals:

public slots:

private:
    QSharedPointer< xArchive::Archive > m_pCurrentArchive;
};
