#include "stdafx.h"
#include "ui_name_value_edit.h"

CNameValueEdit::CNameValueEdit(const char* p_name, const char* p_value) : m_name(p_name), m_value(p_value) {}

BOOL CNameValueEdit::OnInitDialog(HWND hwndFocus, LPARAM lParam)
{
	uSendDlgItemMessageText(m_hWnd, IDC_EDIT_NAME, WM_SETTEXT, 0, m_name);
	uSendDlgItemMessageText(m_hWnd, IDC_EDIT_VALUE, WM_SETTEXT, 0, m_value);

	SendDlgItemMessage(IDC_EDIT_VALUE, EM_SETSEL, 0, -1);
	::SetFocus(GetDlgItem(IDC_EDIT_VALUE));

	return FALSE;
}

LRESULT CNameValueEdit::OnCommand(UINT codeNotify, int id, HWND hwndCtl)
{
	if (id == IDOK || id == IDCANCEL)
	{
		if (id == IDOK)
		{
			uGetDlgItemText(m_hWnd, IDC_EDIT_VALUE, m_value);
		}

		EndDialog(id);
	}

	return 0;
}

void CNameValueEdit::GetValue(pfc::string_base& p_value)
{
	p_value = m_value;
}
