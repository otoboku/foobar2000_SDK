#pragma once

namespace system_time_periods {
	static constexpr t_filetimestamp second = filetimestamp_1second_increment;
	static constexpr t_filetimestamp minute = second * 60;
	static constexpr t_filetimestamp hour = minute * 60;
	static constexpr t_filetimestamp day = hour * 24;
	static constexpr t_filetimestamp week = day * 7;
};
class system_time_callback {
public:
	virtual void on_time_changed(t_filetimestamp newVal) = 0;
};
//! \since 0.9.6
class system_time_keeper : public service_base {
public:
	//! The callback object receives an on_changed() call with the current time inside the register_callback() call.
	virtual void register_callback(system_time_callback * callback, t_filetimestamp resolution) = 0;

	virtual void unregister_callback(system_time_callback * callback) = 0;

	FB2K_MAKE_SERVICE_COREAPI(system_time_keeper)
};

class system_time_callback_impl : public system_time_callback {
public:
	system_time_callback_impl() {}
	~system_time_callback_impl() {stop_timer();}

	void stop_timer() {
		if (m_registered) {
			system_time_keeper::get()->unregister_callback(this);
			m_registered = false;
		}
	}
	//! You get a on_changed() call inside the initialize_timer() call.
	void initialize_timer(t_filetimestamp period) {
		stop_timer();
		system_time_keeper::get()->register_callback(this, period);
		m_registered = true;
	}

	//! Override me
	void on_time_changed(t_filetimestamp) override {}

	PFC_CLASS_NOT_COPYABLE_EX(system_time_callback_impl)
private:
	bool m_registered = false;
};

