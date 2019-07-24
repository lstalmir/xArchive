#include "xArchive.h"
#include <algorithm>
#include <fstream>
#include <sstream>

#pragma pack( 1 )

namespace xArchive
{
    Archive* Archive::Open( const std::string& filename, ArchiveOpenFlags flags )
    {
        ArchiveFileOpenMode mode = ArchiveFileOpenMode::eReadOnly;

        if( !(static_cast<int>(flags) & static_cast<int>(ArchiveOpenFlags::eReadonly)) )
            mode = ArchiveFileOpenMode::eReadWrite;

        return new Archive( filename, mode );
    }

    Archive* Archive::Create( const std::string& filename, uint32_t allocationSize )
    {
        UniqueArchiveFile file = std::make_unique<CompressedArchiveFile>( filename, ArchiveFileOpenMode::eWriteOnly );
        UniqueArchiveHeader header = std::make_unique<ArchiveHeader>();

        header->Magic = ArchiveMagic::eArchive;
        header->AllocationSize = allocationSize;
        memset( header->AllocationTable, 0, sizeof( header->AllocationTable ) );

        header->Root.Magic = ArchiveMagic::eDirectory;
        header->Root.Parent = 0;
        header->Root.Next = 0;
        header->Root.NumEntries = 0;
        memset( header->Root.Entries, 0, sizeof( header->Root.Entries ) );

        file->Write( header.get(), sizeof( ArchiveHeader ) );
        file->Close();

        return new Archive( filename, ArchiveFileOpenMode::eReadWrite );
    }

    Archive::Archive( const std::string& filename, ArchiveFileOpenMode mode )
        : m_pArchiveFile( nullptr )
        , m_Mode( mode )
        , m_pAllocator( nullptr )
        , m_pHeader( nullptr )
        , m_pCurrentDirectory( nullptr )
        , m_CurrentDirectoryPath( "/" )
        , m_pDirectoryFree( std::bind( &Archive::_FreeNonRoot, this, std::placeholders::_1 ) )
    {
        m_pArchiveFile = std::make_unique<CompressedArchiveFile>( filename, mode );

        m_pHeader = std::make_unique<ArchiveHeader>();
        m_pArchiveFile->Read( m_pHeader.get(), sizeof( ArchiveHeader ) );

        if( m_pHeader->Magic != ArchiveMagic::eArchive )
            throw std::runtime_error( (filename + " is not archive").c_str() );

        if( m_pHeader->Root.Magic != ArchiveMagic::eDirectory )
            throw std::runtime_error( (filename + " is corrupted").c_str() );

        m_pCurrentDirectory = SharedArchiveDirectory( &m_pHeader->Root, m_pDirectoryFree );
        m_CurrentDirectoryOffset = OffsetOf( ArchiveHeader, Root );

        AllocatorCallbacks allocatorCallbacks;
        allocatorCallbacks.pfnFlushAllocationTable = &Archive::_AllocationTableUpdated;
        allocatorCallbacks.pfnReallocateEntry = &Archive::_ReallocationHandler;

        m_pAllocator = std::make_unique<ArchiveAllocator>(
            this,
            m_pHeader->AllocationTable,
            static_cast<uint32_t>(ExtentOf( ElementOf( ArchiveHeader, AllocationTable ) )),
            m_pHeader->AllocationSize,
            allocatorCallbacks );

        m_pAllocator->SetAllocationBase( sizeof( ArchiveHeader ) );
    }

    Archive::ArchiveEntry::ArchiveEntry()
        : Name()
        , Offset( 0 )
        , Size( 0 )
        , Type( ArchiveEntryType( -1 ) )
    {
        memset( Name, 0, sizeof( Name ) );
    }

    Archive::ArchiveEntry::ArchiveEntry( const std::string& name, uint32_t offset, uint32_t size, ArchiveEntryType type )
        : Name()
        , Offset( offset )
        , Size( size )
        , Type( type )
    {
        StringToArray( name, Name );
    }

    Archive::ArchiveDirectoryEntry::ArchiveDirectoryEntry( const std::string& name, uint32_t offset )
        : ArchiveEntry( name, offset, sizeof( ArchiveDirectory ), ArchiveEntryType::eDirectory )
    {
    }

    Archive::ArchiveFileEntry::ArchiveFileEntry( const std::string& name, uint32_t offset, uint32_t size )
        : ArchiveEntry( name, offset, size, ArchiveEntryType::eFile )
    {
    }

    Archive::ArchiveDirectory::ArchiveDirectory( uint32_t parent )
        : Magic( ArchiveMagic::eDirectory )
        , Parent( parent )
        , Next( 0 )
        , NumEntries( 0 )
        , Entries()
    {
        memset( Entries, 0, sizeof( Entries ) );
    }

    void Archive::ArchiveDirectory::AddEntry( const ArchiveEntry& entry )
    {
        if( NumEntries == ExtentOf( Entries ) )
            throw std::runtime_error( "Out of memory" );

        Entries[NumEntries] = entry;

        NumEntries++;
    }

    void Archive::ArchiveDirectory::RemoveEntry( uint32_t n )
    {
        if( NumEntries == 0 )
            throw std::runtime_error( "No entries in directory" );

        if( n >= NumEntries )
            throw std::out_of_range( "Entry index out of range" );

        memmove( &Entries[n], &Entries[n + 1], SizeOfElement( Entries ) * (NumEntries - n) );

        NumEntries--;
    }

    Archive::ArchiveEntry& Archive::ArchiveDirectory::GetEntry( uint32_t n )
    {
        if( n >= NumEntries )
            throw std::out_of_range( "Entry index out of range" );

        return Entries[n];
    }

    const Archive::ArchiveEntry& Archive::ArchiveDirectory::GetEntry( uint32_t n ) const
    {
        if( n >= NumEntries )
            throw std::out_of_range( "Entry index out of range" );

        return Entries[n];
    }

    bool Archive::ArchiveDirectory::HasFreeSpace() const
    {
        return NumEntries < ExtentOf( Entries );
    }

    void Archive::CreateDirectory( const std::string& path )
    {
        _CheckWrite();

        auto components = StringSplit( path, "/" );

        const std::string entryName = components.back();

        // Remove last component from the path
        components.pop_back();

        std::string path_ = "";
        if( !StringStartsWith( path, "/" ) )
            path_ = "./";

        path_.append( StringJoin( components, "/" ) );

        // Get directory entry of the parent
        auto parentDirectoryOffset = _GetDirectoryOffset( path_ );
        auto parentDirectory = _ReadDirectory( parentDirectoryOffset );

        while( !parentDirectory->HasFreeSpace() )
        {
            if( parentDirectory->Next != 0 )
            {
                parentDirectoryOffset = parentDirectory->Next;
                parentDirectory = _ReadDirectory( parentDirectory->Next );
                continue;
            }

            uint32_t allocationOffset = m_pAllocator->Allocate( sizeof( ArchiveDirectory ) );

            parentDirectory->Next = allocationOffset;

            m_pArchiveFile->Seek( parentDirectoryOffset );
            m_pArchiveFile->Write( parentDirectory.get(), sizeof( ArchiveDirectory ) );

            auto parentDirectoryExt = SharedArchiveDirectory(
                new ArchiveDirectory( parentDirectory->Parent ), m_pDirectoryFree );

            parentDirectoryOffset = allocationOffset;
            parentDirectory = parentDirectoryExt;
        }

        uint32_t directoryAllocationOffset = m_pAllocator->Allocate( sizeof( ArchiveDirectory ) );

        auto directory = SharedArchiveDirectory(
            new ArchiveDirectory( parentDirectoryOffset ), m_pDirectoryFree );

        parentDirectory->AddEntry( ArchiveDirectoryEntry( entryName, directoryAllocationOffset ) );

        m_pArchiveFile->Seek( parentDirectoryOffset );
        m_pArchiveFile->Write( parentDirectory.get(), sizeof( ArchiveDirectory ) );
        m_pArchiveFile->Seek( directoryAllocationOffset );
        m_pArchiveFile->Write( directory.get(), sizeof( ArchiveDirectory ) );
        m_pArchiveFile->Flush();

        if( parentDirectoryOffset == OffsetOf( ArchiveHeader, Root ) )
        {
            // Header has been invalidated
            std::memcpy( &m_pHeader->Root, &*parentDirectory, sizeof( ArchiveDirectory ) );
        }

        if( parentDirectoryOffset == m_CurrentDirectoryOffset )
        {
            // Current directory has been invalidated
            m_pCurrentDirectory = parentDirectory;
        }
    }

    void Archive::RemoveDirectory( const std::string& path )
    {
        (path);
    }

    void Archive::SetCurrentDirectory( const std::string& path )
    {
        _CheckRead();

        const uint32_t dirOffset = _GetDirectoryOffset( path );

        m_CurrentDirectoryOffset = dirOffset;
        m_pCurrentDirectory = _ReadDirectory( dirOffset );

        if( StringStartsWith( path, "/" ) )
            m_CurrentDirectoryPath = path;
        else
            m_CurrentDirectoryPath.append( path + "/" );

        _NormalizeCurrentDirectoryPath();
    }

    std::string Archive::GetCurrentDirectory() const
    {
        return m_CurrentDirectoryPath;
    }

    std::vector<std::string> Archive::ListDirectory( const std::string& path )
    {
        _CheckRead();

        std::vector<std::string> entries;

        auto currentDirectory = _GetDirectory( path );
        while( currentDirectory )
        {
            for( uint32_t i = 0; i < currentDirectory->NumEntries; ++i )
                entries.push_back( currentDirectory->Entries[i].Name );

            currentDirectory = _ReadDirectory( currentDirectory->Next );
        }

        return entries;
    }

    void Archive::CreateFile( const std::string& path, const void* data, size_t size )
    {
        _CheckWrite();

        auto components = StringSplit( path, "/" );

        const std::string entryName = components.back();

        // Remove last component from the path
        components.pop_back();

        std::string path_ = "";
        if( !StringStartsWith( path, "/" ) )
            path_ = "./";

        path_.append( StringJoin( components, "/" ) );

        // Get directory entry of the parent
        auto parentDirectoryOffset = _GetDirectoryOffset( path_ );
        auto parentDirectory = _ReadDirectory( parentDirectoryOffset );

        while( !parentDirectory->HasFreeSpace() )
        {
            if( parentDirectory->Next != 0 )
            {
                parentDirectoryOffset = parentDirectory->Next;
                parentDirectory = _ReadDirectory( parentDirectory->Next );
                continue;
            }

            uint32_t allocationOffset = m_pAllocator->Allocate( sizeof( ArchiveDirectory ) );

            parentDirectory->Next = allocationOffset;

            m_pArchiveFile->Seek( parentDirectoryOffset );
            m_pArchiveFile->Write( parentDirectory.get(), sizeof( ArchiveDirectory ) );

            auto parentDirectoryExt = SharedArchiveDirectory(
                new ArchiveDirectory( parentDirectory->Parent ), m_pDirectoryFree );

            parentDirectoryOffset = allocationOffset;
            parentDirectory = parentDirectoryExt;
        }

        uint32_t fileAllocationOffset = m_pAllocator->Allocate( static_cast<uint32_t>(size) );

        parentDirectory->AddEntry( ArchiveFileEntry( entryName, fileAllocationOffset, static_cast<uint32_t>(size) ) );

        m_pArchiveFile->Seek( parentDirectoryOffset );
        m_pArchiveFile->Write( parentDirectory.get(), sizeof( ArchiveDirectory ) );
        m_pArchiveFile->Seek( fileAllocationOffset, std::fstream::beg );
        m_pArchiveFile->Write( data, size );
        m_pArchiveFile->Flush();

        if( parentDirectoryOffset == OffsetOf( ArchiveHeader, Root ) )
        {
            // Header has been invalidated
            memcpy( &m_pHeader->Root, &*parentDirectory, sizeof( ArchiveDirectory ) );
        }

        if( parentDirectoryOffset == m_CurrentDirectoryOffset )
        {
            // Current directory has been invalidated
            m_pCurrentDirectory = parentDirectory;
        }
    }

    void Archive::UpdateFile( const std::string& path, const void* data, size_t size )
    {
        (path, data, size);
    }

    void Archive::RemoveFile( const std::string& path )
    {
        (path);
    }

    size_t Archive::GetFileSize( const std::string& path )
    {
        _CheckRead();
        auto entry = _GetEntry( path );

        return static_cast<size_t>(entry.Size);
    }

    void Archive::ReadFile( const std::string& path, void* buffer, size_t bufferSize )
    {
        _CheckRead();
        auto entry = _GetEntry( path );

        // Check if provided buffer is sufficient
        if( bufferSize < entry.Size )
        {
            std::stringstream stringBuilder;
            stringBuilder
                << "Insufficient buffer. The buffer is to small "
                "(" << bufferSize << "B) to store file " << path.c_str() <<
                "(" << entry.Size << "B) in it.";

            throw std::invalid_argument( stringBuilder.str() );
        }

        m_pArchiveFile->Seek( entry.Offset );
        m_pArchiveFile->Read( buffer, entry.Size );

        // Fill remaining bytes in buffer with 0
        memset( reinterpret_cast<char*>(buffer) + entry.Size, 0, bufferSize - entry.Size );
    }

    std::vector<char> Archive::ReadFile( const std::string& path )
    {
        // Get file byte size
        const size_t fileSize = GetFileSize( path );

        std::vector<char> fileBuffer;
        fileBuffer.resize( fileSize );

        // Read bytes
        ReadFile( path, fileBuffer.data(), fileSize );

        return fileBuffer;
    }

    Archive::ArchiveEntry Archive::_GetEntry( const std::string& path )
    {
        auto components = StringSplit( path, "/" );

        const std::string entryName = components.back();

        // Remove last component from the path
        components.pop_back();

        std::string path_ = "";
        if( !StringStartsWith( path, "/" ) )
            path_ = "./";

        path_.append( StringJoin( components, "/" ) );

        // Get directory entry of the parent
        SharedArchiveDirectory parentDirectory = _GetDirectory( path_ );

        for( uint32_t i = 0; i < parentDirectory->NumEntries; ++i )
        {
            if( parentDirectory->Entries[i].Name == entryName )
            {
                return parentDirectory->Entries[i];
            }
        }

        throw std::invalid_argument( (entryName + " not found").c_str() );
    }

    uint32_t Archive::_GetDirectoryOffset( const std::string& path )
    {
        std::string path_ = path;

        uint32_t currentDirectoryOffset = m_CurrentDirectoryOffset;
        SharedArchiveDirectory currentDirectory = m_pCurrentDirectory;

        if( StringStartsWith( path, "/" ) )
        {
            currentDirectoryOffset = OffsetOf( ArchiveHeader, Root );
            currentDirectory.reset( &m_pHeader->Root, m_pDirectoryFree );
            path_ = path.substr( 1, std::string::npos );
        }

        for( auto component : StringSplit( path_, "/" ) )
        {
            bool directoryFound = false;

            if( component == "." )
                continue;

            if( component == ".." )
            {
                if( currentDirectory->Parent == 0 )
                    throw std::invalid_argument( "Invalid path" );

                currentDirectoryOffset = currentDirectory->Parent;
                currentDirectory = _ReadDirectory( currentDirectory->Parent );
                directoryFound = true;
            }

            while( !directoryFound )
            {
                for( uint32_t i = 0; i < currentDirectory->NumEntries; ++i )
                {
                    ArchiveEntry* entry = currentDirectory->Entries + i;

                    if( entry->Name == component )
                    {
                        if( entry->Type != ArchiveEntryType::eDirectory )
                            throw std::invalid_argument( (component + " is not a directory").c_str() );

                        currentDirectoryOffset = entry->Offset;
                        currentDirectory = _ReadDirectory( entry->Offset );
                        directoryFound = true;
                    }
                }

                if( !directoryFound )
                {
                    if( currentDirectory->Next == 0 )
                        throw std::invalid_argument( (component + " not found").c_str() );

                    currentDirectory = _ReadDirectory( currentDirectory->Next );
                }
            }
        }

        return currentDirectoryOffset;
    }

    Archive::SharedArchiveDirectory Archive::_GetDirectory( const std::string& path )
    {
        return _ReadDirectory( _GetDirectoryOffset( path ) );
    }

    Archive::SharedArchiveDirectory Archive::_ReadDirectory( uint32_t offset )
    {
        if( offset == 0 )
            return nullptr;
        
        std::vector<uint32_t> buffer;
        buffer.resize( sizeof( ArchiveDirectory ) / SizeOfElement( buffer ) );

        m_pArchiveFile->Seek( offset );
        m_pArchiveFile->Read( buffer.data(), sizeof( ArchiveDirectory ) );

        if( static_cast<ArchiveMagic>(buffer[0]) != ArchiveMagic::eDirectory )
            throw std::runtime_error( "Archive file corrupted" );

        return SharedArchiveDirectory(
            new ArchiveDirectory( *reinterpret_cast<ArchiveDirectory*>(buffer.data()) ), m_pDirectoryFree );
    }

    void Archive::_NormalizeCurrentDirectoryPath()
    {
        std::vector<std::string> normalizedPathComponents;

        for( auto component : StringSplit( m_CurrentDirectoryPath.substr( 1, std::string::npos ), "/" ) )
        {
            if( component == "." )
                continue;

            if( component == ".." )
                normalizedPathComponents.pop_back();

            else
                normalizedPathComponents.push_back( component );
        }

        const std::string sep = "/";

        if( normalizedPathComponents.size() == 0 )
            m_CurrentDirectoryPath = sep;
        else
            m_CurrentDirectoryPath = sep + StringJoin( normalizedPathComponents, sep ) + sep;
    }

    void Archive::_CheckRead() const
    {
        if( m_Mode == ArchiveFileOpenMode::eWriteOnly )
            throw std::runtime_error( "Archive not opened in read mode" );
    }

    void Archive::_CheckWrite() const
    {
        if( m_Mode == ArchiveFileOpenMode::eReadOnly )
            throw std::runtime_error( "Archive not opened in write mode" );
    }

    void Archive::_FreeNonRoot( ArchiveDirectory* dirPtr )
    {
        if( dirPtr == &m_pHeader->Root )
            return;

        delete dirPtr;
    }

    void Archive::_AllocationTableUpdated()
    {
        m_pArchiveFile->Seek( 0 );
        m_pArchiveFile->Write( m_pHeader.get(), sizeof( ArchiveHeader ) );
    }

    void Archive::_ReallocationHandler( uint32_t oldOffset, uint32_t newOffset, uint32_t size )
    {
        std::vector<char> dataBuffer( size );
        void* pData = dataBuffer.data();

        m_pArchiveFile->Seek( oldOffset );
        m_pArchiveFile->Read( pData, size );
        m_pArchiveFile->Seek( newOffset );
        m_pArchiveFile->Write( pData, size );
    }
}
