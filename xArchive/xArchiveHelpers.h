#pragma once
#include <type_traits>
#include <vector>
#include <string>
#include <cctype>

#define ElementOf( Struct, Element ) (((Struct*)0)->Element)
#define OffsetOf( Struct, Element )  (offsetof( Struct, Element ))

#define BSwap( Value ) ((Value<<24)|((Value&0xFF00)<<8)|((Value>>8)&0xFF00)|(Value>>24))

namespace xArchive
{
    template<typename Type>
    constexpr size_t BitSizeOf = 8 * sizeof( Type );

    template<typename ArrayElementType>
    constexpr size_t SizeOfElement(
        const ArrayElementType* )
    {
        return sizeof( ArrayElementType );
    }

    template<typename ArrayElementType>
    constexpr size_t SizeOfElement(
        const std::vector<ArrayElementType>& )
    {
        return sizeof( ArrayElementType );
    }

    template<typename ArrayElementType>
    constexpr size_t BitSizeOfElement(
        const ArrayElementType* )
    {
        return BitSizeOf<ArrayElementType>;
    }

    template<typename ArrayElementType, size_t ArrayLength>
    constexpr size_t ExtentOf(
        const ArrayElementType( & )[ArrayLength] )
    {
        return ArrayLength;
    }

    inline bool StringStartsWith(
        const std::string& a,
        const std::string& b )
    {
        const char* c_this = a.data();
        const char* c_other = b.data();

        while( c_this && c_other && *c_this && *c_other && *c_this == *c_other )
        {
            c_this++;
            c_other++;
        }

        return !c_other || !*c_other;
    }

    inline bool StringEndsWith(
        const std::string& a,
        const std::string& b )
    {
        const char* c_this = a.data() + a.length();
        const char* c_other = b.data() + b.length();

        while( *c_this == *c_other && c_this != a.data() - 1 && c_other != b.data() - 1 )
        {
            c_this--;
            c_other--;
        }

        return c_other == b.data() - 1;
    }

    template<size_t ArraySize>
    inline void StringToArray(
        const std::string& string,
        char( &array )[ArraySize] )
    {
        if( ArraySize < string.length() + 1 )
            throw std::invalid_argument( "Insufficient buffer" );

        memcpy( array, string.data(), std::min( ArraySize - 1, string.length() + 1 ) );
        array[ArraySize - 1] = 0;
    }

    inline std::string StringJoin(
        const std::vector<std::string>& strings,
        const std::string& sep )
    {
        size_t requiredSize = sep.length() * strings.size();

        for( const auto& element : strings )
            requiredSize += element.length();

        std::string joined;
        joined.reserve( requiredSize );

        for( size_t i = 0; i < strings.size(); ++i )
        {
            joined.append( strings[i] );

            if( i < strings.size() - 1 )
                joined.append( sep );
        }

        return joined;
    }

    inline std::vector<std::string> StringSplit(
        const std::string& string,
        const std::string& sep )
    {
        std::vector<std::string> splitted;

        const char* currentBegin = string.data();
        const char* currentEnd = string.data();

        if( sep.length() > 0 )
        {
            while( static_cast<size_t>(currentBegin - string.data()) < string.length() )
            {
                while( *currentEnd && *currentEnd != *sep.data() ) currentEnd++;

                size_t sepOffset = 0;
                while( currentEnd[sepOffset] && sep[sepOffset] &&
                    currentEnd[sepOffset] == sep[sepOffset] ) sepOffset++;

                if( *currentEnd && sepOffset < sep.length() )
                {
                    currentEnd++;
                    continue;
                }

                splitted.push_back( std::string( currentBegin, currentEnd - currentBegin ) );

                currentEnd += sep.length();
                currentBegin = currentEnd;
            }
        }
        else
        {
            while( static_cast<size_t>(currentBegin - string.data()) < string.length() )
            {
                while( *currentEnd && !std::isspace( *currentEnd ) ) currentEnd++;

                splitted.push_back( std::string( currentBegin, currentEnd - currentBegin ) );

                while( *currentEnd && std::isspace( *currentEnd ) ) currentEnd++;
                currentBegin = currentEnd;
            }
        }

        return splitted;
    }
}
