#pragma once

#include <list>
#include <string>
#include <functional>
#include <memory>
#include "wtl-pp.h"

#include "CButtonLite.h"

class CEditWithButtons : public CEditPPHooks {
public:
	CEditWithButtons(::ATL::CMessageMap * hookMM = nullptr, int hookMMID = 0) : CEditPPHooks(hookMM, hookMMID) {}

	static constexpr UINT MSG_CHECKCONDITIONS = WM_USER + 13;
	static constexpr WPARAM MSG_CHECKCONDITIONS_MAGIC1 = 0xaec66f0c;
	static constexpr LPARAM MSG_CHECKCONDITIONS_MAGIC2 = 0x180c2f35;

	BEGIN_MSG_MAP_EX(CEditWithButtons)
		MSG_WM_CREATE(OnCreate)
		MSG_WM_SETFONT(OnSetFont)
		MSG_WM_WINDOWPOSCHANGED(OnPosChanged)
		MSG_WM_CTLCOLORBTN(OnColorBtn)
		MESSAGE_HANDLER_EX(WM_GETDLGCODE, OnGetDlgCode)
		MSG_WM_KEYDOWN(OnKeyDown)
		MSG_WM_CHAR(OnChar)
		// MSG_WM_SETFOCUS( OnSetFocus )
		// MSG_WM_KILLFOCUS( OnKillFocus )
		MSG_WM_ENABLE(OnEnable)
		MSG_WM_SETTEXT( OnSetText )
		MESSAGE_HANDLER_EX(WM_PAINT, CheckConditionsTrigger)
		MESSAGE_HANDLER_EX(WM_CUT, CheckConditionsTrigger)
		MESSAGE_HANDLER_EX(WM_PASTE, CheckConditionsTrigger)
		MESSAGE_HANDLER_EX(MSG_CHECKCONDITIONS, OnCheckConditions)
		CHAIN_MSG_MAP(CEditPPHooks)
	END_MSG_MAP()

	BOOL SubclassWindow( HWND wnd ) {
		if (!CEditPPHooks::SubclassWindow(wnd)) return FALSE;
		m_initialParent = GetParent();
		this->ModifyStyle(0, WS_CLIPCHILDREN);
		RefreshButtons();
		return TRUE;
	}
	typedef std::function<void () > handler_t;
	typedef std::function<bool (const wchar_t*) > condition_t;

	void AddMoreButton( std::function<void ()> f );
	void AddClearButton( const wchar_t * clearVal = L"", bool bHandleEsc = false);
	void AddButton( const wchar_t * str, handler_t handler, condition_t condition = nullptr, const wchar_t * drawAlternateText = nullptr );

	void SetFixedWidth(unsigned fw) {
		m_fixedWidth = fw; m_fixedWidthAuto = false;
		RefreshButtons();
	}
	void SetFixedWidth() {
		m_fixedWidth = 0; m_fixedWidthAuto = true;
		RefreshButtons();
	}
	CRect RectOfButton( const wchar_t * text );

	void Invalidate() {
		__super::Invalidate();
		for( auto & i : m_buttons ) {
			if (i.wnd != NULL) i.wnd.Invalidate();
		}
	}
	void SetShellFolderAutoComplete() {
		SetShellAutoComplete(SHACF_FILESYS_DIRS);
	}
	void SetShellFileAutoComplete() {
		SetShellAutoComplete(SHACF_FILESYS_ONLY);
	}
	void SetShellAutoComplete(DWORD flags) {
		SHAutoComplete(*this, flags);
		SetHasAutoComplete();
	}
	void SetHasAutoComplete(bool bValue = true) {
		m_hasAutoComplete = bValue;
	}
	void RefreshConditions(const wchar_t * newText = nullptr);
private:
	int OnCreate(LPCREATESTRUCT lpCreateStruct) {
		m_initialParent = GetParent();
		SetMsgHandled(FALSE);
		return 0;
	}
	LRESULT OnCheckConditions( UINT msg, WPARAM wp, LPARAM lp ) {
		if ( msg == MSG_CHECKCONDITIONS && wp == MSG_CHECKCONDITIONS_MAGIC1 && lp == MSG_CHECKCONDITIONS_MAGIC2 ) {
			this->RefreshConditions();
		} else {
			SetMsgHandled(FALSE);
		}		
		return 0;
	}
	bool HaveConditions() {
		for( auto i = m_buttons.begin(); i != m_buttons.end(); ++i ) {
			if ( i->condition ) return true;
		}
		return false;
	}
	void PostCheckConditions() {
		if ( HaveConditions() ) {
			PostMessage( MSG_CHECKCONDITIONS, MSG_CHECKCONDITIONS_MAGIC1, MSG_CHECKCONDITIONS_MAGIC2 );
		}
	}
	LRESULT CheckConditionsTrigger( UINT, WPARAM, LPARAM ) {
		PostCheckConditions();
		SetMsgHandled(FALSE);
		return 0;
	}
	int OnSetText(LPCTSTR) {
		PostCheckConditions();
		SetMsgHandled(FALSE);
		return 0;
	}
	void OnEnable(BOOL bEnable) {
		for( auto & i : m_buttons ) {
			if ( i.wnd != NULL ) {
				i.wnd.EnableWindow( bEnable );
			}
		}
		SetMsgHandled(FALSE); 
	}
	void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) {
		(void)nRepCnt; (void)nFlags;
		if (nChar == VK_TAB ) {
			return;
		}
		PostCheckConditions();
		SetMsgHandled(FALSE);
	}
	void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) {
		(void)nRepCnt; (void)nFlags;
		if ( nChar == VK_TAB ) {
			return;
		}
		SetMsgHandled(FALSE);
	}
	bool canStealTab() {
		if (m_hasAutoComplete) return false;
		if (IsShiftPressed()) return false;
		if (m_buttons.size() == 0) return false;
		return true;
	}
	UINT OnGetDlgCode(UINT, WPARAM wp, LPARAM) {
		if ( wp == VK_TAB && canStealTab() ) {
			for (auto i = m_buttons.begin(); i != m_buttons.end(); ++ i ) {
				if ( i->visible ) {
					TabFocusThis(i->wnd);
					return DLGC_WANTTAB;
				}
			}
		}
		SetMsgHandled(FALSE); return 0;
	}
	void OnSetFocus(HWND) {
		this->ModifyStyleEx(0, WS_EX_CONTROLPARENT ); SetMsgHandled(FALSE);
	}
	void OnKillFocus(HWND) {
		this->ModifyStyleEx(WS_EX_CONTROLPARENT, 0 ); SetMsgHandled(FALSE);
	}
	HBRUSH OnColorBtn(CDCHandle dc, CButton) {
		if ( (this->GetStyle() & ES_READONLY) != 0 || !this->IsWindowEnabled() ) {
			return (HBRUSH) GetParent().SendMessage( WM_CTLCOLORSTATIC, (WPARAM) dc.m_hDC, (LPARAM) m_hWnd );
		} else {
			return (HBRUSH) GetParent().SendMessage( WM_CTLCOLOREDIT, (WPARAM) dc.m_hDC, (LPARAM) m_hWnd );
		}
	}
	void OnPosChanged(LPWINDOWPOS) {
		Layout(); SetMsgHandled(FALSE);
	}
	
	struct Button_t {
		std::wstring title, titleDraw;
		handler_t handler;
		CButtonLite wnd;
		bool visible;
		condition_t condition;
	};
	
	void OnSetFont(CFontHandle font, BOOL bRedraw);

	void RefreshButtons() {
		if ( m_hWnd != NULL && m_buttons.size() > 0 ) {
			Layout();
		}
	}
	void Layout( ) {
		CRect rc;
		if (GetClientRect(&rc)) {
			Layout(rc.Size(), NULL);
		}
	}
	static bool IsShiftPressed() {
		return (GetKeyState(VK_SHIFT) & 0x8000) ? true : false;
	}
	CWindow FindDialog() {
		// Return a window that we can send WM_NEXTDLGCTL to
		// PROBLEM: There is no clear way of obtaining one - something in our GetParent() hierarchy is usually a dialog, but we don't know which one
		// Assume our initial parent window to be the right thing to talk to
		PFC_ASSERT(m_initialParent != NULL);
		return m_initialParent;
	}
	void TabFocusThis(HWND wnd) {
		FindDialog().PostMessage(WM_NEXTDLGCTL, (WPARAM) wnd, TRUE );
	}
	void TabFocusPrevNext(bool bPrev) {
		FindDialog().PostMessage(WM_NEXTDLGCTL, bPrev ? TRUE : FALSE, FALSE);
	}
	void TabCycleButtons(HWND wnd);

	bool ButtonWantTab( HWND wnd );
	bool EvalCondition( Button_t & btn, const wchar_t * newText );
	void Layout(CSize size, CFontHandle fontSetMe);
	unsigned MeasureButton(Button_t const & button );

	unsigned m_fixedWidth = 0;
	bool m_fixedWidthAuto = false;
	std::list< Button_t > m_buttons;
	bool m_hasAutoComplete = false;
	CWindow m_initialParent;
};
