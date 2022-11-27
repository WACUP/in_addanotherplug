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

#include <string>

#define DFL_SUBSONG 0
#define DFL_SNDBUFSIZE 576

enum t_output {
  emuts,         /* Tatsuyuki Satoh */
  emuks,         /* Ken Silverman */
  opl2_unused,   /* unused */
  disk,          /* Disk Writer */
  emuwo,         /* WoodyOPL */
  emunk,         /* Nuked OPL3 */
  emunone        /* used for alt_synth */
};

struct t_config_data {
  int            replayfreq;
  bool           useoutputplug;
  bool           harmonic;
  bool           use16bit;
  bool           stereo;
  enum t_output  useoutput;
  enum t_output  useoutput_alt;
  bool           testloop;
  bool           subseq;
  int            stdtimer;
  wstring        diskdir;
  wstring        ignored;
  wstring        db_file;
  bool           usedb;
  bool           lastprefstab;
};

class Config
{
 public:

  Config();

  void		load(void);
  void		save(void);

  void		get(t_config_data *cfg);
  void		set(t_config_data *cfg);

  const wchar_t *	get_ignored(void);
  void		set_ignored(const wchar_t *ignore_list);

  bool		useoutputplug;

 private:

  void		apply(bool testout);
  bool		use_database(void);

  void		check(void);

#ifdef FOR_XMPLAY
  bool		test_xmplay(void);
#endif

  static CAdPlugDatabase	*mydb;

  t_config_data	work, next;
};
