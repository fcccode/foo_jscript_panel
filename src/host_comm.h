#pragma once
#include "config.h"
#include "script_interface_impl.h"
#include "script_preprocessor.h"

// Smart pointers for Active Scripting
_COM_SMARTPTR_TYPEDEF(IActiveScriptParse, IID_IActiveScriptParse);

class host_comm : public js_panel_vars
{
protected:
	host_comm();
	virtual ~host_comm();

	HBITMAP m_gr_bmp;
	HBITMAP m_gr_bmp_bk;
	HDC m_hdc;
	HWND m_hwnd;
	POINT m_max_size;
	POINT m_min_size;
	bool m_paint_pending;
	bool m_suppress_drawing;
	int m_height;
	int m_instance_type;
	int m_width;
	panel_tooltip_param_ptr m_panel_tooltip_param_ptr;
	t_script_info m_script_info;
	ui_selection_holder::ptr m_selection_holder;

public:
	enum
	{
		KInstanceTypeCUI = 0,
		KInstanceTypeDUI,
	};

	HDC GetHDC();
	HWND GetHWND();
	POINT& MaxSize();
	POINT& MinSize();
	int GetHeight();
	int GetWidth();
	panel_tooltip_param_ptr& PanelTooltipParam();
	t_script_info& ScriptInfo();
	t_size GetInstanceType();
	virtual DWORD GetColourUI(t_size type) = 0;
	virtual HFONT GetFontUI(t_size type) = 0;
	void Redraw();
	void RefreshBackground(LPRECT lprcUpdate = NULL);
	void Repaint(bool force = false);
	void RepaintRect(LONG x, LONG y, LONG w, LONG h, bool force = false);
};
