#include "file.h"
#include "util.h"

#include <asm-generic/errno-base.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/cdefs.h>
#include <sys/sendfile.h>
#include <sys/stat.h>

#include <linux/limits.h>

void get_default_file_name(char *default_file_name, Args *args) {
  time_t rawtime = time(NULL);
  struct tm *tm = localtime(&rawtime);
  strftime(default_file_name, FILENAME_MAX, "evid%Y%m%d%H%M%S", tm);
  if (args->gif) {
    strcat(default_file_name, ".gif");
  } else {
    strcat(default_file_name, ".mp4");
  }
}

int get_tmp_file(char *tmp_file, size_t tmp_file_size, Args *args) {
  char *tmp_dir = getenv("TMPDIR");
  if (tmp_dir == NULL) {
    tmp_dir = "/tmp";
  }

  char default_file_name[FILENAME_MAX];
  get_default_file_name(default_file_name, args);
  return snprintf(tmp_file, tmp_file_size, "%s/%s", tmp_dir, default_file_name);
}

int get_output_file(char *new_file, size_t new_file_size, Args *args) {
  char default_file_name[FILENAME_MAX];
  get_default_file_name(default_file_name, args);

  if (args->output) {
    int output_fd = open(args->output, O_RDONLY);
    if (output_fd == -1 && errno != ENOENT) {
      close(output_fd);
      die("fatal error obtaining file status, failed with "
          "error: %s\n",
          strerror(errno));
    }
    struct stat st;
    if (fstat(output_fd, &st) != -1 && S_ISDIR(st.st_mode)) {
      return snprintf(new_file, new_file_size, "%s/%s", args->output,
                      default_file_name);
    }
    return snprintf(new_file, new_file_size, "%s", args->output);
  } else {
    FILE *pipep = NULL;
#ifdef HAVE_ZENITY
    if (args->use_zenity) {
      char zenity[63 + PATH_MAX];
      snprintf(zenity, 63 + PATH_MAX,
               "zenity --file-selection --save --confirm-overwrite "
               "--filename=%s",
               default_file_name);
      // Try to open a file save dialog with zenity
      pipep = popen(zenity, "r");
    }
#endif
    if (pipep) {
      fgets(new_file, PATH_MAX, pipep);
      new_file[strcspn(new_file, "\n")] = 0;
      pclose(pipep);
      return 0;
    } else {
      const char *xdg_videos_folder = getenv("XDG_VIDEOS_DIR");
      if (xdg_videos_folder) {
        return snprintf(new_file, new_file_size, "%s/%s", xdg_videos_folder,
                        default_file_name);
      } else {
        const char *hfolder = getenv("HOME");
        if (hfolder) {
          if (snprintf(new_file, new_file_size, "%s/%s/%s/%s", hfolder,
                       "Videos", PROGRAM_NAME, default_file_name) < 0) {
            return -1;
          }
          if (mkdirp(new_file) == -1) {
            die("couldn't create directory %s, failed with error: %s\n",
                new_file, strerror(errno));
          }
          return 0;
        }
      }
    }
  }
  return -1;
}

int remove_file(const char *file) {
  int rm = remove(file);
  if (rm) {
    if (errno != ENOENT) {
      error("there was an error deleting the file %s\n", file);
    }
  }
  return rm;
}

void move_file(const char *source_file, const char *dest_file) {
  int mv = rename(source_file, dest_file);
  if (mv) {
    if (errno == EXDEV) {
      int oldfilefd = open(source_file, O_RDONLY);
      if (oldfilefd == -1) {
        die("couldn't open %s, failed with error: %s\n", source_file,
            strerror(errno));
      }
      int newfilefd = open(dest_file, O_WRONLY | O_TRUNC | O_CREAT, 0755);
      if (newfilefd == -1) {
        close(oldfilefd);
        die("couldn't open %s, failed with error: %s\n", dest_file,
            strerror(errno));
      }
      struct stat st;
      if (fstat(oldfilefd, &st) == -1) {
        close(oldfilefd);
        close(newfilefd);
        die("fatal error obtaining file status, failed with "
            "error: "
            "%s\n",
            strerror(errno));
      }
      int sizeleft = st.st_size;
      while (sizeleft > 0) {
        int bytes = sendfile(newfilefd, oldfilefd, NULL, 64 * 1024 * 1024);
        if (bytes == -1) {
          close(oldfilefd);
          close(newfilefd);
          die("fatal error saving the file, failed with error: "
              "%s\n",
              strerror(errno));
        }
        sizeleft -= bytes;
      }
      close(oldfilefd);
      close(newfilefd);
    }
  }
}

int mkdirp(const char *dir) {
  for (char *p = strchr(dir + 1, '/'); p; p = strchr(p + 1, '/')) {
    *p = '\0';
    if (mkdir(dir, S_IRWXU) == -1) {
      if (errno != EEXIST) {
        *p = '/';
        return -1;
      }
    }
    *p = '/';
  }
  return 0;
}
