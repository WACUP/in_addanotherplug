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
#include "api.h"
#define WA_UTILS_SIMPLE
#include <../../loader/loader/utils.h>
#include <../../sdk/nu/autocharfn.h>

extern In_Module plugin;
extern Config config;
extern GuiDlgInfo dlg_info;
extern TEmulInfo infoEmuls[MAX_EMULATORS];

int thread_priority[] = {
  THREAD_PRIORITY_IDLE,
  THREAD_PRIORITY_LOWEST,
  THREAD_PRIORITY_BELOW_NORMAL,
  THREAD_PRIORITY_NORMAL,
  THREAD_PRIORITY_ABOVE_NORMAL,
  THREAD_PRIORITY_HIGHEST,
  THREAD_PRIORITY_TIME_CRITICAL
};

TEmulInfo *getEmulInfo(int emul)
{
  for (int i = 0; i < MAX_EMULATORS; i++)
  {
    if (infoEmuls[i].emul == emul)
      return &infoEmuls[i];
  }
  return NULL; // by default
}

bool MyPlayer::open(const char* fname, int* bps, int* nch, int* srate)
{
  Copl *opl;

  config.get(&work);

  // new file ?
  if (plr.fname.compare(fname))
  {
    plr.subsong = DFL_SUBSONG;
    plr.fname = fname;
  }

  // init opl
  opl = opl_init();
  if (!opl)
    return false;

  // init adplug player
  player = CAdPlug::factory(plr.fname.c_str(),opl);
  if (!player)
  {
    opl_done();
    return false;
  }

  // init player data
  plr.playing = 1;
  plr.paused = 0;
  plr.maxsubsong = player->getsubsongs();
  plr.outtime = 0.0f;
  plr.fulltime = get_length(plr.fname.c_str(), plr.subsong);
  plr.seek = -1;
  plr.stilltoadd = 0;

  // rewind song
  player->rewind(plr.subsong);

  *bps = (work.use16bit ? 16 : 8);
  *nch = (work.stereo ? 2 : 1);
  *srate = work.replayfreq;
  return true;
}

int MyPlayer::decode(char* sndbuf, size_t sndbuflen)
{
  if (!plr.playing)
    return 0;

  int sampsize = 1;
  if (work.use16bit)
    sampsize *= 2;
  if (work.stereo)
    sampsize *= 2;

  // fill sound buffer
  long towrite = (sndbuflen / sampsize);
  char *sndbufpos = sndbuf;

  while (towrite > 0)
  {
    while (plr.stilltoadd < 0)
	{
	  plr.stilltoadd += work.replayfreq;
	  plr.playing = player->update();
	}

	long i = min(towrite,(long)(plr.stilltoadd/player->getrefresh()+4)&~3);

	output.emu->update((short *)sndbufpos,i);
	sndbufpos += i * sampsize;
    towrite -= i;
	i = (long)(i * player->getrefresh());
    plr.stilltoadd -= max(1, i);
  }
  return (sndbuflen - (towrite * sampsize));
}

int MyPlayer::play(const char *fname)
{
  Copl *opl;

  config.get(&work);

  // new file ?
  if (plr.fname.compare(fname))
  {
    plr.subsong = DFL_SUBSONG;
    plr.fname = fname;
  }

  // init opl
  opl = opl_init();
  if (!opl)
    return 2;

  // init adplug player
  player = CAdPlug::factory(plr.fname.c_str(),opl);
  if (!player)
    {
      opl_done();
      return -1;
    }

  // init output
  if (!output_init())
    {
      delete player;
      opl_done();
      return 2;
    }

  // init player data
  plr.playing = 1;
  plr.paused = 0;
  plr.maxsubsong = player->getsubsongs();
  plr.outtime = 0.0f;
  plr.fulltime = get_length(plr.fname.c_str(),plr.subsong);
  plr.seek = -1;

  // rewind song
  player->rewind(plr.subsong);

  // set winamp info
  plugin.SetInfo(120000,work.replayfreq/1000,(work.stereo ? 2 : 1),1);

  // init thread
  if (!thread_init())
    {
      plr.playing = 0;
      output_done();
      delete player;
      opl_done();
      return 2;
    }

  dlg_info.dock();	
	
  return 0;
}

void MyPlayer::stop()
{
  if (!plr.playing)
    return;

  plr.playing = 0;

  thread_done();
  output_done();

  dlg_info.undock();

  delete player;
  opl_done();
}

void MyPlayer::close()
{
  delete player;
  opl_done();
}

int MyPlayer::is_playing()
{
  return plr.playing;
}

void MyPlayer::pause()
{
  plr.paused = 1;
}

void MyPlayer::unpause()
{
  plr.paused = 0;
}

int MyPlayer::is_paused()
{
  return plr.paused;
}

const char *MyPlayer::get_file()
{
  return plr.fname.c_str();
}

CPlayer *MyPlayer::get_player()
{
  return player;
}

int MyPlayer::get_subsong()
{
  return plr.subsong;
}

void MyPlayer::set_subsong(int subsong)
{
  plr.subsong = subsong;

  SendMessage(plugin.hMainWindow,WM_COMMAND,WINAMP_BUTTON2,0);
}

int MyPlayer::get_length()
{
  return plr.fulltime;
}

int MyPlayer::get_length(const char *fname, int subsong)
{
  CSilentopl silent;

  CPlayer *p = CAdPlug::factory(fname,&silent);
  if (!p)
    return 0;

  int fulltime = p->songlength(subsong);

  delete p;

  return fulltime;
}

int MyPlayer::get_position()
{
  switch (work.useoutput)
    {
    case emuts:
    case emuks:
    case emuwo:
    case emunk:
      return (plugin.outMod ? plugin.outMod->GetOutputTime() : 0);
    case disk:
      return (int)plr.outtime;
    }

  return 0;
}

void MyPlayer::seek(int pos)
{
  plr.seek = pos;
}

void MyPlayer::set_volume(int vol)
{
  if (!plr.playing)
    config.get(&work);

  plr.volume = (int)(63 - vol/(255/63));

  switch (work.useoutput)
    {
    case emuts:
    case emuks:
    case emuwo:
    case emunk:
      if (plugin.outMod)
      {
        plugin.outMod->SetVolume(vol);
      }
      break;
    case disk:
      // disk writer does not obey volume control
      break;
    }
}

void MyPlayer::set_panning(int pan)
{
  if (!plr.playing)
    config.get(&work);

  switch (work.useoutput)
    {
    case emuts:
    case emuks:
    case emuwo:
    case emunk:
      if (plugin.outMod)
      {
        plugin.outMod->SetPan(pan);
      }
      break;

    case disk:
      // Disk writer does not support panning
      break;
    }
}

Copl *MyPlayer::make_opl(enum t_output type, bool stereo)
{
  switch (type) {
  case emuwo:
    return new CWemuopl(work.replayfreq, work.use16bit, stereo);
  case emuts:
    return new CEmuopl(work.replayfreq, work.use16bit, stereo);
  case emuks:
    return new CKemuopl(work.replayfreq, work.use16bit, stereo);
  case emunk:
    return new CNemuopl(work.replayfreq);
  case disk:
    return new CDiskopl(get_diskfile(plr.fname));
  }
  return NULL;
}

Copl *MyPlayer::opl_init()
{
  COPLprops a, b;
  Copl *opl = NULL;
  TEmulInfo *info = NULL;

  // Stereo as far as the OPL synth is concerned is only true if set to stereo (not mono or surround)
  a.stereo = b.stereo = !work.harmonic && work.stereo;
  a.use16bit = b.use16bit = work.use16bit;

  info = getEmulInfo(work.useoutput);
  if (info != NULL && !info->s_mono)
    a.stereo = true;
  opl = output.emu = make_opl(work.useoutput, a.stereo);
  if (!opl) return NULL;

  if (work.useoutput == disk) {
    output.disk = (CDiskopl *)opl;
  } else {
    if (work.harmonic == true) {
      t_output b_out = work.useoutput_alt == emunone ? work.useoutput : work.useoutput_alt;
      info = getEmulInfo(b_out);
      if (info != NULL && !info->s_mono)
        b.stereo = true;
      a.opl = opl;
      b.opl = make_opl(b_out, b.stereo);
      // CSurroundopl will take ownership of "a.opl" and "b.opl", and will free them upon destruction.
      opl = output.emu = new CSurroundopl(&a, &b, work.use16bit);
    }
  }

  return opl;
}

void MyPlayer::opl_done()
{
  switch (work.useoutput) {
  case emuts:
  case emuks:
  case emuwo:
  case emunk:
    delete output.emu;
    break;
  case disk:
    delete output.disk;
    break;
  }
}

bool MyPlayer::output_init()
{
  switch (work.useoutput)
    {
    case emuts:
    case emuks:
    case emuwo:
    case emunk:
      maxlatency = (plugin.outMod->Open && work.replayfreq ? plugin.outMod->Open(work.replayfreq,
                                  (work.stereo ? 2 : 1), (work.use16bit ? 16 : 8), -1, -1) : -1);
      if (maxlatency < 0)
	return false;
      plugin.outMod->SetVolume(-666);
      plugin.SAVSAInit(maxlatency,work.replayfreq);
      plugin.VSASetInfo((work.stereo ? 2 : 1),work.replayfreq);
      break;
    case disk:
      // no init necessary
      break;
    }

  return true;
}

void MyPlayer::output_done()
{
  switch (work.useoutput)
    {
    case emuts:
    case emuks:
    case emuwo:
    case emunk:
	  if (plugin.outMod && plugin.outMod->Close)
	  {
	    plugin.outMod->Close();
	  }
      if (plugin.outMod)
      {
        plugin.SAVSADeInit();
      }
      break;
    case disk:
      // no deinit necessary
      break;
    }
}

bool MyPlayer::thread_init()
{
  switch (work.useoutput)
    {
    case emuts:
    case emuks:
    case emuwo:
    case emunk:
      thread.emuts = StartThread(callback_emuts, this, static_cast<int>(plugin.config->
                                            GetInt(playbackConfigGroupGUID, L"priority",
                                                    THREAD_PRIORITY_HIGHEST)), 0, NULL);
      if (!thread.emuts)
	    return false;
      break;
    case disk:
      thread.disk = StartThread(callback_disk, this, THREAD_PRIORITY_HIGHEST, 0, NULL);
      if (!thread.disk)
	    return false;
      break;
    }

  return true;
}

void MyPlayer::thread_done()
{
  DWORD wait_ms = (DWORD)(7 * 1000 / player->getrefresh());
  switch (work.useoutput)
    {
    case emuts:
    case emuks:
    case emuwo:
    case emunk:
      if (WaitForSingleObject(thread.emuts, wait_ms) == WAIT_TIMEOUT)
        TerminateThread(thread.emuts, 0);
      CloseHandle(thread.emuts);
      break;
    case disk:
      if (WaitForSingleObject(thread.disk, wait_ms) == WAIT_TIMEOUT)
        TerminateThread(thread.disk, 0);
      CloseHandle(thread.disk);
      break;
    }
}

const char *MyPlayer::get_diskfile(string& fname)
{
  diskfile.assign(AutoCharFn(work.diskdir.c_str()));
  diskfile.append(strrchr(fname.c_str(),'\\'));

  if (plr.subsong)
  {
    char bufstr[11] = { 0 };	  
    I2AStr(plr.subsong,bufstr,10);

    diskfile.append(".");
    diskfile.append(bufstr);
  }

  diskfile.append(".raw");

  return diskfile.c_str();
}
