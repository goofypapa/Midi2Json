#include "midi.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

void parse_midi( midi_t ** p_midi, const char * p_filePath  )
{
    FILE * _file;

    midi_t * t_midi;
    size_t t_size;
    u_int8_t n;
    midi_track_t * t_track;

    _file = fopen( p_filePath, "r" );
    if( !_file )
    {
        printf( "------>>>> %s is not found \r\n", p_filePath );
        return;
    }

    t_midi = (midi_t *)malloc( sizeof( midi_t ) );
    
    t_size = fread( t_midi, 14, 1, _file );

    t_midi->size = BigLittleSwap32( t_midi->size );
    t_midi->type = BigLittleSwap16( t_midi->type );
    t_midi->track_count = BigLittleSwap16( t_midi->track_count );
    t_midi->tick_count = BigLittleSwap16( t_midi->tick_count );

    t_midi->tracks = (midi_track_t *)malloc( (size_t)(sizeof( midi_track_t ) * t_midi->track_count ) );

    for( n = 0; n < t_midi->track_count; ++n )
    {
        t_track = t_midi->tracks + n;
        fread( t_track, 8, 1, _file );

        t_track->size = BigLittleSwap32( t_track->size );

        t_track->data = (unsigned char *)malloc( (size_t)t_track->size );
        t_track->_read_delta_tick = 0;
        t_track->_read_offset = NULL;
        fread( t_track->data, t_track->size, 1, _file );
    }

    fclose( _file );

    *p_midi = t_midi;
}

void free_midi( midi_t ** p_midi )
{
    unsigned int i;
    midi_t * t_midi;
    midi_track_t * t_track;

    t_midi = *p_midi;

    for( i = 0; i < t_midi->track_count; ++i )
    {
        t_track = t_midi->tracks + i;

        free( t_track->data );
    }

    free( t_midi->tracks );
    free( t_midi );
    *p_midi = NULL;
}

void get_next_event( midi_track_t * p_midi_track, midi_event_t * p_event )
{

    unsigned int t_delta_time;
    unsigned char t_flag;
    unsigned short t_dynamic_data_size;

    if( p_midi_track->_read_offset == NULL )
    {
        p_midi_track->_read_offset = p_midi_track->data;
        p_midi_track->_read_delta_tick = 0;
    }

    t_delta_time = parse_dynamic_data( p_midi_track->_read_offset, &t_dynamic_data_size );

    p_midi_track->_read_offset += t_dynamic_data_size;

    t_flag = *p_midi_track->_read_offset;

    if( t_delta_time == 0x0 && t_flag == 0xFF )
    {
        p_midi_track->_read_offset += 1;

        p_midi_track->_read_event = *p_midi_track->_read_offset;

        p_midi_track->_read_offset += 1;
        
        p_event->data_size = parse_dynamic_data( p_midi_track->_read_offset, &t_dynamic_data_size );
        p_midi_track->_read_offset += t_dynamic_data_size;

        p_event->delta = p_midi_track->_read_delta_tick;
        p_event->event = p_midi_track->_read_event;
        p_event->data = p_midi_track->_read_offset;

        p_midi_track->_read_offset += p_event->data_size;
    }else{
        if( t_flag > 0x7F )
        {
            p_midi_track->_read_event = t_flag;
            p_midi_track->_read_offset += 1;
        }

        p_event->data_size = 2;

        switch ( p_midi_track->_read_event )
        {
        case 0xD0:
            p_event->data_size = 1;
        break;
        }

        p_midi_track->_read_delta_tick += t_delta_time;
        
        p_event->delta = p_midi_track->_read_delta_tick;
        p_event->event = p_midi_track->_read_event;
        p_event->data = p_midi_track->_read_offset;

        p_midi_track->_read_offset += p_event->data_size;
    }

    if( p_midi_track->_read_offset - p_midi_track->data > p_midi_track->size )
    {
        p_event->event = 0x2F;
    }

    //end
    if( p_event->event == 0x2F )
    {
        p_midi_track->_read_offset = p_midi_track->data;
        p_midi_track->_read_delta_tick = 0;
    }
}

void init_midi_event_iterator( midi_event_iterator_t * p_event_iterator, midi_t * p_midi )
{
    u_int8_t i;
    midi_track_t * t_track;

    for( i = 0; i < p_midi->track_count; ++i )
    {
        t_track = p_midi->tracks + i;

        get_next_event( t_track, &p_event_iterator->_buff[i] );
    }
    p_event_iterator->midi = p_midi;
}

void read_midi_event( midi_event_iterator_t * p_event_iterator )
{
    unsigned char i;
    midi_t * t_midi = p_event_iterator->midi;

    p_event_iterator->index = t_midi->track_count;
    for( i = 0; i < t_midi->track_count; ++i )
    {

        if( p_event_iterator->_buff[i].event == 0x2F )
        {
            continue;
        }

        if( p_event_iterator->index == t_midi->track_count || p_event_iterator->_buff[i].delta < p_event_iterator->_buff[p_event_iterator->index].delta)
        {
            p_event_iterator->index = i;
        }

        if( p_event_iterator->_buff[i].delta == 0 )
        {
            break;
        }
    }

    if( p_event_iterator->index >= t_midi->track_count )
    {
        p_event_iterator->event.event = 0x2F;
        p_event_iterator->event.data_size = 0;
        return;
    }

    p_event_iterator->event.delta = p_event_iterator->_buff[p_event_iterator->index].delta;
    p_event_iterator->event.event = p_event_iterator->_buff[p_event_iterator->index].event;
    p_event_iterator->event.data_size = p_event_iterator->_buff[p_event_iterator->index].data_size;
    p_event_iterator->event.data = p_event_iterator->_buff[p_event_iterator->index].data;

    get_next_event( t_midi->tracks + p_event_iterator->index, &p_event_iterator->_buff[p_event_iterator->index] );
}

unsigned short get_dynamic_data_size( unsigned char * p_data )
{
    unsigned short t_result = 0;

    for(;;)
    {
        if( !( *(p_data + t_result) & 0x80 ) )
        {
            break;
        }
        t_result++;
    }

    return ++t_result;
}

unsigned int parse_dynamic_data( unsigned char * p_data, unsigned short * p_dynamic_data_size )
{
    unsigned int t_result = 0;
    unsigned short t_dynamic_data_size = 0, i;

    for(;;)
    {
        if( !( *(p_data + t_dynamic_data_size) & 0x80 ) )
        {
            break;
        }
        t_dynamic_data_size++;
    }

    t_dynamic_data_size++;

    if( p_dynamic_data_size )
    {
        *p_dynamic_data_size = t_dynamic_data_size;
    }

    for( i = 0; i < t_dynamic_data_size ; ++i )
    {
        t_result += pow( 0x80, t_dynamic_data_size - i - 1 ) * ( *(p_data + i) & 0x7F );
    }

    return t_result;
}
