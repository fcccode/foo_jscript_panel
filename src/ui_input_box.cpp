#include "stdafx.h"
#include "ui_input_box.h"

CInputBox::CInputBox(const char* p_prompt, const char* p_caption, const char* p_value) : m_prompt(p_prompt), m_caption(p_caption), m_value(p_value) {}

LRESULT CInputBox::OnInitDialog(HWND hwndFocus, LPARAM lParam)
{
	uSetWindowText(m_hWnd, m_caption);
	uSendDlgItemMessageText(m_hWnd, IDC_INPUT_PROMPT, WM_SETTEXT, 0, m_prompt);
	uSendDlgItemMessageText(m_hWnd, IDC_INPUT_VALUE, WM_SETTEXT, 0, m_value);

	// Select all
	SendDlgItemMessage(IDC_INPUT_VALUE, EM_SETSEL, 0, -1);
	::SetFocus(GetDlgItem(IDC_INPUT_VALUE));
	CenterWindow();

	return FALSE;
}

LRESULT CInputBox::OnCommand(UINT codeNotify, int id, HWND hwndCtl)
{
	if (id == IDOK || id == IDCANCEL)
	{
		if (id == IDOK)
		{
			uGetDlgItemText(m_hWnd, IDC_INPUT_VALUE, m_value);
		}

		EndDialog(id);
	}

	return 0;
}

void CInputBox::GetValue(pfc::string_base& p_value)
{
	p_value = m_value;
}
