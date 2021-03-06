#include "StdAfx.h"
#include "UIContainer.h"

ContainerUI::ContainerUI() : 
m_hwndScroll(NULL), 
    m_iPadding(0),
    m_iScrollPos(0),
    m_bAutoDestroy(true),
    m_bAllowScrollbars(false)
{
    m_cxyFixed.cx = m_cxyFixed.cy = 0;
    ::ZeroMemory(&m_rcInset, sizeof(m_rcInset));
}

ContainerUI::~ContainerUI()
{
    RemoveAll();
}

const char* ContainerUI::GetClass() const
{
    return "ContainerUI";
}

void* ContainerUI::GetInterface(const char* name)
{
    if (str::Eq(name, "Container"))  return static_cast<IContainerUI*>(this);
    return ControlUI::GetInterface(name);
}

ControlUI* ContainerUI::GetItem(int idx) const
{
    if (idx < 0 || idx >= m_items.GetSize())
        return NULL;
    return m_items[idx];
}

int ContainerUI::GetCount() const
{
    return m_items.GetSize();
}

bool ContainerUI::Add(ControlUI* ctrl)
{
    if (m_mgr != NULL)  m_mgr->InitControls(ctrl, this);
    if (m_mgr != NULL)  m_mgr->UpdateLayout();
    return m_items.Add(ctrl);
}

bool ContainerUI::Remove(ControlUI* ctrl)
{
    for (int it = 0; m_bAutoDestroy && it < m_items.GetSize(); it++)  {
        if (m_items[it] == ctrl)  {
            if (m_mgr != NULL)  m_mgr->UpdateLayout();
            delete ctrl;
            return m_items.RemoveAt(it);
        }
    }
    return false;
}

void ContainerUI::RemoveAll()
{
    for (int it = 0; m_bAutoDestroy && it < m_items.GetSize(); it++)  delete static_cast<ControlUI*>(m_items[it]);
    m_items.Empty();
    m_iScrollPos = 0;
    if (m_mgr != NULL)  m_mgr->UpdateLayout();
}

void ContainerUI::SetAutoDestroy(bool bAuto)
{
    m_bAutoDestroy = bAuto;
}

void ContainerUI::SetInset(SIZE szInset)
{
    m_rcInset.left = m_rcInset.right = szInset.cx;
    m_rcInset.top = m_rcInset.bottom = szInset.cy;
}

void ContainerUI::SetInset(RECT rcInset)
{
    m_rcInset = rcInset;
}

void ContainerUI::SetPadding(int iPadding)
{
    m_iPadding = iPadding;
}

void ContainerUI::SetWidth(int cx)
{
    m_cxyFixed.cx = cx;
}

void ContainerUI::SetHeight(int cy)
{
    m_cxyFixed.cy = cy;
}

void ContainerUI::SetVisible(bool visible)
{
    // Hide possible scrollbar control
    // TODO: doesn't show if visible == true but no need to show the scrollbar
    if (m_hwndScroll != NULL)
        ::ShowScrollBar(m_hwndScroll, SB_CTL, visible);
    // Hide children as well
    for (int it = 0; it < m_items.GetSize(); it++)  {
        m_items[it]->SetVisible(visible);
    }
    ControlUI::SetVisible(visible);
}

void ContainerUI::Event(TEventUI& event)
{
    if (!IsScrollYVisible()) {
        ControlUI::Event(event);
        return;
    }
        
    if (event.type == UIEVENT_VSCROLL)  
    {
        switch (LOWORD(event.wParam))  {
        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
            {
                SCROLLINFO si = { 0 };
                si.cbSize = sizeof(SCROLLINFO);
                si.fMask = SIF_TRACKPOS;
                ::GetScrollInfo(m_hwndScroll, SB_CTL, &si);
                SetScrollPos(si.nTrackPos);
            }
            break;
        case SB_LINEUP:
            SetScrollPos(GetScrollPos() - 5);
            break;
        case SB_LINEDOWN:
            SetScrollPos(GetScrollPos() + 5);
            break;
        case SB_PAGEUP:
            SetScrollPos(GetScrollPos() - GetScrollPage());
            break;
        case SB_PAGEDOWN:
            SetScrollPos(GetScrollPos() + GetScrollPage());
            break;
        }
    }

    if (event.type == UIEVENT_KEYDOWN)  
    {
        switch (event.chKey)  {
        case VK_DOWN:
            SetScrollPos(GetScrollPos() + 5);
            return;
        case VK_UP:
            SetScrollPos(GetScrollPos() - 5);
            return;
        case VK_NEXT:
            SetScrollPos(GetScrollPos() + GetScrollPage());
            return;
        case VK_PRIOR:
            SetScrollPos(GetScrollPos() - GetScrollPage());
            return;
        case VK_HOME:
            SetScrollPos(0);
            return;
        case VK_END:
            SetScrollPos(9999);
            return;
        }
    }

    ControlUI::Event(event);
}

int ContainerUI::GetScrollPos() const
{
    return m_iScrollPos;
}

int ContainerUI::GetScrollPage() const
{
    // TODO: Determine this dynamically
    return 40;
}

SIZE ContainerUI::GetScrollRange() const
{
    if (!IsScrollYVisible())
        return CSize();
    int cx = 0, cy = 0;
    ::GetScrollRange(m_hwndScroll, SB_CTL, &cx, &cy);
    return CSize(cx, cy);
}

void ContainerUI::SetScrollPos(int iScrollPos)
{
    if (!IsScrollYVisible())  return;
    int iRange1 = 0, iRange2 = 0;
    ::GetScrollRange(m_hwndScroll, SB_CTL, &iRange1, &iRange2);
    iScrollPos = CLAMP(iScrollPos, iRange1, iRange2);
    ::SetScrollPos(m_hwndScroll, SB_CTL, iScrollPos, TRUE);
    m_iScrollPos = ::GetScrollPos(m_hwndScroll, SB_CTL);
    // Reposition children to the new viewport.
    SetPos(m_rcItem);
    Invalidate();
}

void ContainerUI::EnableScrollBar(bool bEnable)
{
    if (m_bAllowScrollbars == bEnable)  return;
    m_iScrollPos = 0;
    m_bAllowScrollbars = bEnable;
}

int ContainerUI::FindSelectable(int idx, bool bForward /*= true*/) const
{
    // NOTE: This is actually a helper-function for the list/combo/ect controls
    //       that allow them to find the next enabled/available selectable item
    if (GetCount() == 0)  return -1;
    idx = CLAMP(idx, 0, GetCount() - 1);
    if (bForward)  {
        for (int i = idx; i < GetCount(); i++)  {
            if (GetItem(i)->GetInterface("ListItem") != NULL 
                && GetItem(i)->IsVisible()
                && GetItem(i)->IsEnabled())  return i;
        }
        return -1;
    } else {
        for (int i = idx; i >= 0; --i)  {
            if (GetItem(i)->GetInterface("ListItem") != NULL 
                && GetItem(i)->IsVisible()
                && GetItem(i)->IsEnabled()) {
                    return i;
            }
        }
        return FindSelectable(0, true);
    }
}

void ContainerUI::SetPos(RECT rc)
{
    ControlUI::SetPos(rc);
    if (m_items.IsEmpty())  return;
    rc.left += m_rcInset.left;
    rc.top += m_rcInset.top;
    rc.right -= m_rcInset.right;
    rc.bottom -= m_rcInset.bottom;
    // We'll position the first child in the entire client area.
    // Do not leave a container empty.
    ASSERT(m_items.GetSize()==1);
    m_items[0]->SetPos(rc);
}

SIZE ContainerUI::EstimateSize(SIZE /*szAvailable*/)
{
    return m_cxyFixed;
}

void ContainerUI::SetAttribute(const char* name, const char* value)
{
    int n;
    if (ParseInt(name, value, "inset", n))
        SetInset(CSize(n, n));
    else if (ParseInt(name, value, "padding", n))
        SetPadding(n);
    else if (ParseWidth(name, value, n))
        SetWidth(n);
    else if (ParseHeight(name, value, n))
        SetHeight(n);
    else if (str::Eq(name, "scrollbar"))
        EnableScrollBar(str::Eq(value, "true"));
    else ControlUI::SetAttribute(name, value);
}

void ContainerUI::SetManager(PaintManagerUI* manager, ControlUI* parent)
{
    for (int it = 0; it < m_items.GetSize(); it++)  {
        m_items[it]->SetManager(manager, this);
    }
    ControlUI::SetManager(manager, parent);
}

ControlUI* ContainerUI::FindControl(FINDCONTROLPROC Proc, void* data, UINT uFlags)
{
    // Check if this guy is valid
    if (IsFlSet(uFlags, UIFIND_VISIBLE) && !IsVisible())  return NULL;
    if (IsFlSet(uFlags, UIFIND_ENABLED) && !IsEnabled())  return NULL;
    if (IsFlSet(uFlags, UIFIND_HITTEST) && !::PtInRect(&m_rcItem, *(static_cast<LPPOINT>(data))))  return NULL;
    if (IsFlSet(uFlags, UIFIND_ME_FIRST))  {
        ControlUI* ctrl = ControlUI::FindControl(Proc, data, uFlags);
        if (ctrl != NULL)  return ctrl;
    }
    for (int it = 0; it != m_items.GetSize(); it++)  {
        ControlUI* ctrl = m_items[it]->FindControl(Proc, data, uFlags);
        if (ctrl != NULL)  return ctrl;
    }
    return ControlUI::FindControl(Proc, data, uFlags);
}

void ContainerUI::PaintBackground(HDC hDC, const RECT& rcPaint)
{
    if (-1 == m_bgCol && (UICOLOR__INVALID == m_bgColIdx || UICOLOR_TRANSPARENT == m_bgColIdx))
        return;
    COLORREF bgCol = m_bgCol;
    if (m_bgColIdx != UICOLOR__INVALID) {
        bgCol = m_mgr->GetThemeColor(m_bgColIdx);
    }
    BlueRenderEngineUI::DoFillRect(hDC, m_mgr, rcPaint, bgCol);
}

void ContainerUI::DoPaint(HDC hDC, const RECT& rcPaint)
{
    RECT rcTemp = { 0 };
    if (!::IntersectRect(&rcTemp, &rcPaint, &m_rcItem))
        return;

    RenderClip clip;
    BlueRenderEngineUI::GenerateClip(hDC, m_rcItem, clip);

    PaintBackground(hDC, rcTemp);

    for (int it = 0; it < m_items.GetSize(); it++)  {
        ControlUI* ctrl = m_items[it];
        if (!ctrl->IsVisible())
            continue;
        if (!::IntersectRect(&rcTemp, &rcPaint, &ctrl->GetPos()))
            continue;
        if (!::IntersectRect(&rcTemp, &m_rcItem, &ctrl->GetPos()))
            continue;
        ctrl->DoPaint(hDC, rcPaint);
    }
}

void ContainerUI::ProcessScrollbar(RECT rc, int cyRequired)
{
    // Need the scrollbar control, but it's been created already?
    if (cyRequired > RectDy(rc) && m_hwndScroll == NULL && m_bAllowScrollbars)  {
        m_hwndScroll = ::CreateWindowExA(0, WC_SCROLLBARA, NULL, WS_CHILD | SBS_VERT, 0, 0, 0, 0, m_mgr->GetPaintWindow(), NULL, m_mgr->GetResourceInstance(), NULL);
        ASSERT(::IsWindow(m_hwndScroll));
        ::SetPropA(m_hwndScroll, "WndX", static_cast<HANDLE>(this));
        ::SetScrollPos(m_hwndScroll, SB_CTL, 0, TRUE);
        ::ShowWindow(m_hwndScroll, SW_SHOWNOACTIVATE);
        SetPos(m_rcItem);
        return;
    }
    // No scrollbar required
    if (m_hwndScroll == NULL)
        return;
    // Move it into place
    int cxScroll = m_mgr->GetSystemMetrics().cxvscroll;
    ::MoveWindow(m_hwndScroll, rc.right, rc.top, cxScroll, RectDy(rc), TRUE);
    // Scroll not needed anymore?
    int cyScroll = cyRequired - RectDy(rc);
    if (cyScroll < 0)  {
        if (m_iScrollPos != 0)
            SetScrollPos(0);
        cyScroll = 0;
    }
    // Scroll range changed?
    int cyOld1, cyOld2;
    ::GetScrollRange(m_hwndScroll, SB_CTL, &cyOld1, &cyOld2);
    if (cyOld2 != cyScroll)  {
        ::SetScrollRange(m_hwndScroll, SB_CTL, 0, cyScroll, FALSE);
        if (cyScroll == 0) {
            ::ShowScrollBar(m_hwndScroll, SB_CTL, FALSE);
        } else {
            ::ShowScrollBar(m_hwndScroll, SB_CTL, TRUE);
            //::EnableScrollBar(m_hwndScroll, SB_CTL, ESB_ENABLE_BOTH);
        }
    }
}

bool ContainerUI::IsScrollYVisible() const
{
    if (!m_hwndScroll)
        return false;
    return IsWindowVisible(m_hwndScroll) != 0;
}

CanvasUI::CanvasUI() : m_hBitmap(NULL), m_iOrientation(HTBOTTOMRIGHT)
{
}

CanvasUI::~CanvasUI()
{
    ::DeleteObject(m_hBitmap);
}

const char* CanvasUI::GetClass() const
{
    return "CanvasUI";
}

bool CanvasUI::SetWatermark(UINT iBitmapRes, int iOrientation)
{
    return SetWatermark(MAKEINTRESOURCEA(iBitmapRes), iOrientation);
}

bool CanvasUI::SetWatermark(const char* pstrBitmap, int iOrientation)
{
    ::DeleteObject(m_hBitmap);
    m_hBitmap = (HBITMAP) ::LoadImageA(m_mgr->GetResourceInstance(), pstrBitmap, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    ASSERT(m_hBitmap!=NULL);
    if (m_hBitmap == NULL)  return false;
    ::GetObject(m_hBitmap, sizeof(BITMAP), &m_BitmapInfo);
    m_iOrientation = iOrientation;
    Invalidate();
    return true;
}

void CanvasUI::DoPaint(HDC hDC, const RECT& rcPaint)
{
    // Fill background
    RECT rcFill = { 0 };
    if (::IntersectRect(&rcFill, &rcPaint, &m_rcItem))  {
        PaintBackground(hDC, rcFill);
    }
    // Paint watermark bitmap
    if (m_hBitmap != NULL)  {
        RECT rcBitmap = { 0 };
        int left = m_rcItem.right - m_BitmapInfo.bmWidth;
        int right = m_rcItem.right;
        switch (m_iOrientation)  {
        case HTTOPRIGHT:
            ::SetRect(&rcBitmap, left, m_rcItem.top,                            right, m_rcItem.top + m_BitmapInfo.bmHeight);
            break;
        case HTBOTTOMRIGHT:
            ::SetRect(&rcBitmap, left, m_rcItem.bottom - m_BitmapInfo.bmHeight, right, m_rcItem.bottom);
            break;
        default:
            ::SetRect(&rcBitmap, left, m_rcItem.bottom - m_BitmapInfo.bmHeight, right, m_rcItem.bottom);
            break;
        }
        RECT rcTemp = { 0 };
        if (::IntersectRect(&rcTemp, &rcPaint, &rcBitmap))  {
            RenderClip clip;
            BlueRenderEngineUI::GenerateClip(hDC, m_rcItem, clip);
            BlueRenderEngineUI::DoPaintBitmap(hDC, m_mgr, m_hBitmap, rcBitmap);
        }
    }

    // a hack to disable painting the background in ContainerUI::DoPaint()
    COLORREF col = m_bgCol;
    UITYPE_COLOR colIdx = m_bgColIdx;
    m_bgCol = -1;
    m_bgColIdx = UICOLOR_TRANSPARENT;
    ContainerUI::DoPaint(hDC, rcPaint);
    m_bgColIdx = colIdx;
    m_bgCol = col;
}

void CanvasUI::SetAttribute(const char* name, const char* value)
{
    if (str::Eq(name, "watermark"))  SetWatermark(value);
    else ContainerUI::SetAttribute(name, value);
}

ControlCanvasUI::ControlCanvasUI()
{
    SetInset(CSize(0, 0));
    m_bgColIdx = UICOLOR_CONTROL_BACKGROUND_NORMAL;
}

const char* ControlCanvasUI::GetClass() const
{
    return "ControlCanvasUI";
}

VerticalLayoutUI::VerticalLayoutUI() : m_cyNeeded(0)
{
}

const char* VerticalLayoutUI::GetClass() const
{
    return "VertialLayoutUI";
}

void VerticalLayoutUI::SetPos(RECT rc)
{
    m_rcItem = rc;
    // Adjust for inset
    rc.left += m_rcInset.left;
    rc.top += m_rcInset.top;
    rc.right -= m_rcInset.right;
    rc.bottom -= m_rcInset.bottom;
    if (IsScrollYVisible())
        rc.right -= m_mgr->GetSystemMetrics().cxvscroll;
    // Determine the minimum size
    SIZE szAvailable = { RectDx(rc), RectDy(rc) };
    int nAdjustables = 0;
    int cyFixed = 0;
    for (int it1 = 0; it1 < m_items.GetSize(); it1++)  {
        ControlUI* ctrl = m_items[it1];
        if (!ctrl->IsVisible())
            continue;
        SIZE sz = ctrl->EstimateSize(szAvailable);
        if (sz.cy == 0)  nAdjustables++;
        cyFixed += sz.cy + m_iPadding;
    }
    // Place elements
    int cyNeeded = 0;
    int cyExpand = 0;
    if (nAdjustables > 0)
        cyExpand = MAX(0, (szAvailable.cy - cyFixed) / nAdjustables);
    // Position the elements
    SIZE szRemaining = szAvailable;
    int posY = rc.top - m_iScrollPos;
    int iAdjustable = 0;
    for (int it2 = 0; it2 < m_items.GetSize(); it2++)  {
        ControlUI* ctrl = m_items[it2];
        if (!ctrl->IsVisible())
            continue;
        SIZE sz = ctrl->EstimateSize(szRemaining);
        if (sz.cy == 0)  {
            iAdjustable++;
            sz.cy = cyExpand;
            // Distribute remaining to last element (usually round-off left-overs)
            if (iAdjustable == nAdjustables)
                sz.cy += MAX(0, szAvailable.cy - (cyExpand * nAdjustables) - cyFixed);
        }
        RECT rcCtrl = { rc.left, posY, rc.right, posY + sz.cy };
        ctrl->SetPos(rcCtrl);
        posY += sz.cy + m_iPadding;
        cyNeeded += sz.cy + m_iPadding;
        szRemaining.cy -= sz.cy + m_iPadding;
    }
    // Handle overflow with scrollbars
    ProcessScrollbar(rc, cyNeeded);
}

HorizontalLayoutUI::HorizontalLayoutUI()
{
}

const char* HorizontalLayoutUI::GetClass() const
{
    return "HorizontalLayoutUI";
}

void HorizontalLayoutUI::SetPos(RECT rc)
{
    m_rcItem = rc;
    // Adjust for inset
    rc.left += m_rcInset.left;
    rc.top += m_rcInset.top;
    rc.right -= m_rcInset.right;
    rc.bottom -= m_rcInset.bottom;
    // Determine the width of elements that are sizeable
    SIZE szAvailable = { RectDx(rc), RectDy(rc) };
    int nAdjustables = 0;
    int cxFixed = 0;
    for (int it1 = 0; it1 < m_items.GetSize(); it1++)  {
        ControlUI* ctrl = m_items[it1];
        if (!ctrl->IsVisible())
            continue;
        SIZE sz = ctrl->EstimateSize(szAvailable);
        if (sz.cx == 0)  nAdjustables++;
        cxFixed += sz.cx + m_iPadding;
    }
    int cxExpand = 0;
    if (nAdjustables > 0)
        cxExpand = MAX(0, (szAvailable.cx - cxFixed) / nAdjustables);
    // Position the elements
    SIZE szRemaining = szAvailable;
    int posX = rc.left;
    int iAdjustable = 0;
    for (int it2 = 0; it2 < m_items.GetSize(); it2++)  {
        ControlUI* ctrl = m_items[it2];
        if (!ctrl->IsVisible())
            continue;
        SIZE sz = ctrl->EstimateSize(szRemaining);
        if (sz.cx == 0)  {
            iAdjustable++;
            sz.cx = cxExpand;
            if (iAdjustable == nAdjustables)
                sz.cx += MAX(0, szAvailable.cx - (cxExpand * nAdjustables) - cxFixed);
        }
        RECT rcCtrl = { posX, rc.top, posX + sz.cx, rc.bottom };
        ctrl->SetPos(rcCtrl);
        posX += sz.cx + m_iPadding;
        szRemaining.cx -= sz.cx + m_iPadding;
    }
}

TileLayoutUI::TileLayoutUI() : m_nColumns(2), m_cyNeeded(0)
{
    SetPadding(10);
    SetInset(CSize(10, 10));
}

const char* TileLayoutUI::GetClass() const
{
    return "TileLayoutUI";
}

void TileLayoutUI::SetColumns(int nCols)
{
    if (nCols <= 0)  return;
    m_nColumns = nCols;
    UpdateLayout();
}

void TileLayoutUI::SetPos(RECT rc)
{
    m_rcItem = rc;
    // Adjust for inset
    rc.left += m_rcInset.left;
    rc.top += m_rcInset.top;
    rc.right -= m_rcInset.right;
    rc.bottom -= m_rcInset.bottom;
    if (IsScrollYVisible())
        rc.right -= m_mgr->GetSystemMetrics().cxvscroll;
    // Position the elements
    int cxWidth = RectDx(rc) / m_nColumns;
    int cyHeight = 0;
    int iCount = 0;
    POINT ptTile = { rc.left, rc.top - m_iScrollPos };
    for (int it1 = 0; it1 < m_items.GetSize(); it1++)  {
        ControlUI* ctrl = m_items[it1];
        if (!ctrl->IsVisible())
            continue;
        // Determine size
        RECT rcTile = { ptTile.x, ptTile.y, ptTile.x + cxWidth, ptTile.y };
        // Adjust with element padding
        if ((iCount % m_nColumns) == 0)
            rcTile.right -= m_iPadding / 2;
        else if ((iCount % m_nColumns) == m_nColumns - 1)
            rcTile.left += m_iPadding / 2;
        else
            ::InflateRect(&rcTile, -(m_iPadding / 2), 0);
        // If this panel expands vertically
        if (m_cxyFixed.cy == 0) {
            SIZE szAvailable = { RectDx(rcTile), 9999 };
            int idx = iCount;
            for (int it2 = it1; it2 < m_items.GetSize(); it2++)  {
                SIZE szTile = m_items[it2]->EstimateSize(szAvailable);
                cyHeight = MAX(cyHeight, szTile.cy);
                if ((++idx % m_nColumns) == 0)
                    break;
            }
        }
        // Set position
        rcTile.bottom = rcTile.top + cyHeight;
        ctrl->SetPos(rcTile);
        // Move along...
        if ((++iCount % m_nColumns) == 0)  {
            ptTile.x = rc.left;
            ptTile.y += cyHeight + m_iPadding;
            cyHeight = 0;
        } else {
            ptTile.x += cxWidth;
        }
        m_cyNeeded = rcTile.bottom - (rc.top - m_iScrollPos);
    }
    // Process the scrollbar
    ProcessScrollbar(rc, m_cyNeeded);
}

DialogLayoutUI::DialogLayoutUI() : m_bFirstResize(true), m_aModes(sizeof(StretchMode))
{
    ::ZeroMemory(&m_rcDialog, sizeof(m_rcDialog));
}

const char* DialogLayoutUI::GetClass() const
{
    return "DialogLayoutUI";
}

void* DialogLayoutUI::GetInterface(const char* name)
{
    if (str::Eq(name, "DialogLayout"))  return this;
    return ContainerUI::GetInterface(name);
}

void DialogLayoutUI::SetStretchMode(ControlUI* ctrl, UINT uMode)
{
    StretchMode mode;
    mode.ctrl = ctrl;
    mode.uMode = uMode;
    mode.rcItem = ctrl->GetPos();
    m_aModes.Add(&mode);
}

SIZE DialogLayoutUI::EstimateSize(SIZE szAvailable)
{
    RecalcArea();
    return CSize(RectDx(m_rcDialog), RectDy(m_rcDialog));
}

void DialogLayoutUI::SetPos(RECT rc)
{
    m_rcItem = rc;
    RecalcArea();

    ProcessScrollbar(rc, RectDy(m_rcDialog));
    if (IsScrollYVisible())
        rc.right -= m_mgr->GetSystemMetrics().cxvscroll;
    // Determine how "scaled" the dialog is compared to the original size
    int cxDiff = RectDx(rc) - RectDx(m_rcDialog);
    int cyDiff = RectDy(rc) - RectDy(m_rcDialog);
    if (cxDiff < 0)  cxDiff = 0;
    if (cyDiff < 0)  cyDiff = 0;
    // Stretch each control
    // Controls are coupled in "groups", which determine a scaling factor.
    // A "line" indicator is used to apply the same scaling to a new group of controls.
    int nCount, cxStretch, cyStretch, cxMove, cyMove;
    for (int i = 0; i < m_aModes.GetSize(); i++)  {
        StretchMode* item = static_cast<StretchMode*>(m_aModes[i]);
        if (i == 0 || IsFlSet(item->uMode, UISTRETCH_NEWGROUP))  {
            nCount = 0;
            for (int j = i + 1; j < m_aModes.GetSize(); j++)  {
                StretchMode* pNext = static_cast<StretchMode*>(m_aModes[j]);
                if (IsFlSet(pNext->uMode, (UISTRETCH_NEWGROUP | UISTRETCH_NEWLINE)))  break;
                if (IsFlSet(pNext->uMode, (UISTRETCH_SIZE_X | UISTRETCH_SIZE_Y)))  nCount++;
            }
            if (nCount == 0)  nCount = 1;
            cxStretch = cxDiff / nCount;
            cyStretch = cyDiff / nCount;
            cxMove = 0;
            cyMove = 0;
        }
        if (IsFlSet(item->uMode, UISTRETCH_NEWLINE))  {
            cxMove = 0;
            cyMove = 0;
        }
        RECT rcPos = item->rcItem;
        ::OffsetRect(&rcPos, rc.left, rc.top - m_iScrollPos);
        if (IsFlSet(item->uMode, UISTRETCH_MOVE_X))
            ::OffsetRect(&rcPos, cxMove, 0);
        if (IsFlSet(item->uMode, UISTRETCH_MOVE_Y))
            ::OffsetRect(&rcPos, 0, cyMove);
        if (IsFlSet(item->uMode, UISTRETCH_SIZE_X))
            rcPos.right += cxStretch;
        if (IsFlSet(item->uMode, UISTRETCH_SIZE_Y))
            rcPos.bottom += cyStretch;
        if (IsFlSet(item->uMode, (UISTRETCH_SIZE_X | UISTRETCH_SIZE_Y)))  {
            cxMove += cxStretch;
            cyMove += cyStretch;
        }      
        item->ctrl->SetPos(rcPos);
    }
}

void DialogLayoutUI::RecalcArea()
{
    if (!m_bFirstResize)  return;
    // Add the remaining control to the list
    // Controls that have specific stretching needs will define them in the XML resource
    // and by calling SetStretchMode(). Other controls need to be added as well now...
    for (int it = 0; it < m_items.GetSize(); it++)  {         
        ControlUI* ctrl = m_items[it];
        bool bFound = false;
        for (int i = 0; !bFound && i < m_aModes.GetSize(); i++)  {
            if (static_cast<StretchMode*>(m_aModes[i])->ctrl == ctrl)  bFound = true;
        }
        if (!bFound)  {
            StretchMode mode;
            mode.ctrl = ctrl;
            mode.uMode = UISTRETCH_NEWGROUP;
            mode.rcItem = ctrl->GetPos();
            m_aModes.Add(&mode);
        }
    }
    // Figure out the actual size of the dialog so we can add proper scrollbars later
    CRect rcDialog(9999, 9999, 0,0);
    for (int i = 0; i < m_items.GetSize(); i++)  {
        const RECT& rcPos = m_items[i]->GetPos();
        rcDialog.Join(rcPos);
    }
    for (int j = 0; j < m_aModes.GetSize(); j++)  {
        RECT& rcPos = static_cast<StretchMode*>(m_aModes[j])->rcItem;
        ::OffsetRect(&rcPos, -rcDialog.left, -rcDialog.top);
    }
    m_rcDialog = rcDialog;
    // We're done with initialization
    m_bFirstResize = false;
}
