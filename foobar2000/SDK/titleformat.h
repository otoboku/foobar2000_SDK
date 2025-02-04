#pragma once

#include "titleformat_object.h"

namespace titleformat_inputtypes {
	extern const GUID meta, unknown;
};

class NOVTABLE titleformat_text_out {
public:
	virtual void write(const GUID & p_inputtype,const char * p_data,t_size p_data_length = SIZE_MAX) = 0;
	void write_int(const GUID & p_inputtype,t_int64 val);
	void write_int_padded(const GUID & p_inputtype,t_int64 val,t_int64 maxval);
protected:
	titleformat_text_out() {}
	~titleformat_text_out() {}
};

//! This class allows custom processing of title formatting output, aware of whole substrings being passed, etc.
class NOVTABLE titleformat_text_filter {
public:
	virtual void write(const GUID & p_inputtype,pfc::string_receiver & p_out,const char * p_data,t_size p_data_length) = 0;
protected:
	titleformat_text_filter() {}
	~titleformat_text_filter() {}
};

class NOVTABLE titleformat_hook_function_params
{
public:
	virtual t_size get_param_count() = 0;
	virtual void get_param(t_size index,const char * & p_string,t_size & p_string_len) = 0;//warning: not a null-terminated string
	
	//helper
	t_size get_param_uint(t_size index);
};

class NOVTABLE titleformat_hook
{
public:
	virtual bool process_field(titleformat_text_out * p_out,const char * p_name,t_size p_name_length,bool & p_found_flag) = 0;
	virtual bool process_function(titleformat_text_out * p_out,const char * p_name,t_size p_name_length,titleformat_hook_function_params * p_params,bool & p_found_flag) = 0;
};

//! \since 2.0
class NOVTABLE titleformat_object_v2 : public titleformat_object {
	FB2K_MAKE_SERVICE_INTERFACE(titleformat_object_v2, titleformat_object);
public:
	//! Walk from idx=0 up until returns null.
	virtual const char* enum_used_fields(size_t idx) = 0;
	//! Walk from idx=0 up until returns null.
	virtual const char* enum_used_functions(size_t idx) = 0;
	//! Returns true if evaluation of this only references fields such as file path, which can be evaluated without accessing actual metadb data.
	virtual bool requires_metadb_info() = 0;
};

//! Standard service for instantiating titleformat_object. Implemented by the core; do not reimplement.
//! To instantiate, use titleformat_compiler::get().
class NOVTABLE titleformat_compiler : public service_base {
	FB2K_MAKE_SERVICE_COREAPI(titleformat_compiler);
public:
	//! Returns false in case of a compilation error.
	virtual bool compile(titleformat_object::ptr & p_out,const char * p_spec) = 0;
	//! Helper;
	void run(titleformat_hook * p_source,pfc::string_base & p_out,const char * p_spec);
	//! Should never fail, falls back to %filename% in case of failure.
	void compile_safe(titleformat_object::ptr & p_out,const char * p_spec);

	//! Falls back to p_fallback in case of failure.
	void compile_safe_ex(titleformat_object::ptr & p_out,const char * p_spec,const char * p_fallback = "<ERROR>");

	//! Crashes when script can't be compiled. For use with hardcoded scripts only.
	void compile_force(titleformat_object::ptr & p_out,const char * p_spec) {if (!compile(p_out,p_spec)) uBugCheck();}

	titleformat_object::ptr compile(const char* spec);
	titleformat_object::ptr compile_force(const char* spec);
	titleformat_object::ptr compile_fallback(const char* spec, const char* fallback);


	static void remove_color_marks(const char * src,pfc::string_base & out);//helper
	static void remove_forbidden_chars(titleformat_text_out * p_out,const GUID & p_inputtype,const char * p_source,t_size p_source_len,const char * p_forbidden_chars);
	static void remove_forbidden_chars_string_append(pfc::string_receiver & p_out,const char * p_source,t_size p_source_len,const char * p_forbidden_chars);
	static void remove_forbidden_chars_string(pfc::string_base & p_out,const char * p_source,t_size p_source_len,const char * p_forbidden_chars);
};

// \since 2.0
class NOVTABLE titleformat_compiler_v2 : public titleformat_compiler {
	FB2K_MAKE_SERVICE_COREAPI_EXTENSION(titleformat_compiler_v2, titleformat_compiler);
public:
	virtual titleformat_object::ptr concat(pfc::list_base_const_t<titleformat_object::ptr> const&) = 0;
};


class titleformat_object_wrapper {
public:
	titleformat_object_wrapper(const char * p_script) {
		titleformat_compiler::get()->compile_force(m_script,p_script);
	}

	operator const service_ptr_t<titleformat_object> &() const {return m_script;}
	
private:
	service_ptr_t<titleformat_object> m_script;
};


//helpers


class titleformat_text_out_impl_filter_chars : public titleformat_text_out
{
public:
	inline titleformat_text_out_impl_filter_chars(titleformat_text_out * p_chain,const char * p_restricted_chars)
		: m_chain(p_chain), m_restricted_chars(p_restricted_chars) {}
	void write(const GUID & p_inputtype,const char * p_data,t_size p_data_length);
private:
	titleformat_text_out * m_chain;
	const char * m_restricted_chars;
};

class titleformat_text_out_impl_string : public titleformat_text_out {
public:
	titleformat_text_out_impl_string(pfc::string_receiver & p_string) : m_string(p_string) {}
	void write(const GUID &,const char * p_data,t_size p_data_length) override {m_string.add_string(p_data,p_data_length);}
private:
	pfc::string_receiver & m_string;
};

class titleformat_common_methods : public service_base {
public:
	virtual bool process_field(const file_info & p_info,const playable_location & p_location,titleformat_text_out * p_out,const char * p_name,t_size p_name_length,bool & p_found_flag) = 0;
	virtual bool process_function(const file_info & p_info,const playable_location & p_location,titleformat_text_out * p_out,const char * p_name,t_size p_name_length,titleformat_hook_function_params * p_params,bool & p_found_flag) = 0;
	virtual bool remap_meta(const file_info & p_info,t_size & p_index, const char * p_name, t_size p_name_length) = 0;
	
	FB2K_MAKE_SERVICE_COREAPI(titleformat_common_methods);
};

class titleformat_hook_impl_file_info : public titleformat_hook
{
public:
	titleformat_hook_impl_file_info(const playable_location & p_location,const file_info * p_info) : m_info(p_info), m_location(p_location) {}//caller must ensure that referenced file_info object is alive as long as the titleformat_hook_impl_file_info instance
	bool process_field(titleformat_text_out * p_out,const char * p_name,t_size p_name_length,bool & p_found_flag);
	bool process_function(titleformat_text_out * p_out,const char * p_name,t_size p_name_length,titleformat_hook_function_params * p_params,bool & p_found_flag);
protected:
	bool remap_meta(t_size & p_index, const char * p_name, t_size p_name_length) {return m_api->remap_meta(*m_info,p_index,p_name,p_name_length);}
	const file_info * m_info;
private:
	const playable_location & m_location;
	const titleformat_common_methods::ptr m_api = titleformat_common_methods::get();
};

class titleformat_hook_impl_splitter : public titleformat_hook {
public:
	inline titleformat_hook_impl_splitter(titleformat_hook * p_hook1,titleformat_hook * p_hook2) : m_hook1(p_hook1), m_hook2(p_hook2) {}
	bool process_field(titleformat_text_out * p_out,const char * p_name,t_size p_name_length,bool & p_found_flag);
	bool process_function(titleformat_text_out * p_out,const char * p_name,t_size p_name_length,titleformat_hook_function_params * p_params,bool & p_found_flag);
private:
	titleformat_hook * m_hook1, * m_hook2;
};

class titleformat_text_filter_impl_reserved_chars : public titleformat_text_filter {
public:
	titleformat_text_filter_impl_reserved_chars(const char * p_reserved_chars) : m_reserved_chars(p_reserved_chars) {}
	void write(const GUID & p_inputtype,pfc::string_receiver & p_out,const char * p_data,t_size p_data_length) override;
private:
	const char * m_reserved_chars;
};

class titleformat_text_filter_impl_filename_chars : public titleformat_text_filter {
public:
	void write(const GUID & p_inputType,pfc::string_receiver & p_out,const char * p_data,t_size p_dataLength) override;
};

class titleformat_text_filter_nontext_chars : public titleformat_text_filter {
public:
	inline static bool isReserved(char c) { return c >= 0 && c < 0x20; }
	void write(const GUID & p_inputtype,pfc::string_receiver & p_out,const char * p_data,t_size p_data_length) override;
};







class titleformat_hook_impl_list : public titleformat_hook {
public:
	titleformat_hook_impl_list(t_size p_index /* zero-based! */,t_size p_total) : m_index(p_index), m_total(p_total) {}
	
	bool process_field(titleformat_text_out * p_out,const char * p_name,t_size p_name_length,bool & p_found_flag) override {
		if (
			pfc::stricmp_ascii_ex(p_name,p_name_length,"list_index",SIZE_MAX) == 0
			) {
			p_out->write_int_padded(titleformat_inputtypes::unknown,m_index+1, m_total);
			p_found_flag = true; return true;
		} else if (
            pfc::stricmp_ascii_ex(p_name,p_name_length,"list_total",SIZE_MAX) == 0
			) {
			p_out->write_int(titleformat_inputtypes::unknown,m_total);
			p_found_flag = true; return true;			
		} else {
			p_found_flag = false; return false;
		}
	}

	bool process_function(titleformat_text_out *,const char *,t_size,titleformat_hook_function_params *,bool &) override {return false;}

private:
	t_size m_index, m_total;
};

class string_formatter_tf : public pfc::string_base {
	[[noreturn]] static void verboten() { FB2K_BugCheck(); }
public:
	string_formatter_tf(titleformat_text_out * out, const GUID & inputType = titleformat_inputtypes::meta) : m_out(out), m_inputType(inputType) {}

	const char * get_ptr() const override {
		verboten();
	}
	void add_string(const char * p_string,t_size p_length) override {
		m_out->write(m_inputType,p_string,p_length);
	}
	void set_string(const char *,t_size) override {
		verboten();
	}
	void truncate(t_size) override {
		verboten();
	}
	t_size get_length() const override {
		verboten();
	}
	char * lock_buffer(t_size) override {
		verboten();
	}
	void unlock_buffer() override {
		verboten();
	}

private:
	titleformat_text_out * const m_out;
	const GUID m_inputType;
};


class titleformat_object_cache {
public:
	titleformat_object_cache(const char * pattern) : m_pattern(pattern) {}
	operator titleformat_object::ptr() {
		PFC_ASSERT(core_api::assert_main_thread());
		if (m_obj.is_empty()) {
			titleformat_compiler::get()->compile_force(m_obj, m_pattern);
		}
		return m_obj;
	}
private:
	const char * const m_pattern;
	titleformat_object::ptr m_obj;
};


class titleformat_patterns {
public:
    static const char * patternAlbumSplit();
    static const char * patternSortTracks();
};
