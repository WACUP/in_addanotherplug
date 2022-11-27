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
#define WA_UTILS_SIMPLE
#include <../../loader/loader/utils.h>

void FileTypes::add(const wchar_t *type, const wchar_t *name, bool _ignore)
{
  work.type.push_back(type);
  work.name.push_back(name);
  work.ignore.push_back(_ignore);
}

wchar_t *FileTypes::export_filetypes(wchar_t *buf)
{
  wchar_t *retval = buf;
  const int count = get_size();
  for (int i=0;i<count;i++)
    if(!work.ignore[i]) {
      memcpy(buf, work.type[i].c_str(),work.type[i].length()*2);
      buf += work.type[i].length();
      *buf++ = 0;

      memcpy(buf,work.name[i].c_str(),work.name[i].length()*2);
      buf += work.name[i].length();
      *buf++ = 0;
    }

  *(int *)buf = 0;

  return retval;
}

int FileTypes::get_size() const
{
  return work.type.size();
}

const wchar_t *FileTypes::get_name(int i)
{
  return work.name[i].c_str();
}

bool FileTypes::get_ignore(int i) const
{
  return work.ignore[i];
}

void FileTypes::set_ignore(int i, bool val)
{
  work.ignore[i] = val;
}

int FileTypes::grata(const wchar_t *fname) const
{
  const wchar_t *tmpstr = wcsrchr(fname,L'.');
  if (!tmpstr)
    return -1;

  wchar_t* p = (wchar_t*)calloc(wcslen(++tmpstr) + 1, sizeof(wchar_t));
  if (!p)
    return -1;

  wcscpy(p,tmpstr);

  const int count = get_size();
  for (int i=0;i<count;i++)
    {
    if (work.ignore[i])
	continue;

    wstring tmpxstr = work.type[i];

    const wchar_t *ext = tmpxstr.c_str();
    /*const char *str = wcsstr(ext,_wcslwr(p));/*/
    const wchar_t*str = wcsistr(ext,p);/**/

      if (str)
	{
	  // for "aaa;bbb;ccc" and "ccc"
	  if (wcslen(p) == wcslen(str))
	    {
	      free(p);
	    return i;
	    }

	  if (str[wcslen(p)] == L';')
	    {
	      // for "aaa;bbb;ccc" and "aaa"
	      if (ext == str)
		{
		  free(p);
		  return i;
		}

	      // for "aaa;bbb;ccc" and "bbb"
	    if (ext[str-ext-1] == L';')
		{
		  free(p);
		  return i;
		}
	    }
	}
    }

  free(p);
  return -1;
}

void FileTypes::set_ignore_list(const wchar_t *ignore_list)
{
  wchar_t *str,*spos;
  str = spos = (wchar_t *)ignore_list;

  while (*str)
    {
    while (*spos != L';')
	spos++;
      *spos = 0;

    int ipos = WStr2I(str);
      if (ipos < get_size())
	work.ignore[ipos] = true;

    *spos++ = L';';
      str = spos;
    }
}

const wchar_t *FileTypes::get_ignore_list(void)
{
  wchar_t tmpstr[11] = {0};

  xstrlist.clear();

  for (int i=0;i<get_size();i++)
    if (work.ignore[i])
      {
		xstrlist.append(I2WStr(i, tmpstr, ARRAYSIZE(tmpstr)));
	    xstrlist.append(L";");
      }

  return xstrlist.c_str();
}
