// This file is part of LibPkg.
// Copyright � 2003 Graham Shaw.            
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !LibPkg.Copyright.

#include <fstream>

#include "rtk/os/os.h"

#include "libpkg/filesystem.h"
#include "libpkg/dirstream.h"
#include "libpkg/pkgbase.h"
#include "libpkg/sysvars.h"

namespace {

/** Convert filename to system variable name.
 * Variable names are represented using quoted-printable encoding
 * with "=" as the escape character.
 */
string filename_to_varname(const string& filename)
{
	string varname;
	enum state_type
	{
		state_normal,
		state_escape1,
		state_escape2
	};
	state_type state=state_normal;
	for (string::const_iterator i=filename.begin();i!=filename.end();++i)
	{
		switch (state)
		{
		case state_normal:
			varname+=*i;
			if (*i=='=') state=state_escape1;
			break;
		case state_escape1:
			varname+=*i;
			if (*i=='=') state=state_escape1;
			else if (isxdigit(*i)) state=state_escape2;
			else state=state_normal;
			break;
		case state_escape2:
			varname+=*i;
			if (*i=='=') state=state_escape1;
			else if (isxdigit(*i))
			{
				char hex[3];
				hex[0]=varname[varname.length()-2];
				hex[1]=varname[varname.length()-1];
				hex[2]=0;
				varname[varname.length()-3]=strtol(hex,0,16);
				varname.resize(varname.length()-2);
				state=state_normal;
			}
			else state=state_normal;
			break;
		}
	}
	return varname;
}

}; /* anonymous namespace */

namespace pkg {

// At present update_sysvars() does not immediately unset variables that
// are no longer in use.  This could be done, but it is not entirely clear
// that it is the correct/desirable behaviour.  Certainly, leaving them
// set is easier to implement and does little if any harm.

void update_sysvars(pkgbase& pb)
{
	// Set pathnames.
	string dst_pathname=pb.setvars_pathname();
	string tmp_pathname=dst_pathname+string("++");
	string bak_pathname=dst_pathname+string("--");

	// Write obey file.
	ofstream out(tmp_pathname.c_str());
	out << "| This file is automatically generated by !RiscPkg." << endl;
	out << "| Alterations will not be preserved." << endl;
	out << endl;
	if (object_type(pb.sysvars_pathname())!=0)
	{
		dirstream ds(pb.sysvars_pathname());
		while (ds)
		{
			dirstream::object obj;
			ds >> obj;
			if (obj.filetype==0xfff)
			{
				// If file is a text file then use file content as value.
				string varname=filename_to_varname(obj.name);
				string pathname=pb.sysvars_pathname()+string(".")+obj.name;
				ifstream in(pathname.c_str());
				string varval;
				getline(in,varval);
				out << "Set " << varname << " \"" << varval << "\"" << endl;
				rtk::os::OS_SetVarVal(varname.c_str(),varval.c_str(),
					varval.length(),0,0,0,0);
			}
		}
	}
	out.close();
	write_filetype(tmp_pathname,0xfeb);

	// Backup existing obey file if it exists.
	if (object_type(dst_pathname)!=0)
	{
		force_move(dst_pathname,bak_pathname,true);
	}

	// Move new obey file to destination.
	force_move(tmp_pathname,dst_pathname,false);

	// Delete backup.
	force_delete(bak_pathname);
}

}; /* namespace pkg */
