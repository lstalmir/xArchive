// xArchiver.cpp

#include <xArchive.h>

#include <Windows.h>

#undef CreateFile
#undef CreateDirectory
#undef SetCurrentDirectory

#include <iostream>
#include <fstream>

using namespace xArchive;
using namespace std;


void export_directory( Archive& archive, const std::string& path )
{
    const std::string searchPath = path + "\\*";

    WIN32_FIND_DATAA findFileData;
    HANDLE hFindFile = FindFirstFileA( searchPath.c_str(), &findFileData );

    for( bool end = false;
        !end && hFindFile != INVALID_HANDLE_VALUE;
        end = !FindNextFileA( hFindFile, &findFileData ) )
    {
        if( std::strcmp( findFileData.cFileName, "." ) == 0 ||
            std::strcmp( findFileData.cFileName, ".." ) == 0 )
        {
            continue;
        }

        if( findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
        {
            archive.CreateDirectory( findFileData.cFileName );
            archive.SetCurrentDirectory( findFileData.cFileName );

            export_directory( archive, path + "\\" + findFileData.cFileName );

            archive.SetCurrentDirectory( ".." );
        }

        else
        {
            ifstream file( (path + "\\" + findFileData.cFileName).c_str(), ios::in | ios::binary );

            vector<char> fileBytes( static_cast<size_t>(file.seekg( 0, fstream::end ).tellg()) );

            file.seekg( 0, fstream::beg )
                .read( fileBytes.data(), fileBytes.size() );

            archive.CreateFile( findFileData.cFileName, fileBytes.data(), fileBytes.size() );
        }
    }

    FindClose( hFindFile );
}

int main( int argc, char** argv )
{
    if( argc < 3 )
    {
        cerr << "Usage: " << argv[0] << " <archive> <directory>" << endl;
        return -1;
    }

    // Usage: VAHArchiver-Standalone <archive>(1) <directory>(2)
    const char* outputFilename = argv[1];
    const char* inputDirectory = argv[2];

    auto archive = Archive::Create( outputFilename );

    export_directory( archive, inputDirectory );

    return 0;
}
