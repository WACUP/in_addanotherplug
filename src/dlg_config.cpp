/*
  Copyright (c) 1999 - 2006 Simon Peter <dn.tlp@gmx.net>
  Copyright (c) 2002 Nikita V. Kalaganov <riven@ok.ru>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "plugin.h"
#include <commdlg.h>
#include <../../loader/loader/paths.h>
#include <../../loader/loader/utils.h>
#include "api.h"

#define STRING_TRUNC	55
#define TOOLTIP_WIDTH	300

extern In_Module	plugin;
extern Config		config;
extern FileTypes	filetypes;
extern TEmulInfo	infoEmuls[MAX_EMULATORS];

//GuiCtrlTooltip		*tooltip;

void AdjustComboBoxHeight(HWND hWndCmbBox, DWORD MaxVisItems) {
RECT rc;
  GetWindowRect(hWndCmbBox, &rc);
  rc.right -= rc.left;
  ScreenToClient(GetParent(hWndCmbBox), (POINT *) &rc);
  rc.bottom = (MaxVisItems + 2) * SendMessage(hWndCmbBox, CB_GETITEMHEIGHT, 0, 0);
  MoveWindow(hWndCmbBox, rc.left, rc.top, rc.right, rc.bottom, TRUE);
  SetWindowLongPtr(hWndCmbBox, GWL_STYLE, (GetWindowLongPtr(hWndCmbBox, GWL_STYLE) | CBS_NOINTEGRALHEIGHT) ^ CBS_NOINTEGRALHEIGHT);
}

int getComboIndexByEmul(int emul, bool duo)
{
  int idx = 0;
  for (int i = 0; i < MAX_EMULATORS; i++)
  {
    if (infoEmuls[i].emul == emul && (!duo || infoEmuls[i].s_multi))
      return idx;
    if (!duo || infoEmuls[i].s_multi)
      idx++;
  }
  return 0; // by default
}

t_output getEmulByComboIndex(int combo, bool duo)
{
  int idx = 0;
  for (int i = 0; i < MAX_EMULATORS; i++)
  {
    if (idx == combo && (!duo || infoEmuls[i].s_multi))
      return infoEmuls[i].emul;
    if (!duo || infoEmuls[i].s_multi)
      idx++;
  }
  return emunone; // by default
}

void GuiDlgConfig::open(HWND parent)
{
  config.get(&next);

  SetWindowLongPtr(parent, GWLP_USERDATA, (LONG_PTR)this);
  DlgProc_Wrapper(parent, WM_INITDIALOG, 0, 0);
}

BOOL APIENTRY GuiDlgConfig::DlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
#ifdef DEBUG
  //printf("GuiDlgConfig::DlgProc(): Message 0x%08X received. wParam = 0x%08X, lParam = 0x%08X\n", message, wParam, lParam);
#endif

  switch (message)
    {
    case WM_INITDIALOG:
    {
      TCITEM tci = { 0 };
      DarkModeSetup(hwndDlg);

      // enable tooltips
      /*tooltip = new GuiCtrlTooltip(hwndDlg);

      // enable tooltip trigger
      tooltip->trigger(GetDlgItem(hwndDlg,IDC_TOOLTIPS));*/

      // init tab control
      tci.mask = TCIF_TEXT;

      // TODO localise
      tci.pszText = TEXT("Playback Options");
      SendDlgItemMessage(hwndDlg,IDC_TABBED_PREFS_TAB,TCM_INSERTITEM,0,(LPARAM)&tci);
		
      tci.pszText = TEXT("Supported Formats");
      SendDlgItemMessage(hwndDlg,IDC_TABBED_PREFS_TAB,TCM_INSERTITEM,1,(LPARAM)&tci);
		
      // display new tab window
      HWND hTab = GetDlgItem(hwndDlg,IDC_TABBED_PREFS_TAB);

      RECT r = { 0 };
      GetClientRect(hTab, &r);
      TabCtrl_AdjustRect(hTab, 0, &r);
      for (int i = 0; i < 2; i++)
      {
        HWND tab_hwnd = CreateDialogParam(plugin.hDllInstance, MAKEINTRESOURCEW((!i ?
                                          IDD_CFG_OUTPUT : IDD_CFG_FORMATS)), hwndDlg,
                                          (DLGPROC)(!i ? OutputTabDlgProc_Wrapper :
                                          FormatsTabDlgProc_Wrapper), (LPARAM)this);
		if (IsWindow(tab_hwnd))
        {
          UXThemeFunc((WPARAM)tab_hwnd);
		  // make sure that we're creating the window before the tab control
		  // otherwise the tab order can be a bit odd & skinned prefs fails!
		  SetWindowPos(tab_hwnd, HWND_TOP, r.left, r.top,
					   (r.right - r.left), (r.bottom - r.top),
					   SWP_SHOWWINDOW | SWP_NOACTIVATE);
		  SetWindowLongPtr(tab_hwnd, GWL_ID, 10000 + i);
		}
      }

      // set default tab index
      NMHDR nmh = { hTab, IDC_TABBED_PREFS_TAB, TCN_SELCHANGE };
      SendMessage(nmh.hwndFrom, TCM_SETCURSEL, next.lastprefstab, 0);
      SendMessage(hwndDlg, WM_NOTIFY, IDC_TABBED_PREFS_TAB, (LPARAM)&nmh);
    }
    case WM_NOTIFY:
    {
	  if (LOWORD(wParam) == IDC_TABBED_PREFS_TAB)
      {
        if (((LPNMHDR)lParam)->code == TCN_SELCHANGE)
        {
          next.lastprefstab = !!TabCtrl_GetCurSel(((LPNMHDR)lParam)->hwndFrom);

          for (size_t i = 0; i < 2; i++)
          {
            HWND tab_hwnd = GetDlgItem(hwndDlg, 10000 + i);
            if (IsWindow(tab_hwnd))
	{
              const bool show_tab = (!!i == next.lastprefstab);
              ShowWindow(tab_hwnd, (show_tab ? SW_SHOWNA : SW_HIDE));
            }
          }
        }
      }
      return TRUE;
	}
    case (WM_APP + 374):
	{
	  RECT r = { 0 };
	  HWND hTab = GetDlgItem(hwndDlg,IDC_TABBED_PREFS_TAB);
	  GetClientRect(hTab, &r);
	  TabCtrl_AdjustRect(hTab, 0, &r);

      for (int i = 0; i < 2; i++)
	  {
	    SetWindowPos(GetDlgItem(hwndDlg, 10000 + i), NULL, r.left,
                     r.top, (r.right - r.left), (r.bottom - r.top),
					 SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE);
	  }
	  break;
	}
    case WM_DESTROY:
    {
      // make sure the child dialogs are closed
      // so any changes from them will be saved
      DestroyControl(hwndDlg, 10000);
      DestroyControl(hwndDlg, 10001);

      config.set(&next);

      config.save();

      const wchar_t* ignore_list = filetypes.get_ignore_list();
      config.set_ignored(ignore_list);

      SetFileExtensions(ignore_list);
    }
  }
	  return FALSE;
	}

int CALLBACK APIENTRY BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	if (uMsg == BFFM_INITIALIZED)
	{
		DarkModeSetup(hwnd);

		SendMessage(hwnd, BFFM_SETSELECTIONW, 1, lpData);
	}
	return 0;
}

BOOL APIENTRY GuiDlgConfig::OutputTabDlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
#ifdef DEBUG
  //printf("GuiDlgConfig::OutputTabDlgProc(): Message 0x%08X received.\n",message);
#endif
  int i;

  switch (message)
    {
    case WM_INITDIALOG: {
      DarkModeSetup(hwndDlg);
#if 0
      // add tooltips
      tooltip->add(GetDlgItem(hwndDlg,IDC_FREQ1),      "freq1",      "Set 11 kHz frequency.  Be aware that some notes will be the wrong pitch if this rate is used.");
      tooltip->add(GetDlgItem(hwndDlg,IDC_FREQ2),      "freq2",      "Set 22 kHz frequency.  Be aware that some notes will be the wrong pitch if this rate is used.");
      tooltip->add(GetDlgItem(hwndDlg,IDC_FREQ3),      "freq3",      "Set 44 kHz frequency.  Be aware that some notes will be the wrong pitch if this rate is used.");
      tooltip->add(GetDlgItem(hwndDlg,IDC_FREQ4),      "freq4",      "Set 48 kHz frequency.  Be aware that some notes will be the wrong pitch if this rate is used.");
      tooltip->add(GetDlgItem(hwndDlg,IDC_FREQ5),      "freq5",      "Set 49.7 kHz sampling rate.  This is the rate that the original OPL chip used and provides the most accurate playback.");
      tooltip->add(GetDlgItem(hwndDlg,IDC_FREQC),      "freqc",      "Set custom frequency (in Hz).");
      tooltip->add(GetDlgItem(hwndDlg,IDC_FREQC_VALUE),"freqc_value","Specify custom frequency value (in Hz).");
      tooltip->add(GetDlgItem(hwndDlg,IDC_QUALITY8),   "quality8",   "Set 8-Bits quality.");
      tooltip->add(GetDlgItem(hwndDlg,IDC_QUALITY16),  "quality16",  "Set 16-Bits quality.");
      tooltip->add(GetDlgItem(hwndDlg,IDC_MONO),       "mono",       "Set mono output.");
      tooltip->add(GetDlgItem(hwndDlg,IDC_STEREO),     "stereo",     "Set stereo output.");
      tooltip->add(GetDlgItem(hwndDlg,IDC_SURROUND),   "surround",   "Set stereo output with a harmonic chorus effect.");
      tooltip->add(GetDlgItem(hwndDlg,IDC_DIRECTORY),  "directory",  "Select output directory for Disk Writer.");
      tooltip->add(GetDlgItem(hwndDlg,IDC_TESTLOOP),"autoend" ,"Enable song-end auto-detection:\r\nIf disabled, the song will loop endlessly, and Winamp won't advance in the playlist.");
      tooltip->add(GetDlgItem(hwndDlg,IDC_SUBSEQ),"subseq" ,"Play all subsongs in the file:\r\nIf enabled, the player will switch to the next available subsong on the song end. This option requires song-end auto-detection to be enabled.");
      tooltip->add(GetDlgItem(hwndDlg,IDC_STDTIMER),"stdtimer","Use actual replay speed for Disk Writer output:\r\nDisable this for full speed disk writing. Never disable this if you also disabled song-end auto-detection!");
      tooltip->add(GetDlgItem(hwndDlg,IDC_DATABASE),"database","Set path to Database file to be used for replay information.");
      tooltip->add(GetDlgItem(hwndDlg,IDC_USEDB),"usedb","If unchecked, the Database will be disabled.");
#endif

      // set "output"
      if (next.useoutput == disk)
        CheckDlgButton(hwndDlg, IDC_OUTDISK, BST_CHECKED);

      // fill comboboxes
      for (i = 0; i < MAX_EMULATORS; i++) {
        SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)infoEmuls[i].name);
        if (infoEmuls[i].s_multi)
          SendDlgItemMessage(hwndDlg, IDC_COMBO2, CB_ADDSTRING, 0, (LPARAM)infoEmuls[i].name);
      }

      // adjust combobox height or dropdown lists won't be visible at all
      AdjustComboBoxHeight(GetDlgItem(hwndDlg, IDC_COMBO1), 5);
      AdjustComboBoxHeight(GetDlgItem(hwndDlg, IDC_COMBO2), 5);
      i = getComboIndexByEmul(next.useoutput, false);
      // check range
      if ((i < 0) || (i >= MAX_EMULATORS)) { i = 0; }
      SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_SETCURSEL, i, 0);
      i = getComboIndexByEmul(next.useoutput_alt, true);
      // check range
      if ((i < 0) || (i >= MAX_EMULATORS)) { i = 0; }
      SendDlgItemMessage(hwndDlg, IDC_COMBO2, CB_SETCURSEL, i, 0);

      if (next.useoutput_alt != emunone)
        CheckDlgButton(hwndDlg, IDC_ALTSYNTH, BST_CHECKED);

      // set "frequency"
      if (next.replayfreq == 11025)
        CheckRadioButton(hwndDlg,IDC_FREQ1,IDC_FREQC,IDC_FREQ1);
      else if (next.replayfreq == 22050)
        CheckRadioButton(hwndDlg,IDC_FREQ1,IDC_FREQC,IDC_FREQ2);
      else if (next.replayfreq == 44100)
        CheckRadioButton(hwndDlg,IDC_FREQ1,IDC_FREQC,IDC_FREQ3);
      else if (next.replayfreq == 48000)
        CheckRadioButton(hwndDlg,IDC_FREQ1,IDC_FREQC,IDC_FREQ4);
      else if (next.replayfreq == 49716)
        CheckRadioButton(hwndDlg,IDC_FREQ1,IDC_FREQC,IDC_FREQ5);
      else
	{
	  CheckRadioButton(hwndDlg,IDC_FREQ1,IDC_FREQC,IDC_FREQC);
	  SetDlgItemInt(hwndDlg,IDC_FREQC_VALUE,next.replayfreq,FALSE);
	}

      // set "resolution"
      if (next.use16bit)
	CheckRadioButton(hwndDlg,IDC_QUALITY8,IDC_QUALITY16,IDC_QUALITY16);
      else
	CheckRadioButton(hwndDlg,IDC_QUALITY8,IDC_QUALITY16,IDC_QUALITY8);

      // set "channels"
      if (next.harmonic)
        CheckRadioButton(hwndDlg,IDC_MONO,IDC_SURROUND,IDC_SURROUND);
      else if (next.stereo)
        CheckRadioButton(hwndDlg,IDC_MONO,IDC_SURROUND,IDC_STEREO);
      else
        CheckRadioButton(hwndDlg,IDC_MONO,IDC_SURROUND,IDC_MONO);

      // set "directory"
      SetDlgItemText(hwndDlg,IDC_DIRECTORY, next.diskdir.c_str());

      syncControlStates(hwndDlg);

      // set checkboxes
      if (next.testloop)
	    CheckDlgButton(hwndDlg,IDC_TESTLOOP,BST_CHECKED);
      if (next.subseq)
	    CheckDlgButton(hwndDlg,IDC_SUBSEQ,BST_CHECKED);
      if (next.stdtimer)
	    CheckDlgButton(hwndDlg,IDC_STDTIMER,BST_CHECKED);
      if (next.usedb)
	    CheckDlgButton(hwndDlg,IDC_USEDB,BST_CHECKED);

	  EnableWindow(GetDlgItem(hwndDlg, IDC_SUBSEQ), next.testloop);

      // set "database"
      SetDlgItemText(hwndDlg,IDC_DATABASE,next.db_file.c_str());

      // move tab content on top
      SetWindowPos(hwndDlg,HWND_TOP,3,22,0,0,SWP_NOSIZE);

      break;
    }
    case WM_DESTROY:
	{
      // check "frequency"
      if (IsDlgButtonChecked(hwndDlg,IDC_FREQ1) == BST_CHECKED)
        next.replayfreq = 11025;
      else if (IsDlgButtonChecked(hwndDlg,IDC_FREQ2) == BST_CHECKED)
        next.replayfreq = 22050;
      else if (IsDlgButtonChecked(hwndDlg,IDC_FREQ3) == BST_CHECKED)
        next.replayfreq = 44100;
      else if (IsDlgButtonChecked(hwndDlg,IDC_FREQ4) == BST_CHECKED)
        next.replayfreq = 48000;
      else if (IsDlgButtonChecked(hwndDlg,IDC_FREQ5) == BST_CHECKED)
        next.replayfreq = 49716;
      else
        next.replayfreq = GetDlgItemInt(hwndDlg,IDC_FREQC_VALUE,NULL,FALSE);

      // check "resolution"
      if (IsDlgButtonChecked(hwndDlg,IDC_QUALITY16) == BST_CHECKED)
        next.use16bit = true;
      else
        next.use16bit = false;

      // check "channels"
      if (IsDlgButtonChecked(hwndDlg,IDC_SURROUND) == BST_CHECKED) {
        next.stereo = true;
        next.harmonic = true;
      } else if (IsDlgButtonChecked(hwndDlg,IDC_STEREO) == BST_CHECKED) {
        next.stereo = true;
        next.harmonic = false;
      } else {
        next.stereo = false;
        next.harmonic = false;
      }

      // check "emulator"
      i = SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_GETCURSEL, 0, 0);
      i = max(i, 0);
      next.useoutput = getEmulByComboIndex(i, false);
      i = SendDlgItemMessage(hwndDlg, IDC_COMBO2, CB_GETCURSEL, 0, 0);
      i = max(i, 0);
      next.useoutput_alt = getEmulByComboIndex(i, true);

      if (IsDlgButtonChecked(hwndDlg, IDC_OUTDISK) == BST_CHECKED)
        next.useoutput = disk;

      // check secondary emulator
      if (IsDlgButtonChecked(hwndDlg,IDC_ALTSYNTH) != BST_CHECKED) {
        next.useoutput_alt = emunone;
      }

      // check checkboxes :)
      next.testloop = (IsDlgButtonChecked(hwndDlg,IDC_TESTLOOP) == BST_CHECKED);
      next.subseq = (IsDlgButtonChecked(hwndDlg,IDC_SUBSEQ) == BST_CHECKED);
      next.stdtimer = (IsDlgButtonChecked(hwndDlg,IDC_STDTIMER) == BST_CHECKED);
      next.usedb = (IsDlgButtonChecked(hwndDlg,IDC_USEDB) == BST_CHECKED);
      break;
    }
    case WM_COMMAND:
    {
      switch (LOWORD(wParam))
	{
	case IDC_DIRECTORY:
        {
	  // display folder selection dialog
	      wchar_t shd[_MAX_PATH] = { 0 };

          BROWSEINFO bi = { 0 };
	  bi.hwndOwner = hwndDlg;
	  bi.pszDisplayName = shd;
	      bi.lpszTitle = TEXT("Select output path for Disk Writer:");
	      bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
          bi.lpfn = BrowseCallbackProc;
          bi.lParam = (LONG_PTR)next.diskdir.c_str();

          LPITEMIDLIST pidl = BrowseForFolder(&bi);
	      if (pidl)
	      {
            next.diskdir = PathFromPIDL(pidl, shd, ARRAYSIZE(shd), true);
            WritePrivateProfileString(L"in_adlib", L"diskdir", next.diskdir.c_str(), GetPaths()->winamp_ini_file);
            SetDlgItemText(hwndDlg, IDC_DIRECTORY, next.diskdir.c_str());
		}
          break;
	    }
    case IDC_COMBO1:
    case IDC_COMBO2:
    case IDC_ALTSYNTH:
    case IDC_OUTDISK:
        {
      syncControlStates(hwndDlg);
      break;
    }
	    case IDC_DATABASE:
	{
          OPENFILENAME ofn = { 0 };

	  ofn.lStructSize = sizeof(ofn);
	  ofn.hwndOwner = hwndDlg;
	      ofn.lpstrFilter = TEXT("AdPlug Database Files (*.DB)\0*.db\0\0");
	      ofn.lpstrFile = (LPWSTR)plugin.memmgr->sysMalloc(_MAX_PATH * sizeof(wchar_t));
	      wcscpy(ofn.lpstrFile, next.db_file.c_str());
	  ofn.nMaxFile = _MAX_PATH;
	      ofn.lpstrTitle = TEXT("Select Database File");
	  ofn.Flags = OFN_FILEMUSTEXIST;

	      if (GetFileName(&ofn))
	    {
	        next.db_file = ofn.lpstrFile;
            WritePrivateProfileString(L"in_adlib", L"database", next.db_file.c_str(), GetPaths()->winamp_ini_file);

	        SetDlgItemText(hwndDlg,IDC_DATABASE, next.db_file.c_str());
		}
          plugin.memmgr->sysFree(ofn.lpstrFile);
          break;
	    }
	case IDC_TESTLOOP:
        {
	  bool bTestloop = IsDlgButtonChecked(hwndDlg, IDC_TESTLOOP) == BST_CHECKED;
	  EnableWindow(GetDlgItem(hwndDlg, IDC_SUBSEQ), bTestloop);
	  break;
	}
    }
    }
  }

  return FALSE;
}

BOOL APIENTRY GuiDlgConfig::FormatsTabDlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
#ifdef DEBUG
  //printf("GuiDlgConfig::FormatsTabDlgProc(): Message 0x%08X received.\n",message);
#endif
  int i;

  switch (message)
    {
    case WM_INITDIALOG: {
      DarkModeSetup(hwndDlg);
#if 0
      // add tooltips
      tooltip->add(GetDlgItem(hwndDlg,IDC_FORMATLIST),"formatlist","All supported formats are listed here:\r\nDeselected formats will be ignored by AdPlug to make room for other plugins to play these.");
      tooltip->add(GetDlgItem(hwndDlg,IDC_FTSELALL),  "ftselall",  "Select all formats.");
      tooltip->add(GetDlgItem(hwndDlg,IDC_FTDESELALL),  "ftdeselall",  "Deselect all formats.");
#endif

      // need to make sure that we've got a list
      // of file types loaded which may not have
      // yet happened if nothing has played, etc
      extern void __cdecl GetFileExtensions(void);
      GetFileExtensions();

      // fill listbox
      for (i=0;i<filetypes.get_size();i++)
	{
	  SendDlgItemMessage(hwndDlg,IDC_FORMATLIST,LB_ADDSTRING,0,(LPARAM)filetypes.get_name(i));
	  SendDlgItemMessage(hwndDlg,IDC_FORMATLIST,LB_SETSEL,(WPARAM)!filetypes.get_ignore(i),i);
	}

      // move tab content on top
      SetWindowPos(hwndDlg,HWND_TOP,3,22,0,0,SWP_NOSIZE);

      break;
    }

    case WM_DESTROY:
    {
      // read listbox
      for (i=0;i<filetypes.get_size();i++)
	filetypes.set_ignore(i,SendDlgItemMessage(hwndDlg,IDC_FORMATLIST,LB_GETSEL,i,0) ? false : true);

      break;
    }

    case WM_COMMAND:
      switch (LOWORD(wParam))
	{
	case IDC_FTSELALL:
	  SendDlgItemMessage(hwndDlg,IDC_FORMATLIST,LB_SETSEL,TRUE,-1);
        break;

	case IDC_FTDESELALL:
	  SendDlgItemMessage(hwndDlg,IDC_FORMATLIST,LB_SETSEL,FALSE,-1);
        break;
	}
    }

  return FALSE;
}

BOOL APIENTRY GuiDlgConfig::DlgProc_Wrapper(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  GuiDlgConfig *the = (GuiDlgConfig *)GetWindowLongPtr(hwndDlg,GWLP_USERDATA);

  if (!the)
    return FALSE;

  return the->DlgProc(hwndDlg,message,wParam,lParam);
}

BOOL APIENTRY GuiDlgConfig::OutputTabDlgProc_Wrapper(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  if (message == WM_INITDIALOG)
    SetWindowLongPtr(hwndDlg,GWLP_USERDATA,(LONG_PTR)lParam);

  GuiDlgConfig *the = (GuiDlgConfig *)GetWindowLongPtr(hwndDlg,GWLP_USERDATA);

  if (!the)
    return FALSE;

  return the->OutputTabDlgProc(hwndDlg,message,wParam,lParam);
}

BOOL APIENTRY GuiDlgConfig::FormatsTabDlgProc_Wrapper(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  if (message == WM_INITDIALOG)
    SetWindowLongPtr(hwndDlg,GWLP_USERDATA,(LONG_PTR)lParam);

  GuiDlgConfig *the = (GuiDlgConfig *)GetWindowLongPtr(hwndDlg,GWLP_USERDATA);

  if (!the)
    return FALSE;

  return the->FormatsTabDlgProc(hwndDlg,message,wParam,lParam);
}

void GuiDlgConfig::syncControlStates(HWND hwndDlg) const
{
  int i = SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_GETCURSEL, 0, 0);
  if (i == CB_ERR) return;

  bool bEmuMulti = infoEmuls[i].s_multi;
  bool bEmuMono = infoEmuls[i].s_mono;
  bool bEmu8bit = infoEmuls[i].s_8bit;
  bool bOutDisk = IsDlgButtonChecked(hwndDlg, IDC_OUTDISK) == BST_CHECKED;
  if (/*(i >= 0) && */(i < MAX_EMULATORS)) {
    SetDlgItemText(hwndDlg, IDC_EMUINFO, infoEmuls[i].description);
  }
  bool bAltSynth = IsDlgButtonChecked(hwndDlg, IDC_ALTSYNTH) == BST_CHECKED;
  //bool bIsStereo = IsDlgButtonChecked(hwndDlg, IDC_STEREO) == BST_CHECKED;
  bool bIsSurround = IsDlgButtonChecked(hwndDlg, IDC_SURROUND) == BST_CHECKED;
  bool bWasSurroundEnabled = IsWindowEnabled(GetDlgItem(hwndDlg, IDC_SURROUND)) == TRUE;

  // Figure out which controls we will enable and disable
  bool enMono = !bOutDisk && !bAltSynth && bEmuMono;
  bool enStereo = !bOutDisk && !bAltSynth;
  bool enSurround = !bOutDisk && (bAltSynth || bEmuMulti);

  EnableWindow(GetDlgItem(hwndDlg, IDC_COMBO1), !bOutDisk);
  EnableWindow(GetDlgItem(hwndDlg, IDC_ALTSYNTH), !bOutDisk);
  EnableWindow(GetDlgItem(hwndDlg, IDC_MONO), enMono);
  EnableWindow(GetDlgItem(hwndDlg, IDC_STEREO), enStereo);
  EnableWindow(GetDlgItem(hwndDlg, IDC_SURROUND), enSurround);

  // Switch the alternate synth choices on and off depending on the checkbox
  EnableWindow(GetDlgItem(hwndDlg, IDC_COMBO2), bAltSynth && !bOutDisk);

  if (bIsSurround && !enSurround) {
    // Surround was selected but it's been disabled, move it to stereo
    CheckRadioButton(hwndDlg, IDC_MONO, IDC_SURROUND, IDC_STEREO);
  } else if (enSurround && !bWasSurroundEnabled) {
    // Surround has just become enabled so select it
    CheckRadioButton(hwndDlg, IDC_MONO, IDC_SURROUND, IDC_SURROUND);
  } else if (!enMono && !enStereo && enSurround) {
    // Surround is the only option, select it
    CheckRadioButton(hwndDlg, IDC_MONO, IDC_SURROUND, IDC_SURROUND);
  }
  EnableWindow(GetDlgItem(hwndDlg, IDC_QUALITY8), bEmu8bit);
  if (!bEmu8bit)
    CheckRadioButton(hwndDlg, IDC_QUALITY8, IDC_QUALITY16, IDC_QUALITY16);
  return;
}
