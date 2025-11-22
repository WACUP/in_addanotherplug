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
#include <../../loader/loader/paths.h>
#define WA_UTILS_SIMPLE
#include <../../loader/loader/utils.h>
#include <../../sdk/nu/autochar.h>
#include <../../sdk/nu/autocharfn.h>

extern In_Module plugin;

#define MSGA_WINAMP	TEXT("You must now restart WACUP after switching from Disk Writer to Emulator output mode.")
#define MSGC_DISK 	TEXT("You have selected *full speed* and *endless* Disk Writing modes. This combination of options is not recommended.")
//#define MSGC_DATABASE	TEXT("An external Database could not be loaded!")

#ifdef FOR_XMPLAY
#define MSGE_XMPLAY	"Hardware OPL2 output is not supported when this plugin is used within XMPlay. An emulator must be used for output, instead."
#endif

#define DFL_EMU			    emunk
#define DFL_REPLAYFREQ		49716
#define DFL_HARMONIC		false
#define DFL_USE16BIT		true
#define DFL_STEREO		    true
#define DFL_USEOUTPUT		DFL_EMU
#define DFL_USEOUTPUT_ALT	emunone
#define DFL_TESTLOOP		true
#define DFL_SUBSEQ		    true
#define DFL_STDTIMER		true
#define DFL_DISKDIR		    TEXT(".\\")
#define DFL_IGNORED	    	TEXT("")
#define DFL_DBFILE		    TEXT("adplug.db")
#define DFL_USEDB		    true
#define DFL_LASTPREFSTAB    false

CAdPlugDatabase *Config::mydb = 0;

Config::Config(void)
{
#if 0   // dro change
  useoutputplug = true;
#endif
}

void Config::load(void)
{
  wchar_t bufstr[MAX_PATH+1] = {0}, dbfile[MAX_PATH] = {0}, curdir[MAX_PATH + 1] = {0};

  // get default path to .ini file
  /*
  GetModuleFileName(NULL,bufstr,MAX_PATH);

  _strlwr(strrchr(bufstr,'\\'));

  fname.assign(bufstr);
  fname.resize(fname.size() - 3);
  fname.append("ini");
  */
  /*fname.assign(std::getenv("APPDATA"));
  if (fname.empty())
    fname.assign(std::getenv("ALLUSERSPROFILE"));
  fname.append("\\Winamp");
  if (!PathIsDirectoryA(fname.c_str()))
    CreateDirectoryA(fname.c_str(), NULL);
  fname.append("\\winamp.ini");*/

  const winamp_paths *paths = GetPaths();
  const wchar_t *fname = paths->winamp_ini_file;

  // load configuration from .ini file
  int bufval;

  bufval = GetPrivateProfileInt(L"in_adlib",L"replayfreq",DFL_REPLAYFREQ,fname);
  if (bufval != -1)
    next.replayfreq = bufval;

  bufval = GetPrivateProfileInt(L"in_adlib",L"harmonic",DFL_HARMONIC,fname);
  if (bufval != -1)
    next.harmonic = bufval ? true : false;

  bufval = GetPrivateProfileInt(L"in_adlib",L"use16bit",DFL_USE16BIT,fname);
  if (bufval != -1)
    next.use16bit = bufval ? true : false;

  bufval = GetPrivateProfileInt(L"in_adlib",L"stereo",DFL_STEREO,fname);
  if (bufval != -1)
    next.stereo = bufval ? true : false;

  bufval = GetPrivateProfileInt(L"in_adlib",L"useoutput",DFL_USEOUTPUT,fname);
  if (bufval != -1)
    next.useoutput = (enum t_output)bufval;

  bufval = GetPrivateProfileInt(L"in_adlib",L"useoutput_alt",DFL_USEOUTPUT_ALT,fname);
  if (bufval != -1)
    next.useoutput_alt = (enum t_output)bufval;

  LPCWSTR settings_dir = paths->settings_dir;
  CombinePath(curdir, settings_dir, L"");
  /*strcpy(curdir, std::getenv("USERPROFILE"));
  if (curdir == "")
    strcpy(curdir, std::getenv("ALLUSERSPROFILE"));
  if (curdir == "")
    GetCurrentDirectoryA(MAX_PATH, curdir);*/
  if (curdir[0])
  {
    wcscat(curdir, L"\\");
    GetPrivateProfileString(L"in_adlib", L"diskdir", curdir, bufstr, MAX_PATH, fname);
    if (IsDirectoryPath(bufstr))
      next.diskdir = bufstr;
    else
      next.diskdir = curdir;
  }
  else
    next.diskdir = DFL_DISKDIR;

  bufval = GetPrivateProfileInt(L"in_adlib",L"testloop",DFL_TESTLOOP,fname);
  if (bufval != -1)
    next.testloop = bufval ? true : false;

  bufval = GetPrivateProfileInt(L"in_adlib",L"subseq",DFL_SUBSEQ,fname);
  if (bufval != -1)
    next.subseq = bufval ? true : false;

  bufval = GetPrivateProfileInt(L"in_adlib",L"stdtimer",DFL_STDTIMER,fname);
  if (bufval != -1)
    next.stdtimer = bufval ? true : false;

  GetPrivateProfileString(L"in_adlib",L"ignored",DFL_IGNORED,bufstr,MAX_PATH,fname);
  next.ignored = bufstr;

  // Build database default path (in winamp plugin directory)
  //GetModuleFileNameA(GetModuleHandleA("in_adlib"), dbfile, MAX_PATH);
  //strcpy(strrchr(dbfile, '\\') + 1, DFL_DBFILE);
  CombinePath(dbfile, settings_dir, DFL_DBFILE);

  GetPrivateProfileString(L"in_adlib",L"database",dbfile,bufstr,MAX_PATH,fname);
  next.db_file = bufstr;

  bufval = GetPrivateProfileInt(L"in_adlib",L"usedb",DFL_USEDB,fname);
  if (bufval != -1)
    next.usedb = bufval ? true : false;

  bufval = GetPrivateProfileInt(L"in_adlib",L"lastprefstab",DFL_LASTPREFSTAB,fname);
  if (bufval != -1)
    next.lastprefstab = bufval ? true : false;

  apply(false);
}

void WriteIniInt(const wchar_t* name, const int value, const int def_value)
{
  wchar_t bufstr[11]/* = { 0 }*/;
  WritePrivateProfileString(L"in_adlib", name, ((value != def_value) ?
                            I2WStr(value, bufstr, ARRAYSIZE(bufstr)) :
                                  NULL), GetPaths()->winamp_ini_file);
}

void Config::save(void)
{
  WriteIniInt(L"replayfreq", next.replayfreq, DFL_REPLAYFREQ);
  WriteIniInt(L"harmonic", next.harmonic, DFL_HARMONIC);
  WriteIniInt(L"use16bit", next.use16bit, DFL_USE16BIT);
  WriteIniInt(L"stereo", next.stereo, DFL_STEREO);
  WriteIniInt(L"useoutput", next.useoutput, DFL_USEOUTPUT);
  WriteIniInt(L"useoutput_alt", next.useoutput_alt, DFL_USEOUTPUT_ALT);
  WriteIniInt(L"testloop", next.testloop, DFL_TESTLOOP);
  WriteIniInt(L"subseq", next.subseq, DFL_SUBSEQ);
  WriteIniInt(L"stdtimer", next.stdtimer, DFL_STDTIMER);
  WriteIniInt(L"usedb", next.usedb, DFL_USEDB);
  WriteIniInt(L"lastprefstab", next.lastprefstab, DFL_LASTPREFSTAB);
}

void Config::check()
{
  switch (next.useoutput) {
  case emuts:
  case emuks:
  case emuwo:
  case emunk:
    next.useoutputplug = true;
    break;
  default:
    next.useoutputplug = false;
    break;
  }

#ifdef FOR_XMPLAY
  if (!next.useoutputplug)
    if (test_xmplay())
      {
	next.useoutput = DFL_EMU;
	next.useoutputplug = true;

	MessageBox(NULL,MSGE_XMPLAY,"AdPlug :: Error",MB_ICONERROR | MB_TASKMODAL);
      }
#endif

#if 0   // dro change
  if (next.useoutput == disk)
    if (!next.stdtimer)
      if (!next.testloop)
	TimedMessageBox(plugin.hMainWindow,MSGC_DISK,TEXT("AdPlug :: Caution"),MB_ICONWARNING | MB_TASKMODAL, 5000);

  if (next.useoutputplug > useoutputplug)
    TimedMessageBox(plugin.hMainWindow,MSGA_WINAMP,TEXT("AdPlug :: Attention"),MB_ICONINFORMATION | MB_TASKMODAL, 5000);
#endif

  use_database();/*/
  if (!use_database())
    TimedMessageBox(plugin.hMainWindow,MSGC_DATABASE,TEXT("AdPlug :: Caution"),MB_ICONWARNING | MB_TASKMODAL, 5000);/**/
}

void Config::apply(bool testout)
{
  check();

  work.replayfreq	= next.replayfreq;
  work.harmonic		= next.harmonic;
  work.use16bit		= next.use16bit;
  work.stereo		= next.stereo;
  work.testloop		= next.testloop;
  work.subseq		= next.subseq;
  work.usedb		= next.usedb;
  work.lastprefstab = next.lastprefstab;
  work.stdtimer		= next.stdtimer;
  work.diskdir		= next.diskdir;
  work.ignored		= next.ignored;
  work.db_file		= next.db_file;

#if 0   // dro change
  if (!testout || (next.useoutputplug <= useoutputplug))
#else
  if (!testout/* || (next.useoutputplug <= useoutputplug)*/)
#endif
    {
      work.useoutput = next.useoutput;
      work.useoutput_alt = next.useoutput_alt;
#if 0   // dro change
      useoutputplug  = next.useoutputplug;
#endif
    }
}

void Config::get(t_config_data *cfg)
{
  *cfg = work;
}

void Config::set(t_config_data *cfg)
{
  next = *cfg;

  apply(true);
}

const wchar_t *Config::get_ignored(void)
{
  return work.ignored.c_str();
}

void Config::set_ignored(const wchar_t *ignore_list)
{
  next.ignored = ignore_list;
  WritePrivateProfileString(L"in_adlib", L"ignored", (!next.ignored.empty() ?
                            next.ignored.c_str() : NULL), GetPaths()->winamp_ini_file);
}

bool Config::use_database(void)
{
  bool success = true;

  if(mydb) { delete mydb; mydb = 0; }
  if(next.usedb) {
    mydb = new CAdPlugDatabase;
    const AutoCharFn db_file(next.db_file.c_str());
    success = mydb->load(std::string(db_file));
  }
  CAdPlug::set_database(mydb);

  return success;
}

#ifdef FOR_XMPLAY
bool Config::test_xmplay(void)
{
  return GetModuleHandle("xmplay.exe") ? true : false;
}
#endif