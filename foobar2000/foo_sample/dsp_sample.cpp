#include "stdafx.h"
#include "dsp_sample.h"

#ifdef _WIN32
#include <helpers/DarkMode.h>
#include "resource.h"

static void RunDSPConfigPopup(const dsp_preset & p_data,HWND p_parent,dsp_preset_edit_callback & p_callback);
#endif

#ifdef __APPLE__
// fooSampleDSPView.mm
service_ptr ConfigureSampleDSP( fb2k::hwnd_t parent, dsp_preset_edit_callback_v2::ptr callback );
#endif

class dsp_sample : public dsp_impl_base
{
public:
	dsp_sample(dsp_preset const & in) : m_gain(parse_preset(in)) {
	}

	static GUID g_get_guid() {
		return dsp_sample_common::guid;
	}

	static void g_get_name(pfc::string_base & p_out) { p_out = "Sample DSP";}

	bool on_chunk(audio_chunk * chunk,abort_callback &) {
		// Perform any operations on the chunk here.
		// The most simple DSPs can just alter the chunk in-place here and skip the following functions.

		
		// trivial DSP code: apply our gain to the audio data.
		chunk->scale( (audio_sample)audio_math::gain_to_scale( m_gain ) );

		// To retrieve the currently processed track, use get_cur_file().
		// Warning: the track is not always known - it's up to the calling component to provide this data and in some situations we'll be working with data that doesn't originate from an audio file.
		// If you rely on get_cur_file(), you should change need_track_change_mark() to return true to get accurate information when advancing between tracks.
 		
		return true; //Return true to keep the chunk or false to drop it from the chain.
	}

	void on_endofplayback(abort_callback &) {
		// The end of playlist has been reached, we've already received the last decoded audio chunk.
		// We need to finish any pending processing and output any buffered data through insert_chunk().
	}
	void on_endoftrack(abort_callback &) {
		// Should do nothing except for special cases where your DSP performs special operations when changing tracks.
		// If this function does anything, you must change need_track_change_mark() to return true.
		// If you have pending audio data that you wish to output, you can use insert_chunk() to do so.		
	}

	void flush() {
		// If you have any audio data buffered, you should drop it immediately and reset the DSP to a freshly initialized state.
		// Called after a seek etc.
	}

	double get_latency() {
		// If the DSP buffers some amount of audio data, it should return the duration of buffered data (in seconds) here.
		return 0;
	}

	bool need_track_change_mark() {
		// Return true if you need on_endoftrack() or need to accurately know which track we're currently processing
		// WARNING: If you return true, the DSP manager will fire on_endofplayback() at DSPs that are before us in the chain on track change to ensure that we get an accurate mark, so use it only when needed.
		return false;
	}
	static bool g_get_default_preset(dsp_preset & p_out) {
		make_preset(0, p_out);
		return true;
	}
#ifdef _WIN32
	static void g_show_config_popup(const dsp_preset & p_data,HWND p_parent,dsp_preset_edit_callback & p_callback) {
		::RunDSPConfigPopup(p_data, p_parent, p_callback);
	}
#endif // _WIN32
#ifdef __APPLE__
    static service_ptr g_show_config_popup( fb2k::hwnd_t parent, dsp_preset_edit_callback_v2::ptr callback ) {
        return ConfigureSampleDSP( parent, callback );
    }
#endif // __APPLE__
	static bool g_have_config_popup() {return true;}
private:
	float m_gain;
};

// Use dsp_factory_nopreset_t<> instead of dsp_factory_t<> if your DSP does not provide preset/configuration functionality.
static dsp_factory_t<dsp_sample> g_dsp_sample_factory;

#ifdef _WIN32

class CMyDSPPopup : public CDialogImpl<CMyDSPPopup> {
public:
	CMyDSPPopup(const dsp_preset & initData, dsp_preset_edit_callback & callback) : m_initData(initData), m_callback(callback) {}

	enum { IDD = IDD_DSP };

	enum {
		RangeMin = -20,
		RangeMax = 20,

		RangeTotal = RangeMax - RangeMin
	};

	BEGIN_MSG_MAP_EX(CMyDSPPopup)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDOK, BN_CLICKED, OnButton)
		COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, OnButton)
		MSG_WM_HSCROLL(OnHScroll)
	END_MSG_MAP()

private:

	BOOL OnInitDialog(CWindow, LPARAM) {
		m_dark.AddDialogWithControls(m_hWnd);
		m_slider = GetDlgItem(IDC_SLIDER);
		m_slider.SetRange(0, RangeTotal);

		{
			float val = parse_preset(m_initData);
			m_slider.SetPos( pfc::clip_t<t_int32>( pfc::rint32(val), RangeMin, RangeMax ) - RangeMin );
			RefreshLabel(val);
		}
		return TRUE;
	}

	void OnButton(UINT, int id, CWindow) {
		EndDialog(id);
	}

	void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar) {
		float val;
		val = (float) ( m_slider.GetPos() + RangeMin );

		{
			dsp_preset_impl preset;
			make_preset(val, preset);
			m_callback.on_preset_changed(preset);
		}
		RefreshLabel(val);
	}

	void RefreshLabel(float val) {
		pfc::string_formatter msg; msg << pfc::format_float(val) << " dB";
		::uSetDlgItemText(*this, IDC_SLIDER_LABEL, msg);
	}

	const dsp_preset & m_initData; // modal dialog so we can reference this caller-owned object.
	dsp_preset_edit_callback & m_callback;

	CTrackBarCtrl m_slider;
	fb2k::CDarkModeHooks m_dark;
};

static void RunDSPConfigPopup(const dsp_preset & p_data,HWND p_parent,dsp_preset_edit_callback & p_callback) {
	CMyDSPPopup popup(p_data, p_callback);
	if (popup.DoModal(p_parent) != IDOK) {
		// If the dialog exited with something else than IDOK,k 
		// tell host that the editing has been cancelled by sending it old preset data that we got initialized with
		p_callback.on_preset_changed(p_data);
	}
}
#endif
