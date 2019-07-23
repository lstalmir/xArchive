#pragma once
#include <cstdint>
#include <memory>

namespace xArchive
{
    class Archive;

    using PFNREALLOCATEENTRY = void(Archive::*)(
        uint32_t oldOffset,
        uint32_t newOffset,
        uint32_t size);

    using PFNFLUSHALLOCATIONTABLE = void(Archive::*)();

    struct AllocatorCallbacks
    {
        PFNREALLOCATEENTRY      pfnReallocateEntry;
        PFNFLUSHALLOCATIONTABLE pfnFlushAllocationTable;
    };

    class ArchiveAllocator
    {
    public:
        ArchiveAllocator(
            Archive* archive,
            uint32_t* allocationTable,
            uint32_t allocationTableSize,
            uint32_t allocationSize,
            AllocatorCallbacks callbacks );

        void     SetAllocationBase( uint32_t base );
        uint32_t GetAllocationBase() const;
        uint32_t Allocate( uint32_t size );
        uint32_t Reallocate( uint32_t offset, uint32_t oldSize, uint32_t newSize );
        void     Free( uint32_t offset, uint32_t size );

    protected:
        uint32_t* m_pAllocationTable;
        uint32_t  m_AllocationTableSize;
        uint32_t  m_AllocationSize;
        uint32_t  m_AllocationBase;
        Archive* m_pArchive;
        AllocatorCallbacks m_AllocationCallbacks;

        uint32_t _Allocate( uint32_t size );
        uint32_t _Reallocate( uint32_t offset, uint32_t oldSize, uint32_t newSize );
        void     _ShrinkAllocation( uint32_t offset, uint32_t oldSize, uint32_t newSize );
        bool     _ExpandAllocation( uint32_t offset, uint32_t oldSize, uint32_t newSize );
        void     _Free( uint32_t offset, uint32_t size );
    };

    using UniqueArchiveAllocator = std::unique_ptr<ArchiveAllocator>;
}
