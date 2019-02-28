#pragma once

template <typename T1, typename T2 = char, typename T3 = char>
struct callback_data : public pfc::refcounted_object_root
{
	callback_data(const T1& p_item1) : m_item1(p_item1) {}
	callback_data(const T1& p_item1, const T2& p_item2) : m_item1(p_item1), m_item2(p_item2) {}
	callback_data(const T1& p_item1, const T2& p_item2, const T3& p_item3) : m_item1(p_item1), m_item2(p_item2), m_item3(p_item3) {}

	T1 m_item1;
	T2 m_item2;
	T3 m_item3;
};

struct metadb_callback_data : public pfc::refcounted_object_root
{
	metadb_callback_data(metadb_handle_list_cref p_items) : m_items(p_items) {}

	metadb_handle_list m_items;
};

template <class T>
class callback_data_scope_releaser
{
public:
	template <class TParam>
	callback_data_scope_releaser(TParam p_data)
	{
		m_data = reinterpret_cast<T*>(p_data);
	}

	template <class TParam>
	callback_data_scope_releaser(TParam* p_data)
	{
		m_data = reinterpret_cast<T*>(p_data);
	}

	~callback_data_scope_releaser()
	{
		m_data->refcount_release();
	}

	T* operator->()
	{
		return m_data;
	}

private:
	T* m_data;
};

class panel_manager
{
public:
	panel_manager();

	static panel_manager& instance();
	t_size get_count();
	void add_window(HWND p_wnd);
	void post_msg_to_all(UINT p_msg, WPARAM p_wp = 0, LPARAM p_lp = 0);
	void post_msg_to_all_pointer(UINT p_msg, pfc::refcounted_object_root* p_param);
	void remove_window(HWND p_wnd);
	void send_msg_to_all(UINT p_msg, WPARAM p_wp, LPARAM p_lp);
	void send_msg_to_others_pointer(HWND p_wnd_except, UINT p_msg, pfc::refcounted_object_root* p_param);

private:
	pfc::list_t<HWND> m_hwnds;
	static panel_manager instance_;

	PFC_CLASS_NOT_COPYABLE_EX(panel_manager)
};

class my_config_object_notify : public config_object_notify
{
public:
	GUID get_watched_object(t_size p_index) override;
	t_size get_watched_object_count() override;
	void on_watched_object_changed(const config_object::ptr& p_object) override;
};

class my_dsp_config_callback : public dsp_config_callback
{
public:
	void on_core_settings_change(const dsp_chain_config& p_newdata) override;
};

class my_initquit : public initquit, public ui_selection_callback, public replaygain_core_settings_notify, public output_config_change_callback
{
public:
	void on_changed(const t_replaygain_config& cfg) override;
	void on_init() override;
	void on_quit() override;
	void on_selection_changed(metadb_handle_list_cref p_selection) override;
	void outputConfigChanged() override;
};

class my_library_callback : public library_callback
{
public:
	void on_items_added(metadb_handle_list_cref p_data) override;
	void on_items_modified(metadb_handle_list_cref p_data) override;
	void on_items_removed(metadb_handle_list_cref p_data) override;
};

class my_metadb_io_callback : public metadb_io_callback
{
public:
	void on_changed_sorted(metadb_handle_list_cref p_items_sorted, bool p_fromhook) override;
};

class my_play_callback_static : public play_callback_static
{
public:
	t_size get_flags() override;
	void on_playback_dynamic_info(const file_info& info) override;
	void on_playback_dynamic_info_track(const file_info& info) override;
	void on_playback_edited(metadb_handle_ptr track) override;
	void on_playback_new_track(metadb_handle_ptr track) override;
	void on_playback_pause(bool state) override;
	void on_playback_seek(double time) override;
	void on_playback_starting(playback_control::t_track_command cmd, bool paused) override;
	void on_playback_stop(playback_control::t_stop_reason reason) override;
	void on_playback_time(double time) override;
	void on_volume_change(float newval) override;
};

class my_playback_queue_callback : public playback_queue_callback
{
public:
	void on_changed(t_change_origin p_origin) override;
};

class my_playback_statistics_collector : public playback_statistics_collector
{
public:
	void on_item_played(metadb_handle_ptr p_item) override;
};

class my_playlist_callback_static : public playlist_callback_static
{
public:
	void on_default_format_changed() override {}
	void on_items_modified(t_size p_playlist, const pfc::bit_array& p_mask) override {}
	void on_items_modified_fromplayback(t_size p_playlist, const pfc::bit_array& p_mask, playback_control::t_display_level p_level) override {}
	void on_items_removing(t_size p_playlist, const pfc::bit_array& p_mask, t_size p_old_count, t_size p_new_count) override {}
	void on_items_replaced(t_size p_playlist, const pfc::bit_array& p_mask, const pfc::list_base_const_t<t_on_items_replaced_entry>& p_data) override {}
	void on_playlists_removing(const pfc::bit_array& p_mask, t_size p_old_count, t_size p_new_count) override {}

	t_size get_flags() override;
	void on_item_ensure_visible(t_size p_playlist, t_size p_idx) override;
	void on_item_focus_change(t_size p_playlist, t_size p_from, t_size p_to) override;
	void on_items_added(t_size p_playlist, t_size p_start, metadb_handle_list_cref p_data, const pfc::bit_array& p_selection) override;
	void on_items_removed(t_size p_playlist, const pfc::bit_array& p_mask, t_size p_old_count, t_size p_new_count) override;
	void on_items_reordered(t_size p_playlist, const t_size* p_order, t_size p_count) override;
	void on_items_selection_change(t_size p_playlist, const pfc::bit_array& p_affected, const pfc::bit_array& p_state) override;
	void on_playback_order_changed(t_size p_new_index) override;
	void on_playlist_activate(t_size p_old, t_size p_new) override;
	void on_playlist_created(t_size p_index, const char* p_name, t_size p_name_len) override;
	void on_playlist_locked(t_size p_playlist, bool p_locked) override;
	void on_playlist_renamed(t_size p_index, const char* p_new_name, t_size p_new_name_len) override;
	void on_playlists_removed(const pfc::bit_array& p_mask, t_size p_old_count, t_size p_new_count) override;
	void on_playlists_reorder(const t_size* p_order, t_size p_count) override;

private:
	void on_playlist_switch();
	void on_playlists_changed();
};
