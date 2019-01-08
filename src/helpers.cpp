#include "stdafx.h"
#include "helpers.h"
#include "script_interface_impl.h"
#include "user_message.h"

#include <MLang.h>

namespace helpers
{
	COLORREF convert_argb_to_colorref(DWORD argb)
	{
		return RGB(argb >> RED_SHIFT, argb >> GREEN_SHIFT, argb >> BLUE_SHIFT);
	}

	DWORD convert_colorref_to_argb(COLORREF color)
	{
		// COLORREF : 0x00bbggrr
		// ARGB : 0xaarrggbb
		return (GetRValue(color) << RED_SHIFT) |
			(GetGValue(color) << GREEN_SHIFT) |
			(GetBValue(color) << BLUE_SHIFT) |
			0xff000000;
	}

	HBITMAP create_hbitmap_from_gdiplus_bitmap(Gdiplus::Bitmap* bitmap_ptr)
	{
		BITMAP bm;
		Gdiplus::Rect rect;
		Gdiplus::BitmapData bmpdata;
		HBITMAP hBitmap;

		rect.X = rect.Y = 0;
		rect.Width = bitmap_ptr->GetWidth();
		rect.Height = bitmap_ptr->GetHeight();

		if (bitmap_ptr->LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppPARGB, &bmpdata) != Gdiplus::Ok)
		{
			// Error
			return NULL;
		}

		bm.bmType = 0;
		bm.bmWidth = bmpdata.Width;
		bm.bmHeight = bmpdata.Height;
		bm.bmWidthBytes = bmpdata.Stride;
		bm.bmPlanes = 1;
		bm.bmBitsPixel = 32;
		bm.bmBits = bmpdata.Scan0;

		hBitmap = CreateBitmapIndirect(&bm);
		bitmap_ptr->UnlockBits(&bmpdata);
		return hBitmap;
	}

	IGdiBitmap* get_album_art(const metadb_handle_ptr& handle, t_size art_id, bool need_stub, pfc::string_base& image_path, bool no_load)
	{
		GUID what = convert_artid_to_guid(art_id);
		abort_callback_dummy abort;
		auto api = album_art_manager_v2::get();
		album_art_extractor_instance_v2::ptr ptr;
		album_art_data_ptr data;
		Gdiplus::Bitmap* bitmap = NULL;
		IGdiBitmap* ret = NULL;

		try
		{
			ptr = api->open(pfc::list_single_ref_t<metadb_handle_ptr>(handle), pfc::list_single_ref_t<GUID>(what), abort);
			data = ptr->query(what, abort);
		}
		catch (...)
		{
			if (need_stub)
			{
				try
				{
					ptr = api->open_stub(abort);
					data = ptr->query(what, abort);
				}
				catch (...) {}
			}
		}

		if (data.is_valid())
		{
			if (!no_load && read_album_art_into_bitmap(data, &bitmap))
			{
				ret = new com_object_impl_t<GdiBitmap>(bitmap);
			}
			album_art_path_list::ptr pathlist = ptr->query_paths(what, abort);
			if (pathlist->get_count() > 0)
			{
				image_path.set_string(file_path_display(pathlist->get_path(0)));
			}
		}
		return ret;
	}

	IGdiBitmap* get_album_art_embedded(BSTR rawpath, t_size art_id)
	{
		pfc::stringcvt::string_utf8_from_wide urawpath(rawpath);
		IGdiBitmap* ret = NULL;

		album_art_extractor::ptr ptr;
		if (album_art_extractor::g_get_interface(ptr, urawpath))
		{
			album_art_extractor_instance_ptr aaep;
			GUID what = convert_artid_to_guid(art_id);
			abort_callback_dummy abort;

			try
			{	
				aaep = ptr->open(NULL, urawpath, abort);

				album_art_data_ptr data = aaep->query(what, abort);
				Gdiplus::Bitmap* bitmap = NULL;

				if (read_album_art_into_bitmap(data, &bitmap))
				{
					ret = new com_object_impl_t<GdiBitmap>(bitmap);
				}
			}
			catch (...)
			{
			}
		}
		return ret;
	}

	IGdiBitmap* load_image(BSTR path)
	{
		IGdiBitmap* ret = NULL;
		IStreamPtr pStream;
		HRESULT hr = SHCreateStreamOnFileEx(path, STGM_READ | STGM_SHARE_DENY_WRITE, GENERIC_READ, FALSE, NULL, &pStream);
		if (SUCCEEDED(hr))
		{
			Gdiplus::Bitmap* img = new Gdiplus::Bitmap(pStream, PixelFormat32bppPARGB);
			if (helpers::ensure_gdiplus_object(img))
			{
				ret = new com_object_impl_t<GdiBitmap>(img);
			}
			else
			{
				if (img) delete img;
				img = NULL;
			}
		}
		return ret;
	}

	bool execute_context_command_by_name(const char* p_command, metadb_handle_list_cref p_handles, t_size flags)
	{
		contextmenu_manager::ptr cm;
		contextmenu_manager::g_create(cm);
		pfc::string8_fast dummy("");

		if (p_handles.get_count() > 0)
		{
			cm->init_context(p_handles, flags);
		}
		else
		{
			if (!cm->init_context_now_playing(flags))
			{
				return false;
			}
		}

		return execute_context_command_recur(p_command, dummy, cm->get_root());
	}

	bool execute_context_command_recur(const char* p_command, pfc::string_base& p_path, contextmenu_node* p_parent)
	{
		for (t_size i = 0; i < p_parent->get_num_children(); ++i)
		{
			contextmenu_node* child = p_parent->get_child(i);
			pfc::string8_fast path = p_path + child->get_name();

			switch (child->get_type())
			{
			case contextmenu_item_node::type_group:
				path.add_char('/');
				if (execute_context_command_recur(p_command, path, child))
				{
					return true;
				}
				break;
			case contextmenu_item_node::type_command:
				if (_stricmp(p_command, path) == 0)
				{
					child->execute();
					return true;
				}
				break;
			}
		}
		return false;
	}

	bool execute_mainmenu_command_by_name(const char* p_command)
	{
		pfc::map_t<GUID, mainmenu_group::ptr> group_guid_map;

		{
			service_enum_t<mainmenu_group> e;
			mainmenu_group::ptr ptr;

			while (e.next(ptr))
			{
				GUID guid = ptr->get_guid();
				group_guid_map.find_or_add(guid) = ptr;
			}
		}

		service_enum_t<mainmenu_commands> e;
		mainmenu_commands::ptr ptr;

		while (e.next(ptr))
		{
			for (t_size i = 0; i < ptr->get_command_count(); ++i)
			{
				pfc::string8_fast path;

				GUID parent = ptr->get_parent();
				while (parent != pfc::guid_null)
				{
					mainmenu_group::ptr group_ptr = group_guid_map[parent];
					mainmenu_group_popup::ptr group_popup_ptr;

					if (group_ptr->cast(group_popup_ptr))
					{
						pfc::string8_fast temp;
						group_popup_ptr->get_display_string(temp);
						if (temp.get_length())
						{
							temp.add_char('/');
							temp.add_string(path);
							path = temp;
						}
					}
					parent = group_ptr->get_parent();
				}

				mainmenu_commands_v2::ptr v2_ptr;
				if (ptr->cast(v2_ptr) && v2_ptr->is_command_dynamic(i))
				{
					mainmenu_node::ptr node = v2_ptr->dynamic_instantiate(i);
					if (execute_mainmenu_command_recur(p_command, path, node))
					{
						return true;
					}
				}
				else
				{
					pfc::string8_fast command;
					ptr->get_name(i, command);
					path.add_string(command);
					if (_stricmp(p_command, path) == 0)
					{
						ptr->execute(i, NULL);
						return true;
					}
				}
			}
		}
		return false;
	}

	bool execute_mainmenu_command_recur(const char* p_command, pfc::string8_fast path, mainmenu_node::ptr node)
	{
		pfc::string8_fast text;
		t_size flags;
		node->get_display(text, flags);
		path += text;

		switch (node->get_type())
		{
		case mainmenu_node::type_group:
			if (text.get_length())
			{
				path.add_char('/');
			}
			for (t_size i = 0; i < node->get_children_count(); ++i)
			{
				mainmenu_node::ptr child = node->get_child(i);
				if (execute_mainmenu_command_recur(p_command, path, child))
				{
					return true;
				}
			}
			break;
		case mainmenu_node::type_command:
			if (_stricmp(p_command, path) == 0)
			{
				node->execute(NULL);
				return true;
			}
			break;
		}
		return false;
	}

	bool read_album_art_into_bitmap(const album_art_data_ptr& data, Gdiplus::Bitmap** bitmap)
	{
		*bitmap = NULL;

		if (!data.is_valid())
			return false;

		// Using IStream
		IStreamPtr is;
		Gdiplus::Bitmap* bmp = NULL;
		bool ret = true;
		HRESULT hr = CreateStreamOnHGlobal(NULL, TRUE, &is);

		if (SUCCEEDED(hr) && bitmap && is)
		{
			ULONG bytes_written = 0;

			hr = is->Write(data->get_ptr(), data->get_size(), &bytes_written);

			if (SUCCEEDED(hr) && bytes_written == data->get_size())
			{
				bmp = new Gdiplus::Bitmap(is, PixelFormat32bppPARGB);

				if (!ensure_gdiplus_object(bmp))
				{
					ret = false;
					if (bmp) delete bmp;
					bmp = NULL;
				}
			}
		}

		*bitmap = bmp;
		return ret;
	}

	bool read_file_wide(unsigned codepage, const wchar_t* path, pfc::array_t<wchar_t>& content)
	{
		HANDLE hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if (hFile == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		HANDLE hFileMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);

		if (hFileMapping == NULL)
		{
			CloseHandle(hFile);
			return false;
		}

		DWORD dwFileSize = GetFileSize(hFile, NULL);
		LPCBYTE pAddr = (LPCBYTE)MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);

		if (pAddr == NULL)
		{
			CloseHandle(hFileMapping);
			CloseHandle(hFile);
			return false;
		}

		if (dwFileSize == INVALID_FILE_SIZE)
		{
			UnmapViewOfFile(pAddr);
			CloseHandle(hFileMapping);
			CloseHandle(hFile);
			return false;
		}

		bool status = false;

		if (dwFileSize > 3)
		{
			// UTF16 LE?
			if (pAddr[0] == 0xFF && pAddr[1] == 0xFE)
			{
				const wchar_t* pSource = (const wchar_t *)(pAddr + 2);
				t_size len = (dwFileSize - 2) >> 1;

				content.set_size(len + 1);
				pfc::__unsafe__memcpy_t(content.get_ptr(), pSource, len);
				content[len] = 0;
				status = true;
			}
			// UTF8-BOM?
			else if (pAddr[0] == 0xEF && pAddr[1] == 0xBB && pAddr[2] == 0xBF)
			{
				const char* pSource = (const char *)(pAddr + 3);
				t_size pSourceSize = dwFileSize - 3;

				const t_size size = pfc::stringcvt::estimate_utf8_to_wide_quick(pSource, pSourceSize);
				content.set_size(size);
				pfc::stringcvt::convert_utf8_to_wide(content.get_ptr(), size, pSource, pSourceSize);
				status = true;
			}
		}

		if (!status)
		{
			const char* pSource = (const char *)(pAddr);
			t_size pSourceSize = dwFileSize;

			t_size tmp = detect_charset(pfc::stringcvt::string_utf8_from_wide(path));
			if (tmp == CP_UTF8)
			{
				const t_size size = pfc::stringcvt::estimate_utf8_to_wide_quick(pSource, pSourceSize);
				content.set_size(size);
				pfc::stringcvt::convert_utf8_to_wide(content.get_ptr(), size, pSource, pSourceSize);
			}
			else
			{
				const t_size size = pfc::stringcvt::estimate_codepage_to_wide(codepage, pSource, pSourceSize);
				content.set_size(size);
				pfc::stringcvt::convert_codepage_to_wide(codepage, content.get_ptr(), size, pSource, pSourceSize);
			}
			status = true;
		}

		UnmapViewOfFile(pAddr);
		CloseHandle(hFileMapping);
		CloseHandle(hFile);
		return status;
	}

	bool supports_chakra()
	{
		HKEY hKey;
		return RegOpenKeyExW(HKEY_CLASSES_ROOT, L"CLSID\\{16d51579-a30b-4c8b-a276-0ff4dc41e755}", 0, KEY_READ, &hKey) == ERROR_SUCCESS;
	}

	bool write_file(const char* path, const pfc::string_base& content, bool write_bom)
	{
		int offset = write_bom ? 3 : 0;
		HANDLE hFile = uCreateFile(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

		if (hFile == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		HANDLE hFileMapping = uCreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, content.get_length() + offset, NULL);

		if (hFileMapping == NULL)
		{
			CloseHandle(hFile);
			return false;
		}

		PBYTE pAddr = (PBYTE)MapViewOfFile(hFileMapping, FILE_MAP_WRITE, 0, 0, 0);

		if (pAddr == NULL)
		{
			CloseHandle(hFileMapping);
			CloseHandle(hFile);
			return false;
		}

		if (write_bom)
		{
			const BYTE utf8_bom[] = { 0xef, 0xbb, 0xbf };
			memcpy(pAddr, utf8_bom, 3);
		}
		memcpy(pAddr + offset, content.get_ptr(), content.get_length());

		UnmapViewOfFile(pAddr);
		CloseHandle(hFileMapping);
		CloseHandle(hFile);
		return true;
	}

	const GUID convert_artid_to_guid(t_size art_id)
	{
		const GUID* guids[] = {
			&album_art_ids::cover_front,
			&album_art_ids::cover_back,
			&album_art_ids::disc,
			&album_art_ids::icon,
			&album_art_ids::artist,
		};

		if (art_id < _countof(guids))
		{
			return *guids[art_id];
		}
		return *guids[0];
	}

	int get_encoder_clsid(const wchar_t* format, CLSID* pClsid)
	{
		int ret = -1;
		t_size num = 0;
		t_size size = 0;

		Gdiplus::GetImageEncodersSize(&num, &size);
		if (size == 0) return ret;

		Gdiplus::ImageCodecInfo* pImageCodecInfo = new Gdiplus::ImageCodecInfo[size];
		if (pImageCodecInfo == NULL) return ret;

		Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);

		for (t_size i = 0; i < num; ++i)
		{
			if (wcscmp(pImageCodecInfo[i].MimeType, format) == 0)
			{
				*pClsid = pImageCodecInfo[i].Clsid;
				ret = i;
				break;
			}
		}

		delete[] pImageCodecInfo;
		return ret;
	}

	int get_text_height(HDC hdc, const wchar_t* text, int len)
	{
		SIZE size;
		GetTextExtentPoint32(hdc, text, len, &size);
		return size.cy;
	}

	int get_text_width(HDC hdc, const wchar_t* text, int len)
	{
		SIZE size;
		GetTextExtentPoint32(hdc, text, len, &size);
		return size.cx;
	}

	int is_wrap_char(wchar_t current, wchar_t next)
	{
		if (iswpunct(current))
			return false;

		if (next == '\0')
			return true;

		if (iswspace(current))
			return true;

		int currentAlphaNum = iswalnum(current);

		if (currentAlphaNum)
		{
			if (iswpunct(next))
				return false;
		}

		return currentAlphaNum == 0 || iswalnum(next) == 0;
	}

	pfc::string8_fast get_fb2k_component_path()
	{
		pfc::string8_fast path;
		uGetModuleFileName(core_api::get_my_instance(), path);
		path = pfc::string_directory(path);
		path.add_char('\\');
		return path;
	}

	pfc::string8_fast get_fb2k_path()
	{
		pfc::string8_fast path;
		uGetModuleFileName(NULL, path);
		path = pfc::string_directory(path);
		path.add_char('\\');
		return path;
	}

	pfc::string8_fast get_profile_path()
	{
		pfc::string8_fast path = file_path_display(core_api::get_profile_path());
		path.fix_dir_separator('\\');
		return path;
	}

	pfc::string8_fast iterator_to_string(json::iterator j)
	{
		std::string value = j.value().type() == json::value_t::string ? j.value().get<std::string>() : j.value().dump();
		return value.c_str();
	}

	t_size detect_charset(const char* fileName)
	{
		_COM_SMARTPTR_TYPEDEF(IMultiLanguage2, IID_IMultiLanguage2);
		IMultiLanguage2Ptr lang;
		HRESULT hr;

		hr = lang.CreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER);
		// mlang is not working...
		if (FAILED(hr)) return 0;

		const int maxEncodings = 2;
		int encodingCount = maxEncodings;
		DetectEncodingInfo encodings[maxEncodings];
		pfc::string8_fast text;
		int textSize = 0;

		try
		{
			file_ptr io;
			abort_callback_dummy abort;
			filesystem::g_open_read(io, fileName, abort);
			io->read_string_raw(text, abort);
			textSize = text.get_length();
		}
		catch (...)
		{
			return 0;
		}

		hr = lang->DetectInputCodepage(MLDETECTCP_NONE, 0, const_cast<char *>(text.get_ptr()), &textSize, encodings, &encodingCount);

		if (FAILED(hr)) return 0;

		unsigned codepage = 0;
		bool found = false;

		// MLang fine tunes
		if (encodingCount == 2 && encodings[0].nCodePage == 1252)
		{
			switch (encodings[1].nCodePage)
			{
			case 850:
			case 65001:
				found = true;
				codepage = 65001;
				break;
				// DBCS
			case 932: // shift-jis
			case 936: // gbk
			case 949: // korean
			case 950: // big5
			{
				// '��', <= special char
				// "ve" "d" "ll" "m" 't' 're'
				bool fallback = true;
				t_size index;
				if (index = text.find_first("\x92") != pfc_infinite)
				{
					if ((index < text.get_length() - 1) &&
						(strchr("vldmtr ", text[index + 1])))
					{
						codepage = encodings[0].nCodePage;
						fallback = false;
					}
				}
				if (fallback)
					codepage = encodings[1].nCodePage;
				found = true;
			}
			break;
			}
		}

		if (!found)
			codepage = encodings[0].nCodePage;
		// ASCII?
		if (codepage == 20127)
			codepage = 0;

		return codepage;
	}

	t_size get_colour_from_variant(VARIANT v)
	{
		return (v.vt == VT_R8) ? static_cast<unsigned>(v.dblVal) : v.lVal;
	}

	void estimate_line_wrap(HDC hdc, const wchar_t* text, int len, int width, pfc::list_t<wrapped_item>& out)
	{
		for (;;)
		{
			const wchar_t* next = wcschr(text, '\n');
			if (next == NULL)
			{
				estimate_line_wrap_recur(hdc, text, wcslen(text), width, out);
				break;
			}

			const wchar_t* walk = next;

			while (walk > text && walk[-1] == '\r')
			{
				--walk;
			}

			estimate_line_wrap_recur(hdc, text, walk - text, width, out);
			text = next + 1;
		}
	}

	void estimate_line_wrap_recur(HDC hdc, const wchar_t* text, int len, int width, pfc::list_t<wrapped_item>& out)
	{
		int textLength = len;
		int textWidth = get_text_width(hdc, text, len);

		if (textWidth <= width || len <= 1)
		{
			wrapped_item item =
			{
				SysAllocStringLen(text, len),
				textWidth
			};
			out.add_item(item);
		}
		else
		{
			textLength = (len * width) / textWidth;

			if (get_text_width(hdc, text, textLength) < width)
			{
				while (get_text_width(hdc, text, min(len, textLength + 1)) <= width)
				{
					++textLength;
				}
			}
			else
			{
				while (get_text_width(hdc, text, textLength) > width && textLength > 1)
				{
					--textLength;
				}
			}

			{
				int fallbackTextLength = max(textLength, 1);

				while (textLength > 0 && !is_wrap_char(text[textLength - 1], text[textLength]))
				{
					--textLength;
				}

				if (textLength == 0)
				{
					textLength = fallbackTextLength;
				}

				wrapped_item item =
				{
					SysAllocStringLen(text, textLength),
					get_text_width(hdc, text, textLength)
				};
				out.add_item(item);
			}

			if (textLength < len)
			{
				estimate_line_wrap_recur(hdc, text + textLength, len - textLength, width, out);
			}
		}
	}

	void read_file(const char* path, pfc::string_base& content)
	{
		HANDLE hFile = uCreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if (hFile == INVALID_HANDLE_VALUE)
		{
			return;
		}

		HANDLE hFileMapping = uCreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);

		if (hFileMapping == NULL)
		{
			CloseHandle(hFile);
			return;
		}

		DWORD dwFileSize = GetFileSize(hFile, NULL);
		LPCBYTE pAddr = (LPCBYTE)MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);

		if (pAddr == NULL)
		{
			CloseHandle(hFileMapping);
			CloseHandle(hFile);
			return;
		}

		if (dwFileSize == INVALID_FILE_SIZE)
		{
			UnmapViewOfFile(pAddr);
			CloseHandle(hFileMapping);
			CloseHandle(hFile);
			return;
		}

		t_size offset = dwFileSize >= 3 && pAddr[0] == 0xEF && pAddr[1] == 0xBB && pAddr[2] == 0xBF ? 3 : 0;
		const char* pSource = (const char*)(pAddr + offset);
		content.set_string(pSource);

		UnmapViewOfFile(pAddr);
		CloseHandle(hFileMapping);
		CloseHandle(hFile);
	}

	wchar_t* make_sort_string(const char* in)
	{
		wchar_t* out = new wchar_t[pfc::stringcvt::estimate_utf8_to_wide(in) + 1];
		out[0] = ' ';//StrCmpLogicalW bug workaround.
		pfc::stringcvt::convert_utf8_to_wide_unchecked(out + 1, in);
		return out;
	}

	void album_art_async::run()
	{
		pfc::string8_fast image_path;
		FbMetadbHandle* handle = NULL;
		IGdiBitmap* bitmap = NULL;

		if (m_handle.is_valid())
		{
			if (m_only_embed)
			{
				bitmap = get_album_art_embedded(m_rawpath, m_art_id);
				if (bitmap)
				{
					image_path = file_path_display(m_handle->get_path());
				}
			}
			else
			{
				bitmap = get_album_art(m_handle, m_art_id, m_need_stub, image_path, m_no_load);
			}

			handle = new com_object_impl_t<FbMetadbHandle>(m_handle);
		}

		t_param param(handle, m_art_id, bitmap, image_path);
		SendMessage(m_notify_hwnd, CALLBACK_UWM_ON_GET_ALBUM_ART_DONE, 0, (LPARAM)&param);
	}

	void load_image_async::run()
	{
		IGdiBitmap* bitmap = load_image(m_path);
		t_param param(reinterpret_cast<unsigned>(this), bitmap, m_path);
		SendMessage(m_notify_hwnd, CALLBACK_UWM_ON_LOAD_IMAGE_DONE, 0, (LPARAM)&param);
	}
}
