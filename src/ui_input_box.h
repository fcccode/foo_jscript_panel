#pragma once

class CInputBox : public CDialogImpl<CInputBox>
{
public:
	CInputBox(const char* p_prompt, const char* p_caption, const char* p_value);

	BEGIN_MSG_MAP(CInputBox)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_COMMAND(OnCommand)
	END_MSG_MAP()

	enum
	{
		IDD = IDD_DIALOG_INPUT
	};

	LRESULT OnInitDialog(HWND hwndFocus, LPARAM lParam);
	LRESULT OnCommand(UINT codeNotify, int id, HWND hwndCtl);
	void GetValue(pfc::string_base& p_value);

private:
	pfc::string8_fast m_prompt;
	pfc::string8_fast m_caption;
	pfc::string8_fast m_value;
};
