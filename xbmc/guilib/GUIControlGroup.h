/*!
\file GUIControlGroup.h
\brief
*/

#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <vector>

#include "GUIControlLookup.h"

/*!
 \ingroup controls
 \brief group of controls, useful for remembering last control + animating/hiding together
 */
class CGUIControlGroup : public CGUIControlLookup
{
public:
  CGUIControlGroup();
  CGUIControlGroup(int parentID, int controlID, float posX, float posY, float width, float height);
  CGUIControlGroup(const CGUIControlGroup &from);
  virtual ~CGUIControlGroup(void);
  virtual CGUIControlGroup *Clone() const { return new CGUIControlGroup(*this); };

  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Render();
  virtual void RenderEx();
  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool SendControlMessage(CGUIMessage& message);
  virtual bool HasFocus() const;
  virtual void AllocResources();
  virtual void FreeResources(bool immediately = false);
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual bool CanFocus() const;

  virtual EVENT_RESULT SendMouseEvent(const CPoint &point, const CMouseEvent &event);
  virtual void UnfocusFromPoint(const CPoint &point);

  virtual void SetInitialVisibility();

  virtual bool IsAnimating(ANIMATION_TYPE anim);
  virtual bool HasAnimation(ANIMATION_TYPE anim);
  virtual void QueueAnimation(ANIMATION_TYPE anim);
  virtual void ResetAnimation(ANIMATION_TYPE anim);
  virtual void ResetAnimations();

  int GetFocusedControlID() const;
  CGUIControl *GetFocusedControl() const;
  virtual CGUIControl *GetControl(int id, std::vector<CGUIControl*> *idCollector = nullptr);
  virtual CGUIControl *GetFirstFocusableControl(int id);

  virtual void AddControl(CGUIControl *control, int position = -1);
  bool InsertControl(CGUIControl *control, const CGUIControl *insertPoint);
  virtual bool RemoveControl(const CGUIControl *control);
  virtual void ClearAll();
  void SetDefaultControl(int id, bool always) { m_defaultControl = id; m_defaultAlways = always; };
  void SetRenderFocusedLast(bool renderLast) { m_renderFocusedLast = renderLast; };

  virtual void SaveStates(std::vector<CControlState> &states);

  virtual bool IsGroup() const { return true; };

#ifdef _DEBUG
  virtual void DumpTextureUse();
#endif
protected:
  // sub controls
  std::vector<CGUIControl *> m_children, m_idCollector;
  typedef std::vector<CGUIControl *>::iterator iControls;
  typedef std::vector<CGUIControl *>::const_iterator ciControls;
  typedef std::vector<CGUIControl *>::reverse_iterator rControls;
  typedef std::vector<CGUIControl *>::const_reverse_iterator crControls;

  int  m_defaultControl;
  bool m_defaultAlways;
  int m_focusedControl;
  bool m_renderFocusedLast;
};

