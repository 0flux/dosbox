/*
 *  Copyright (C) 2002-2010  The DOSBox Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* $Id: drive_local.cpp,v 1.82 2009-07-18 18:42:55 c2woody Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "dosbox.h"
#include "dos_inc.h"
#include "drives.h"
#include "support.h"
#include "cross.h"
#include "inout.h"

#include "drive_local_gboxer.h"
#include "Coalface.h"


bool localDrive::FileCreate(DOS_File * * file,const char * name,Bit16u /*attributes*/) {
//TODO Maybe care for attributes but not likely
	char newname[CROSS_LEN];
	strcpy(newname,basedir);
	strcat(newname,name);
	CROSS_FILENAME(newname);
	char* temp_name = dirCache.GetExpandName(newname); //Can only be used in till a new drive_cache action is preformed */
	/* Test if file exists (so we need to truncate it). don't add to dirCache then */
	bool existing_file = false;
	
	//--Added 2010-01-18 by Alun Bestor to allow Boxer to selectively deny write access to files
	if (!boxer_shouldAllowWriteAccessToPath((const char *)newname, this))
	{
		DOS_SetError(DOSERR_ACCESS_DENIED);
		return false;
	}
	//--End of modifications

	//-- Modified 2012-07-24 by Alun Bestor to allow Boxer to shadow local file access
	//FILE * test = fopen_wrap(temp_name,"rb+");
	//FILE * test = boxer_openLocalFile(temp_name, this, "rb+");
	ADB::VFILE *test = GBoxer::Coalface::open_local_file(temp_name, this, "rb+");
	//--End of modifications
	if(test) {
		// fclose(test);
		GBoxer::Coalface::close_local_file(test);
		existing_file=true;

	}
	
	//-- Modified 2012-07-24 by Alun Bestor to allow Boxer to shadow local file access
	//FILE * hand = fopen_wrap(temp_name,"wb+");
	//FILE * hand = boxer_openLocalFile(temp_name, this, "wb+");
	ADB::VFILE *hand = GBoxer::Coalface::open_local_file(temp_name, this, "wb+");
	//--End of modifications
	if (!hand){
		LOG_MSG("Warning: file creation failed: %s",newname);
		return false;
	}
   
	if(!existing_file) {
		strcpy(newname,basedir);
		strcat(newname,name);
		CROSS_FILENAME(newname);
		dirCache.AddEntry(newname, true);
	}

	/*
	FileStat_Block statBlock;
	bool gotStat = FileStat(name, &statBlock);
	if (!gotStat) {
		LOG_MSG("Warning: FileStat FAILED!!!");
	}
	*/

	/* Make the 16 bit device information */
	*file = new GBoxer::LocalFile(name, hand);
	(*file)->flags = OPEN_READWRITE;


	//--Added 2010-08-21 by Alun Bestor to let Boxer monitor DOSBox's file operations
	boxer_didCreateLocalFile(temp_name, this);
	//--End of modifications

	return true;
}

bool localDrive::FileOpen(DOS_File * * file,const char * name,Bit32u flags) {
	const char* type;
	switch (flags&0xf) {
	case OPEN_READ:type="rb"; break;
	case OPEN_WRITE:type="rb+"; break;
	case OPEN_READWRITE:type="rb+"; break;
	default:
		DOS_SetError(DOSERR_ACCESS_CODE_INVALID);
		return false;
	}
	char newname[CROSS_LEN];
	strcpy(newname,basedir);
	strcat(newname,name);
	CROSS_FILENAME(newname);
	dirCache.ExpandName(newname);

	//--Added 2010-01-18 by Alun Bestor to allow Boxer to selectively deny write access to files
	if (!strcmp(type, "rb+"))
	{
		if (!boxer_shouldAllowWriteAccessToPath((const char *)newname, this))
		{
			//Copy-pasted from cdromDrive::FileOpen
			if ((flags&3)==OPEN_READWRITE) {
				flags &= ~OPEN_READWRITE;
			} else {
				DOS_SetError(DOSERR_ACCESS_DENIED);
				return false;
			}			
		}
	}
	//--End of modifications

	//-- Modified 2012-07-24 by Alun Bestor to allow Boxer to shadow local file access
	//FILE * hand = fopen_wrap(newname,type);
	//FILE * hand = boxer_openLocalFile(newname, this, type);
	ADB::VFILE *hand = GBoxer::Coalface::open_local_file(newname, this, type);
	//--End of modifications

//	Bit32u err=errno;
	if (!hand) { 
		if((flags&0xf) != OPEN_READ) {
            //-- Modified 2012-07-24 by Alun Bestor to allow Boxer to shadow local file access
            //FILE * hmm = fopen_wrap(newname,"rb");
            //FILE * hmm = boxer_openLocalFile(newname, this, "rb");
			ADB::VFILE *hmm = GBoxer::Coalface::open_local_file(newname, this, "rb");
            //--End of modifications

			if (hmm) {
				// fclose(hmm);
				GBoxer::Coalface::close_local_file(hmm);
				LOG_MSG("Warning: file %s exists and failed to open in write mode.\nPlease Remove write-protection",newname);
			}
		}
		return false;
	}

	// *file=new localFile(name,hand);
	*file = new GBoxer::LocalFile(name, hand);
	(*file)->flags=flags;  //for the inheritance flag and maybe check for others.
//	(*file)->SetFileName(newname);
	return true;
}

FILE * localDrive::GetSystemFilePtr(char const * const name, char const * const type) {

	char newname[CROSS_LEN];
	strcpy(newname,basedir);
	strcat(newname,name);
	CROSS_FILENAME(newname);
	dirCache.ExpandName(newname);

	//-- Modified 2012-07-24 by Alun Bestor to allow Boxer to shadow local file access
	//return fopen_wrap(newname,type);
	//return boxer_openLocalFile(newname, this, type);
	return fopen_wrap(newname,type);
	//return GBoxer::Coalface::open_local_file(newname, this, type);
	//--End of modifications
}

bool localDrive::GetSystemFilename(char *sysName, char const * const dosName) {

	strcpy(sysName, basedir);
	strcat(sysName, dosName);
	CROSS_FILENAME(sysName);
	dirCache.ExpandName(sysName);
	return true;
}

bool localDrive::FileUnlink(const char * name) {
	char newname[CROSS_LEN];
	strcpy(newname,basedir);
	strcat(newname,name);
	CROSS_FILENAME(newname);
	char *fullname = dirCache.GetExpandName(newname);

	//--Added 2010-12-29 by Alun Bestor to let Boxer selectively prevent file operations
	if (!boxer_shouldAllowWriteAccessToPath((const char *)fullname, this))
	{
		DOS_SetError(DOSERR_ACCESS_DENIED);
		return false;
	}
	//--End of modifications

	//-- Modified 2012-07-24 by Alun Bestor to allow Boxer to shadow local file access
	//if (unlink(fullname)) {
	if (!boxer_removeLocalFile(fullname, this)) {
		//Unlink failed for some reason try finding it.
		struct stat buffer;
        //if(stat(fullname,&buffer)) return false; // File not found.
        if (!boxer_getLocalPathStats(fullname, this, &buffer)) return false;
		
		//FILE* file_writable = fopen_wrap(fullname,"rb+");
        //FILE* file_writable = boxer_openLocalFile(fullname, this, "rb+");
		ADB::VFILE *file_writable = GBoxer::Coalface::open_local_file(fullname, this, "rb+");

		if(!file_writable) return false; //No acces ? ERROR MESSAGE NOT SET. FIXME ?
		// fclose(file_writable);
		GBoxer::Coalface::close_local_file(file_writable);

		//File exists and can technically be deleted, nevertheless it failed.
		//This means that the file is probably open by some process.
		//See if We have it open.
		bool found_file = false;
		for(Bitu i = 0;i < DOS_FILES;i++){
			if(Files[i] && Files[i]->IsName(name)) {
				Bitu max = DOS_FILES;
				while(Files[i]->IsOpen() && max--) {
					Files[i]->Close();
					if (Files[i]->RemoveRef()<=0) break;
				}
				found_file=true;
			}
		}
		if(!found_file) return false;
		//if (!unlink(fullname)) {
		if (boxer_removeLocalFile(fullname, this)) {
			dirCache.DeleteEntry(newname);

			//--Added 2010-08-21 by Alun Bestor to let Boxer monitor DOSBox's file operations
			boxer_didRemoveLocalFile(fullname, this);
			//--End of modifications
			return true;
		}
		return false;
	//--End of modifications
	} else {
		dirCache.DeleteEntry(newname);

		//--Added 2010-08-21 by Alun Bestor to let Boxer monitor DOSBox's file operations
		boxer_didRemoveLocalFile(fullname, this);
		//--End of modifications
		return true;
	}
}

bool localDrive::FindFirst(const char * _dir,DOS_DTA & dta,bool fcb_findfirst) {
	char tempDir[CROSS_LEN];
	strcpy(tempDir,basedir);
	strcat(tempDir,_dir);
	CROSS_FILENAME(tempDir);

	if (allocation.mediaid==0xF0 ) {
		EmptyCache(); //rescan floppie-content on each findfirst
	}

	char end[2]={CROSS_FILESPLIT,0};
	if (tempDir[strlen(tempDir)-1]!=CROSS_FILESPLIT) strcat(tempDir,end);

	Bit16u id;
	if (!dirCache.FindFirst(tempDir,id)) {
		DOS_SetError(DOSERR_PATH_NOT_FOUND);
		return false;
	}
	strcpy(srchInfo[id].srch_dir,tempDir);
	dta.SetDirID(id);

	Bit8u sAttr;
	dta.GetSearchParams(sAttr,tempDir);

	if (this->isRemote() && this->isRemovable()) {
		// cdroms behave a bit different than regular drives
		if (sAttr == DOS_ATTR_VOLUME) {
			dta.SetResult(dirCache.GetLabel(),0,0,0,DOS_ATTR_VOLUME);
			return true;
		}
	} else {
		if (sAttr == DOS_ATTR_VOLUME) {
			if ( strcmp(dirCache.GetLabel(), "") == 0 ) {
//				LOG(LOG_DOSMISC,LOG_ERROR)("DRIVELABEL REQUESTED: none present, returned  NOLABEL");
//				dta.SetResult("NO_LABEL",0,0,0,DOS_ATTR_VOLUME);
//				return true;
				DOS_SetError(DOSERR_NO_MORE_FILES);
				return false;
			}
			dta.SetResult(dirCache.GetLabel(),0,0,0,DOS_ATTR_VOLUME);
			return true;
		} else if ((sAttr & DOS_ATTR_VOLUME)  && (*_dir == 0) && !fcb_findfirst) {
		//should check for a valid leading directory instead of 0
		//exists==true if the volume label matches the searchmask and the path is valid
			if (WildFileCmp(dirCache.GetLabel(),tempDir)) {
				dta.SetResult(dirCache.GetLabel(),0,0,0,DOS_ATTR_VOLUME);
				return true;
			}
		}
	}
	return FindNext(dta);
}

bool localDrive::FindNext(DOS_DTA & dta) {

	char * dir_ent;
	struct stat stat_block;
	char full_name[CROSS_LEN];
	char dir_entcopy[CROSS_LEN];

	Bit8u srch_attr;char srch_pattern[DOS_NAMELENGTH_ASCII];
	Bit8u find_attr;

	dta.GetSearchParams(srch_attr,srch_pattern);
	Bit16u id = dta.GetDirID();

again:
	if (!dirCache.FindNext(id,dir_ent)) {
		DOS_SetError(DOSERR_NO_MORE_FILES);
		return false;
	}
	if(!WildFileCmp(dir_ent,srch_pattern)) goto again;

	strcpy(full_name,srchInfo[id].srch_dir);
	strcat(full_name,dir_ent);

	//GetExpandName might indirectly destroy dir_ent (by caching in a new directory
	//and due to its design dir_ent might be lost.)
	//Copying dir_ent first
	strcpy(dir_entcopy,dir_ent);
    //Modified 2012-07-24 by Alun Bestor to wrap local file operations
	//if (stat(dirCache.GetExpandName(full_name),&stat_block)!=0) {
    if (!boxer_getLocalPathStats(dirCache.GetExpandName(full_name), this, &stat_block)) {
    //--End of modifications
		goto again;//No symlinks and such
	}

	if(stat_block.st_mode & S_IFDIR) find_attr=DOS_ATTR_DIRECTORY;
	else find_attr=DOS_ATTR_ARCHIVE;
 	if (~srch_attr & find_attr & (DOS_ATTR_DIRECTORY | DOS_ATTR_HIDDEN | DOS_ATTR_SYSTEM)) goto again;

	/*file is okay, setup everything to be copied in DTA Block */
	char find_name[DOS_NAMELENGTH_ASCII];Bit16u find_date,find_time;Bit32u find_size;

	if(strlen(dir_entcopy)<DOS_NAMELENGTH_ASCII){
		strcpy(find_name,dir_entcopy);
		upcase(find_name);
	}

	find_size=(Bit32u) stat_block.st_size;
	struct tm *time;
	if((time=localtime(&stat_block.st_mtime))!=0){
		find_date=DOS_PackDate((Bit16u)(time->tm_year+1900),(Bit16u)(time->tm_mon+1),(Bit16u)time->tm_mday);
		find_time=DOS_PackTime((Bit16u)time->tm_hour,(Bit16u)time->tm_min,(Bit16u)time->tm_sec);
	} else {
		find_time=6;
		find_date=4;
	}
	dta.SetResult(find_name,find_size,find_date,find_time,find_attr);
	return true;
}

bool localDrive::GetFileAttr(const char * name,Bit16u * attr) {
	char newname[CROSS_LEN];
	strcpy(newname,basedir);
	strcat(newname,name);
	CROSS_FILENAME(newname);
	dirCache.ExpandName(newname);

	struct stat status;

    //Modified 2012-07-24 by Alun Bestor to wrap local file operations
	//if (stat(newname,&status)==0) {
    if (boxer_getLocalPathStats(newname, this, &status)) {
    //--End of modifications
		*attr=DOS_ATTR_ARCHIVE;
		if(status.st_mode & S_IFDIR) *attr|=DOS_ATTR_DIRECTORY;
		return true;
	}
	*attr=0;
	return false;
}

bool localDrive::MakeDir(const char * dir) {
	char newdir[CROSS_LEN];
	strcpy(newdir,basedir);
	strcat(newdir,dir);
	CROSS_FILENAME(newdir);
	char * fullname=dirCache.GetExpandName(newdir);

	//--Modified 2010-12-29 by Alun Bestor to allow Boxer to selectively prevent file operations,
	//and to prevent DOSBox from creating folders with the wrong file permissions.
	/*
	 #if defined (WIN32)						// MS Visual C++
	 int temp=mkdir(fullname);
	 #else
	 int temp=mkdir(fullname,0700);
	 #endif
	 */

	if (!boxer_shouldAllowWriteAccessToPath((const char *)fullname, this))
	{
		DOS_SetError(DOSERR_ACCESS_DENIED);
		return false;
	}

	//int temp=mkdir(fullname, 0777);
	//if (temp==0) dirCache.CacheOut(newdir,true);
	//return (temp==0);// || ((temp!=0) && (errno==EEXIST));

    bool created = boxer_createLocalDir(fullname, this);
    if (created) dirCache.CacheOut(newdir,true);
    return created;
	//--End of modifications

}

bool localDrive::RemoveDir(const char * dir) {
	char newdir[CROSS_LEN];
	strcpy(newdir,basedir);
	strcat(newdir,dir);
	CROSS_FILENAME(newdir);

	//--Modified 2010-12-29 by Alun Bestor to allow Boxer to selectively prevent file operations
	char *fullname = dirCache.GetExpandName(newdir);

	if (!boxer_shouldAllowWriteAccessToPath((const char *)fullname, this))
	{
		DOS_SetError(DOSERR_ACCESS_DENIED);
		return false;
	}

	//int temp=rmdir(fullname);
	//if (temp==0) dirCache.DeleteEntry(newdir,true);
    //return (temp==0);

    bool removed = boxer_removeLocalDir(fullname, this);
	if (removed) dirCache.DeleteEntry(newdir,true);
    return removed;
    //--End of modifications
}

bool localDrive::TestDir(const char * dir) {
	char newdir[CROSS_LEN];
	strcpy(newdir,basedir);
	strcat(newdir,dir);
	CROSS_FILENAME(newdir);
	dirCache.ExpandName(newdir);

	//--Modified 2012-04-27 by Alun Bestor to wrap local file operations
    /*
	// Skip directory test, if "\"
	size_t len = strlen(newdir);
	if (len && (newdir[len-1]!='\\')) {
		// It has to be a directory !
		struct stat test;
		if (stat(newdir,&test))			return false;
		if ((test.st_mode & S_IFDIR)==0)	return false;
	};
	int temp=access(newdir,F_OK);
	return (temp==0);
     */
    return boxer_localDirectoryExists(newdir, this);
	//--End of modifications
}

bool localDrive::Rename(const char * oldname,const char * newname) {
	char newold[CROSS_LEN];
	strcpy(newold,basedir);
	strcat(newold,oldname);
	CROSS_FILENAME(newold);
	dirCache.ExpandName(newold);

	char newnew[CROSS_LEN];
	strcpy(newnew,basedir);
	strcat(newnew,newname);
	CROSS_FILENAME(newnew);
	char *fullname = dirCache.GetExpandName(newnew);

	//--Modified 2012-04-27 by Alun Bestor to wrap local file operations
	if (!boxer_shouldAllowWriteAccessToPath((const char *)newold, this) ||
		!boxer_shouldAllowWriteAccessToPath((const char *)fullname, this))
	{
		DOS_SetError(DOSERR_ACCESS_DENIED);
		return false;
	}

	//int temp=rename(newold,fullname);
    //if (temp==0) dirCache.CacheOut(newnew);
	//return (temp==0);
    bool moved = boxer_moveLocalFile(newold, fullname, this);
    if (moved) dirCache.CacheOut(newnew);
    return moved;
	//End of modifications

}

bool localDrive::AllocationInfo(Bit16u * _bytes_sector,Bit8u * _sectors_cluster,Bit16u * _total_clusters,Bit16u * _free_clusters) {
	/* Always report 100 mb free should be enough */
	/* Total size is always 1 gb */
	*_bytes_sector=allocation.bytes_sector;
	*_sectors_cluster=allocation.sectors_cluster;
	*_total_clusters=allocation.total_clusters;
	*_free_clusters=allocation.free_clusters;
	return true;
}

bool localDrive::FileExists(const char* name) {
	char newname[CROSS_LEN];
	strcpy(newname,basedir);
	strcat(newname,name);
	CROSS_FILENAME(newname);
	dirCache.ExpandName(newname);
	//--Modified 2012-04-27 by Alun Bestor to wrap local file operations
	//FILE* Temp=fopen(newname,"rb"); //No reading done, so no wrapping for 0.74-3 (later code uses different calls)
	//if(Temp==NULL) return false;
	//fclose(Temp);
	//return true;
	return boxer_localFileExists(newname, this);
	//--End of modifications
}

bool localDrive::FileStat(const char* name, FileStat_Block * const stat_block) {
	char newname[CROSS_LEN];
	strcpy(newname,basedir);
	strcat(newname,name);
	CROSS_FILENAME(newname);
	dirCache.ExpandName(newname);
	struct stat temp_stat;

	//--Modified 2012-04-27 by Alun Bestor to wrap local file operations
	//if(stat(newname,&temp_stat)!=0) return false;
    if (!boxer_getLocalPathStats(newname, this, &temp_stat)) return false;
    //--End of modifications

	/* Convert the stat to a FileStat */
	struct tm *time;
	if((time=localtime(&temp_stat.st_mtime))!=0) {
		stat_block->time=DOS_PackTime((Bit16u)time->tm_hour,(Bit16u)time->tm_min,(Bit16u)time->tm_sec);
		stat_block->date=DOS_PackDate((Bit16u)(time->tm_year+1900),(Bit16u)(time->tm_mon+1),(Bit16u)time->tm_mday);
	} else {

	}
	stat_block->size=(Bit32u)temp_stat.st_size;
	return true;
}


Bit8u localDrive::GetMediaByte(void) {
	return allocation.mediaid;
}

bool localDrive::isRemote(void) {
	return false;
}

bool localDrive::isRemovable(void) {
	return false;
}

Bits localDrive::UnMount(void) {
	delete this;
	return 0;
}

/* helper functions for drive cache */
//--Modified 2012-07-25 by Alun Bestor to wrap local filesystem access
void *localDrive::opendir(const char *name) {
	// return open_directory(name);
	return boxer_openLocalDirectory(name, this);
}

void localDrive::closedir(void *handle) {
	// close_directory((dir_information*)handle);
	boxer_closeLocalDirectory(handle);
}

bool localDrive::read_directory_first(void *handle, char* entry_name, bool& is_directory) {
	// return ::read_directory_first((dir_information*)handle, entry_name, is_directory);
	return boxer_getNextDirectoryEntry(handle, entry_name, is_directory);
}

bool localDrive::read_directory_next(void *handle, char* entry_name, bool& is_directory) {
	// return ::read_directory_next((dir_information*)handle, entry_name, is_directory);
	return boxer_getNextDirectoryEntry(handle, entry_name, is_directory);
}
//--End of modifications

localDrive::localDrive(const char * startdir,Bit16u _bytes_sector,Bit8u _sectors_cluster,Bit16u _total_clusters,Bit16u _free_clusters,Bit8u _mediaid) {
	strcpy(basedir,startdir);
	sprintf(info,"local directory %s",startdir);
	allocation.bytes_sector=_bytes_sector;
	allocation.sectors_cluster=_sectors_cluster;
	allocation.total_clusters=_total_clusters;
	allocation.free_clusters=_free_clusters;
	allocation.mediaid=_mediaid;

	//--Added 2009-10-25 by Alun Bestor to allow Boxer to track the system path for DOSBox drives
	strcpy(systempath, startdir);
	//--End of modifications

	dirCache.SetBaseDir(basedir,this);
}


// ********************************************
// CDROM DRIVE
// ********************************************

int  MSCDEX_RemoveDrive(char driveLetter);
int  MSCDEX_AddDrive(char driveLetter, const char* physicalPath, Bit8u& subUnit);
bool MSCDEX_HasMediaChanged(Bit8u subUnit);
bool MSCDEX_GetVolumeName(Bit8u subUnit, char* name);


cdromDrive::cdromDrive(const char letter, const char * startdir,Bit16u _bytes_sector,Bit8u _sectors_cluster,Bit16u _total_clusters,Bit16u _free_clusters,Bit8u _mediaid, int& error)
		   :localDrive(startdir,_bytes_sector,_sectors_cluster,_total_clusters,_free_clusters,_mediaid),
		    subUnit(0),
		    driveLetter('\0')
{
	// Init mscdex
	error = MSCDEX_AddDrive(letter,startdir,subUnit);
	strcpy(info, "CDRom ");
	strcat(info, startdir);
	this->driveLetter = letter;
	// Get Volume Label
	char name[32];
	if (MSCDEX_GetVolumeName(subUnit,name)) dirCache.SetLabel(name,true,true);
}

bool cdromDrive::FileOpen(DOS_File * * file,const char * name,Bit32u flags) {
	if ((flags&0xf)==OPEN_READWRITE) {
		flags &= ~OPEN_READWRITE;
	} else if ((flags&0xf)==OPEN_WRITE) {
		DOS_SetError(DOSERR_ACCESS_DENIED);
		return false;
	}
	bool retcode = localDrive::FileOpen(file,name,flags);
	if(retcode) (dynamic_cast<GBoxer::LocalFile*>(*file))->FlagReadOnlyMedium();
	return retcode;
}

bool cdromDrive::FileCreate(DOS_File * * /*file*/,const char * /*name*/,Bit16u /*attributes*/) {
	DOS_SetError(DOSERR_ACCESS_DENIED);
	return false;
}

bool cdromDrive::FileUnlink(const char * /*name*/) {
	DOS_SetError(DOSERR_ACCESS_DENIED);
	return false;
}

bool cdromDrive::RemoveDir(const char * /*dir*/) {
	DOS_SetError(DOSERR_ACCESS_DENIED);
	return false;
}

bool cdromDrive::MakeDir(const char * /*dir*/) {
	DOS_SetError(DOSERR_ACCESS_DENIED);
	return false;
}

bool cdromDrive::Rename(const char * /*oldname*/,const char * /*newname*/) {
	DOS_SetError(DOSERR_ACCESS_DENIED);
	return false;
}

bool cdromDrive::GetFileAttr(const char * name,Bit16u * attr) {
	bool result = localDrive::GetFileAttr(name,attr);
	if (result) *attr |= DOS_ATTR_READ_ONLY;
	return result;
}

bool cdromDrive::FindFirst(const char * _dir,DOS_DTA & dta,bool /*fcb_findfirst*/) {
	// If media has changed, reInit drivecache.
	if (MSCDEX_HasMediaChanged(subUnit)) {
		dirCache.EmptyCache();
		// Get Volume Label
		char name[32];
		if (MSCDEX_GetVolumeName(subUnit,name)) dirCache.SetLabel(name,true,true);
	}
	return localDrive::FindFirst(_dir,dta);
}

void cdromDrive::SetDir(const char* path) {
	// If media has changed, reInit drivecache.
	if (MSCDEX_HasMediaChanged(subUnit)) {
		dirCache.EmptyCache();
		// Get Volume Label
		char name[32];
		if (MSCDEX_GetVolumeName(subUnit,name)) dirCache.SetLabel(name,true,true);
	}
	localDrive::SetDir(path);
}

bool cdromDrive::isRemote(void) {
	return true;
}

bool cdromDrive::isRemovable(void) {
	return true;
}

Bits cdromDrive::UnMount(void) {
	if(MSCDEX_RemoveDrive(driveLetter)) {
		delete this;
		return 0;
	}
	return 2;
}
