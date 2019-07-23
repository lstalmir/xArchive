#include "ArchiveManager.h"

ArchiveManager::ArchiveManager( QObject* parent )
    : QObject( parent )
{

}

void ArchiveManager::createNewArchive( QString filename )
{
    m_pCurrentArchive.reset( xArchive::Archive::Create( filename.toStdString() ) );
}

void ArchiveManager::openArchive (QString filename )
{
    m_pCurrentArchive.reset( xArchive::Archive::Open( filename.toStdString() ) );
}
