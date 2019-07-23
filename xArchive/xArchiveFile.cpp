#include "xArchiveFile.h"
#include <algorithm>
#include <stdexcept>
#include <zlib.h>

namespace xArchive
{
    ArchiveFile::ArchiveFile( const std::string& filename, ArchiveFileOpenMode mode )
        : m_Filename( filename )
        , m_Mode( mode )
    {
    }

    ArchiveFile::~ArchiveFile()
    {
    }

    std::string ArchiveFile::Name() const
    {
        return m_Filename;
    }

    ArchiveFileOpenMode ArchiveFile::Mode() const
    {
        return m_Mode;
    }


    UncompressedArchiveFile::UncompressedArchiveFile( const std::string& filename, ArchiveFileOpenMode mode )
        : ArchiveFile( filename, mode )
        , m_pFile( nullptr )
    {
        const char* filename_ = filename.c_str();
        const char* mode_ = nullptr;

        switch( mode )
        {
        case ArchiveFileOpenMode::eReadOnly: mode_ = "rb"; break;
        case ArchiveFileOpenMode::eWriteOnly: mode_ = "wb"; break;
        case ArchiveFileOpenMode::eReadWrite: mode_ = "r+b"; break;
        default: throw std::invalid_argument( "Unsupported open mode" );
        }

        if( mode == ArchiveFileOpenMode::eReadWrite )
        {
            // The file may not exist now, create it without trunctation
            fopen_s( &m_pFile, filename_, "a" );

            if( !m_pFile )
                throw std::runtime_error( "Cannot open archive file" );

            fclose( m_pFile );
        }

        fopen_s( &m_pFile, filename_, mode_ );

        if( !m_pFile )
            throw std::runtime_error( "Cannot open archive file" );
    }

    UncompressedArchiveFile::~UncompressedArchiveFile()
    {
        Close();
    }

    void UncompressedArchiveFile::Write( const void* data, size_t size )
    {
        fwrite( data, 1, size, m_pFile );
    }

    void UncompressedArchiveFile::Read( void* buffer, size_t size )
    {
        fread_s( buffer, size, 1, size, m_pFile );
    }

    void UncompressedArchiveFile::Seek( ptrdiff_t offset, int mode )
    {
        fseek( m_pFile, static_cast<long>(offset), mode );
    }

    size_t UncompressedArchiveFile::Tell() const
    {
        return ftell( m_pFile );
    }

    void UncompressedArchiveFile::Flush()
    {
        fflush( m_pFile );
    }

    void UncompressedArchiveFile::Close()
    {
        if( m_pFile ) fclose( m_pFile );
        m_pFile = nullptr;
    }


    CompressedArchiveFile::CompressedArchiveFile( const std::string& filename, ArchiveFileOpenMode mode )
        : ArchiveFile( filename, mode )
        , m_pBuffer()
        , m_PointerOffset( 0 )
        , m_IsOpen( true )
    {
        const char* filename_ = filename.c_str();

        if( mode == ArchiveFileOpenMode::eReadWrite )
        {
            // The file may not exist now, create it without trunctation
            gzFile tmp_ = gzopen( filename_, "ab" );
            gzclose( tmp_ );
        }

        if( mode == ArchiveFileOpenMode::eWriteOnly )
        {
            // Contents will be overwritten anyway
            return;
        }

        size_t uncompressedSize = 0;

        // Read and uncompress data from gz file
        {
            gzFile file = gzopen( filename_, "rb" );

            // Temporary buffer used to determine size of uncompressed data
            std::vector<char> tmp_( static_cast<size_t>(1024 * 1024) );

            while( !gzeof( file ) )
            {
                int bytesDecompressed = gzread( file, tmp_.data(), static_cast<uint32_t>(tmp_.size()) );

                if( bytesDecompressed <= 0 )
                {
                    if( gzeof( file ) )
                        break;

                    throw std::runtime_error( "Error while reading archive file" );
                }

                uncompressedSize += bytesDecompressed;
            }

            m_pBuffer.resize( uncompressedSize );

            gzrewind( file );
            gzread( file, m_pBuffer.data(), static_cast<unsigned int>(uncompressedSize) );
            gzclose( file );
        }
    }

    CompressedArchiveFile::~CompressedArchiveFile()
    {
        Close();
    }

    void CompressedArchiveFile::Write( const void* data, size_t size )
    {
        if( m_PointerOffset + size > m_pBuffer.size() )
        {
            m_pBuffer.resize( m_PointerOffset + size );
        }

        std::memcpy( m_pBuffer.data() + m_PointerOffset, data, size );

        m_PointerOffset += size;
    }

    void CompressedArchiveFile::Read( void* buffer, size_t size )
    {
        std::memcpy( buffer, m_pBuffer.data() + m_PointerOffset,
            std::min( size, m_pBuffer.size() - m_PointerOffset ) );

        m_PointerOffset += size;
    }

    void CompressedArchiveFile::Seek( ptrdiff_t offset, int mode )
    {
        switch( mode )
        {
        case SEEK_SET: m_PointerOffset = offset; return;
        case SEEK_CUR: m_PointerOffset = m_PointerOffset + offset; return;
        case SEEK_END: m_PointerOffset = m_pBuffer.size() + offset; return;
        }

        if( m_PointerOffset > m_pBuffer.size() )
        {
            m_pBuffer.resize( m_PointerOffset );
        }
    }

    size_t CompressedArchiveFile::Tell() const
    {
        return m_PointerOffset;
    }

    void CompressedArchiveFile::Flush()
    {
    }

    void CompressedArchiveFile::Close()
    {
        if( !m_IsOpen || m_Mode == ArchiveFileOpenMode::eReadOnly )
            return;

        gzFile file = gzopen( m_Filename.c_str(), "wb" );

        gzwrite( file, m_pBuffer.data(), static_cast<uint32_t>(m_pBuffer.size()) );

        gzflush( file, Z_FINISH );
        gzclose( file );
    }
}
