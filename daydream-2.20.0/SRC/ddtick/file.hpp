/* $Id: file.hpp,v 1.1 2010-03-27 20:59:41 bo Exp $
 *  Provides crc calculation routines
 *  Collected & written by Stas Degteff <g@grumbler.org> 2:5080/102
 *  (c) Stas Degteff
 *  (c) HUSKY Developers Team
 *
 * Latest version may be foind on http://husky.sourceforge.net
 *
 *
 * HUSKYLIB: common defines, types and functions for HUSKY
 *
 * This is part of The HUSKY Fidonet Software project:
 * see http://husky.sourceforge.net for details
 *
 *
 * HUSKYLIB is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * HUSKYLIB is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; see file COPYING. If not, write to the
 * Free Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * See also http://www.gnu.org, license may be found here.
 */


class File {
public:
  File();
  File(string _filename); 
  int Size();
  int CRC();
  File Copy(string dest);
  File Move(string dest);
  string GetFilename();

private:
  bool match_ics();
  bool ok();
  int memcrc32(const char *str, int size, int initcrc);
  int filecrc32(const char *filename);
  int* crc32tab();

  string filename;
  bool valid;
  enum { CRC_BUFFER_SIZE = 80000 };
};
