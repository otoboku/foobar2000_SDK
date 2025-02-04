#pragma once

#ifdef FOOBAR2000_HAVE_CFG_VAR_LEGACY
#include <vector>
#include "filesystem.h" // stream_reader, stream_writer

#if FOOBAR2000_SUPPORT_CFG_VAR_DOWNGRADE
#include "configStore.h"
#include "initquit.h"
#endif

namespace cfg_var_legacy {
#define CFG_VAR_ASSERT_SAFEINIT PFC_ASSERT(!core_api::are_services_available());/*imperfect check for nonstatic instantiation*/

	//! Reader part of cfg_var object. In most cases, you should use cfg_var instead of using cfg_var_reader directly.
	class NOVTABLE cfg_var_reader {
	public:
		//! @param p_guid GUID of the variable, used to identify variable implementations owning specific configuration file entries when reading the configuration file back. You must generate a new GUID every time you declare a new cfg_var.
		cfg_var_reader(const GUID& guid) : m_guid(guid) { CFG_VAR_ASSERT_SAFEINIT; m_next = g_list; g_list = this; }
		~cfg_var_reader() { CFG_VAR_ASSERT_SAFEINIT; }

		//! Sets state of the variable. Called only from main thread, when reading configuration file.
		//! @param p_stream Stream containing new state of the variable.
		//! @param p_sizehint Number of bytes contained in the stream; reading past p_sizehint bytes will fail (EOF).
		virtual void set_data_raw(stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort) = 0;

#if FOOBAR2000_SUPPORT_CFG_VAR_DOWNGRADE
	private:
		pfc::string8 m_downgrade_name;
	public:
		pfc::string8 downgrade_name() const;
		void downgrade_set_name( const char * arg ) { m_downgrade_name = arg; }

		//! Implementation of config downgrade for this var, see FOOBAR2000_SUPPORT_CFG_VAR_DOWNGRADE for more info. \n
		//! Most components should not use this.
		virtual void downgrade_check(fb2k::configStore::ptr api) {}
		//! Config downgrade main for your component, see FOOBAR2000_SUPPORT_CFG_VAR_DOWNGRADE for more info. \n
		//! Put FOOBAR2000_IMPLEMENT_CFG_VAR_DOWNGRADE somewhere in your component to call on startup, or call from your code as early as possible after config read. \n
		//! If you call it more than once, spurious calls will be ignored.
		static void downgrade_main();
#endif

		//! For internal use only, do not call.
		static void config_read_file(stream_reader* p_stream, abort_callback& p_abort);

		const GUID m_guid;
	private:
		static cfg_var_reader* g_list;
		cfg_var_reader* m_next;

		PFC_CLASS_NOT_COPYABLE_EX(cfg_var_reader)
	};

	//! Writer part of cfg_var object. In most cases, you should use cfg_var instead of using cfg_var_writer directly.
	class NOVTABLE cfg_var_writer {
	public:
		//! @param p_guid GUID of the variable, used to identify variable implementations owning specific configuration file entries when reading the configuration file back. You must generate a new GUID every time you declare a new cfg_var.
		cfg_var_writer(const GUID& guid) : m_guid(guid) { CFG_VAR_ASSERT_SAFEINIT; m_next = g_list; g_list = this; }
		~cfg_var_writer() { CFG_VAR_ASSERT_SAFEINIT; }

		//! Retrieves state of the variable. Called only from main thread, when writing configuration file.
		//! @param p_stream Stream receiving state of the variable.
		virtual void get_data_raw(stream_writer* p_stream, abort_callback& p_abort) = 0;

		//! For internal use only, do not call.
		static void config_write_file(stream_writer* p_stream, abort_callback& p_abort);

		const GUID m_guid;
	private:
		static cfg_var_writer* g_list;
		cfg_var_writer* m_next;

		PFC_CLASS_NOT_COPYABLE_EX(cfg_var_writer)
	};

	//! Base class for configuration variable classes; provides self-registration mechaisms and methods to set/retrieve configuration data; those methods are automatically called for all registered instances by backend when configuration file is being read or written.\n
	//! Note that cfg_var class and its derivatives may be only instantiated statically (as static objects or members of other static objects), NEVER dynamically (operator new, local variables, members of objects instantiated as such).
	class NOVTABLE cfg_var : public cfg_var_reader, public cfg_var_writer {
	protected:
		//! @param p_guid GUID of the variable, used to identify variable implementations owning specific configuration file entries when reading the configuration file back. You must generate a new GUID every time you declare a new cfg_var.
		cfg_var(const GUID& p_guid) : cfg_var_reader(p_guid), cfg_var_writer(p_guid) {}
	public:
		GUID get_guid() const { return cfg_var_reader::m_guid; }
	};

#if FOOBAR2000_SUPPORT_CFG_VAR_DOWNGRADE
	int64_t downgrade_this( fb2k::configStore::ptr api, const char*, int64_t current);
	uint64_t downgrade_this( fb2k::configStore::ptr api, const char*, uint64_t current);
	int32_t downgrade_this( fb2k::configStore::ptr api, const char*, int32_t current);
	uint32_t downgrade_this( fb2k::configStore::ptr api, const char*, uint32_t current);
	bool downgrade_this( fb2k::configStore::ptr api, const char*, bool current);
	double downgrade_this( fb2k::configStore::ptr api, const char*, double current);
	GUID downgrade_this( fb2k::configStore::ptr api, const char*, GUID current);
#endif

	//! Generic integer config variable class. Template parameter can be used to specify integer type to use.\n
	//! Note that cfg_var class and its derivatives may be only instantiated statically (as static objects or members of other static objects), NEVER dynamically (operator new, local variables, members of objects instantiated as such).
	template<typename t_inttype>
	class cfg_int_t : public cfg_var {
	private:
		t_inttype m_val;
	protected:
		void get_data_raw(stream_writer* p_stream, abort_callback& p_abort) override { p_stream->write_lendian_t(m_val, p_abort); }
		void set_data_raw(stream_reader* p_stream, t_size, abort_callback& p_abort) override {
			t_inttype temp;
			p_stream->read_lendian_t(temp, p_abort);//alter member data only on success, this will throw an exception when something isn't right
			m_val = temp;
		}
#if FOOBAR2000_SUPPORT_CFG_VAR_DOWNGRADE
		void downgrade_check(fb2k::configStore::ptr api) override {
			m_val = downgrade_this(api, this->downgrade_name(), m_val);
		}
#endif
	public:
		//! @param p_guid GUID of the variable, used to identify variable implementations owning specific configuration file entries when reading the configuration file back. You must generate a new GUID every time you declare a new cfg_var.
		//! @param p_default Default value of the variable.
		explicit inline cfg_int_t(const GUID& p_guid, t_inttype p_default) : cfg_var(p_guid), m_val(p_default) {}

		inline const cfg_int_t<t_inttype>& operator=(const cfg_int_t<t_inttype>& p_val) { m_val = p_val.m_val; return *this; }
		inline t_inttype operator=(t_inttype p_val) { m_val = p_val; return m_val; }

		inline operator t_inttype() const { return m_val; }

		inline t_inttype get_value() const { return m_val; }
		inline t_inttype get() const { return m_val; }
	};

	typedef cfg_int_t<t_int32> cfg_int;
	typedef cfg_int_t<t_uint32> cfg_uint;
	typedef cfg_int_t<GUID> cfg_guid; // ANNOYING OLD DESIGN. THIS DOESN'T BELONG HERE BUT CANNOT BE CHANGED WITHOUT BREAKING PEOPLE'S STUFF. BLARFGH.
	typedef cfg_int_t<bool> cfg_bool; // See above.
	typedef cfg_int_t<float> cfg_float; // See above %$!@#$
	typedef cfg_int_t<double> cfg_double; // ....

	//! String config variable. Stored in the stream with int32 header containing size in bytes, followed by non-null-terminated UTF-8 data.\n
	//! Note that cfg_var class and its derivatives may be only instantiated statically (as static objects or members of other static objects), NEVER dynamically (operator new, local variables, members of objects instantiated as such).
	class cfg_string : public cfg_var, public pfc::string8 {
	protected:
		void get_data_raw(stream_writer* p_stream, abort_callback& p_abort) override;
		void set_data_raw(stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort) override;

#if FOOBAR2000_SUPPORT_CFG_VAR_DOWNGRADE
		void downgrade_check(fb2k::configStore::ptr) override;
#endif
	public:
		//! @param p_guid GUID of the variable, used to identify variable implementations owning specific configuration file entries when reading the configuration file back. You must generate a new GUID every time you declare a new cfg_var.
		//! @param p_defaultval Default/initial value of the variable.
		explicit inline cfg_string(const GUID& p_guid, const char* p_defaultval) : cfg_var(p_guid), pfc::string8(p_defaultval) {}

		const cfg_string& operator=(const cfg_string& p_val) { set_string(p_val); return *this; }
		const cfg_string& operator=(const char* p_val) { set_string(p_val); return *this; }
		const cfg_string& operator=(pfc::string8 && p_val) { set(std::move(p_val)); return *this; }


		inline operator const char* () const { return get_ptr(); }
		const pfc::string8& get() const { return *this; }
		void set( const char * arg ) { this->set_string(arg); }
		void set(pfc::string8&& arg) { pfc::string8 * pThis = this; *pThis = std::move(arg); }
	};


	class cfg_string_mt : public cfg_var {
	protected:
		void get_data_raw(stream_writer* p_stream, abort_callback& p_abort) override {
			pfc::string8 temp;
			get(temp);
			p_stream->write_object(temp.get_ptr(), temp.length(), p_abort);
		}
		void set_data_raw(stream_reader* p_stream, t_size, abort_callback& p_abort) override {
			pfc::string8_fastalloc temp;
			p_stream->read_string_raw(temp, p_abort);
			set(temp);
		}
#if FOOBAR2000_SUPPORT_CFG_VAR_DOWNGRADE
		void downgrade_check(fb2k::configStore::ptr) override;
#endif
	public:
		cfg_string_mt(const GUID& id, const char* defVal) : cfg_var(id), m_val(defVal) {}
		void get(pfc::string_base& out) const { inReadSync(m_sync); out = m_val; }
		pfc::string8 get() const { inReadSync(m_sync); return m_val; }
		void set(const char* val, t_size valLen = SIZE_MAX) { inWriteSync(m_sync); m_val.set_string(val, valLen); }
		void set( pfc::string8 && val ) { inWriteSync(m_sync); m_val = std::move(val); }
	private:
		mutable pfc::readWriteLock m_sync;
		pfc::string8 m_val;
	};

	//! Struct config variable template. Warning: not endian safe, should be used only for nonportable code.\n
	//! Note that cfg_var class and its derivatives may be only instantiated statically (as static objects or members of other static objects), NEVER dynamically (operator new, local variables, members of objects instantiated as such).
	template<typename t_struct>
	class cfg_struct_t : public cfg_var {
	private:
		t_struct m_val = {};
	protected:

		void get_data_raw(stream_writer* p_stream, abort_callback& p_abort) { p_stream->write_object(&m_val, sizeof(m_val), p_abort); }
		void set_data_raw(stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort) {
			t_struct temp;
			p_stream->read_object(&temp, sizeof(temp), p_abort);
			m_val = temp;
		}
	public:
		//! @param p_guid GUID of the variable, used to identify variable implementations owning specific configuration file entries when reading the configuration file back. You must generate a new GUID every time you declare a new cfg_var.
		inline cfg_struct_t(const GUID& p_guid, const t_struct& p_val) : cfg_var(p_guid), m_val(p_val) {}
		//! @param p_guid GUID of the variable, used to identify variable implementations owning specific configuration file entries when reading the configuration file back. You must generate a new GUID every time you declare a new cfg_var.
		inline cfg_struct_t(const GUID& p_guid, int filler) : cfg_var(p_guid) { memset(&m_val, filler, sizeof(t_struct)); }
		//! @param p_guid GUID of the variable, used to identify variable implementations owning specific configuration file entries when reading the configuration file back. You must generate a new GUID every time you declare a new cfg_var.
		inline cfg_struct_t(const GUID& p_guid) : cfg_var(p_guid) {}

		inline const cfg_struct_t<t_struct>& operator=(const cfg_struct_t<t_struct>& p_val) { m_val = p_val.get_value(); return *this; }
		inline const cfg_struct_t<t_struct>& operator=(const t_struct& p_val) { m_val = p_val; return *this; }

		inline const t_struct& get_value() const { return m_val; }
		inline t_struct& get_value() { return m_val; }
		inline operator t_struct() const { return m_val; }

		void set(t_struct&& arg) { m_val = std::move(arg); }
		void set(t_struct const& arg) { m_val = arg; }
		t_struct get() const { return m_val; }
	};


	template<typename TObj>
	class cfg_objList : public cfg_var, public pfc::list_t<TObj> {
	public:
		typedef TObj item_t;
		typedef cfg_objList<item_t> t_self;
		cfg_objList(const GUID& guid) : cfg_var(guid) {}
		template<typename TSource, unsigned Count> cfg_objList(const GUID& guid, const TSource(&source)[Count]) : cfg_var(guid) {
			reset(source);
		}
		template<typename TSource, unsigned Count> void reset(const TSource(&source)[Count]) {
			this->set_size(Count); for (t_size walk = 0; walk < Count; ++walk) (*this)[walk] = source[walk];
		}
		void get_data_raw(stream_writer* p_stream, abort_callback& p_abort) {
			stream_writer_formatter<> out(*p_stream, p_abort);
			out << pfc::downcast_guarded<t_uint32>(this->get_size());
			for (t_size walk = 0; walk < this->get_size(); ++walk) out << (*this)[walk];
		}
		void set_data_raw(stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort) {
			try {
				stream_reader_formatter<> in(*p_stream, p_abort);
				t_uint32 count; in >> count;
				this->set_count(count);
				for (t_uint32 walk = 0; walk < count; ++walk) in >> (*this)[walk];
			} catch (...) {
				this->remove_all();
				throw;
			}
		}
		template<typename t_in> t_self& operator=(t_in const& source) { this->remove_all(); this->add_items(source); return *this; }
		template<typename t_in> t_self& operator+=(t_in const& p_source) { this->add_item(p_source); return *this; }
		template<typename t_in> t_self& operator|=(t_in const& p_source) { this->add_items(p_source); return *this; }


		std::vector<item_t> get() const {
			std::vector<item_t> ret; ret.reserve(this->size());
			for (auto& item : *this) ret.push_back(item);
			return ret;
		}
		template<typename arg_t> void set(arg_t&& arg) {
			this->remove_all();
			this->prealloc(std::size(arg));
			for (auto& item : arg) {
				this->add_item(item);
			}
		}
	};
	template<typename TList>
	class cfg_objListEx : public cfg_var, public TList {
	public:
		typedef cfg_objListEx<TList> t_self;
		cfg_objListEx(const GUID& guid) : cfg_var(guid) {}
		void get_data_raw(stream_writer* p_stream, abort_callback& p_abort) {
			stream_writer_formatter<> out(*p_stream, p_abort);
			out << pfc::downcast_guarded<t_uint32>(this->get_count());
			for (typename TList::const_iterator walk = this->first(); walk.is_valid(); ++walk) out << *walk;
		}
		void set_data_raw(stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort) {
			this->remove_all();
			stream_reader_formatter<> in(*p_stream, p_abort);
			t_uint32 count; in >> count;
			for (t_uint32 walk = 0; walk < count; ++walk) {
				typename TList::t_item item; in >> item; this->add_item(item);
			}
		}
		template<typename t_in> t_self& operator=(t_in const& source) { this->remove_all(); this->add_items(source); return *this; }
		template<typename t_in> t_self& operator+=(t_in const& p_source) { this->add_item(p_source); return *this; }
		template<typename t_in> t_self& operator|=(t_in const& p_source) { this->add_items(p_source); return *this; }
	};

	template<typename TObj>
	class cfg_obj : public cfg_var, public TObj {
	public:
		cfg_obj(const GUID& guid) : cfg_var(guid), TObj() {}
		template<typename TInitData> cfg_obj(const GUID& guid, const TInitData& initData) : cfg_var(guid), TObj(initData) {}

		TObj& val() { return *this; }
		TObj const& val() const { return *this; }
		TObj get() const { return val(); }
		template<typename arg_t> void set(arg_t&& arg) { val() = std::forward<arg_t>(arg); }

		void get_data_raw(stream_writer* p_stream, abort_callback& p_abort) {
			stream_writer_formatter<> out(*p_stream, p_abort);
			const TObj* ptr = this;
			out << *ptr;
		}
		void set_data_raw(stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort) {
			stream_reader_formatter<> in(*p_stream, p_abort);
			TObj* ptr = this;
			in >> *ptr;
		}
	};

	template<typename TObj, typename TImport> class cfg_objListImporter : private cfg_var_reader {
	public:
		typedef cfg_objList<TObj> TMasterVar;
		cfg_objListImporter(TMasterVar& var, const GUID& guid) : m_var(var), cfg_var_reader(guid) {}

	private:
		void set_data_raw(stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort) {
			TImport temp;
			try {
				stream_reader_formatter<> in(*p_stream, p_abort);
				t_uint32 count; in >> count;
				m_var.set_count(count);
				for (t_uint32 walk = 0; walk < count; ++walk) {
					in >> temp;
					m_var[walk] = temp;
				}
			} catch (...) {
				m_var.remove_all();
				throw;
			}
		}
		TMasterVar& m_var;
	};
	template<typename TMap> class cfg_objMap : private cfg_var, public TMap {
	public:
		cfg_objMap(const GUID& id) : cfg_var(id) {}
	private:
		void get_data_raw(stream_writer* p_stream, abort_callback& p_abort) {
			stream_writer_formatter<> out(*p_stream, p_abort);
			out << pfc::downcast_guarded<t_uint32>(this->get_count());
			for (typename TMap::const_iterator walk = this->first(); walk.is_valid(); ++walk) {
				out << walk->m_key << walk->m_value;
			}
		}
		void set_data_raw(stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort) {
			this->remove_all();
			stream_reader_formatter<> in(*p_stream, p_abort);
			t_uint32 count; in >> count;
			for (t_uint32 walk = 0; walk < count; ++walk) {
				typename TMap::t_key key; in >> key; PFC_ASSERT(!this->have_item(key));
				try { in >> this->find_or_add(key); } catch (...) { this->remove(key); throw; }
			}
		}
	};
#if FOOBAR2000_SUPPORT_CFG_VAR_DOWNGRADE
#define FOOBAR2000_IMPLEMENT_CFG_VAR_DOWNGRADE FB2K_RUN_ON_INIT(cfg_var_reader::downgrade_main)
#endif // FOOBAR2000_SUPPORT_CFG_VAR_DOWNGRADE
} // cfg_var_legacy
#else
namespace cfg_var_legacy {
	// Dummy class
	class cfg_var_reader {
	public:
		cfg_var_reader(const GUID& id) : m_guid(id) {}
		const GUID m_guid;
	};
}
#endif

#ifndef FOOBAR2000_IMPLEMENT_CFG_VAR_DOWNGRADE
#define FOOBAR2000_IMPLEMENT_CFG_VAR_DOWNGRADE
#endif