#ifndef __MIDI_H__
#define __MIDI_H__


#ifdef __cplusplus
    extern "C"{
#endif //__cplusplus

#define BigLittleSwap16(A)        ((((unsigned short)(A) & 0xff00) >> 8) | \
                                                       (((unsigned short)(A) & 0x00ff) << 8))
 
 
#define BigLittleSwap32(A)        ((((unsigned int)(A) & 0xff000000) >> 24) | \
                                                       (((unsigned int)(A) & 0x00ff0000) >> 8) | \
                                                       (((unsigned int)(A) & 0x0000ff00) << 8) | \
                                                       (((unsigned int)(A) & 0x000000ff) << 24))

typedef struct _midi_event_t{
    unsigned int delta;
    unsigned short data_size;
    unsigned char event;
    unsigned char * data;
}midi_event_t;

typedef struct _midi_track_t{
    char head[4];
    unsigned int size;
    unsigned char * data;
    unsigned int _read_delta_tick;
    unsigned char * _read_offset;
    unsigned char _read_event;
}midi_track_t;

typedef struct _midi_t{
    char head[4];
    unsigned int size;
    unsigned short type;
    unsigned short track_count;
    unsigned short tick_count;
    midi_track_t * tracks;
}midi_t;

typedef struct _midi_event_iterator_t{
    midi_event_t _buff[0x0F];
    midi_t * midi;
    midi_event_t event;

    unsigned char index;
}midi_event_iterator_t;

unsigned short get_dynamic_data_size( unsigned char * p_data );
unsigned int parse_dynamic_data( unsigned char * p_data, unsigned short * p_dynamic_data_size );

void parse_midi( midi_t ** p_midi, const char * p_filePath );
void free_midi( midi_t ** p_midi );

void get_next_event( midi_track_t * p_midi_track, midi_event_t * p_event );

void init_midi_event_iterator( midi_event_iterator_t * p_event_iterator, midi_t * p_midi );
void read_midi_event( midi_event_iterator_t * p_event_iterator );

#ifdef __cplusplus
    }
#endif //__cplusplus

#endif //__MIDI_H__
