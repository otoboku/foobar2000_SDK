#pragma once

#include "menu_common.h"
#include "metadb.h"
#include "contextmenu.h"

#ifdef FOOBAR2000_HAVE_KEYBOARD_SHORTCUTS
class NOVTABLE keyboard_shortcut_manager : public service_base {
public:
	enum shortcut_type
	{
		TYPE_MAIN,
		TYPE_CONTEXT,
		TYPE_CONTEXT_PLAYLIST,
		TYPE_CONTEXT_NOW_PLAYING,
	};


	virtual bool process_keydown(shortcut_type type,metadb_handle_list_cref data,unsigned keycode)=0;
	virtual bool process_keydown_ex(shortcut_type type,metadb_handle_list_cref data,unsigned keycode,const GUID & caller)=0;
    
#ifdef _WIN32
	bool on_keydown(shortcut_type type,WPARAM wp);
	bool on_keydown_context(const pfc::list_base_const_t<metadb_handle_ptr> & data,WPARAM wp,const GUID & caller);
	
	bool on_keydown_auto(WPARAM wp);
	bool on_keydown_auto_playlist(WPARAM wp);
	bool on_keydown_auto_context(const pfc::list_base_const_t<metadb_handle_ptr> & data,WPARAM wp,const GUID & caller);

	bool on_keydown_restricted_auto(WPARAM wp);
	bool on_keydown_restricted_auto_playlist(WPARAM wp);
	bool on_keydown_restricted_auto_context(const pfc::list_base_const_t<metadb_handle_ptr> & data,WPARAM wp,const GUID & caller);
#endif
	virtual bool get_key_description_for_action(const GUID & p_command,const GUID & p_subcommand, pfc::string_base & out, shortcut_type type, bool is_global)=0;

#ifdef _WIN32
    static bool is_text_key(t_uint32 vkCode);
    static bool is_typing_key(t_uint32 vkCode);
    static bool is_typing_key_combo(t_uint32 vkCode, t_uint32 modifiers);
    static bool is_typing_modifier(t_uint32 flags);
	static bool is_typing_message(HWND editbox, const MSG * msg);
	static bool is_typing_message(const MSG * msg);
#endif
    
	FB2K_MAKE_SERVICE_COREAPI(keyboard_shortcut_manager);
};


//! New in 0.9.5.
class keyboard_shortcut_manager_v2 : public keyboard_shortcut_manager {
public:
	//! Deprecates old keyboard_shortcut_manager methods. If the action requires selected items, they're obtained from ui_selection_manager API automatically.
	virtual bool process_keydown_simple(t_uint32 keycode) = 0;

#ifdef _WIN32
	//! Helper for use with message filters.
	bool pretranslate_message(const MSG * msg, HWND thisPopupWnd);
#endif
    
	FB2K_MAKE_SERVICE_COREAPI_EXTENSION(keyboard_shortcut_manager_v2,keyboard_shortcut_manager);
};
#endif

class NOVTABLE contextmenu_node {
public:
	virtual contextmenu_item_node::t_type get_type()=0;
	virtual const char * get_name()=0;
	virtual t_size get_num_children()=0;//TYPE_POPUP only
	virtual contextmenu_node * get_child(t_size n)=0;//TYPE_POPUP only
	virtual unsigned get_display_flags()=0;//TYPE_COMMAND/TYPE_POPUP only, see contextmenu_item::FLAG_*
	virtual unsigned get_id()=0;//TYPE_COMMAND only, returns zero-based index (helpful for win32 menu command ids)
	virtual void execute()=0;//TYPE_COMMAND only
	virtual bool get_description(pfc::string_base & out)=0;//TYPE_COMMAND only
	virtual bool get_full_name(pfc::string_base & out)=0;//TYPE_COMMAND only
	virtual void * get_glyph()=0;//RESERVED, do not use
protected:
	contextmenu_node() {}
	~contextmenu_node() {}
};



class NOVTABLE contextmenu_manager : public service_base
{
public:
	enum
	{
		flag_show_shortcuts = 1 << 0,
		flag_show_shortcuts_global = 1 << 1,
		//! \since 1.0
		//! To control which commands are shown, you should specify either flag_view_reduced or flag_view_full. If neither is specified, the implementation will decide automatically based on shift key being pressed, for backwards compatibility.
		flag_view_reduced = 1 << 2,
		//! \since 1.0
		//! To control which commands are shown, you should specify either flag_view_reduced or flag_view_full. If neither is specified, the implementation will decide automatically based on shift key being pressed, for backwards compatibility.
		flag_view_full = 1 << 3,

		//for compatibility
		FLAG_SHOW_SHORTCUTS = 1,
		FLAG_SHOW_SHORTCUTS_GLOBAL = 2,
	};

	virtual void init_context(metadb_handle_list_cref data,unsigned flags) = 0;
	virtual void init_context_playlist(unsigned flags) = 0;
	virtual contextmenu_node * get_root() = 0;//releasing contextmenu_manager service releaases nodes; root may be null in case of error or something
	virtual contextmenu_node * find_by_id(unsigned id)=0;
#ifdef _WIN32
	virtual void set_shortcut_preference(const keyboard_shortcut_manager::shortcut_type * data,unsigned count)=0;
#endif

	static void g_create(service_ptr_t<contextmenu_manager>& p_out);
	static service_ptr_t<contextmenu_manager> g_create();

#ifdef WIN32
	static void win32_build_menu(HMENU menu,contextmenu_node * parent,int base_id,int max_id);//menu item identifiers are base_id<=N<base_id+max_id (if theres too many items, they will be clipped)
	static void win32_run_menu_context(HWND parent,metadb_handle_list_cref data, const POINT * pt = 0,unsigned flags = 0);
	static void win32_run_menu_context_playlist(HWND parent,const POINT * pt = 0,unsigned flags = 0);
	void win32_run_menu_popup(HWND parent,const POINT * pt = 0);
	void win32_build_menu(HMENU menu,int base_id,int max_id) {win32_build_menu(menu,get_root(),base_id,max_id);}
#endif

	virtual void init_context_ex(metadb_handle_list_cref data,unsigned flags,const GUID & caller)=0;
	virtual bool init_context_now_playing(unsigned flags)=0;//returns false if not playing

	bool execute_by_id(unsigned id) noexcept;

	bool get_description_by_id(unsigned id,pfc::string_base & out);

	//! Safely prevent destruction from worker threads (some components attempt that).
	static bool serviceRequiresMainThreadDestructor() { return true; }

	FB2K_MAKE_SERVICE_COREAPI(contextmenu_manager);
};

//! \since 1.0
class NOVTABLE contextmenu_group_manager : public service_base {
	FB2K_MAKE_SERVICE_COREAPI(contextmenu_group_manager)
public:
	virtual GUID path_to_group(const char * path) = 0;
	virtual void group_to_path(const GUID & group, pfc::string_base & path) = 0;
};


//! \since 2.0
class contextmenu_manager_v2 : public contextmenu_manager {
	FB2K_MAKE_SERVICE_COREAPI_EXTENSION(contextmenu_manager_v2, contextmenu_manager)
public:
	virtual menu_tree_item::ptr build_menu() = 0;
};
