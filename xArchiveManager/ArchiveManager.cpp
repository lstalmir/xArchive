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

QStringList ArchiveManager::getDirectoryContents( QString directory )
{
    if( m_pCurrentArchive )
    {
        auto contents = m_pCurrentArchive->ListDirectory( directory.toStdString() );

        QStringList dir;
        for( auto file : contents )
            dir.push_back( QString( file.c_str() ) );

        return dir;
    }
    return {};
}
