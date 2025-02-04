#include "foobar2000-sdk-pch.h"
#include "foosort.h"
#include "threadPool.h"
#include "foosortstring.h"
#include <vector>
#include "titleformat.h"
#include "library_manager.h"
#include "genrand.h"

namespace {

	struct custom_sort_data_multi {
		static constexpr unsigned numLocal = 4;
		void setup(size_t count) {
			if (count > numLocal) texts2 = std::make_unique< fb2k::sortString_t[] >( count - numLocal );
		}
		fb2k::sortString_t& operator[] (size_t which) {
			return which < numLocal ? texts1[which] : texts2[which - numLocal];
		}
		const fb2k::sortString_t& operator[] (size_t which) const {
			return which < numLocal ? texts1[which] : texts2[which - numLocal];
		}

		fb2k::sortString_t texts1[numLocal];
		std::unique_ptr< fb2k::sortString_t[] > texts2;
		size_t index;
	};
	struct custom_sort_data {
		fb2k::sortString_t text;
		size_t index;
	};
	template<int direction>
	static int custom_sort_compare(const custom_sort_data& elem1, const custom_sort_data& elem2) {
		int ret = direction * fb2k::sortStringCompare(elem1.text, elem2.text);
		if (ret == 0) ret = pfc::sgn_t((t_ssize)elem1.index - (t_ssize)elem2.index);
		return ret;
	}

}

void metadb_handle_list_helper::sort_by_format(metadb_handle_list_ref p_list,const char * spec,titleformat_hook * p_hook)
{
	service_ptr_t<titleformat_object> script;
	if (titleformat_compiler::get()->compile(script,spec))
		sort_by_format(p_list,script,p_hook);
}

void metadb_handle_list_helper::sort_by_format_get_order(metadb_handle_list_cref p_list,t_size* order,const char * spec,titleformat_hook * p_hook)
{
	service_ptr_t<titleformat_object> script;
	if (titleformat_compiler::get()->compile(script,spec))
		sort_by_format_get_order(p_list,order,script,p_hook);
}

void metadb_handle_list_helper::sort_by_format(metadb_handle_list_ref p_list,const service_ptr_t<titleformat_object> & p_script,titleformat_hook * p_hook, int direction)
{
	const t_size count = p_list.get_count();
	pfc::array_t<t_size> order; order.set_size(count);
	sort_by_format_get_order(p_list,order.get_ptr(),p_script,p_hook,direction);
	p_list.reorder(order.get_ptr());
}

namespace {

	class tfhook_sort : public titleformat_hook {
	public:
		tfhook_sort() {
			m_API->seed();
		}
		bool process_field(titleformat_text_out *,const char *,t_size,bool &) override {
			return false;
		}
		bool process_function(titleformat_text_out * p_out,const char * p_name,t_size p_name_length,titleformat_hook_function_params * p_params,bool & p_found_flag) override {
			if (stricmp_utf8_ex(p_name, p_name_length, "rand", SIZE_MAX) == 0) {
				t_size param_count = p_params->get_param_count();
				t_uint32 val;
				if (param_count == 1) {
					t_uint32 mod = (t_uint32)p_params->get_param_uint(0);
					if (mod > 0) {
						val = m_API->genrand(mod);
					} else {
						val = 0;
					}
				} else {
					val = m_API->genrand(0xFFFFFFFF);
				}
				p_out->write_int(titleformat_inputtypes::unknown, val);
				p_found_flag = true;
				return true;
			} else {
				return false;
			}
		}
	private:
		genrand_service::ptr m_API = genrand_service::get();
	};
}

void metadb_handle_list_helper::sort_by_format_get_order(metadb_handle_list_cref p_list,t_size* order,const service_ptr_t<titleformat_object> & p_script,titleformat_hook * p_hook,int p_direction)
{
	sort_by_format_get_order_v2(p_list, order, p_script, p_hook, p_direction, fb2k::noAbort );
}

void metadb_handle_list_helper::sort_by_relative_path(metadb_handle_list_ref p_list)
{
	const t_size count = p_list.get_count();
	pfc::array_t<t_size> order; order.set_size(count);
	sort_by_relative_path_get_order(p_list,order.get_ptr());
	p_list.reorder(order.get_ptr());
}

void metadb_handle_list_helper::sort_by_relative_path_get_order(metadb_handle_list_cref p_list,t_size* order)
{
	const t_size count = p_list.get_count();
	t_size n;
	std::vector<custom_sort_data> data;
	data.resize(count);
	auto api = library_manager::get();
	
	pfc::string8_fastalloc temp;
	temp.prealloc(512);
	for(n=0;n<count;n++)
	{
		metadb_handle_ptr item;
		p_list.get_item_ex(item,n);
		if (!api->get_relative_path(item,temp)) temp = "";
		data[n].index = n;
		data[n].text = fb2k::makeSortString(temp);
		//data[n].subsong = item->get_subsong_index();
	}

	pfc::sort_t(data,custom_sort_compare<1>,count);
	//qsort(data.get_ptr(),count,sizeof(custom_sort_data),(int (__cdecl *)(const void *elem1, const void *elem2 ))custom_sort_compare);

	for(n=0;n<count;n++)
	{
		order[n]=data[n].index;
	}
}

void metadb_handle_list_helper::remove_duplicates(metadb_handle_list_ref p_list)
{
	t_size count = p_list.get_count();
	if (count>0)
	{
		pfc::bit_array_bittable mask(count);
		pfc::array_t<t_size> order; order.set_size(count);
		order_helper::g_fill(order);

		p_list.sort_get_permutation_t(pfc::compare_t<metadb_handle_ptr,metadb_handle_ptr>,order.get_ptr());
		
		t_size n;
		bool found = false;
		for(n=0;n<count-1;n++)
		{
			if (p_list.get_item(order[n])==p_list.get_item(order[n+1]))
			{
				found = true;
				mask.set(order[n+1],true);
			}
		}
		
		if (found) p_list.remove_mask(mask);
	}
}

void metadb_handle_list_helper::sort_by_pointer_remove_duplicates(metadb_handle_list_ref p_list)
{
	const t_size count = p_list.get_count();
	if (count>0)
	{
		sort_by_pointer(p_list);
		bool b_found = false;
		for(size_t n=0;n<count-1;n++)
		{
			if (p_list.get_item(n)==p_list.get_item(n+1))
			{
				b_found = true;
				break;
			}
		}

		if (b_found)
		{
			pfc::bit_array_bittable mask(count);
			for(size_t n=0;n<count-1;n++)
			{
				if (p_list.get_item(n)==p_list.get_item(n+1))
					mask.set(n+1,true);
			}
			p_list.remove_mask(mask);
		}
	}
}

void metadb_handle_list_helper::sort_by_path_quick(metadb_handle_list_ref p_list)
{
	p_list.sort_t(metadb::path_compare_metadb_handle);
}


void metadb_handle_list_helper::sort_by_pointer(metadb_handle_list_ref p_list)
{
	p_list.sort();
}

t_size metadb_handle_list_helper::bsearch_by_pointer(metadb_handle_list_cref p_list,const metadb_handle_ptr & val)
{
	t_size blah;
	if (p_list.bsearch_t(pfc::compare_t<metadb_handle_ptr,metadb_handle_ptr>,val,blah)) return blah;
	else return SIZE_MAX;
}


void metadb_handle_list_helper::sorted_by_pointer_extract_difference(metadb_handle_list const & p_list_1,metadb_handle_list const & p_list_2,metadb_handle_list & p_list_1_specific,metadb_handle_list & p_list_2_specific)
{
	t_size found_1, found_2;
	const t_size count_1 = p_list_1.get_count(), count_2 = p_list_2.get_count();
	t_size ptr_1, ptr_2;

	found_1 = found_2 = 0;
	ptr_1 = ptr_2 = 0;
	while(ptr_1 < count_1 || ptr_2 < count_2)
	{
		while(ptr_1 < count_1 && (ptr_2 == count_2 || p_list_1[ptr_1] < p_list_2[ptr_2]))
		{
			found_1++;
			t_size ptr_1_new = ptr_1 + 1;
			while(ptr_1_new < count_1 && p_list_1[ptr_1_new] == p_list_1[ptr_1]) ptr_1_new++;
			ptr_1 = ptr_1_new;
		}
		while(ptr_2 < count_2 && (ptr_1 == count_1 || p_list_2[ptr_2] < p_list_1[ptr_1]))
		{
			found_2++;
			t_size ptr_2_new = ptr_2 + 1;
			while(ptr_2_new < count_2 && p_list_2[ptr_2_new] == p_list_2[ptr_2]) ptr_2_new++;
			ptr_2 = ptr_2_new;
		}
		while(ptr_1 < count_1 && ptr_2 < count_2 && p_list_1[ptr_1] == p_list_2[ptr_2]) {ptr_1++; ptr_2++;}
	}

	

	p_list_1_specific.set_count(found_1);
	p_list_2_specific.set_count(found_2);
	if (found_1 > 0 || found_2 > 0)
	{
		found_1 = found_2 = 0;
		ptr_1 = ptr_2 = 0;

		while(ptr_1 < count_1 || ptr_2 < count_2)
		{
			while(ptr_1 < count_1 && (ptr_2 == count_2 || p_list_1[ptr_1] < p_list_2[ptr_2]))
			{
				p_list_1_specific[found_1++] = p_list_1[ptr_1];
				t_size ptr_1_new = ptr_1 + 1;
				while(ptr_1_new < count_1 && p_list_1[ptr_1_new] == p_list_1[ptr_1]) ptr_1_new++;
				ptr_1 = ptr_1_new;
			}
			while(ptr_2 < count_2 && (ptr_1 == count_1 || p_list_2[ptr_2] < p_list_1[ptr_1]))
			{
				p_list_2_specific[found_2++] = p_list_2[ptr_2];
				t_size ptr_2_new = ptr_2 + 1;
				while(ptr_2_new < count_2 && p_list_2[ptr_2_new] == p_list_2[ptr_2]) ptr_2_new++;
				ptr_2 = ptr_2_new;
			}
			while(ptr_1 < count_1 && ptr_2 < count_2 && p_list_1[ptr_1] == p_list_2[ptr_2]) {ptr_1++; ptr_2++;}
		}

	}
}

double metadb_handle_list_helper::calc_total_duration_v2(metadb_handle_list_cref p_list, unsigned maxThreads, abort_callback & aborter) {
	const size_t count = p_list.get_count();
	size_t numThreads = pfc::getOptimalWorkerThreadCountEx( pfc::min_t<size_t>(maxThreads, count / 2000 ));
	if (numThreads == 1) {
		double ret = 0;
		for (size_t n = 0; n < count; n++)
		{
			double temp = p_list.get_item(n)->get_length();
			if (temp > 0) ret += temp;
		}
		return ret;
	}

	pfc::array_t<double> sums; sums.resize(numThreads); sums.fill_null();

	{
		pfc::refcounter walk = 0, walkSums = 0;

		auto worker = [&] {
			double ret = 0;
			for (;;) {
				size_t idx = walk++;
				if (idx >= count || aborter.is_set()) break;

				double temp = p_list.get_item(idx)->get_length();
				if (temp > 0) ret += temp;
			}
			sums[walkSums++] = ret;
		};

		fb2k::cpuThreadPool::runMultiHelper(worker, numThreads);
	}
	aborter.check();
	double ret = 0;
	for (size_t walk = 0; walk < numThreads; ++walk) ret += sums[walk];
	return ret;	
}

pfc::string8 metadb_handle_list_helper::format_total_size(metadb_handle_list_cref p_list) {
    pfc::string8 temp;
    bool unknown = false;
    t_filesize val = metadb_handle_list_helper::calc_total_size_ex(p_list,unknown);
    if (unknown) temp << "> ";
    temp << pfc::format_file_size_short(val);
    return temp;
}

double metadb_handle_list_helper::calc_total_duration(metadb_handle_list_cref p_list)
{
	double ret = 0;
	for (auto handle : p_list) {
		double temp = handle->get_length();
		if (temp > 0) ret += temp;
	}
	return ret;
}

void metadb_handle_list_helper::sort_by_path(metadb_handle_list_ref p_list)
{
	sort_by_format(p_list,"%path_sort%",NULL);
}

void metadb_handle_list_helper::sort_by_format_v2(metadb_handle_list_ref p_list, const service_ptr_t<titleformat_object> & script, titleformat_hook * hook, int direction, abort_callback & aborter) {
	pfc::array_t<size_t> order; order.set_size( p_list.get_count() );
	sort_by_format_get_order_v2( p_list, order.get_ptr(), script, hook, direction, aborter );
	p_list.reorder( order.get_ptr() );
}

void metadb_handle_list_helper::sort_by_format_get_order_v2(metadb_handle_list_cref p_list, size_t * order, const service_ptr_t<titleformat_object> & p_script, titleformat_hook * p_hook, int p_direction, abort_callback & aborter) {
	sorter_t s = { p_script, p_direction, p_hook };
	size_t total = p_list.get_count();
	for (size_t walk = 0; walk < total; ++walk) order[walk] = walk;
	sort_by_format_get_order_v3(p_list, order, &s, 1, aborter);
}

void metadb_handle_list_helper::sort_by_format_get_order_v3(metadb_handle_list_cref p_list, size_t* order,sorter_t const* sorters, size_t nSorters, abort_callback& aborter) {
	// pfc::hires_timer timer; timer.start();

	typedef custom_sort_data_multi data_t;

	const t_size count = p_list.get_count();
	if (count == 0) return;

	PFC_ASSERT(pfc::permutation_is_valid(order, count));

	auto data = std::make_unique< data_t[] >(count);

#if FOOBAR2000_TARGET_VERSION >= 81
	bool need_info = false;
	for (size_t iSorter = 0; iSorter < nSorters; ++iSorter) {
		auto& s = sorters[iSorter];
		PFC_ASSERT(s.direction == -1 || s.direction == 1);
		if (s.obj->requires_metadb_info_()) {
			need_info = true; break;
		}
	}
	if (need_info) {
		// FB2K_console_formatter() << "sorting with queryMultiParallelEx_<>";
		struct qmpc_context {
			qmpc_context() {
				temp.prealloc(512);
			}
			tfhook_sort myHook;
			pfc::string8 temp;
		};
		metadb_v2::get()->queryMultiParallelEx_< qmpc_context >(p_list, [&](size_t idx, metadb_v2::rec_t const& rec, qmpc_context& ctx) {
			aborter.check();
			auto& out = data[idx];
			out.setup(nSorters);
			out.index = order[idx];

			auto h = p_list[idx];
			
			for (size_t iSorter = 0; iSorter < nSorters; ++iSorter) {
				auto& s = sorters[iSorter];
				if (s.hook) {
					titleformat_hook_impl_splitter hookSplitter(&ctx.myHook, s.hook);
					h->formatTitle_v2_(rec, &hookSplitter, ctx.temp, s.obj, nullptr);
				} else {
					h->formatTitle_v2_(rec, &ctx.myHook, ctx.temp, s.obj, nullptr);
				}
				out[iSorter] = fb2k::makeSortString(ctx.temp);
			}
		});
	} else {
		// FB2K_console_formatter() << "sorting with blank metadb info";
		auto api = fb2k::cpuThreadPool::get();
		pfc::counter walk = 0;
		api->runMulti_([&] {
			pfc::string8 temp;
			const metadb_v2_rec_t rec = {};
			tfhook_sort myHook;
			for (;;) {
				aborter.check();
				size_t idx = walk++;
				if (idx >= count) return;

				auto& out = data[idx];
				out.setup(nSorters);
				out.index = order[idx];

				for (size_t iSorter = 0; iSorter < nSorters; ++iSorter) {
					auto& s = sorters[iSorter];
					if (s.hook) {
						titleformat_hook_impl_splitter hookSplitter(&myHook, s.hook);
						p_list[idx]->formatTitle_v2_(rec, &hookSplitter, temp, s.obj, nullptr);
					} else {
						p_list[idx]->formatTitle_v2_(rec, &myHook, temp, s.obj, nullptr);
					}
					
					out[iSorter] = fb2k::makeSortString(temp);
				}
			}
			}, api->numRunsSanity((count + 1999) / 2000));

	}
#else
	{
		pfc::counter counter(0);

		auto work = [&] {
			tfhook_sort myHook;

			pfc::string8_fastalloc temp; temp.prealloc(512);
			for (;; ) {
				const t_size index = (counter)++;
				if (index >= count || aborter.is_set()) break;

				auto& out = data[index];
				out.setup(nSorters);
				out.index = order[index];

				for (size_t iSorter = 0; iSorter < nSorters; ++iSorter) {
					auto& s = sorters[iSorter];
					if (s.hook) {
						titleformat_hook_impl_splitter hookSplitter(&myHook, s.hook);
						p_list[index]->format_title(&hookSplitter, temp, s.obj, 0);
					} else {
						p_list[index]->format_title(&myHook, temp, s.obj, 0);
					}
					
					out[iSorter] = fb2k::makeSortString(temp);
				}
			}
		};

		size_t nThreads = pfc::getOptimalWorkerThreadCountEx(count / 128);
		if (nThreads == 1) {
			work();
		} else {
			fb2k::cpuThreadPool::runMultiHelper(work, nThreads);
		}
	}
#endif
	aborter.check();
	//	console::formatter() << "metadb_handle sort: prepared in " << pfc::format_time_ex(timer.query(),6);


	{
		auto compare = [&](data_t const& elem1, data_t const& elem2) -> int {
			for (size_t iSorter = 0; iSorter < nSorters; ++iSorter) {
				int v = fb2k::sortStringCompare(elem1[iSorter], elem2[iSorter]);
				if (v) return v * sorters[iSorter].direction;
			}
			
			return pfc::sgn_t((t_ssize)elem1.index - (t_ssize)elem2.index);
		};

		typedef decltype(data) container_t;
		typedef decltype(compare) compare_t;
		pfc::sort_callback_impl_simple_wrap_t<container_t, compare_t> cb(data, compare);

		size_t concurrency = pfc::getOptimalWorkerThreadCountEx(count / 4096);
		fb2k::sort(cb, count, concurrency, aborter);
	}

	//qsort(data.get_ptr(),count,sizeof(custom_sort_data),p_direction > 0 ? _custom_sort_compare<1> : _custom_sort_compare<-1>);


	//	console::formatter() << "metadb_handle sort: sorted in " << pfc::format_time_ex(timer.query(),6);

	for (t_size n = 0; n < count; n++)
	{
		order[n] = data[n].index;
	}

	// FB2K_console_formatter() << "metadb_handle sort: finished in " << pfc::format_time_ex(timer.query(),6);

}

t_filesize metadb_handle_list_helper::calc_total_size(metadb_handle_list_cref p_list, bool skipUnknown) {
	pfc::avltree_t< const char*, metadb::path_comparator > beenHere;
//	metadb_handle_list list(p_list);
//	list.sort_t(metadb::path_compare_metadb_handle);

	t_filesize ret = 0;
	t_size n, m = p_list.get_count();
	for(n=0;n<m;n++) {
		bool isNew;
		metadb_handle_ptr h; p_list.get_item_ex(h, n);
		beenHere.add_ex( h->get_path(), isNew);
		if (isNew) {
			t_filesize t = h->get_filesize();
			if (t == filesize_invalid) {
				if (!skipUnknown) return filesize_invalid;
			} else {
				ret += t;
			}
		}
	}
	return ret;
}

t_filesize metadb_handle_list_helper::calc_total_size_ex(metadb_handle_list_cref p_list, bool & foundUnknown) {
	foundUnknown = false;
	metadb_handle_list list(p_list);
	list.sort_t(metadb::path_compare_metadb_handle);

	t_filesize ret = 0;
	t_size n, m = list.get_count();
	for(n=0;n<m;n++) {
		if (n==0 || metadb::path_compare(list[n-1]->get_path(),list[n]->get_path())) {
			t_filesize t = list[n]->get_filesize();
			if (t == filesize_invalid) {
				foundUnknown = true;
			} else {
				ret += t;
			}
		}
	}
	return ret;
}

bool metadb_handle_list_helper::extract_folder_path(metadb_handle_list_cref list, pfc::string_base & folderOut) {
	const t_size total = list.get_count();
	if (total == 0) return false;
	pfc::string_formatter temp, folder;
	folder = list[0]->get_path();
	folder.truncate_to_parent_path();
	for(size_t walk = 1; walk < total; ++walk) {
		temp = list[walk]->get_path();
		temp.truncate_to_parent_path();
		if (metadb::path_compare(folder, temp) != 0) return false;
	}
	folderOut = folder;
	return true;
}
bool metadb_handle_list_helper::extract_single_path(metadb_handle_list_cref list, const char * &pathOut) {
	const t_size total = list.get_count();
	if (total == 0) return false;
	const char * path = list[0]->get_path();
	for(t_size walk = 1; walk < total; ++walk) {
		if (metadb::path_compare(path, list[walk]->get_path()) != 0) return false;
	}
	pathOut	= path;
	return true;
}
