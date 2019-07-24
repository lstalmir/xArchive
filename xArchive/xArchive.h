#pragma once
#include "xArchiveFile.h"
#include "xArchiveAllocator.h"
#include "xArchiveHelpers.h"
#include <functional>
#include <vector>
#include <string>

namespace xArchive
{
    enum class ArchiveOpenFlags : uint32_t
    {
        eReadonly = 1
    };

    class Archive
    {
    public:
        static Archive* Open(
            const std::string& filename,
            ArchiveOpenFlags flags = ArchiveOpenFlags() );

        static Archive* Create(
            const std::string& filename,
            uint32_t allocationSize = 4096 );

        virtual void CreateDirectory( const std::string& path );
        virtual void RemoveDirectory( const std::string& path );
        virtual void SetCurrentDirectory( const std::string& path );
        virtual std::string GetCurrentDirectory() const;
        virtual std::vector<std::string> ListDirectory( const std::string& path );
        virtual void ReadFile( const std::string& path, void* buffer, size_t bufferSize );
        virtual std::vector<char> ReadFile( const std::string& path );
        virtual void CreateFile( const std::string& path, const void* data, size_t size );
        virtual void UpdateFile( const std::string& path, const void* data, size_t size );
        virtual void RemoveFile( const std::string& path );
        virtual size_t GetFileSize( const std::string& path );

    private:
        Archive( const std::string& filename, ArchiveFileOpenMode mode );

        enum class ArchiveMagic
            : uint32_t
        {
            eArchive                = BSwap( 'ARCH' ),
            eDirectory              = BSwap( 'DIR ' ),
            eFile                   = BSwap( 'FILE' )
        };

        enum class ArchiveEntryType
            : uint32_t
        {
            eDirectory,
            eFile
        };

        struct ArchiveEntry
        {
            char                    Name[32];
            uint32_t                Offset;
            uint32_t                Size;
            ArchiveEntryType        Type;

            ArchiveEntry();
            ArchiveEntry( const std::string& name, uint32_t offset, uint32_t size, ArchiveEntryType type );
        };

        struct ArchiveDirectoryEntry
            : ArchiveEntry
        {
            ArchiveDirectoryEntry( const std::string& name, uint32_t offset );
        };

        struct ArchiveFileEntry
            : ArchiveEntry
        {
            ArchiveFileEntry( const std::string& name, uint32_t offset, uint32_t size );
        };

        struct ArchiveDirectory
        {
            ArchiveMagic            Magic;
            uint32_t                Parent;
            uint32_t                Next;
            uint32_t                NumEntries;
            ArchiveEntry            Entries[16];

            ArchiveDirectory( uint32_t parent = 0 );

            void AddEntry( const ArchiveEntry& entry );
            void RemoveEntry( uint32_t n );
            ArchiveEntry& GetEntry( uint32_t n );
            const ArchiveEntry& GetEntry( uint32_t n ) const;
            bool HasFreeSpace() const;
        };

        using SharedArchiveDirectory = std::shared_ptr<ArchiveDirectory>;
        using ArchiveDirectoryDeleter = std::function<void( ArchiveDirectory* )>;

        // Each allocation is recorded with 1 bit and equals to 1 allocation unit.
        // Archive may hold up to 32k allocations.
        typedef uint32_t ArchiveAllocationTable[1024];

        struct ArchiveHeader
        {
            ArchiveMagic            Magic;
            uint32_t                AllocationSize;
            ArchiveAllocationTable  AllocationTable;
            ArchiveDirectory        Root;
        };

        using UniqueArchiveHeader = std::unique_ptr<ArchiveHeader>;

        UniqueArchiveFile           m_pArchiveFile;
        ArchiveFileOpenMode         m_Mode;
        ArchiveDirectoryDeleter     m_pDirectoryFree;
        UniqueArchiveAllocator      m_pAllocator;
        UniqueArchiveHeader         m_pHeader;
        SharedArchiveDirectory      m_pCurrentDirectory;
        std::string                 m_CurrentDirectoryPath;
        uint32_t                    m_CurrentDirectoryOffset;

        ArchiveEntry _GetEntry( const std::string& path );
        uint32_t _GetDirectoryOffset( const std::string& path );
        SharedArchiveDirectory _GetDirectory( const std::string& path );
        SharedArchiveDirectory _ReadDirectory( uint32_t offset );
        void _NormalizeCurrentDirectoryPath();
        void _CheckRead() const;
        void _CheckWrite() const;
        void _FreeNonRoot( ArchiveDirectory* dirPtr );
        void _AllocationTableUpdated();
        void _ReallocationHandler( uint32_t oldOffset, uint32_t newOffset, uint32_t size );
    };

    using UniqueArchive = std::unique_ptr<Archive>;
}
