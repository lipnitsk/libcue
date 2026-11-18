/*
 * Private header: give libcue's internal functions a cue_ prefix.
 *
 * These functions are not exported from the shared library, but in a static
 * library their generic names (cdtext_init, cdtext_set, track_init, ...)
 * clash with other libraries, notably libcdio (#8).  Every internal header
 * includes this file first, so the sources can keep the short names.
 *
 * For license terms, see the file COPYING in this distribution.
 */

#ifndef CUE_NAMESPACE_H
#define CUE_NAMESPACE_H

#define cd_add_track          cue_cd_add_track
#define cd_dump               cue_cd_dump
#define cd_init               cue_cd_init
#define cd_set_catalog        cue_cd_set_catalog
#define cd_set_cdtextfile     cue_cd_set_cdtextfile
#define cd_set_mode           cue_cd_set_mode
#define cdtext_delete         cue_cdtext_delete
#define cdtext_dump           cue_cdtext_dump
#define cdtext_get_key        cue_cdtext_get_key
#define cdtext_init           cue_cdtext_init
#define cdtext_is_empty       cue_cdtext_is_empty
#define cdtext_set            cue_cdtext_set
#define rem_dump              cue_rem_dump
#define rem_free              cue_rem_free
#define rem_is_empty          cue_rem_is_empty
#define rem_new               cue_rem_new
#define rem_set               cue_rem_set
#define time_frame_to_mmssff  cue_time_frame_to_mmssff
#define time_frame_to_msf     cue_time_frame_to_msf
#define time_msf_to_frame     cue_time_msf_to_frame
#define track_clear_flag      cue_track_clear_flag
#define track_delete          cue_track_delete
#define track_init            cue_track_init
#define track_set_filename    cue_track_set_filename
#define track_set_flag        cue_track_set_flag
#define track_set_index       cue_track_set_index
#define track_set_isrc        cue_track_set_isrc
#define track_set_length      cue_track_set_length
#define track_set_mode        cue_track_set_mode
#define track_set_start       cue_track_set_start
#define track_set_sub_mode    cue_track_set_sub_mode
#define track_set_zero_post   cue_track_set_zero_post
#define track_set_zero_pre    cue_track_set_zero_pre

#endif
