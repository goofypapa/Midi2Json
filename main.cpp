#include "ws_log.h"
#include "json.h"
#include "midi.h"
#include <string>
#include <sstream>


std::string getStringData( const midi_event_t & p_midi_event );

void convertMidi2Json( midi_t * p_midi, ws_core::node * p_json );

int main( int argc, char ** argv )
{
    midi_t * t_midi;
    ws_core::node * json;
    FILE * t_outFile;
    const char * t_filePath;
    const char * t_fileName;
    int t_pathLength, n;
    std::string t_jsonStr;
    std::stringstream t_outFileNameSStr;

    char t_fileNameBuff[1024];
    
    unsigned short i;
    
    if( argc < 2 )
    {
        std::cout << "Midi2Json [midi file]" << std::endl;
        return 1;
    }
    
    for( i = 1; i < argc; ++i )
    {
        t_filePath = argv[i];
        parse_midi( &t_midi, t_filePath );
        
        do{

            if( strncmp( t_midi->head, "MThd", 4 ) )
            {
                debug( t_filePath << " not is midi" )
                break;
            }

            t_pathLength = strlen( t_filePath );

            for( n = t_pathLength; ; --n )
            {
                if( t_filePath[n] == '/' || n == 0 )
                {
                    break;
                }
            }

            t_fileName = t_filePath + n;


            memset( t_fileNameBuff, 0, sizeof(t_fileNameBuff) );

            t_pathLength = strlen( t_fileName );
            for( n = 0; ; ++n )
            {
                if( t_filePath[n] == '.' || n >= t_pathLength - 1 )
                {
                    break;
                }
            }

            memcpy( t_fileNameBuff, t_fileName, n );

            t_outFileNameSStr << t_fileNameBuff << ".json";

            json = ws_core::create_json_node( ws_core::OBJECT );
            
            convertMidi2Json( t_midi, json );

            t_jsonStr = json->to_string();

            t_outFile = fopen( t_outFileNameSStr.str().c_str(), "wr+" );

            if( t_outFile )
            {
                fwrite( t_jsonStr.c_str(), t_jsonStr.size(), 1, t_outFile );
                fclose( t_outFile );

                info( "out file " << t_outFileNameSStr.str() );
            }
            
            ws_core::free_json_node( &json );

        }while(0);
        free_midi( &t_midi );
    }
    
    return 0;
}

std::string getStringData( const midi_event_t & p_midi_event )
{
    std::string t_result = "";

    char *t_str = (char *)malloc( p_midi_event.data_size + 1 );

    memset( t_str, 0, p_midi_event.data_size + 1 );
    memcpy( t_str, p_midi_event.data, p_midi_event.data_size );
    
    t_result = t_str;

    free( t_str );

    return t_result;
}

void convertMidi2Json( midi_t * p_midi, ws_core::node * p_json )
{
    unsigned short i;
    midi_event_t t_midi_event;
    unsigned int t_number;
    ws_core::node * tracks, * track, * events, * event;
    
    tracks = ws_core::create_json_node( ws_core::ARRAY );

    // printf( "----->> %s %d \r\n", p_midi->head, p_midi->size );

    // printf( "----->> type: %d \r\n", p_midi->type );
    // printf( "----->> track count: %d \r\n", p_midi->track_count );
    // printf( "----->> ticks: %d \r\n", p_midi->tick_count );

    p_json->append( "tick", p_midi->tick_count );
    p_json->append( tracks->set_key( "tracks" ) );

    for( i = 0; i < p_midi->track_count; ++i )
    {
        track = ws_core::create_json_node( ws_core::OBJECT );
        events = ws_core::create_json_node( ws_core::ARRAY );
       
        for(;;)
        {
            get_next_event( p_midi->tracks + i, &t_midi_event );

            if( t_midi_event.event < 0x80 )
            {
                switch( t_midi_event.event )
                {
                    case 0x03:
                        p_json->append( "music name", getStringData( t_midi_event ).c_str() );
                    break;
                    case 0x04:
                        track->append( "musical instrument", getStringData( t_midi_event ).c_str() );
                    break;
                    case 0x51:
                        t_number = ( *(unsigned int *)t_midi_event.data );
                        t_number = BigLittleSwap32( t_number ) >> 8;
                        p_json->append( "speed", (int)t_number );
                    break;
                    case 0x58:
                        event = ws_core::create_json_node( ws_core::ARRAY );

                        event->append( (int)*t_midi_event.data );
                        event->append( (int)*( t_midi_event.data + 1 ) );
                        event->append( (int)*( t_midi_event.data + 2 ) );
                        event->append( (int)*( t_midi_event.data + 3 ) );

                        p_json->append( event->set_key( "beat" ) );
                    break;
                }
            }else{
                switch( t_midi_event.event & 0xF0 )
                {
                    case 0x80:
                    case 0x90:
                        event = ws_core::create_json_node( ws_core::OBJECT );

                        event->append( "delta", (int)t_midi_event.delta );
                        event->append( "event", (int)t_midi_event.event );
                        event->append( "tone", (int)*t_midi_event.data );
                        event->append( "intensity", (int)*( t_midi_event.data + 1 ) );

                        events->append( event );
                    break;
                }
            }

            // printf( "---->>> track: %d delta: %x evnet: %x %x \r\n", i, t_midi_event.delta, t_midi_event.event, t_midi_event.data_size );

            if( t_midi_event.event == 0x2F )
            {
                break;
            }
        }
        
        if( events->get_length() == 0 )
        {
            ws_core::free_json_node( &track );
            ws_core::free_json_node( &events );
            continue;
        }

        tracks->append( track );
        track->append( events->set_key( "events" ) );
    }
}
