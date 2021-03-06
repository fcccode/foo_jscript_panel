#pragma once

namespace stats
{
	struct fields
	{
		t_size playcount = 0;
		t_size loved = 0;
		pfc::string8_fast first_played;
		pfc::string8_fast last_played;
		t_size rating = 0;
	};

	bool hashHandle(const metadb_handle_ptr& handle, metadb_index_hash& hash);
	fields get(metadb_index_hash hash);
	metadb_index_manager::ptr theAPI();
	void set(metadb_index_hash hash, fields f);
}
