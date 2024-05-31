/*
  Copyright (c) 1999 - 2007 Simon Peter <dn.tlp@gmx.net>
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
#include"api.h"

#include <../../loader/loader/paths.h>
#define WA_UTILS_SIMPLE
#include <../../loader/loader/utils.h>
#include <../../sdk/nu/autocharfn.h>

#include <../../wacup_version.h>

#ifdef DEBUG
#	include "debug.h"
#endif

extern In_Module	plugin;

static prefsDlgRecW* preferences;

CRITICAL_SECTION g_ft_cs = { 0 };
Config			config;
FileTypes		filetypes;
MyPlayer		my_player;
GuiDlgConfig	dlg_config;
GuiDlgInfo		dlg_info;

TEmulInfo infoEmuls[MAX_EMULATORS] = {
  {emunk, TEXT("Nuked OPL3 (Nuke.YKT, 2017)"),
    TEXT("Nuked OPL3 emulator by Alexey Khokholov (Nuke.YKT). "
         "Set output frequency to 49716 Hz for best quality."),
    true, false, false},
  {emuwo, TEXT("WoodyOPL (DOSBox, 2016)"),
    TEXT("This is the most accurate OPL emulator, especially "
         "when the frequency is set to 49716 Hz."),
    true, true, true},
  {emuts, TEXT("Tatsuyuki Satoh 0.72 (MAME, 2003)"),
    TEXT("While not perfect, this synth comes very close "
         "and for many years was the best there was."),
    true, true, true},
  {emuks, TEXT("Ken Silverman (2001)"),
    TEXT("While inaccurate by today's standards, this emulator was "
         "one of the earliest open-source OPL synths available."),
    false, true, true},
};

void read_config(void)
{
  static bool config_read;
  if (!config_read)
  {
    config_read = true;

    t_config_data	cfg;
    config.load();
    config.get(&cfg);

    plugin.UsesOutputPlug = config.useoutputplug;
  }
}

/*typedef struct
{
    UINT ctrl_id, dlg_id, name;
    DLGPROC proc;
} TABDESC;

#define IDC_CONFIG_TAB1 10000
#define IDC_CONFIG_TAB2 10001

static TABDESC tabs[] =
{
    {IDC_CONFIG_TAB1, DlgCfgPlayback, IDD_CFG_OUTPUT, CfgDlgPlaybackProc},
    {IDC_CONFIG_TAB2, DlgCfgMuting, IDD_CFG_FORMATS, CfgDlgMutingProc},
};*/

// Dialogue box callback function
INT_PTR CALLBACK ConfigDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  if (message == WM_INITDIALOG)
  {
    // make sure that we're all initialised as we can be
    // accessed via a few different ways outside of just
    // having our config(..) called so we don't crash...
    read_config();

    dlg_config.open(hwndDlg);
    return TRUE;
  }
  return dlg_config.DlgProc_Wrapper(hwndDlg, message, wParam, lParam);
}

int wa2_Init(void)
{
  // TODO localise
  wchar_t description[256] = { 0 };
  // "AdPlug v" ADPLUG_VERSION "/v" PLUGIN_VERSION
  StringCchPrintfW(description, ARRAYSIZE(description), L"AdPlug (AdLib) Player v%hs", PLUGIN_VERSION);
  plugin.description = (char*)plugin.memmgr->sysDupStr(description);

  preferences = (prefsDlgRecW*)GlobalAlloc(GPTR, sizeof(prefsDlgRecW));
  if (preferences)
  {
      // TODO localise
      preferences->hInst = GetModuleHandle(GetPaths()->wacup_core_dll);
      preferences->dlgID = IDD_TABBED_PREFS_DIALOG;
      preferences->name = /*WASABI_API_LNGSTRINGW_DUP(IDS_VGM)*/plugin.memmgr->sysDupStr(L"ADLIB | ADPLUG");
      preferences->proc = ConfigDialogProc;
      preferences->where = 10;
      preferences->_id = 100;
      preferences->next = (_prefsDlgRec*)0xACE;
      AddPrefsPage((WPARAM)preferences, TRUE);
  }

#ifdef DEBUG
  debug_init();
#endif

  InitializeCriticalSectionEx(&g_ft_cs, 400, CRITICAL_SECTION_NO_DEBUG_INFO);

  return IN_INIT_SUCCESS;
}

void wa2_Quit(void)
{
  /*free(plugin.FileExtensions);
  config.set_ignored(filetypes.get_ignore_list());
  config.save();*/
  DeleteCriticalSection(&g_ft_cs);
}

/*int wa2_IsOurFile(const in_char *file)
{
  AutoCharFn fn(file);
  if (filetypes.grata(fn) != -1)
  {
    CSilentopl silent;

    // try to make adplug player
    CPlayer* p = CAdPlug::factory(std::string(fn), &silent);

    if (p)
	{
	  delete p;
	  return 1;
	}
  }
  return 0;
}*/

void wa2_GetFileInfo(const in_char *file, in_char *title, int *length_in_ms)
{
  AutoCharFn fn(file);
  const char *my_file;

  // current file ?
  if ((!file) || (!(*file)))
    my_file = my_player.get_file();
  else
  {
    // our file ?
    /*if (!wa2_IsOurFile(file))
	  return;*/

    my_file = fn;
  }

  // set default info
  if (title)
  {
    const char* fn = strrchr(my_file, '\\');
    if (fn && *fn)
    {
      ConvertANSI(fn + 1, CP_ACP, title, GETFILEINFO_TITLE_LENGTH);
    }
  }

  if (length_in_ms)
    *length_in_ms = 0;

  // try to get real info
  CSilentopl silent;

  CPlayer* p = CAdPlug::factory(my_file, &silent);

  if (p)
  {
      // TODO
      if (title)
      {
        // Wraithverge: modified this code to show the "Author" + "Title"
        // in the "Song Title" window, instead of just "Title", but only
        // if both Tag-data exists.
        if (!p->getauthor().empty() && !p->gettitle().empty()) {
          StringCchPrintf(title, GETFILEINFO_TITLE_LENGTH, L"%hs - %hs",
                          p->getauthor().c_str(), p->gettitle().c_str());
        }
        else if (!p->gettitle().empty())
        {
          ConvertANSI(p->gettitle().c_str(), CP_ACP, title, GETFILEINFO_TITLE_LENGTH);
        }
      }

    if (length_in_ms)
      *length_in_ms = my_player.get_length(my_file,(file ? my_player.get_subsong() : DFL_SUBSONG));

    delete p;
  }
}

int wa2_Play(const in_char *file)
{
  read_config();

  AutoCharFn fn(file);
  return my_player.play(fn);
}

void wa2_Pause(void)
{
  my_player.pause();
}

void wa2_UnPause(void)
{
  my_player.unpause();
}

int wa2_IsPaused(void)
{
  return my_player.is_paused();
}

void wa2_Stop(void)
{
  my_player.stop();
}

int wa2_GetLength(void)
{
  return my_player.get_length();
}

int wa2_GetOutputTime(void)
{
  return my_player.get_position();
}

void wa2_SetOutputTime(int time_in_ms)
{
  my_player.seek(time_in_ms);
}

void wa2_SetVolume(int volume)
{
  my_player.set_volume(volume);
}

void wa2_SetPan(int pan)
{
  my_player.set_panning(pan);
}

/*void wa2_EqSet(int on, char data[10], int preamp)
{
  return;
}*/

void wa2_About(HWND hwndParent)
{
  // TODO localise
  wchar_t message[2048] = { 0 };
  StringCchPrintf(message, ARRAYSIZE(message), L"%s (%s)\n\nCopyright "
                  L"© Simon Peter (1999-2010)\nCopyright © Nikita V. "
                  L"Kalaganov (2002)\nCopyright © Wraithverge (2010)\n"
                  L"Copyright © Stas'M (2016-2017)\n\n\nParts of the "
                  L"plug-in & AdPlug library originally\nfrom https://"
                  L"adplug.github.io(as per the LGPL)\n\nWACUP "
                  L"modifications by " WACUP_AUTHOR_STRW L" (%s)\n\nBuild "
                  L"date: %s", (LPCWSTR)plugin.description, L"beta483",
                  L"2022-" WACUP_COPYRIGHT, TEXT(__DATE__));
  AboutMessageBox(hwndParent, message, L"AdPlug (AdLib) Player");
}

void wa2_DlgConfig(HWND hwndParent)
{
  OpenPrefsPage((WPARAM)(preferences ? preferences->_id : -1));
}

int wa2_DlgInfo(const in_char *file, HWND hwndParent)
{
  AutoCharFn fn(file);
  return dlg_info.open(fn, hwndParent);
}

// return 1 if you want winamp to show it's own file info dialogue, 0 if you want to show your own (via In_Module.InfoBox)
// if returning 1, remember to implement winampGetExtendedFileInfo("formatinformation")!
extern "C" __declspec(dllexport) int winampUseUnifiedFileInfoDlg(const wchar_t* fn)
{
    // TODO
    return 0;
}

// should return a child window of 513x271 pixels (341x164 in msvc dlg units), or return NULL for no tab.
// Fill in name (a buffer of namelen characters), this is the title of the tab (defaults to "Advanced").
// filename will be valid for the life of your window. n is the tab number. This function will first be 
// called with n == 0, then n == 1 and so on until you return NULL (so you can add as many tabs as you like).
// The window you return will recieve WM_COMMAND, IDOK/IDCANCEL messages when the user clicks OK or Cancel.
// when the user edits a field which is duplicated in another pane, do a SendMessage(GetParent(hwnd),WM_USER,(WPARAM)L"fieldname",(LPARAM)L"newvalue");
// this will be broadcast to all panes (including yours) as a WM_USER.
extern "C" __declspec(dllexport) HWND winampAddUnifiedFileInfoPane(int n, const wchar_t* filename, HWND parent, wchar_t* name, size_t namelen)
{
    return NULL;
}

extern "C" __declspec(dllexport) int winampGetExtendedFileInfoW(const wchar_t *file, char * metadata, wchar_t *ret, const size_t retlen)
{
  if (ret == NULL || !retlen) return false;

  if (SameStrA(metadata, "type") ||
      SameStrA(metadata, "lossless") ||
      SameStrA(metadata, "streammetadata"))
  {
    ret[0] = L'0';
    ret[1] = L'\0';
    return true;
  }
  else if (SameStrA(metadata, "streamgenre") ||
           SameStrA(metadata, "streamtype") ||
           SameStrA(metadata, "streamurl") ||
           SameStrA(metadata, "streamname") ||
           SameStrA(metadata, "reset"))
  {
    return false;
  }
  else if (SameStrA(metadata, "family"))
  {
    read_config();

    const int index = filetypes.grata(file);
    if (index != -1)
    {
      wcsncpy(ret, filetypes.get_name(index), retlen);
      return true;
    }
    return false;
  }

  read_config();

  AutoCharFn fn(file);
  const char *my_file;

  // current file ?
  if ((!file) || (!(*file)))
    my_file = my_player.get_file();
  else
  {
    // our file ?
    /*if (!wa2_IsOurFile(file))
      return false;*/

    my_file = fn;
  }

  CSilentopl silent;

  CPlayer *p = CAdPlug::factory(my_file, &silent);
  if (!p) return false;

  // default to a blank string
  *ret = L'\0';
  bool result = false;

  if (SameStrA(metadata, "title"))
  {
    result = !p->gettitle().empty() && ((int)p->gettitle().length() < retlen);
    if (result)
    {
      ConvertANSI(p->gettitle().c_str(), CP_ACP, ret, retlen);
    }
  }
  else if (SameStrA(metadata, "artist"))
  {
    result = !p->getauthor().empty() && ((int)p->getauthor().length() < retlen);
    if (result)
    {
      ConvertANSI(p->getauthor().c_str(), CP_ACP, ret, retlen);
    }
  }
  else if (SameStrA(metadata, "comment"))
  {
    result = !p->getdesc().empty() && ((int)p->getdesc().length() < retlen);
    if (result)
    {
      ConvertANSI(p->getdesc().c_str(), CP_ACP, ret, retlen);
    }
  }
  else if (SameStrA(metadata, "formatinformation"))
  {
    result = !p->gettype().empty() && ((int)p->gettype().length() < retlen);
    if (result)
    {
      ConvertANSI(p->gettype().c_str(), CP_ACP, ret, retlen);
    }
  }
  else if (SameStrA(metadata, "length"))
  {
    /*int length_in_ms;
    if (file)
      length_in_ms = my_player.get_length(my_file, my_player.get_subsong());
    else
      length_in_ms = my_player.get_length(my_file, DFL_SUBSONG);
    sprintf(ret, "%d", length_in_ms);*/
    I2WStr(my_player.get_length(my_file, (file ? my_player.get_subsong() : DFL_SUBSONG)), ret, retlen);
    result = true;
  }

  delete p;
  return result;
}

void SetFileExtensions(const wchar_t* ignore_list)
{
  filetypes.set_ignore_list(ignore_list);

  if (plugin.FileExtensions)
  {
    plugin.memmgr->sysFree(plugin.FileExtensions);
  }
  plugin.FileExtensions = (char*)filetypes.export_filetypes((wchar_t*)plugin.memmgr->sysMalloc(4096 * sizeof(wchar_t)));
}

void __cdecl GetFileExtensions(void)
{
    static bool loaded_extensions;
    if (!loaded_extensions)
    {
        loaded_extensions = true;

        EnterCriticalSection(&g_ft_cs);

        // TODO localise
        filetypes.add(L"a2m", L"Adlib Tracker 2 Modules (*.A2M)");
        filetypes.add(L"adl", L"Coktel Vision ADL Files (*.ADL)");
        //filetypes.add(L"adl", L"Westwood ADL Files (*.ADL)");
        filetypes.add(L"amd", L"AMUSIC Modules (*.AMD)");
        filetypes.add(L"bam", L"Bob's Adlib Music Files (*.BAM)");
        filetypes.add(L"bmf", L"Easy AdLib 1.0 by The Brain (*.BMF)");
        filetypes.add(L"cff", L"BoomTracker 4 Modules (*.CFF)");
        filetypes.add(L"cmf", L"Creative Adlib Music Files (*.CMF)");
        //filetypes.add(L"cmf", L"SoundFX Macs Opera (*.CMF)");
        filetypes.add(L"d00", L"Packed EdLib Modules (*.D00)");
        filetypes.add(L"dfm", L"Digital-FM Modules (*.DFM)");
        filetypes.add(L"dmo", L"TwinTeam Modules (*.DMO)");
        //filetypes.add(L"dro", L"DOSBox Raw OPL v1.0 and v2.0 Files (*.DRO)");
        filetypes.add(L"dtm", L"DeFy Adlib Tracker Modules (*.DTM)");
        filetypes.add(L"got", L"God of Thunder Music (*.GOT)");
        filetypes.add(L"hsc", L"HSC-Tracker Modules (*.HSC)");
        filetypes.add(L"hsp", L"Packed HSC-Tracker Modules (*.HSP)");
        filetypes.add(L"hsq;sqx;sdb;agd;ha2", L"HERAD System (*.HSQ;*.SQX;*.SDB;*.AGD;*.HA2)");
        filetypes.add(L"imf;wlf;adlib", L"Apogee IMF Files (*.IMF;*.WLF;*.ADLIB)");
        filetypes.add(L"ims", L"IMPlay Song Files (*.IMS)");
        filetypes.add(L"jbm", L"JBM Adlib Music Files (*.JBM)");
        filetypes.add(L"ksm", L"Ken Silverman's Music Files (*.KSM)");
        filetypes.add(L"laa", L"LucasArts Adlib Audio Files (*.LAA)");
        filetypes.add(L"lds", L"LOUDNESS Sound System Files (*.LDS)");
        filetypes.add(L"m", L"Ultima 6 Music Files (*.M)");
        filetypes.add(L"mad", L"Mlat Adlib Tracker Modules (*.MAD)");
        filetypes.add(L"mdi", L"AdLib MIDIPlay Files (*.MDI)");
        filetypes.add(L"mid;kar", L"MIDI Audio Files (*.MID;*.KAR)");
        filetypes.add(L"mkj", L"MKJamz Audio Files (*.MKJ)");
        filetypes.add(L"msc", L"AdLib MSCplay (*.MSC)");
        filetypes.add(L"mtk", L"MPU-401 Trakker Modules (*.MTK)");
        filetypes.add(L"mus;mdy", L"AdLib MIDI Music Files (*.MUS;*.MDY)");
        filetypes.add(L"rad", L"Reality Adlib Tracker Modules (*.RAD)");
        filetypes.add(L"rac;raw", L"Raw AdLib Capture Files (*.RAC;*.RAW)");
        filetypes.add(L"rix", L"Softstar RIX OPL Music Files (*.RIX)");
        filetypes.add(L"rol", L"Adlib Visual Composer Modules (*.ROL)");
        filetypes.add(L"sa2", L"Surprise! Adlib Tracker 2 Modules (*.SA2)");
        filetypes.add(L"sat", L"Surprise! Adlib Tracker Modules (*.SAT)");
        filetypes.add(L"sci", L"Sierra Adlib Audio Files (*.SCI)");
        filetypes.add(L"sng", L"Adlib Tracker 1.0 Modules (*.SNG)");
        /*filetypes.add(L"sng", L"Faust Music Creator Modules (*.SNG)");
        filetypes.add(L"sng", L"SNGPlay Files (*.SNG)");*/
        filetypes.add(L"sop", L"Note Sequencer by sopepos (*.SOP)");
        //filetypes.add(L"vgm", L"Video Game Music (*.VGM)");
        filetypes.add(L"xad", L"eXotic Adlib Files (*.XAD)");
        filetypes.add(L"xms", L"XMS-Tracker Modules (*.XMS)");
        filetypes.add(L"xsm", L"eXtra Simple Music Files (*.XSM)");

        SetFileExtensions(config.get_ignored());

        LeaveCriticalSection(&g_ft_cs);
    }
}

In_Module plugin =
  {
    IN_VER_WACUP,
    //(char*)PLUGIN_VER,
    (char*)"wacup(in_addanotherplug.dll)",
    0,	// hMainWindow
    0,	// hDllInstance
    0,
    1,	// is_seekable
    1,	// uses output
    wa2_DlgConfig,
    wa2_About,
    wa2_Init,
    wa2_Quit,
    wa2_GetFileInfo,
    wa2_DlgInfo,
    NULL/*/wa2_IsOurFile/**/,
    wa2_Play,
    wa2_Pause,
    wa2_UnPause,
    wa2_IsPaused,
    wa2_Stop,
    wa2_GetLength,
    wa2_GetOutputTime,
    wa2_SetOutputTime,
    wa2_SetVolume,
    wa2_SetPan,
    IN_INIT_VIS_RELATED_CALLS,
    0,0,	// dsp
    IN_INIT_WACUP_EQSET_EMPTY
    NULL,	// setinfo
    0,		// out_mod,
    NULL,	// api_service
    INPUT_HAS_READ_META | //INPUT_USES_UNIFIED_ALT3 |
    INPUT_HAS_FORMAT_CONVERSION_UNICODE/* |
    INPUT_HAS_FORMAT_CONVERSION_SET_TIME_MODE*/,
    GetFileExtensions,	// loading optimisation
    IN_INIT_WACUP_END_STRUCT
  };

extern "C" __declspec(dllexport) In_Module *winampGetInModule2()
{
  return &plugin;
}

extern "C" __declspec(dllexport) int winampUninstallPlugin(HINSTANCE hDllInst, HWND hwndDlg, int param)
{
    // prompt to remove our settings with default as no (just incase)
    /*if (plugin.language->UninstallSettingsPrompt(reinterpret_cast<const wchar_t*>(plugin.description)))
    {
        SaveNativeIniString(PLUGIN_INI, _T("APE Plugin Settings"), 0, 0);
    }*/

    // as we're not hooking anything and have no settings we can support an on-the-fly uninstall action
    return IN_PLUGIN_UNINSTALL_NOW;
}

////////////////////////////////////////////////////////////////////////////////

extern "C" __declspec(dllexport) intptr_t winampGetExtendedRead_openW(const wchar_t* file, int* size, int* bps, int* nch, int* srate)
{
  MyPlayer* decoder = new MyPlayer();
  if (decoder)
  {
    AutoCharFn fn(file);
    if (decoder->open(fn, bps, nch, srate))
    {
      *size = -1; // TODO need to get number of samples, etc
      return (intptr_t)decoder;
    }
    delete decoder;
  }
  return NULL;
}

extern "C" __declspec(dllexport) intptr_t winampGetExtendedRead_getData(intptr_t handle, char* dest,
                                                                        size_t len, int* killswitch)
{
  MyPlayer* decoder = (MyPlayer*)handle;
  return (decoder ? decoder->decode(dest, len) : 0);
}

extern "C" __declspec(dllexport) int winampGetExtendedRead_setTime(intptr_t handle, int millisecs)
{
  /*MyPlayer* decoder = (MyPlayer*)handle;
  if (decoder)
  {
  }*/
  return 0;
}

extern "C" __declspec(dllexport) void winampGetExtendedRead_close(intptr_t handle)
{
  MyPlayer* decoder = (MyPlayer*)handle;
  if (decoder)
  {
    decoder->close();
    delete decoder;
  }
}
}