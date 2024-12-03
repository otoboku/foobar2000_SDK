// This is the master foobar2000 SDK header file; it includes headers for all functionality exposed through the SDK project. 
// For historical reasons, this #includes everything from the SDK.
// In new code, it is recommended to #include "foobar2000-lite.h" then any other headers on need-to-use basis.

#ifndef _FOOBAR2000_H_
#define _FOOBAR2000_H_

#include "foobar2000-lite.h"

#include "completion_notify.h"
#include "abort_callback.h"
#include "componentversion.h"
#include "preferences_page.h"
#include "coreversion.h"
#include "filesystem.h"
#include "filesystem_transacted.h"
#include "archive.h"
#include "audio_chunk.h"
#include "mem_block_container.h"
#include "audio_postprocessor.h"
#include "playable_location.h"
#include "file_info.h"
#include "file_info_impl.h"
#include "hasher_md5.h"
#include "metadb_handle.h"
#include "metadb.h"
#include "metadb_index.h"
#include "metadb_display_field_provider.h"
#include "metadb_callbacks.h"
#include "file_info_filter.h"
#include "console.h"
#include "dsp.h"
#include "dsp_manager.h"
#include "initquit.h"
#include "event_logger.h"
#include "input.h"
#include "input_impl.h"
#include "menu.h"
#include "contextmenu.h"
#include "contextmenu_manager.h"
#include "menu_helpers.h"
#include "modeless_dialog.h"
#include "playback_control.h"
#include "play_callback.h"
#include "playlist.h"
#include "playlist_loader.h"
#include "replaygain.h"
#include "resampler.h"
#include "tag_processor.h"
#include "titleformat.h"
#include "ui.h"
#include "unpack.h"
#include "packet_decoder.h"
#include "commandline.h"
#include "genrand.h"
#include "file_operation_callback.h"
#include "library_manager.h"
#include "library_callbacks.h"
#include "config_io_callback.h"
#include "popup_message.h"
#include "app_close_blocker.h"
#include "config_object.h"
#include "threaded_process.h"
#include "input_file_type.h"
#include "main_thread_callback.h"
#include "advconfig.h"
#include "track_property.h"

#include "album_art.h"
#include "album_art_helpers.h"
#include "icon_remap.h"
#include "search_tools.h"
#include "autoplaylist.h"
#include "replaygain_scanner.h"

#include "system_time_keeper.h"
#include "http_client.h"
#include "exceptions.h"

#include "progress_meter.h"

#include "commonObjects.h"

#include "file_lock_manager.h"

#include "configStore.h"

#include "timer.h"

#include "cfg_var.h"
#include "advconfig_impl.h"


#include "playlistColumnProvider.h"
#include "threadPool.h"
#include "powerManager.h"
#include "keyValueIO.h"
#include "audioEncoder.h"
#include "decode_postprocessor.h"
#include "file_format_sanitizer.h"
#include "imageLoaderLite.h"
#include "imageViewer.h"
#include "playback_stream_capture.h"
#include "message_loop.h"
#include "chapterizer.h"
#include "info_lookup_handler.h"
#include "output.h"
#include "link_resolver.h"
#include "image.h"
#include "fileDialog.h"
#include "console_manager.h"
#include "vis.h"
#include "ole_interaction.h"
#include "library_index.h"
#include "ui_element.h"
#include "ui_edit_context.h"
#include "toolbarDropDown.h"

#include "commonObjects-Apple.h"
#include "ui_element_mac.h"

#endif //_FOOBAR2000_H_

