#include "stdafx.h"
#include <vsstyle.h>
#include <memory>
#include "ppresources.h"
#include "CListControlWithSelection.h"
#include "PaintUtils.h"
#include "IDataObjectUtils.h"
#include "SmartStrStr.h"
#include "CListControl-Cells.h"

namespace {
	class bit_array_selection_CListControl : public pfc::bit_array {
	public:
		bit_array_selection_CListControl(CListControlWithSelectionBase const & list) : m_list(list) {}
		bool get(t_size n) const {return m_list.IsItemSelected(n);}
	private:
		CListControlWithSelectionBase const & m_list;
	};
}

bool CListControlWithSelectionBase::SelectAll() {
	SetSelection(pfc::bit_array_true(), pfc::bit_array_true() );
	return true;
}
void CListControlWithSelectionBase::SelectNone() {
	SetSelection(pfc::bit_array_true(), pfc::bit_array_false() );
}

void CListControlWithSelectionBase::SelectSingle(size_t which) {
	this->SetFocusItem( which );
	SetSelection(pfc::bit_array_true(), pfc::bit_array_one( which ) );
}

LRESULT CListControlWithSelectionBase::OnFocus(UINT msg,WPARAM,LPARAM,BOOL& bHandled) {
	UpdateItems(pfc::bit_array_or(bit_array_selection_CListControl(*this), pfc::bit_array_one(GetFocusItem())));
	bHandled = FALSE;
	return 0;
}

void CListControlWithSelectionBase::OnKeyDown_SetIndexDeltaPageHelper(int p_delta, int p_keys) {
	const CRect rcClient = GetClientRectHook();
	int itemHeight = GetItemHeight();
	if (itemHeight > 0) {
		int delta = rcClient.Height() / itemHeight;
		if (delta < 1) delta = 1;
		OnKeyDown_SetIndexDeltaHelper(delta * p_delta,p_keys);
	}
}

void CListControlWithSelectionBase::OnKeyDown_HomeEndHelper(bool isEnd, int p_keys) {
	if (!isEnd) {
		//SPECIAL FIX - ensure first group header visibility.
		MoveViewOrigin(CPoint(GetViewOrigin().x,0));
	}

	const t_size itemCount = GetItemCount();
	if (itemCount == 0) return;//don't bother

	if ((p_keys & (MK_SHIFT|MK_CONTROL)) == (MK_SHIFT|MK_CONTROL)) {
		RequestMoveSelection( isEnd ? (int)itemCount : -(int)itemCount );
	} else {
		OnKeyDown_SetIndexHelper(isEnd ? (int)(itemCount-1) : 0,p_keys);
	}
}
void CListControlWithSelectionBase::OnKeyDown_SetIndexHelper(int p_index, int p_keys) {
	const t_size count = GetItemCount();
	if (count > 0) {
		t_size idx = (t_size)pfc::clip_t(p_index,0,(int)(count - 1));
		const t_size oldFocus = GetFocusItem();
		if (p_keys & MK_CONTROL) {
			SetFocusItem(idx);
		} else if ((p_keys & MK_SHIFT) != 0 && this->AllowRangeSelect() ) {
			t_size selStart = GetSelectionStart();
			if (selStart == pfc_infinite) selStart = oldFocus;
			if (selStart == pfc_infinite) selStart = idx;
			t_size selFirst, selCount;
			selFirst = pfc::min_t(selStart,idx);
			selCount = pfc::max_t(selStart,idx) + 1 - selFirst;
			SetSelection(pfc::bit_array_true(), pfc::bit_array_range(selFirst,selCount));
			SetFocusItem(idx);
			SetSelectionStart(selStart);
		} else {
			SetFocusItem(idx);
			SetSelection(pfc::bit_array_true(), pfc::bit_array_one(idx));
		}
	}
}

void CListControlWithSelectionBase::SelectGroupHelper(int p_group,int p_keys) {
	t_size base, count;
	if (ResolveGroupRange(p_group,base,count)) {
		if (p_keys & MK_CONTROL) {
			SetGroupFocusByItem(base);
		} /*else if (p_keys & MK_SHIFT) {
		} */else {
			SetGroupFocusByItem(base);
			SetSelection(pfc::bit_array_true(), pfc::bit_array_range(base,count));
		}
	}
}

void CListControlWithSelectionBase::OnKeyDown_SetIndexDeltaLineHelper(int p_delta, int p_keys) {
	if ((p_keys & (MK_SHIFT | MK_CONTROL)) == (MK_SHIFT|MK_CONTROL)) {
		this->RequestMoveSelection(p_delta);
		return;
	}
	const t_size total = GetItemCount();
	t_size current = GetFocusItem(); 
	const int focusGroup = this->GetGroupFocus();
	if (focusGroup >= 0) {
		t_size dummy;
		if (!ResolveGroupRange(focusGroup,current,dummy)) current = pfc_infinite;
	}

	if (current == pfc_infinite) {
		OnKeyDown_SetIndexDeltaHelper(p_delta,p_keys);
		return;
	}

	const int currentGroup = GetItemGroup(current);
	if (GroupFocusActive()) {
		if (p_delta < 0) {
			if (currentGroup > 1) {
				int targetGroup = currentGroup - 1;
				t_size base, count;
				if (ResolveGroupRange(targetGroup,base,count)) {
					OnKeyDown_SetIndexHelper((int) (base + count - 1), p_keys);
				}
			}
		} else if (p_delta > 0) {
			t_size base, count;
			if (ResolveGroupRange(currentGroup,base,count)) {
				OnKeyDown_SetIndexHelper((int) base, p_keys);
			}
		}
	} else {
		if ((p_keys & MK_SHIFT) != 0) {
			OnKeyDown_SetIndexDeltaHelper(p_delta,p_keys);
		} else if (p_delta < 0) {
			if (currentGroup == 0 || (current > 0 && currentGroup == GetItemGroup(current - 1))) {
				OnKeyDown_SetIndexDeltaHelper(p_delta,p_keys);
			} else {
				SelectGroupHelper(currentGroup, p_keys);
			}
		} else if (p_delta > 0) {
			if (current + 1 >= total || currentGroup == GetItemGroup(current + 1)) {
				OnKeyDown_SetIndexDeltaHelper(p_delta,p_keys);
			} else {
				SelectGroupHelper(GetItemGroup(current + 1), p_keys);
			}
		}
	}
}

LRESULT CListControlWithSelectionBase::OnLButtonDblClk(UINT,WPARAM p_wp,LPARAM p_lp,BOOL& bHandled) {
	CPoint pt(p_lp);
	if (OnClickedSpecialHitTest(pt)) {
		return OnButtonDown(WM_LBUTTONDOWN, p_wp, p_lp, bHandled);
	}
	t_size item; int groupId;
	if (ItemFromPoint(pt,item)) {
		ExecuteDefaultAction(item);
		return 0;
	} else if (GroupHeaderFromPoint(pt,groupId)) {
		t_size count;
		if (ResolveGroupRange(groupId,item,count)) {
			ExecuteDefaultActionGroup(item,count);
		}
		return 0;
	} else if (ExecuteCanvasDefaultAction(pt)) {
		return 0;
	} else {
		return OnButtonDown(WM_LBUTTONDOWN,p_wp,p_lp,bHandled);
	}
}
void CListControlWithSelectionBase::ExecuteDefaultActionByFocus() {
	const int groupId = this->GetGroupFocus();
	if (groupId >= 0) {
		t_size item,count;
		if (ResolveGroupRange(groupId,item,count)) {
			ExecuteDefaultActionGroup(item,count);
		}
	} else {
		t_size index = this->GetFocusItem();
		if (index != ~0) this->ExecuteDefaultAction(index);
	}
}

t_size CListControlWithSelectionBase::GetSingleSel() const {
	t_size total = GetItemCount();
	t_size first = ~0;
	for(t_size walk = 0; walk < total; ++walk) {
		if (IsItemSelected(walk)) {
			if (first == ~0) first = walk;
			else return ~0;
		}
	}
	return first;
}

t_size CListControlWithSelectionBase::GetSelectedCount(pfc::bit_array const & mask,t_size max) const {
	const t_size itemCount = this->GetItemCount();
	t_size found = 0;
	for(t_size walk = mask.find_first(true,0,itemCount); walk < itemCount && found < max; walk = mask.find_next(true,walk,itemCount)) {
		if (IsItemSelected(walk)) ++found;
	}
	return found;
}
LRESULT CListControlWithSelectionBase::OnButtonDown(UINT p_msg,WPARAM p_wp,LPARAM p_lp,BOOL&) {
	pfc::vartoggle_t<bool> l_noEnsureVisible(m_noEnsureVisible,true);
	if (m_selectDragMode) {
		AbortSelectDragMode();
		return 0;
	}

	CPoint pt(p_lp);

	if (OnClickedSpecial( (DWORD) p_wp, pt)) {
		return 0;
	}

	
	int groupId; t_size item;
	const bool isRightClick = (p_msg == WM_RBUTTONDOWN || p_msg == WM_RBUTTONDBLCLK);
	const bool gotCtrl = (p_wp & MK_CONTROL) != 0 && !isRightClick;
	const bool gotShift = (p_wp & MK_SHIFT) != 0 && !isRightClick;
	
	

	const bool bCanSelect = !OnClickedSpecialHitTest( pt );

	const bool instaDrag = false;
	const bool ddSupported = IsDragDropSupported();

	if (GroupHeaderFromPoint(pt,groupId)) {
		t_size base,count;
		if (AllowRangeSelect() && ResolveGroupRange(groupId,base,count)) {
			SetGroupFocusByItem(base);
			pfc::bit_array_range groupRange(base,count);
			bool instaDragOverride = false;
			if (gotCtrl) {
				ToggleRangeSelection(groupRange);
			} else if (gotShift) {
				SetSelection(groupRange, pfc::bit_array_true());
			} else {
				if (GetSelectedCount(groupRange) == count) instaDragOverride = true;
				else SetSelection(pfc::bit_array_true(),groupRange);
			}
			if (ddSupported && (instaDrag || instaDragOverride)) {
				PrepareDragDrop(pt,isRightClick);
			} else {
				InitSelectDragMode(pt, isRightClick);
			}
		}
	} else if (ItemFromPoint(pt,item)) {
		const t_size oldFocus = GetFocusItem();
		const t_size selStartBefore = GetSelectionStart();
		if ( bCanSelect ) SetFocusItem(item);
		if (gotShift && AllowRangeSelect() ) {
			if (bCanSelect) {
				t_size selStart = selStartBefore;
				if (selStart == pfc_infinite) selStart = oldFocus;
				if (selStart == pfc_infinite) selStart = item;
				SetSelectionStart(selStart);
				t_size selFirst, selCount;
				selFirst = pfc::min_t(selStart, item);
				selCount = pfc::max_t(selStart, item) + 1 - selFirst;
				pfc::bit_array_range rangeMask(selFirst, selCount);
				pfc::bit_array_true trueMask;
				SetSelection(gotCtrl ? pfc::implicit_cast<const pfc::bit_array&>(rangeMask) : pfc::implicit_cast<const pfc::bit_array&>(trueMask), rangeMask);
				//if (!instaDrag) InitSelectDragMode(pt, isRightClick);
			}
		} else {
			if (gotCtrl) {
				if (bCanSelect) SetSelection(pfc::bit_array_one(item), pfc::bit_array_val(!IsItemSelected(item)));
				if (!instaDrag) InitSelectDragMode(pt, isRightClick);
			} else {
				if (!IsItemSelected(item)) {
					if (bCanSelect) SetSelection(pfc::bit_array_true(), pfc::bit_array_one(item));
					if (ddSupported && instaDrag) {
						PrepareDragDrop(pt,isRightClick);
					} else {
						InitSelectDragMode(pt, isRightClick);
					}
				} else {
					if (ddSupported) {
						PrepareDragDrop(pt,isRightClick);
					} else {
						InitSelectDragMode(pt, isRightClick);
					}
				}
			}
		}
	} else {
		if (!gotShift && !gotCtrl && bCanSelect) SelectNone();
		InitSelectDragMode(pt, isRightClick);
	}
	return 0;
}


void CListControlWithSelectionBase::ToggleRangeSelection(pfc::bit_array const & mask) {
	SetSelection(mask, pfc::bit_array_val(GetSelectedCount(mask,1) == 0));
}
void CListControlWithSelectionBase::ToggleGroupSelection(int p_group) {
	t_size base, count;
	if (ResolveGroupRange(p_group,base,count)) ToggleRangeSelection(pfc::bit_array_range(base,count));
}

LRESULT CListControlWithSelectionBase::OnRButtonUp(UINT,WPARAM p_wp,LPARAM p_lp,BOOL& bHandled) {
	bHandled = FALSE;
	AbortPrepareDragDropMode();
	AbortSelectDragMode();
	return 0;
}

LRESULT CListControlWithSelectionBase::OnMouseMove(UINT,WPARAM p_wp,LPARAM p_lp,BOOL&) {
	if (m_prepareDragDropMode) {
		if (CPoint(p_lp) != m_prepareDragDropOrigin) {
			AbortPrepareDragDropMode();
			if (!m_ownDDActive) {
				pfc::vartoggle_t<bool> ownDD(m_ownDDActive,true);
				RunDragDrop(m_prepareDragDropOrigin + GetViewOffset(),m_prepareDragDropModeRightClick);
			}
		}
	} else if (m_selectDragMode) {
		HandleDragSel(CPoint(p_lp));
	}
	return 0;
}
LRESULT CListControlWithSelectionBase::OnLButtonUp(UINT,WPARAM p_wp,LPARAM p_lp,BOOL&) {
	const bool wasPreparingDD = m_prepareDragDropMode;
	AbortPrepareDragDropMode();
	CPoint pt(p_lp);
	const bool gotCtrl = (p_wp & MK_CONTROL) != 0;
	const bool gotShift = (p_wp & MK_SHIFT) != 0;
	bool click = false;
	bool processSel = wasPreparingDD;
	if (m_selectDragMode) {
		processSel = !m_selectDragMoved;
		AbortSelectDragMode();
	}
	if (processSel) {
		click = true;
		if (!OnClickedSpecialHitTest(pt) ) {
			int groupId; t_size item;
			if (GroupHeaderFromPoint(pt, groupId)) {
				t_size base, count;
				if (ResolveGroupRange(groupId, base, count)) {
					if (gotCtrl) {
					} else {
						SetSelection(pfc::bit_array_true(), pfc::bit_array_range(base, count));
					}
				}
			} else if (ItemFromPoint(pt, item)) {
				const t_size selStartBefore = GetSelectionStart();
				if (gotCtrl) {
				} else if (gotShift) {
					SetSelectionStart(selStartBefore);
				} else {
					SetSelection(pfc::bit_array_true(), pfc::bit_array_one(item));
				}
			}
		}
	}
	if (click && !gotCtrl && !gotShift) {
		int groupId; t_size item;
		if (GroupHeaderFromPoint(pt,groupId)) {
			OnGroupHeaderClicked(groupId,pt);
		} else if (ItemFromPoint(pt,item)) {
			OnItemClicked(item,pt);
		}
	}
	return 0;
}

void CListControlWithSelectionBase::OnKeyDown_SetIndexDeltaHelper(int p_delta, int p_keys) {
	t_size focus = pfc_infinite;
	if (this->GroupFocusActive()) {
		t_size base,count;
		if (this->ResolveGroupRange(this->GetGroupFocus(),base,count)) {
			focus = base;
		}
	} else {
		focus = GetFocusItem();
	}
	int target = 0;
	if (focus != pfc_infinite) target = (int) focus + p_delta;
	OnKeyDown_SetIndexHelper(target,p_keys);
}


static int _get_keyflags() {
	int ret = 0;
	if (IsKeyPressed(VK_CONTROL)) ret |= MK_CONTROL;
	if (IsKeyPressed(VK_SHIFT)) ret |= MK_SHIFT;
	return ret;
}

LRESULT CListControlWithSelectionBase::OnKeyDown(UINT p_msg,WPARAM p_wp,LPARAM p_lp,BOOL& bHandled) {
	switch(p_wp) {
	case VK_NEXT:
		OnKeyDown_SetIndexDeltaPageHelper(1,_get_keyflags());
		return 0;
	case VK_PRIOR:
		OnKeyDown_SetIndexDeltaPageHelper(-1,_get_keyflags());
		return 0;
	case VK_DOWN:
		OnKeyDown_SetIndexDeltaLineHelper(1,_get_keyflags());
		return 0;
	case VK_UP:
		OnKeyDown_SetIndexDeltaLineHelper(-1,_get_keyflags());
		return 0;
	case VK_HOME:
		OnKeyDown_HomeEndHelper(false,_get_keyflags());
		return 0;
	case VK_END:
		OnKeyDown_HomeEndHelper(true,_get_keyflags());
		return 0;
	case VK_SPACE:
		if (!TypeFindCheck()) {
			ToggleSelectedItems();
		}
		return 0;
	case VK_RETURN:
		ExecuteDefaultActionByFocus();
		return 0;
	case VK_DELETE:
		if (GetHotkeyModifierFlags() == 0) {
			RequestRemoveSelection();
			return 0;
		}
		break;
	case 'A':
		if (GetHotkeyModifierFlags() == MOD_CONTROL) {
			if (SelectAll()) {
				return 0;
			}
			// otherwise unhandled
		}
		break;
	}

	bHandled = FALSE;
	return 0;
}

void CListControlWithSelectionBase::ToggleSelectedItems() {
	if (ToggleSelectedItemsHook(bit_array_selection_CListControl(*this))) return;
	if (GroupFocusActive()) {
		ToggleGroupSelection(this->GetGroupFocus());
	} else {
		const t_size focus = GetFocusItem();
		if (focus != pfc_infinite) {
			ToggleRangeSelection(pfc::bit_array_range(focus, 1));
		}
	}
}

LRESULT CListControlWithSelectionBase::OnTimer(UINT,WPARAM p_wp,LPARAM,BOOL& bHandled) {
	switch(p_wp) {
	case KSelectionTimerID:
		if (m_selectDragMode) {
			CPoint pt(GetCursorPos()); ScreenToClient(&pt);
			const CRect client = GetClientRectHook();
			CPoint delta(0,0);
			if (pt.x < client.left) {
				delta.x = pt.x - client.left;
			} else if (pt.x > client.right) {
				delta.x = pt.x - client.right;
			}
			if (pt.y < client.top) {
				delta.y = pt.y - client.top;
			} else if (pt.y > client.bottom) {
				delta.y = pt.y - client.bottom;
			}

			MoveViewOriginDelta(delta);
			HandleDragSel(pt);
		}
		return 0;
	case TDDScrollControl::KTimerID:
		HandleDDScroll();
		return 0;
	default:
		bHandled = FALSE;
		return 0;
	}
}

bool CListControlWithSelectionBase::MoveSelectionProbe(int delta) {
	pfc::array_t<size_t> order; order.set_size(GetItemCount());
	{
		bit_array_selection_CListControl sel(*this);
		pfc::create_move_items_permutation(order.get_ptr(), order.get_size(), sel, delta);
	}
	for( size_t w = 0; w < order.get_size(); ++w ) if ( order[w] != w ) return true;
	return false;
}
void CListControlWithSelectionBase::RequestMoveSelection(int delta) {
	pfc::array_t<size_t> order; order.set_size(GetItemCount());
	{
		bit_array_selection_CListControl sel(*this);
		pfc::create_move_items_permutation(order.get_ptr(), order.get_size(), sel, delta);
	}

	this->RequestReorder(order.get_ptr(), order.get_size());

	if (delta < 0) {
		size_t idx = GetFirstSelected();
		if (idx != pfc_infinite) EnsureItemVisible(idx);
	} else {
		size_t idx = GetLastSelected();
		if (idx != pfc_infinite) EnsureItemVisible(idx);
	}
}

void CListControlWithSelectionBase::ToggleSelection(pfc::bit_array const & mask) {
	const t_size count = GetItemCount();
	pfc::bit_array_bittable table(count);
	for(t_size walk = mask.find_first(true,0,count); walk < count; walk = mask.find_next(true,walk,count)) {
		table.set(walk,!IsItemSelected(walk));
	}
	this->SetSelection(mask,table);
}

static HRGN FrameRectRgn(const CRect & rect) {
	CRect exterior(rect); exterior.InflateRect(1,1);
	CRgn rgn; rgn.CreateRectRgnIndirect(exterior);
	CRect interior(rect); interior.DeflateRect(1,1);
	if (!interior.IsRectEmpty()) {
		CRgn rgn2; rgn2.CreateRectRgnIndirect(interior);
		rgn.CombineRgn(rgn2,RGN_DIFF);
	}
	return rgn.Detach();
}

void CListControlWithSelectionBase::HandleDragSel(const CPoint & p_pt) {
	CPoint pt(p_pt); pt += GetViewOffset();
	if (pt != m_selectDragCurrentAbs) {
		CRect rcOld(m_selectDragOriginAbs,m_selectDragCurrentAbs); rcOld.NormalizeRect();
		m_selectDragCurrentAbs = pt;
		CRect rcNew(m_selectDragOriginAbs,m_selectDragCurrentAbs); rcNew.NormalizeRect();


		if (false) {
			CRect total(pfc::min_t(rcOld.left,rcNew.left),pfc::min_t(rcOld.top,rcNew.top),pfc::max_t(rcOld.right,rcNew.right),pfc::max_t(rcOld.bottom,rcNew.bottom));
			total.InflateRect(1,1);
			total.OffsetRect( - GetViewOffset() );
			InvalidateRect(total);
		} else {
			CRgn rgn = FrameRectRgn(rcNew);
			CRgn rgn2 = FrameRectRgn(rcOld);
			rgn.CombineRgn(rgn2,RGN_OR);
			rgn.OffsetRgn( - GetViewOffset() );
			InvalidateRgn(rgn);
		}

		if (pt != m_selectDragOriginAbs) m_selectDragMoved = true;

		if (m_selectDragChanged || !IsSameItemOrHeaderAbs(pt,m_selectDragOriginAbs)) {
			m_selectDragChanged = true;
			const int keys = _get_keyflags();
			t_size base,count, baseOld, countOld;
			if (!GetItemRangeAbs(rcNew,base,count)) base = count = 0;
			if (!GetItemRangeAbs(rcOld,baseOld,countOld)) baseOld = countOld = 0;
			{
				pfc::bit_array_range rangeNew(base,count), rangeOld(baseOld,countOld);
				if (keys & MK_CONTROL) {
					ToggleSelection(pfc::bit_array_xor(rangeNew,rangeOld));
				} else if (keys & MK_SHIFT) {
					SetSelection(pfc::bit_array_or(rangeNew,rangeOld),rangeNew);
				} else {
					SetSelection(pfc::bit_array_true(),rangeNew);
				}
			}
			int groupId;
			if (ItemFromPointAbs(pt,base)) {
				const CRect rcVisible = GetVisibleRectAbs(), rcItem = GetItemRectAbs(base);
				if (rcItem.top >= rcVisible.top && rcItem.bottom <= rcVisible.bottom) {
					SetFocusItem(base);
				}
			} else if (GroupHeaderFromPointAbs(pt,groupId)) {
				const CRect rcVisible = GetVisibleRectAbs();
				CRect rcGroup;
				if (GetGroupHeaderRectAbs(groupId,rcGroup)) {
					if (rcGroup.top >= rcVisible.top && rcGroup.bottom <= rcVisible.bottom) {
						SetGroupFocus(groupId);
					}
				}
			}
		}
	}
}

void CListControlWithSelectionBase::InitSelectDragMode(const CPoint & p_pt,bool p_rightClick) {

	if (! this->AllowRangeSelect() ) return;
	
	SetTimer(KSelectionTimerID,KSelectionTimerPeriod);
	m_selectDragMode = true;
	m_selectDragOriginAbs = m_selectDragCurrentAbs = p_pt + GetViewOffset();
	m_selectDragChanged = false; m_selectDragMoved = false;
	SetCapture();
}

void CListControlWithSelectionBase::AbortSelectDragMode(bool p_lostCapture) {
	if (m_selectDragMode) {
		m_selectDragMode = false;
		CRect rcSelect(m_selectDragOriginAbs,m_selectDragCurrentAbs); rcSelect.NormalizeRect();
		rcSelect.OffsetRect( - GetViewOffset() );
		if (!p_lostCapture) ::SetCapture(NULL);
		rcSelect.InflateRect(1,1);
		InvalidateRect(rcSelect);
		KillTimer(KSelectionTimerID);
	}
}


LRESULT CListControlWithSelectionBase::OnCaptureChanged(UINT,WPARAM,LPARAM,BOOL&) {
	AbortPrepareDragDropMode(true);
	AbortSelectDragMode(true);
	return 0;
}

void CListControlWithSelectionBase::RenderOverlay(const CRect & p_updaterect,CDCHandle p_dc)  {
	if (m_selectDragMode) {
		CRect rcSelect(m_selectDragOriginAbs,m_selectDragCurrentAbs);
		rcSelect.NormalizeRect();
		PaintUtils::FocusRect(p_dc,rcSelect);
	}
	if (m_dropMark != pfc_infinite) {
		RenderDropMarkerClipped(p_dc, p_updaterect, m_dropMark, m_dropMarkInside);
	}
	TParent::RenderOverlay(p_updaterect,p_dc);
}

void CListControlWithSelectionBase::SetDropMark(size_t mark, bool inside) {
	if (mark != m_dropMark || inside != m_dropMarkInside) {
		CRgn updateRgn; updateRgn.CreateRectRgn(0, 0, 0, 0);
		AddDropMarkToUpdateRgn(updateRgn, m_dropMark, m_dropMarkInside);
		m_dropMark = mark;
		m_dropMarkInside = inside;
		AddDropMarkToUpdateRgn(updateRgn, m_dropMark, m_dropMarkInside);
		RedrawWindow(NULL, updateRgn);
	}
}

static int transformDDScroll(int p_value,int p_width, int p_dpi) {
	if (p_dpi <= 0) p_dpi = 96;
	const double dpiMul = 96.0 / (double) p_dpi;
	double val = (double)(p_width - p_value);
	val *= dpiMul;
	val = pow(val,1.1) * 0.33;
	val /= dpiMul;
	return pfc::rint32(val);
}

void CListControlWithSelectionBase::HandleDDScroll() {
	if (m_ddScroll.m_timerActive) {
		const CPoint position( GetCursorPos() );
		CRect client = GetClientRectHook();
		CPoint delta (0,0);
		if (ClientToScreen(client)) {
			const CSize DPI = QueryScreenDPIEx();
			const int scrollZoneWidthBase = GetItemHeight() * 2;
			const int scrollZoneHeight = pfc::min_t<int>( scrollZoneWidthBase, client.Height() / 4 );
			const int scrollZoneWidth = pfc::min_t<int>( scrollZoneWidthBase, client.Width() / 4 );
		
			if (position.y >= client.top && position.y < client.top + scrollZoneHeight) {
				delta.y -= transformDDScroll(position.y - client.top, scrollZoneHeight, DPI.cy);
			} else if (position.y >= client.bottom - scrollZoneHeight && position.y < client.bottom) {
				delta.y += transformDDScroll(client.bottom - position.y, scrollZoneHeight, DPI.cy);
			}

			if (position.x >= client.left && position.x < client.left + scrollZoneWidth) {
				delta.x -= transformDDScroll(position.x - client.left, scrollZoneWidth, DPI.cx);
			} else if (position.x >= client.right - scrollZoneWidth && position.x < client.right) {
				delta.x += transformDDScroll(client.right - position.x, scrollZoneWidth, DPI.cx);
			}
		}

		if (delta != CPoint(0,0)) MoveViewOriginDelta(delta);
	}
}

void CListControlWithSelectionBase::ToggleDDScroll(bool p_state) {
	if (p_state != m_ddScroll.m_timerActive) {
		if (p_state) {
			SetTimer(m_ddScroll.KTimerID,m_ddScroll.KTimerPeriod);
		} else {
			KillTimer(m_ddScroll.KTimerID);
		}
		m_ddScroll.m_timerActive = p_state;
	}
}

void CListControlWithSelectionBase::PrepareDragDrop(const CPoint & p_point,bool p_isRightClick) {
	m_prepareDragDropMode = true;
	m_prepareDragDropOrigin = p_point;
	m_prepareDragDropModeRightClick = p_isRightClick;
	SetCapture();
}
void CListControlWithSelectionBase::AbortPrepareDragDropMode(bool p_lostCapture) {
	if (m_prepareDragDropMode) {
		m_prepareDragDropMode = false;
		if (!p_lostCapture) ::SetCapture(NULL);
	}
}


void CListControlWithSelectionBase::RenderItem(t_size p_item,const CRect & p_itemRect,const CRect & p_updateRect,CDCHandle p_dc) {
	//console::formatter() << "RenderItem: " << p_item;
	const bool weHaveFocus = ::GetFocus() == m_hWnd;
	const bool isSelected = this->IsItemSelected(p_item);

	const t_uint32 bkColor = GetSysColorHook(colorBackground);
	const t_uint32 hlColor = GetSysColorHook(colorSelection);
	const t_uint32 bkColorUsed = isSelected ? (weHaveFocus ? hlColor : PaintUtils::BlendColor(hlColor,bkColor)) : bkColor;

	bool alternateTextColor = false, dtt = false;
	auto & m_theme = theme();
	if (m_theme != NULL && isSelected && hlColor == GetSysColor(COLOR_HIGHLIGHT) && /*bkColor == GetSysColor(COLOR_WINDOW) && */ IsThemePartDefined(m_theme, LVP_LISTITEM, 0)) {
		//PaintUtils::RenderItemBackground(p_dc,p_itemRect,p_item+GetItemGroup(p_item),bkColor);
		DrawThemeBackground(m_theme, p_dc, LVP_LISTITEM, weHaveFocus ? LISS_SELECTED : LISS_SELECTEDNOTFOCUS , p_itemRect, p_updateRect);
		dtt = true;
	} else {
		this->RenderItemBackground(p_dc, p_itemRect, p_item, bkColorUsed );
		// PaintUtils::RenderItemBackground(p_dc,p_itemRect,p_item+GetItemGroup(p_item),bkColorUsed);
		if (isSelected) alternateTextColor = true;
	}

	{
		DCStateScope backup(p_dc);
		p_dc.SetBkMode(TRANSPARENT);
		p_dc.SetBkColor(bkColorUsed);
		p_dc.SetTextColor(alternateTextColor ? PaintUtils::DetermineTextColor(bkColorUsed) : this->GetSysColorHook(colorText));
		pfc::vartoggle_t<bool> toggle(m_drawThemeText, dtt);
		RenderItemText(p_item,p_itemRect,p_updateRect,p_dc, !alternateTextColor);
	}

	if (IsItemFocused(p_item) && weHaveFocus) {
		PaintUtils::FocusRect2(p_dc,p_itemRect, bkColorUsed);
	}
}
void CListControlWithSelectionBase::RenderSubItemText(t_size item, t_size subItem,const CRect & subItemRect,const CRect & updateRect,CDCHandle dc, bool allowColors) {
	auto ct = GetCellType(item, subItem);
	if ( ct == nullptr ) return;

	if (m_drawThemeText && ct->AllowDrawThemeText() ) for(;;) {
		pfc::string_formatter label;
		if (!GetSubItemText(item,subItem,label)) return;
		const bool weHaveFocus = ::GetFocus() == m_hWnd;
		const bool isSelected = this->IsItemSelected(item);
		pfc::stringcvt::string_os_from_utf8 cvt(label);
		if (PaintUtils::TextContainsCodes(cvt)) break;
		CRect clip = GetItemTextRect(subItemRect);
		const t_uint32 format = PaintUtils::DrawText_TranslateHeaderAlignment(GetColumnFormat(subItem));
		DrawThemeText(theme(), dc, LVP_LISTITEM, weHaveFocus ? LISS_SELECTED : LISS_SELECTEDNOTFOCUS, cvt, (int)cvt.length(), DT_NOPREFIX | DT_END_ELLIPSIS | DT_SINGLELINE | DT_VCENTER | format, 0, clip);
		return;
	} 
	__super::RenderSubItemText(item, subItem, subItemRect, updateRect, dc, allowColors);
}
void CListControlWithSelectionBase::RenderGroupHeader(int p_group,const CRect & p_headerRect,const CRect & p_updateRect,CDCHandle p_dc) {
	TParent::RenderGroupHeader(p_group,p_headerRect,p_updateRect,p_dc);
	if (IsGroupHeaderFocused(p_group)) {
		PaintUtils::FocusRect(p_dc,p_headerRect);
	}
}
CRect CListControlWithSelectionBase::DropMarkerUpdateRect(t_size index,bool bInside) const {
	if (index != ~0) {
		CRect rect;
		if (bInside) {
			rect = GetItemRect(index);
			rect.InflateRect(DropMarkerMargin());
		} else {
			rect = DropMarkerRect(DropMarkerOffset(index));
		}
		return rect;
	} else {
		return CRect(0,0,0,0);
	}
}
void CListControlWithSelectionBase::AddDropMarkToUpdateRgn(HRGN p_rgn, t_size p_index, bool bInside) const {
	CRect rect = DropMarkerUpdateRect(p_index,bInside);
	if (!rect.IsRectEmpty()) PaintUtils::AddRectToRgn(p_rgn,rect);
}

CRect CListControlWithSelectionBase::DropMarkerRect(int offset) const {
	const int delta = MulDiv(5,m_dpi.cy,96);
	CRect rc(0,offset - delta,GetViewAreaWidth(), offset + delta);
	rc.InflateRect(DropMarkerMargin());
	return rc;
}

int CListControlWithSelectionBase::DropMarkerOffset(t_size marker) const {
	return (marker > 0 ? this->GetItemRectAbs(marker-1).bottom : 0) - GetViewOffset().y;
}
bool CListControlWithSelectionBase::RenderDropMarkerClipped(CDCHandle dc, const CRect & update, t_size item, bool bInside) {
	CRect markerRect = DropMarkerUpdateRect(item,bInside); markerRect.OffsetRect( GetViewOffset() );
	CRect affected;
	if (affected.IntersectRect(markerRect,update)) {
		DCStateScope state(dc);
		if (dc.IntersectClipRect(affected)) {
			RenderDropMarker(dc,item,bInside);
			return true;
		}
	}
	return false;
}
void CListControlWithSelectionBase::RenderDropMarker(CDCHandle dc, t_size item, bool bInside) {
	if (item != ~0) {
		if (bInside) {
			CPen pen; MakeDropMarkerPen(pen);
			SelectObjectScope penScope(dc,pen);
			const CRect rc = GetItemRectAbs(item);
			dc.MoveTo(rc.left,rc.top);
			dc.LineTo(rc.right,rc.top);
			dc.LineTo(rc.right,rc.bottom);
			dc.LineTo(rc.left,rc.bottom);
			dc.LineTo(rc.left,rc.top);
		} else {
			RenderDropMarkerByOffset(DropMarkerOffset(item) + GetViewOffset().y,dc);
		}
	}
}

SIZE CListControlWithSelectionBase::DropMarkerMargin() const {
	const int penDeltaX = MulDiv(5 /* we don't know how to translate CreatePen units... */,m_dpi.cx,96);
	const int penDeltaY = MulDiv(5 /* we don't know how to translate CreatePen units... */,m_dpi.cy,96);
	SIZE s = {penDeltaX,penDeltaY};
	return s;
}
void CListControlWithSelectionBase::MakeDropMarkerPen(CPen & out) const {
	WIN32_OP_D( out.CreatePen(PS_SOLID,3,GetSysColorHook(colorText)) != NULL );
}

void CListControlWithSelectionBase::RenderDropMarkerByOffset(int offset,CDCHandle p_dc) {
	CPen pen; MakeDropMarkerPen(pen);
	const int delta = MulDiv(5,m_dpi.cy,96);
	SelectObjectScope penScope(p_dc,pen);
	const int width = GetViewAreaWidth();
	if (width > 0) {
		p_dc.MoveTo(0,offset);
		p_dc.LineTo(width-1,offset);
		p_dc.MoveTo(0,offset-delta);
		p_dc.LineTo(0,offset+delta);
		p_dc.MoveTo(width-1,offset-delta);
		p_dc.LineTo(width-1,offset+delta);
	}
}

void CListControlWithSelectionBase::FocusToUpdateRgn(HRGN rgn) {
	t_size focusItem = GetFocusItem();
	if (focusItem != ~0) AddItemToUpdateRgn(rgn,focusItem);
	else {
		int focusGroup = GetGroupFocus();
		if (focusGroup >= 0) AddGroupHeaderToUpdateRgn(rgn,focusGroup);
	}
}

void CListControlWithSelectionImpl::ReloadData() {
	if ( GetItemCount() != m_selection.get_size() ) {
		this->SelHandleReset();
	}
	__super::ReloadData();
}

void CListControlWithSelectionImpl::SetGroupFocus(int group) {
	t_size base,total;
	if (this->ResolveGroupRange(group,base,total)) {
		SetGroupFocusByItem(base);
	}
}

int CListControlWithSelectionImpl::GetGroupFocus() const {
	return (m_groupFocus && m_focus < GetItemCount()) ? GetItemGroup(m_focus) : -1;
}

void CListControlWithSelectionImpl::SetSelectionImpl(pfc::bit_array const & affected, pfc::bit_array const & status) {
	const t_size total = m_selection.get_size();
	pfc::bit_array_flatIndexList toUpdate;

	// Only fire UpdateItems for stuff that's both on-screen and actually changed
	// Firing for whole affected mask will repaint everything when selecting one item
	t_size base, count;
	if (!GetItemRangeAbs(GetVisibleRectAbs(), base, count)) { base = count = 0; }

	affected.walk( total, [&] (size_t idx) {
		if ( m_selection[idx] != status[idx] && this->CanSelectItem(idx) ) {
			m_selection[idx] = status[idx];
			if ( idx >= base && idx < base+count ) toUpdate.add(idx);
		}
	} );

	if ( toUpdate.get_count() > 0 ) {
		UpdateItems(toUpdate);
	}

	this->OnSelectionChanged( affected, status );
}

void CListControlWithSelectionImpl::SetSelection(pfc::bit_array const & affected, pfc::bit_array const & status) {
	RefreshSelectionSize();


	if ( m_selectionSupport == selectionSupportNone ) return;

	if ( m_selectionSupport == selectionSupportSingle ) {
		size_t single = SIZE_MAX;
		bool selNone = true;
		const size_t total = m_selection.get_size();
		for( size_t walk = 0; walk < total; ++ walk ) {
			if ( affected.get(walk) ) {
				if ( status.get(walk) && single == SIZE_MAX ) {
					single = walk;
				}
			} else if ( IsItemSelected( walk ) ) {
				selNone = false;
			}
		}
		if ( single < total ) {
			SetSelectionImpl( pfc::bit_array_true(), pfc::bit_array_one( single ) );
		} else if ( selNone ) {
			this->SetSelectionImpl( pfc::bit_array_true(), pfc::bit_array_false() );
		}
	} else {
		SetSelectionImpl( affected, status );
	}

}

void CListControlWithSelectionImpl::RefreshSelectionSize() {
	RefreshSelectionSize(GetItemCount());
}
void CListControlWithSelectionImpl::RefreshSelectionSize(t_size total) {
	const t_size oldSize = m_selection.get_size();
	if (total != oldSize) {
		m_selection.set_size(total);
		for(t_size walk = oldSize; walk < total; ++walk) m_selection[walk] = false;
	}
}

void CListControlWithSelectionImpl::SetGroupFocusByItem(t_size item) {
	CRgn update; update.CreateRectRgn(0,0,0,0);
	FocusToUpdateRgn(update);
	m_groupFocus = true; m_focus = item;
	FocusToUpdateRgn(update);
	InvalidateRgn(update);

	const int iGroup = GetItemGroup(item);
	CRect header; 
	if (GetGroupHeaderRectAbs(iGroup,header)) EnsureVisibleRectAbs(header);

	this->OnFocusChangedGroup( iGroup );
}

void CListControlWithSelectionImpl::SetFocusItem(t_size index) {
	CRgn update; update.CreateRectRgn(0,0,0,0);
	FocusToUpdateRgn(update);
	m_groupFocus = false; m_focus = index;
	FocusToUpdateRgn(update);
	InvalidateRgn(update);
	
	EnsureVisibleRectAbs(GetItemRectAbs(index));

	SetSelectionStart(index);

	this->OnFocusChanged( index );
}

static void UpdateIndexOnReorder(t_size & index, const t_size * order, t_size count) {
	index = pfc::permutation_find_reverse(order,count,index);
}
static void UpdateIndexOnRemoval(t_size & index, const pfc::bit_array & mask, t_size oldCount, t_size newCount) {
	if (index >= oldCount || newCount == 0) {index = ~0; return;}
	for(t_size walk = mask.find_first(true,0,oldCount); walk < newCount; ++walk) {
		if (walk < index) --index;
		else break;
	}
	if (index >= newCount) index = newCount - 1;
}
static void UpdateIndexOnInsert(t_size & index, t_size base, t_size count) {
	if (index != ~0 && index >= base) index += count;
}

void CListControlWithSelectionImpl::SelHandleReorder(const t_size * order, t_size count) {
	RefreshSelectionSize();
	UpdateIndexOnReorder(m_focus,order,count);
	UpdateIndexOnReorder(m_selectionStart,order,count);
	pfc::array_t<bool> newSel; newSel.set_size(m_selection.get_size());
	for(t_size walk = 0; walk < m_selection.get_size(); ++walk) newSel[walk] = m_selection[order[walk]];
	m_selection = newSel;
}

void CListControlWithSelectionImpl::SelHandleReset() {
	RefreshSelectionSize(GetItemCount());
	for(t_size walk = 0; walk < m_selection.get_size(); walk++) m_selection[walk] = false;
	m_focus = ~0;
	m_groupFocus = false;

}
void CListControlWithSelectionImpl::SelHandleRemoval(const pfc::bit_array & mask, t_size oldCount) {
	RefreshSelectionSize(oldCount);
	const t_size newCount = GetItemCount();
	UpdateIndexOnRemoval(m_focus,mask,oldCount,newCount);
	UpdateIndexOnRemoval(m_selectionStart,mask,oldCount,newCount);
	pfc::remove_mask_t(m_selection,mask);
}

void CListControlWithSelectionImpl::SelHandleInsertion(t_size base, t_size count, bool select) {
	UpdateIndexOnInsert(m_focus,base,count);
	UpdateIndexOnInsert(m_selectionStart,base,count);
	RefreshSelectionSize(GetItemCount() - count);

	// To behave sanely in single-select mode, we'd have to alter selection of other items from here
	// Let caller worry and outright deny select requests in modes other than multisel
	if ( m_selectionSupport != selectionSupportMulti ) select = false;

	m_selection.insert_multi(select,base,count);
}


LRESULT CListControlWithSelectionBase::OnGetDlgCode(UINT,WPARAM,LPARAM p_lp,BOOL& bHandled) {
	if (p_lp == 0) {
		return DLGC_WANTALLKEYS | DLGC_WANTCHARS | DLGC_WANTARROWS;
	} else {
		const MSG * pmsg = reinterpret_cast<const MSG *>(p_lp);
		switch(pmsg->message) {
		case WM_KEYDOWN:
		case WM_KEYUP:
			switch(pmsg->wParam) {
			case VK_ESCAPE:
			case VK_TAB:
				bHandled = FALSE;
				return 0;
			default:
				return DLGC_WANTMESSAGE;
			}
		case WM_CHAR:
			return DLGC_WANTMESSAGE;
		default:
			bHandled = FALSE;
			return 0;
		}
	}
}


bool CListControlWithSelectionBase::GetFocusRect(CRect & p_rect) {
	if (!GetFocusRectAbs(p_rect)) return false;
	p_rect.OffsetRect( - GetViewOffset() );
	return true;
}
bool CListControlWithSelectionBase::GetFocusRectAbs(CRect & p_rect) {
	t_size item = this->GetFocusItem();
	if (item != ~0) {
		p_rect = this->GetItemRectAbs(item);
		return true;
	}
	int group = this->GetGroupFocus();
	if (group >= 0) {
		return GetGroupHeaderRectAbs(group,p_rect);
	}
	
	return false;
}

CPoint CListControlWithSelectionBase::GetContextMenuPoint(CPoint ptGot) {
	CPoint pt;
	if (ptGot.x == -1 && ptGot.y == -1) {
		CRect rect;
		if (!GetFocusRectAbs(rect)) return 0;
		EnsureVisibleRectAbs(rect);
		pt = rect.CenterPoint() - GetViewOffset();
		ClientToScreen(&pt);
	} else {
		pt = ptGot;
	}
	return pt;
}

CPoint CListControlWithSelectionBase::GetContextMenuPoint(LPARAM lp) {
	CPoint pt;
	if (lp == -1) {
		CRect rect;
		if (!GetFocusRectAbs(rect)) return 0;
		EnsureVisibleRectAbs(rect);
		pt = rect.CenterPoint() - GetViewOffset();
		ClientToScreen(&pt);
	} else {
		pt = lp;
	}
	return pt;
}

bool CListControlWithSelectionBase::MakeDropReorderPermutation(pfc::array_t<t_size> & out, CPoint ptDrop) const {
	t_size insertMark = InsertIndexFromPoint(ptDrop);
	/*if (insertMark != this->GetFocusItem())*/ {
		const t_size count = GetItemCount();
		if (insertMark > count) insertMark = count;
		{
			t_size selBefore = 0;
			for(t_size walk = 0; walk < insertMark; ++walk) {
				if (IsItemSelected(walk)) selBefore++;
			}
			insertMark -= selBefore;
		}
		{
			pfc::array_t<t_size> permutation, selected, nonselected;
			const t_size selcount = this->GetSelectedCount();
			selected.set_size(selcount); nonselected.set_size(count - selcount);
			permutation.set_size(count);
			if (insertMark > nonselected.get_size()) insertMark = nonselected.get_size();
			for(t_size walk = 0, swalk = 0, nwalk = 0; walk < count; ++walk) {
				if (IsItemSelected(walk)) {
					selected[swalk++] = walk;
				} else {
					nonselected[nwalk++] = walk;
				}
			}
			for(t_size walk = 0; walk < insertMark; ++walk) {
				permutation[walk] = nonselected[walk];
			}
			for(t_size walk = 0; walk < selected.get_size(); ++walk) {
				permutation[insertMark + walk] = selected[walk];
			}
			for(t_size walk = insertMark; walk < nonselected.get_size(); ++walk) {
				permutation[selected.get_size() + walk] = nonselected[walk];
			}
			for(t_size walk = 0; walk < permutation.get_size(); ++walk) {
				if (permutation[walk] != walk) {
					out = permutation;
					return true;
				}
			}
		}
	}
	return false;
}

void CListControlWithSelectionBase::EnsureVisibleRectAbs(const CRect & p_rect) {
	if (!m_noEnsureVisible) TParent::EnsureVisibleRectAbs(p_rect);
}

bool CListControlWithSelectionBase::TypeFindCheck(DWORD ts) const {
	if (m_typeFindTS == 0) return false;
	return ts - m_typeFindTS < 1000;
}

void CListControlWithSelectionBase::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) {
	if (nChar < 32) {
		m_typeFindTS = 0;
		return;
	}
	
	const DWORD ts = GetTickCount();
	if (!TypeFindCheck(ts)) m_typeFind.reset();

	if (nChar == ' ' && m_typeFind.is_empty()) {
		m_typeFindTS = 0;
		return;
	}

	m_typeFindTS = ts;
	if (m_typeFindTS == 0) m_typeFindTS = ~0;
	char temp[10] = {};
	pfc::utf8_encode_char(nChar, temp);
	m_typeFind += temp;
	RunTypeFind();
}

static unsigned detectRepetition( pfc::string8 const & str ) {
	size_t count = 0;
	size_t walk = 0;
	uint32_t first;

	while( walk < str.length() ) {
		uint32_t current;
		auto delta = pfc::utf8_decode_char( str.c_str() + walk, current, str.length() - walk ); 
		if ( delta == 0 ) break;
		walk += delta;

		if ( count == 0 ) first = current;
		else if ( first != current ) return 0;
		
		++ count;
	}

	if ( count > 1 ) return first;
	return 0;
}

size_t CListControlWithSelectionBase::EvalTypeFind() {
	if ( GetItemCount() == 0 ) return SIZE_MAX;

	static SmartStrStr tool;

	const size_t itemCount = GetItemCount();
	const size_t colCount = GetColumnCount();
	pfc::string_formatter temp;
	t_size searchBase = this->GetFocusItem();
	if (searchBase >= itemCount) searchBase = 0;

	size_t partial = SIZE_MAX;
	size_t repetition = SIZE_MAX;
	bool useRepetition = false;
	pfc::string8 strRepetitionChar;
	unsigned repChar = detectRepetition( m_typeFind );
	if ( repChar != 0 ) {
		useRepetition = true;
		strRepetitionChar.add_char( repChar );
	}
	
	for(t_size walk = 0; walk < itemCount; ++walk) {
		t_size index = (walk + searchBase) % itemCount;
		for(size_t cWalk = 0; cWalk < colCount; ++cWalk) {
			
			temp.reset();

			if (AllowTypeFindInCell( index, cWalk )) {
				this->GetSubItemText(index, cWalk, temp);
			}
			
			if ( temp.length() == 0 ) {
				continue;
			}
			if (partial == SIZE_MAX) {
				size_t matchAt;
				if (tool.strStrEnd( temp, m_typeFind, & matchAt ) != nullptr) {
					if ( matchAt == 0 ) return index;
					partial = index;
				}
			} else {
				if ( tool.matchHere( temp, m_typeFind ) ) return index;
			}
			if (useRepetition && index != searchBase) {
				if ( tool.matchHere( temp, strRepetitionChar ) ) {
					useRepetition = false;
					repetition = index;
				}
			}
		}
	}
	if (partial < itemCount) return partial;
	if (repetition < itemCount) return repetition;
	return SIZE_MAX;
}

void CListControlWithSelectionBase::RunTypeFind() {
	size_t index = EvalTypeFind();
	if (index < GetItemCount() ) {
		this->SetFocusItem( index );
		this->SetSelection(pfc::bit_array_true(), pfc::bit_array_one(index) );
	} else {
		MessageBeep(0);
	}
}

CRect CListControlWithSelectionBase::GetWholeSelectionRectAbs() const {
	CRect rcTotal;
	const size_t count = GetItemCount();
	for( size_t w = 0; w < count; ++w ) {
		if ( IsItemSelected( w ) ) {
			CRect rcItem = GetItemRectAbs(w);
			if ( rcTotal.IsRectNull( ) ) rcTotal = rcItem;
			else rcTotal|=rcItem;
		}
	}
	return rcTotal;
}

size_t CListControlWithSelectionBase::GetFirstSelected() const {
	const size_t count = GetItemCount();
	for( size_t w = 0; w < count; ++w ) {
		if ( IsItemSelected(w) ) return w;
	}
	return pfc_infinite;
}
size_t CListControlWithSelectionBase::GetLastSelected() const {
	const size_t count = GetItemCount();
	for( size_t w = count - 1; (t_ssize) w >= 0; --w ) {
		if ( IsItemSelected(w) ) return w;
	}
	return pfc_infinite;
}

namespace {
	class CDropTargetImpl : public ImplementCOMRefCounter<IDropTarget> {
	public:
		COM_QI_BEGIN()
			COM_QI_ENTRY(IUnknown)
			COM_QI_ENTRY(IDropTarget)
		COM_QI_END()

		bool valid = true;
		std::function<void(CPoint pt) > Track;
		std::function<DWORD (IDataObject*)> HookAccept;
		std::function<void (IDataObject*, CPoint pt)> HookDrop;
		std::function<void ()> HookLeave;
		
		DWORD m_effect = DROPEFFECT_NONE;
		
		HRESULT STDMETHODCALLTYPE DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) {
			if (pDataObj == NULL || pdwEffect == NULL) return E_INVALIDARG;
			if (!valid) return E_FAIL;
			if ( HookAccept ) {
				m_effect = HookAccept(pDataObj);
			} else {
				m_effect = DROPEFFECT_MOVE;
			}
			*pdwEffect = m_effect;
			return S_OK;
		}
		HRESULT STDMETHODCALLTYPE DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) {
			if (pdwEffect == NULL) return E_INVALIDARG;
			if (!valid) return E_FAIL;
			if ( m_effect != DROPEFFECT_NONE ) Track(CPoint(pt.x, pt.y));
			*pdwEffect = m_effect;
			return S_OK;
		}
		HRESULT STDMETHODCALLTYPE DragLeave() {
			if (HookLeave) HookLeave();
			return S_OK;
		}
		HRESULT STDMETHODCALLTYPE Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) {
			if ( HookDrop && m_effect != DROPEFFECT_NONE ) {
				HookDrop( pDataObj, CPoint(pt.x, pt.y) );
			}
			return S_OK;
		}
	};

	class CDropSourceImpl : public ImplementCOMRefCounter<IDropSource> {
	public:
		CPoint droppedAt;
		bool droppedAtValid = false;
		bool allowReorder = false;

		bool allowDragOutside = false;
		CWindow wndOrigin;

		COM_QI_BEGIN()
			COM_QI_ENTRY(IUnknown)
			COM_QI_ENTRY(IDropSource)
		COM_QI_END()

		HRESULT STDMETHODCALLTYPE GiveFeedback(DWORD dwEffect) {
			m_effect = dwEffect;
			return DRAGDROP_S_USEDEFAULTCURSORS;
		}

		HRESULT STDMETHODCALLTYPE QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState) {

			if (fEscapePressed || (grfKeyState & MK_RBUTTON) != 0) {
				return DRAGDROP_S_CANCEL;
			} else if (!(grfKeyState & MK_LBUTTON)) {
				if (m_effect == DROPEFFECT_NONE) return DRAGDROP_S_CANCEL;

				const CPoint pt(GetCursorPos());
				bool bInside = false;
				if (wndOrigin) {
					CRect rc;
					WIN32_OP_D(wndOrigin.GetWindowRect(rc));
					bInside = rc.PtInRect(pt);
				}
				if (!allowDragOutside && !bInside) return DRAGDROP_S_CANCEL;
				
				if ( allowReorder && bInside) {
					droppedAt = pt;
					droppedAtValid = true;
					return DRAGDROP_S_CANCEL;
				}
				return DRAGDROP_S_DROP;
				
			} else {
				return S_OK;
			}
		}
	private:
		DWORD m_effect = 0;
	};
}

void CListControlWithSelectionBase::RunDragDrop(const CPoint & p_origin, bool p_isRightClick) {

	uint32_t flags = this->QueryDragDropTypes();
	if ( flags == 0 ) {
		PFC_ASSERT(!"How did we get here?");
		return;
	}
	if ( flags == dragDrop_reorder ) {
		if ( p_isRightClick ) return;
		CPoint ptDrop;
		if ( RunReorderDragDrop( p_origin, ptDrop ) ) {
			pfc::array_t<size_t> order;
			if (MakeDropReorderPermutation(order, ptDrop)) {
				this->RequestReorder(order.get_ptr(), order.get_size());
			}
		}
		return;
	}

	auto obj = this->MakeDataObject();
	if (obj.is_empty()) {
		PFC_ASSERT(!"How did we get here? No IDataObject");
		return;
	}

	pfc::com_ptr_t<CDropSourceImpl> source = new CDropSourceImpl();
	source->wndOrigin = *this;
	source->allowDragOutside = true;
	source->allowReorder = (flags & dragDrop_reorder) != 0;

	DWORD outEffect = DROPEFFECT_NONE;
	HRESULT status = DoDragDrop(obj.get_ptr(), source.get_ptr(), DragDropSourceEffects() , &outEffect);

	if ( source->droppedAtValid ) {
		CPoint ptDrop = source->droppedAt;
		WIN32_OP_D(this->ScreenToClient(&ptDrop));
		pfc::array_t<size_t> order;
		if (MakeDropReorderPermutation(order, ptDrop)) {
			this->RequestReorder(order.get_ptr(), order.get_size());
		}
	} else if (status == DRAGDROP_S_DROP) {
		DragDropSourceSucceeded(outEffect);
	}
}

pfc::com_ptr_t<IDataObject> CListControlWithSelectionBase::MakeDataObject() {
	// return dummy IDataObject, presume derived transmits drag and drop payload by other means
	using namespace IDataObjectUtils;
	return new ImplementCOMRefCounter< CDataObjectBase >();
}

bool CListControlWithSelectionBase::RunReorderDragDrop(CPoint ptOrigin, CPoint & ptDrop) {
	pfc::com_ptr_t<CDropSourceImpl> source = new CDropSourceImpl();
	pfc::com_ptr_t<CDropTargetImpl> target = new CDropTargetImpl();
	
	source->wndOrigin = *this;
	source->allowDragOutside = false;
	source->allowReorder = true;

	target->Track = [this](CPoint pt) { 
		WIN32_OP_D(this->ScreenToClient(&pt));
		size_t idx = this->InsertIndexFromPoint(pt);
		this->SetDropMark(idx, false);
	};

	if ( FAILED(RegisterDragDrop(*this, target.get_ptr())) ) {
		// OleInitialize not called?
		PFC_ASSERT( !"Should not get here" );
		return false;
	}

	pfc::onLeaving scope([=] { target->valid = false; RevokeDragDrop(*this); });

	using namespace IDataObjectUtils;
	pfc::com_ptr_t<IDataObject> dataobject = new ImplementCOMRefCounter< CDataObjectBase >();
	DWORD outeffect = 0;

	ToggleDDScroll(true);
	DoDragDrop(dataobject.get_ptr(), source.get_ptr(), DROPEFFECT_MOVE, &outeffect);
	ClearDropMark();
	ToggleDDScroll(false);
	if (source->droppedAtValid ) {
		CPoint pt = source->droppedAt;
		WIN32_OP_D( this->ScreenToClient( &pt ) );
		ptDrop = pt;
		return true;
	}
	return false;
}

int CListControlWithSelectionBase::OnCreatePassThru(LPCREATESTRUCT lpCreateStruct) {
	const uint32_t flags = this->QueryDragDropTypes();
	if ( flags & dragDrop_external ) {

		pfc::com_ptr_t<CDropTargetImpl> target = new CDropTargetImpl();

		std::shared_ptr<bool> showDropMark = std::make_shared<bool>();

		target->HookAccept = [this, flags, showDropMark] ( IDataObject * obj ) {
			*showDropMark = true;
			if (this->m_ownDDActive) {
				// Do not generate OnDrop for reorderings
				if (flags & dragDrop_reorder) return (DWORD)DROPEFFECT_MOVE;
			}
			return this->DragDropAccept(obj, *showDropMark);
		};
		target->HookDrop = [this, flags] ( IDataObject * obj, CPoint pt ) {
			this->ClearDropMark();
			if ( this->m_ownDDActive ) {
				// Do not generate OnDrop for reorderings
				if ( flags & dragDrop_reorder ) return;
			}
			this->OnDrop( obj, pt );
		};
		target->HookLeave = [this] {
			this->ClearDropMark();
		};

		target->Track = [this, showDropMark](CPoint pt) {
			if ( *showDropMark ) {
				WIN32_OP_D(this->ScreenToClient(&pt));
				size_t idx = this->InsertIndexFromPoint(pt);
				this->SetDropMark(idx, false);
			} else {
				this->ClearDropMark();
			}
		};

		RegisterDragDrop(*this, target.get_ptr() );
	}
	SetMsgHandled(FALSE); return 0;
}

void CListControlWithSelectionBase::OnDestroyPassThru() {
	AbortSelectDragMode();
	ToggleDDScroll(false);
	RevokeDragDrop(*this);
	SetMsgHandled(FALSE);
}

size_t CListControlWithSelectionBase::GetPasteTarget(const CPoint * ptPaste) const {
	size_t target = ~0;
	if (ptPaste != nullptr) {
		CPoint pt(*ptPaste); WIN32_OP_D(ScreenToClient(&pt));
		int iGroup;
		if (GroupHeaderFromPoint(pt, iGroup)) {
			t_size base, count;
			if (ResolveGroupRange(iGroup, base, count)) {
				target = base;
			}
		} else if (ItemFromPoint(pt, target)) {
			auto rc = GetItemRect(target);
			auto height = rc.Height();
			if (height > 0) {
				double posInItem = (double)(pt.y - rc.top) / (double)height;
				if (posInItem >= 0.5) ++target;
			}
		}
	} else if (GroupFocusActive()) {
		t_size base, count;
		if (ResolveGroupRange(GetGroupFocus(), base, count)) {
			target = base;
		}
	} else {
		target = GetFocusItem();
	}
	return target;
}


pfc::bit_array_table CListControlWithSelectionImpl::GetSelectionMaskRef() {
	return pfc::bit_array_table(this->GetSelectionArray(), this->GetItemCount());
}
pfc::bit_array_bittable CListControlWithSelectionImpl::GetSelectionMask() {
	pfc::bit_array_bittable ret;
	const auto count = GetItemCount();
	ret.resize( GetItemCount() );
	for( size_t walk = 0; walk < count; ++ walk ) {
		ret.set(walk, IsItemSelected(walk));
	}
	return ret;
}

void CListControlWithSelectionImpl::OnItemsReordered( const size_t * order, size_t count) {
	PFC_ASSERT( count == GetItemCount() );

	SelHandleReorder( order, count );
	__super::OnItemsReordered(order, count);
}

void CListControlWithSelectionImpl::OnItemsRemoved( pfc::bit_array const & mask, size_t oldCount) {
	SelHandleRemoval(mask, oldCount);
	__super::OnItemsRemoved( mask, oldCount );
}

void CListControlWithSelectionImpl::OnItemsInserted( size_t at, size_t count, bool bSelect ) {
	SelHandleInsertion( at, count, bSelect);
	__super::OnItemsInserted( at, count, bSelect );
}

bool CListControlWithSelectionImpl::SelectAll() {
	if ( m_selectionSupport != selectionSupportMulti ) return false;
	return __super::SelectAll();
}
