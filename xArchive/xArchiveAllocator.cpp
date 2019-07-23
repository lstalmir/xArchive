#include "xArchiveAllocator.h"
#include "xArchiveHelpers.h"
#include <system_error>

namespace xArchive
{
    ArchiveAllocator::ArchiveAllocator(
        Archive* archive,
        uint32_t* allocationTable,
        uint32_t allocationTableSize,
        uint32_t allocationSize,
        AllocatorCallbacks allocationCallbacks )
        : m_pAllocationTable( allocationTable )
        , m_AllocationTableSize( allocationTableSize )
        , m_AllocationSize( allocationSize )
        , m_pArchive( archive )
        , m_AllocationCallbacks( allocationCallbacks )
        , m_AllocationBase( 0 )
    {
    }

    void ArchiveAllocator::SetAllocationBase( uint32_t base )
    {
        m_AllocationBase = base;
    }

    uint32_t ArchiveAllocator::GetAllocationBase() const
    {
        return m_AllocationBase;
    }

    uint32_t ArchiveAllocator::Allocate( uint32_t size )
    {
        uint32_t allocationOffset = _Allocate( size );

        (m_pArchive->*m_AllocationCallbacks.pfnFlushAllocationTable)();
        return allocationOffset;
    }

    uint32_t ArchiveAllocator::Reallocate( uint32_t offset, uint32_t oldSize, uint32_t newSize )
    {
        uint32_t allocationOffset = _Reallocate( offset, oldSize, newSize );

        (m_pArchive->*m_AllocationCallbacks.pfnFlushAllocationTable)();
        return allocationOffset;
    }

    void ArchiveAllocator::Free( uint32_t offset, uint32_t size )
    {
        _Free( offset, size );

        (m_pArchive->*m_AllocationCallbacks.pfnFlushAllocationTable)();
    }

    uint32_t ArchiveAllocator::_Allocate( uint32_t bytesize )
    {
        const uint32_t sectorsRequired =
            (((bytesize - 1) / m_AllocationSize) + 1);

        const uint32_t blockSize =
            static_cast<uint32_t>(BitSizeOfElement( m_pAllocationTable ));

        uint32_t sectorsFree = 0;
        uint32_t sectorsFreeBlockOffset = 0;
        uint32_t sectorsFreeSectorOffset = 0;
        uint32_t currentSectorOffset = 0;
        uint32_t currentBlockOffset = 0;

        while( currentBlockOffset < m_AllocationTableSize )
        {
            if( currentSectorOffset == blockSize )
            {
                currentBlockOffset++;
                currentSectorOffset = 0;
            }

            while( (currentSectorOffset < blockSize) &&
                ((m_pAllocationTable[currentBlockOffset] & (1 << currentSectorOffset)) > 0) )
            {
                currentSectorOffset++;

                sectorsFree = 0;
                sectorsFreeBlockOffset = currentBlockOffset;
                sectorsFreeSectorOffset = currentSectorOffset;
            }

            while( currentSectorOffset < blockSize &&
                (sectorsFree < sectorsRequired) &&
                (m_pAllocationTable[currentBlockOffset] & (1 << currentSectorOffset)) == 0 )
            {
                currentSectorOffset++;
                sectorsFree++;
            }

            if( sectorsFree == sectorsRequired )
            {
                uint32_t offset =
                    m_AllocationBase
                    + (sectorsFreeBlockOffset * blockSize + sectorsFreeSectorOffset) * m_AllocationSize;

                currentSectorOffset = sectorsFreeSectorOffset;
                currentBlockOffset = sectorsFreeBlockOffset;

                // Update allocation table
                for( uint32_t sector = 0; sector < sectorsRequired; ++sector, ++currentSectorOffset )
                {
                    if( currentSectorOffset == blockSize )
                    {
                        currentBlockOffset++;
                        currentSectorOffset = 0;
                    }

                    m_pAllocationTable[currentBlockOffset] |= (1 << (currentSectorOffset));
                }

                return offset;
            }
        }

        throw std::runtime_error( "Out of memory" );
    }

    uint32_t ArchiveAllocator::_Reallocate( uint32_t offset, uint32_t oldSize, uint32_t newSize )
    {
        const uint32_t sectorsRequired =
            (((newSize - 1) / m_AllocationSize) + 1);

        const uint32_t sectorsAllocated =
            (((oldSize - 1) / m_AllocationSize) + 1);

        const uint32_t blockSize =
            static_cast<uint32_t>(BitSizeOfElement( m_pAllocationTable ));

        if( sectorsRequired > sectorsAllocated )
        {
            bool needsReallocation =
                !_ExpandAllocation( offset, oldSize, newSize );

            if( needsReallocation )
            {
                uint32_t newAllocation = _Allocate( newSize );

                (m_pArchive->*m_AllocationCallbacks.pfnReallocateEntry)(offset, newAllocation, oldSize);

                _Free( offset, oldSize );
                offset = newAllocation;
            }
        }

        if( sectorsRequired < sectorsAllocated )
        {
            _ShrinkAllocation( offset, oldSize, newSize );
        }

        return offset;
    }

    void ArchiveAllocator::_ShrinkAllocation( uint32_t offset, uint32_t oldSize, uint32_t newSize )
    {
        const uint32_t sectorsRequired =
            (((newSize - 1) / m_AllocationSize) + 1);

        const uint32_t sectorsAllocated =
            (((oldSize - 1) / m_AllocationSize) + 1);

        const uint32_t blockSize =
            static_cast<uint32_t>(BitSizeOfElement( m_pAllocationTable ));

        uint32_t currentSector = sectorsRequired;

        uint32_t currentSectorOffset =
            (offset + sectorsRequired * m_AllocationSize) % blockSize;

        uint32_t currentBlockOffset =
            (offset + sectorsRequired * m_AllocationSize) / blockSize;

        while( currentSector < sectorsAllocated )
        {
            if( currentSectorOffset == blockSize )
            {
                currentBlockOffset++;
                currentSectorOffset = 0;
            }

            m_pAllocationTable[currentBlockOffset] &= ~(1 << currentSectorOffset);

            currentSectorOffset++;
            currentSector++;
        }
    }

    bool ArchiveAllocator::_ExpandAllocation( uint32_t offset, uint32_t oldSize, uint32_t newSize )
    {
        const uint32_t sectorsRequired =
            (((newSize - 1) / m_AllocationSize) + 1);

        const uint32_t sectorsAllocated =
            (((oldSize - 1) / m_AllocationSize) + 1);

        const uint32_t blockSize =
            static_cast<uint32_t>(BitSizeOfElement( m_pAllocationTable ));

        uint32_t currentSector = sectorsAllocated;

        uint32_t currentSectorOffset =
            (offset + sectorsAllocated * m_AllocationSize) % blockSize;

        uint32_t currentBlockOffset =
            (offset + sectorsAllocated * m_AllocationSize) / blockSize;

        // Check if the allocation may be expanded
        while( currentSector < sectorsRequired )
        {
            if( currentSectorOffset == blockSize )
            {
                currentBlockOffset++;
                currentSectorOffset = 0;
            }

            if( m_pAllocationTable[currentBlockOffset] & (1 << currentSectorOffset) )
            {
                // Cannot expand the allocation
                return false;
            }

            currentSectorOffset++;
            currentSector++;
        }

        currentSector = sectorsAllocated;

        currentSectorOffset =
            (offset + sectorsAllocated * m_AllocationSize) % blockSize;

        currentBlockOffset =
            (offset + sectorsAllocated * m_AllocationSize) / blockSize;

        // Expand the allocation
        while( currentSector < sectorsRequired )
        {
            if( currentSectorOffset == blockSize )
            {
                currentBlockOffset++;
                currentSectorOffset = 0;
            }

            m_pAllocationTable[currentBlockOffset] |= (1 << currentSectorOffset);

            currentSectorOffset++;
            currentSector++;
        }

        return true;
    }

    void ArchiveAllocator::_Free( uint32_t offset, uint32_t size )
    {
        const uint32_t blockSize =
            static_cast<uint32_t>(BitSizeOfElement( m_pAllocationTable ));

        uint32_t blockOffset = offset / blockSize;
        uint32_t segmentOffset = offset % blockSize;

        uint32_t allocationSize = (((size - 1) / m_AllocationSize) + 1);

        while( allocationSize > 0 )
        {
            if( segmentOffset == blockSize )
            {
                blockOffset++;
                segmentOffset = 0;
            }

            m_pAllocationTable[blockOffset] &= ~(1 << segmentOffset);

            segmentOffset++;
            allocationSize--;
        }
    }
}
