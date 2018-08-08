/*
    touchwho is a program that monitors subfolder files in a folder
    Copyright (C) 2016  Wei Shuai <cpuwolf@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <sys/stat.h>

#define TRUE 1
#define FALSE 0
#define BUF_LEN 20480

static unsigned int is_exit = FALSE;
static unsigned int total_num_files = 0;
static unsigned int total_num_symlnk = 0;
static unsigned int idx_file = 0;
static unsigned int idx_mfile = 0;
static unsigned int idx_symlnk = 0;
static unsigned int touched_file = 0;
static unsigned int ifd;

struct wd_name {
	int wd;
	ino_t inode;
	char * name;
};

struct sym_name {
	ino_t inode;
	char * name;
};

static struct wd_name *monitor_list = NULL;
static int *wd_list = NULL;

static struct sym_name *symlnk_list = NULL;

typedef int (*fnmonhdl)(int, const char *);

static const char banner[] = {
  0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
  0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
  0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x5f, 0x20, 0x20, 0x5f,
  0x5f, 0x20, 0x0a, 0x20, 0x20, 0x5f, 0x5f, 0x5f, 0x20, 0x5f, 0x20, 0x5f,
  0x5f, 0x20, 0x20, 0x5f, 0x20, 0x20, 0x20, 0x5f, 0x5f, 0x5f, 0x20, 0x20,
  0x20, 0x20, 0x20, 0x20, 0x5f, 0x5f, 0x5f, 0x5f, 0x5f, 0x20, 0x7c, 0x20,
  0x7c, 0x2f, 0x20, 0x5f, 0x7c, 0x0a, 0x20, 0x2f, 0x20, 0x5f, 0x5f, 0x7c,
  0x20, 0x27, 0x5f, 0x20, 0x5c, 0x7c, 0x20, 0x7c, 0x20, 0x7c, 0x20, 0x5c,
  0x20, 0x5c, 0x20, 0x2f, 0x5c, 0x20, 0x2f, 0x20, 0x2f, 0x20, 0x5f, 0x20,
  0x5c, 0x7c, 0x20, 0x7c, 0x20, 0x7c, 0x5f, 0x20, 0x0a, 0x7c, 0x20, 0x28,
  0x5f, 0x5f, 0x7c, 0x20, 0x7c, 0x5f, 0x29, 0x20, 0x7c, 0x20, 0x7c, 0x5f,
  0x7c, 0x20, 0x7c, 0x5c, 0x20, 0x56, 0x20, 0x20, 0x56, 0x20, 0x2f, 0x20,
  0x28, 0x5f, 0x29, 0x20, 0x7c, 0x20, 0x7c, 0x20, 0x20, 0x5f, 0x7c, 0x0a,
  0x20, 0x5c, 0x5f, 0x5f, 0x5f, 0x7c, 0x20, 0x2e, 0x5f, 0x5f, 0x2f, 0x20,
  0x5c, 0x5f, 0x5f, 0x2c, 0x5f, 0x7c, 0x20, 0x5c, 0x5f, 0x2f, 0x5c, 0x5f,
  0x2f, 0x20, 0x5c, 0x5f, 0x5f, 0x5f, 0x2f, 0x7c, 0x5f, 0x7c, 0x5f, 0x7c,
  0x20, 0x20, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x7c, 0x5f, 0x7c, 0x20,
  0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
  0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
  0x20, 0x20, 0x20, 0x20, 0x20, 0x0a, 0x0a, 0x00
};

static void percentage(int idx, int total)
{
	static int percent = 0;
	int p, i;

	p = idx * 10 / total;
	if(p != percent) {
		percent = p;
	} else { return; }
	
	fprintf(stderr, "|");
	for(i=0; i< p; i++)
		fprintf(stderr, "=");
	for(i=0; i< (10-p); i++)
		fprintf(stderr, "-");
	fprintf(stderr, "| %d%%\r", p*10);
}

static void progress(int idx)
{
	const char pic[]={'-','\\','|','/'};
	fprintf(stderr, "%c\r", pic[idx%4]);
}

void create_file_name(char * fname)
{
        time_t tt;
        struct tm *t;
        char timestr[100];

        tt = time(NULL);
        t = localtime(&tt);
        sprintf(timestr, "touchwho_filelist_%04d_%02d_%02d_%02d%02d%02d.txt", t->tm_year+1900, t->tm_mon+1, t->tm_mday,
                t->tm_hour, t->tm_min, t->tm_sec);
        strcpy(fname, timestr);
}

static int monitor_init()
{
	int fd;

	fd = inotify_init();
	if (fd == -1) {
		perror("inotify_init");
		exit(EXIT_FAILURE);
	}
	return fd;
}

static int monitor_add(int fd, const char *ename)
{
	int wd;
	
	if((fd < 0) || (ename == NULL)) {
		perror("monitor_add");
		exit(EXIT_FAILURE);
	}

	wd = inotify_add_watch(fd, ename, IN_OPEN|IN_DONT_FOLLOW|IN_ONESHOT);
	if (wd < 0) {
		perror("inotify_add_watch");
		switch(errno) {
	 		case EACCES:
				break;
			case ENOSPC:
				fprintf(stderr, "\nplease select a number xx greater than %d:\necho xx > /proc/sys/fs/inotify/max_user_watches\n", total_num_files);
			default:
				exit(EXIT_FAILURE);
		}
	}
	monitor_list[idx_mfile].wd = wd;
	idx_mfile++;
	return wd;
}


static void monitor(int fd)
{
	char buf[BUF_LEN];
	ssize_t len, i = 0;

	while(!is_exit) {
		len = read(fd, buf, BUF_LEN);
		while (i < len) {
			struct inotify_event *event = (struct inotify_event *)&buf[i];

			if (event->mask & IN_OPEN)  {
#if 0
				printf ("wd=%d mask=0x%x cookie=%d len=%d dir=%s\n",
					event->wd, event->mask, event->cookie, event->len,
					(event->mask & IN_ISDIR) ? "yes" : "no");

				/* if there is a name, print it */
				if (event->len)
					printf ("name=%s\n", event->name);
#else
				//printf(".");
				progress(touched_file);
#endif
				wd_list[touched_file] = event->wd;
				touched_file++;
				inotify_rm_watch(fd, event->wd);
			}
			/* update the index to the start of the next event */
			i += (offsetof(struct inotify_event, name) + event->len);
		}
		i = 0;
	}
}

static ino_t symlnkpath(const char * path, const char * parentpath)
{
	char rlpath[PATH_MAX];
	struct stat estat;
	if(!realpath(path, rlpath))
	{
		/* skip non-existing path */
		goto sys_error;
	}
	
	if(lstat(rlpath, &estat) == 0) {
		return estat.st_ino;
	}
sys_error:
	return 0;
}
		
static void traveldir(const char * dirname, int bcountonly)
{
	DIR * d;
	int path_len;
	char path[PATH_MAX];

	d = opendir(dirname);
	if(!d) {
		fprintf(stderr, "cannot open dir %s\n", dirname);
		return;
	}

	while(!is_exit) {
		struct dirent * entry;
		const char * ename;
		entry = readdir(d);
		if(!entry) {
			/* no more entries*/
			break;
		}
		ename = entry->d_name;

		if(entry->d_type & DT_DIR) {
			if((strcmp(ename, "..")!= 0) &&
			strncmp(ename, ".", 1) != 0) {
				path_len = snprintf(path, PATH_MAX, "%s/%s", dirname, ename);
				//printf("%s\n", path);
				if(path_len >= PATH_MAX) {
					fprintf(stderr, "dir path length is too long\n");
					exit(EXIT_FAILURE);
				}
				traveldir(path, bcountonly);
			}
		} else {
			struct stat estat;	
			path_len = snprintf(path, PATH_MAX, "%s/%s", dirname, ename);
			if(path_len >= PATH_MAX) {
				fprintf(stderr, "file path length is too long\n");
				exit(EXIT_FAILURE);
			}
#if 1
			/* symbol link */
			if(lstat(path, &estat) == 0) {
				if(S_ISLNK(estat.st_mode)) {
					if(bcountonly) {
						//printf("=>%s 0x%x\n", path, entry->d_type);
						total_num_symlnk ++;
					} else {
						symlnk_list[idx_symlnk].inode = symlnkpath(path, dirname);
						symlnk_list[idx_symlnk].name = malloc(path_len+2);
						strcpy(symlnk_list[idx_symlnk].name, path);
						//printf("=>%s %d\n", path, symlnk_list[idx_symlnk].inode);
						//total_num_symlnk ++;
						idx_symlnk++;
					}
					continue;
				}
			} else {
				fprintf(stderr, "cannot lstat %s\n", path);
				exit(EXIT_FAILURE);
			}
			/* symbol link */
#endif
			if(bcountonly) {
				total_num_files ++;
			} else {
				monitor_list[idx_file].inode = entry->d_ino;
				monitor_list[idx_file].name = malloc(path_len+2);
				strcpy(monitor_list[idx_file].name, path);
				//printf("%s\n", monitor_list[idx_file].name);
				idx_file++;
				progress(idx_file);
			}
			
		}
	}
	if(closedir(d)) {
		fprintf(stderr, "cannot close %s\n", dirname);
		exit(EXIT_FAILURE);
	}
}

void sig_handler(int sig)
{
	if(sig == SIGINT) {
		is_exit = TRUE;
		close(ifd);
	}
}

int main(int argc, char * argv[])
{
	int i,j,k;
	char ofilename[512];
	FILE *ofile;
	char buf[1024];
	char * scandir;
	struct stat estat;
	
	printf("%s", banner);
	printf("Wei Shuai <cpuwolf@gmail.com> (C) 2016-2018\n");
	printf("Touchwho 2.1\nis a Folder Monitoring program, help you find out who has been touched\n\n");

	if(argc <=1) {
		printf("Usage: Touchwho [folder path]\n\n");
		return -1;
	}

	create_file_name(ofilename);
	ofile = fopen(ofilename, "wb+");
	if(!ofile) {
		fprintf(stderr, "cannot create log file %s\n", ofilename);	
		exit(EXIT_FAILURE);
	}

	scandir = argv[1];
	
	if(lstat(scandir, &estat) == 0) {
		if(!S_ISDIR(estat.st_mode)) {
			fprintf(stderr, "[%s] is not a folder\n", scandir);	
			exit(EXIT_FAILURE);
		}
	}

	printf("Please wait! I have to scan folder: [%s]\n", scandir);

	signal(SIGINT, sig_handler);

	traveldir(scandir, 1);

	printf("-- found %u files\n", total_num_files);
	printf("-- found %u symbol links\n", total_num_symlnk);
	monitor_list = (struct wd_name *)malloc(total_num_files * sizeof(struct wd_name));
	if( ! monitor_list) {
		fprintf(stderr, "no memory left\n");
		goto clean_up;
	}
	wd_list = (int *)malloc(total_num_files * sizeof(int));

	symlnk_list = (struct sym_name *)malloc(total_num_symlnk * sizeof(struct sym_name));
	if( ! symlnk_list) {
		fprintf(stderr, "no memory left for symbol links\n");
		goto clean_up;
	}
	
	ifd = monitor_init();
	traveldir(scandir, 0);
	for(j=0; j<total_num_files; j++) {
		monitor_add(ifd, monitor_list[j].name);
	}
	/*debug*/
	if(idx_file != total_num_files) {
		fprintf(stderr, "\nmonitored %d files != found %d\n", idx_file, total_num_files);
	}

	printf("\nI'm watching folder [%s], You can start now. press ctrl+c to exit\n", scandir);
	monitor(ifd);

	printf("\nI found total %u/%u files are touched by some programs\n", touched_file, total_num_files);

	printf("\nI'm writing to file %s\n", ofilename);

	for(i=0; (i<touched_file); i++) {
		for(j=0; j<total_num_files; j++) {
			if(wd_list[i] == monitor_list[j].wd) {
				for(k=0; k<total_num_symlnk; k++) {
					if(symlnk_list[k].inode == monitor_list[j].inode) {
						sprintf(buf, "%s\n", symlnk_list[k].name);
						fwrite(buf, 1, strlen(buf), ofile);
					}
				}
				sprintf(buf, "%s\n", monitor_list[j].name);
				//printf("[%d] %s\n", i, monitor_list[j].name);
				percentage(i+1, touched_file);
				fwrite(buf, 1, strlen(buf), ofile);
				break;
			}
		}
		fflush(ofile);
	}
	printf("\nplease check file %s\n", ofilename);

clean_up:
	fclose(ofile);
	/* free all memory */
	for(i=0; i< idx_file; i++) {
		free(monitor_list[i].name);
	}
	free(monitor_list);
	free(wd_list);
	for(i=0; i< idx_symlnk; i++) {
		free(symlnk_list[i].name);
	}
	free(symlnk_list);
	close(ifd);
	return 0;
}


