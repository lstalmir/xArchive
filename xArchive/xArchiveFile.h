#pragma once
#include <cstdint>
#include <cstdio>
#include <memory>
#include <vector>
#include <string>

namespace xArchive
{
    enum class ArchiveFileOpenMode : uint32_t
    {
        eReadOnly,
        eWriteOnly,
        eReadWrite
    };

    class ArchiveFile
    {
    public:
        virtual ~ArchiveFile();

        virtual void Write( const void* data, size_t size ) = 0;
        virtual void Read( void* buffer, size_t size ) = 0;
        virtual void Seek( ptrdiff_t offset, int mode = SEEK_SET ) = 0;
        virtual size_t Tell() const = 0;
        virtual void Flush() = 0;
        virtual void Close() = 0;
        virtual std::string Name() const;
        virtual ArchiveFileOpenMode Mode() const;

    protected:
        std::string m_Filename;
        ArchiveFileOpenMode m_Mode;

        ArchiveFile( const std::string& filename, ArchiveFileOpenMode mode );
    };

    using UniqueArchiveFile = std::unique_ptr<ArchiveFile>;

    class UncompressedArchiveFile
        : public ArchiveFile
    {
    public:
        UncompressedArchiveFile( const std::string& filename, ArchiveFileOpenMode mode );
        virtual ~UncompressedArchiveFile();

        virtual void Write( const void* data, size_t size ) override;
        virtual void Read( void* buffer, size_t size ) override;
        virtual void Seek( ptrdiff_t offset, int mode = SEEK_SET ) override;
        virtual size_t Tell() const override;
        virtual void Flush() override;
        virtual void Close() override;

    protected:
        FILE* m_pFile;
    };

    class CompressedArchiveFile
        : public ArchiveFile
    {
    public:
        CompressedArchiveFile( const std::string& filename, ArchiveFileOpenMode mode );
        virtual ~CompressedArchiveFile();

        virtual void Write( const void* data, size_t size ) override;
        virtual void Read( void* buffer, size_t size ) override;
        virtual void Seek( ptrdiff_t offset, int mode = SEEK_SET ) override;
        virtual size_t Tell() const override;
        virtual void Flush() override;
        virtual void Close() override;

    protected:
        bool m_IsOpen;
        size_t m_PointerOffset;
        std::vector<char> m_pBuffer;
    };
}
