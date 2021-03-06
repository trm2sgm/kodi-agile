/*
 *      Copyright (C) 2005-2017 Team Kodi
 *      http://kodi.tv
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "DialogSelect.h"
#include "addons/kodi-addon-dev-kit/include/kodi/gui/DialogSelect.h"

#include "addons/AddonDll.h"
#include "dialogs/GUIDialogSelect.h"
#include "guilib/GUIWindowManager.h"
#include "utils/log.h"
#include "utils/Variant.h"

namespace ADDON
{
extern "C"
{

void Interface_GUIDialogSelect::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_gui->dialogSelect.Open = Open;
}

int Interface_GUIDialogSelect::Open(void* kodiBase, const char *heading, const char *entries[], unsigned int size, int selected, bool autoclose)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "ADDON::Interface_GUIDialogSelect::%s - invalid data", __FUNCTION__);
    return -1;
  }

  CGUIDialogSelect* dialog = dynamic_cast<CGUIDialogSelect*>(g_windowManager.GetWindow(WINDOW_DIALOG_SELECT));
  if (!heading || !entries || !dialog)
  {
    CLog::Log(LOGERROR, "ADDON::Interface_GUIDialogSelect::%s - invalid handler data (heading='%p', entries='%p', dialog='%p') on addon '%s'", __FUNCTION__, heading, entries, dialog, addon->ID().c_str());
    return -1;
  }

  dialog->Reset();
  dialog->SetHeading(CVariant{heading});

  for (unsigned int i = 0; i < size; ++i)
    dialog->Add(entries[i]);

  if (selected > 0)
    dialog->SetSelected(selected);
  if (autoclose > 0)
    dialog->SetAutoClose(autoclose);

  dialog->Open();
  return dialog->GetSelectedItem();
}

} /* extern "C" */
} /* namespace ADDON */
