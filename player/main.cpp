//
//  main.cpp
//  SwfTest
//
//  Created by Советов Дмитрий on 08.07.14.
//  Copyright (c) 2014 Plarium. All rights reserved.
//

#include <gameswf/gameswf_player.h>
#include <gameswf/gameswf_root.h>
#include <base/tu_file.h>

#ifdef _WINDOWS
	#include <direct.h>
	#define  getcwd _getcwd
#endif

using namespace gameswf;

std::string gFileName = "DocumentClass.swf";

int main(int argc, const char * argv[])
{
    char cwd[512];
    printf( "%s\n", getcwd( cwd, 512 ) );

    for( int i = 0; i < argc; i++ ) {
        if( strcmp( argv[i], "-file" ) == 0 ) {
            gFileName = argv[i + 1];
        }
    }
    
    struct file_opener {
        static tu_file*	thunk(const char* url)
        // Callback function.  This opens files for the gameswf library.
        {
            return new tu_file(url, "rb");
        }
    };

    set_verbose_action( false );

    gc_ptr<player> player = new gameswf::player;
    register_file_opener_callback(file_opener::thunk);
    gc_ptr<root>   movie  = player->load_file(gFileName.c_str());

    if( movie ) {
        movie->advance(0);
    }

    return 0;
}

