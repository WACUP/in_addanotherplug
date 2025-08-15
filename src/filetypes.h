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

class FileTypes
{
 public:

  void      reserve(const size_t count);

  void		add(const wchar_t *type, const wchar_t *name, bool _ignore = false);

  wchar_t *	export_filetypes(wchar_t *buf);

  int		grata(const wchar_t *fname) const;

  int		get_size() const;

  bool		get_ignore(int i) const;
  void		set_ignore(int i, bool val);

  const wchar_t *	get_ignore_list(void);
  void		set_ignore_list(const wchar_t *ignore_list);

  const wchar_t *	get_name(int i);

 private:

  struct t_filetype_data
  {
    vector<wstring>	type;
    vector<wstring>	name;
    vector<bool>	ignore;
  } work;

  wstring	xstrlist;
};
