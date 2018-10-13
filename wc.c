#include <ctype.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

char *progname;

int cflag = 0;
int lflag = 0;
int wflag = 0;

void
perrorf(char *fmt, ...)
{
  va_list ap;
  int olderrno = errno;

  va_start(ap, fmt);

  vfprintf(stderr, fmt, ap);
  fprintf(stderr, ": %s\n", strerror(olderrno));

  va_end(ap);
}

void
report(char *path, int bytes, int lines, int words)
{
  if (lflag) {
    printf("%8d", lines);
  }

  if (wflag) {
    printf(" %7d", words);
  }

  if (cflag) {
    printf(" %7d", bytes);
  }

  printf(" %s\n", path);
}

void
wc(char *path)
{
  int fd, ret;
  int bytes = 0, lines = 0, words = 0;
  char *p;
  struct stat st;

  fd = open(path, O_RDONLY);
  if (fd == -1) {
    perrorf("%s: %s: open", progname, path);
    exit(1);
  }

  ret = fstat(fd, &st);
  if (ret == -1) {
    perrorf("%s: %s: fstat", progname, path);
    exit(1);
  }

  bytes = st.st_size;

  if (!wflag && !lflag) {
    report(path, bytes, lines, words);
    goto done;
  }

  p = mmap(0, bytes, PROT_READ, MAP_SHARED, fd, 0);
  if (p == MAP_FAILED) {
    perrorf("%s: %s: mmap", progname, path);
    exit(1);
  }


  for (int i = 0; i < bytes; i++) {
    if (p[i] == '\n' || p[i] == '\r') {
      lines++;
    }

    if (isspace(p[i]) && i != 0 && !isspace(p[i-1])) {
      words++;
    }
  }


  ret = munmap(p, bytes);
  if (ret == -1) {
    perrorf("%s: %s: munmap", progname, path);
    exit(1);
  }

  report(path, bytes, lines, words);

done:
  ret = close(fd);
  if (ret == -1) {
    perrorf("%s: %s: close", progname, path);
    exit(1);
  }
}

void
usage(void)
{
  fprintf(stderr, "usage: %s [-clw] [file ...]\n", progname);
  exit(1);
}

int
main(int argc, char *argv[])
{
  int ch;

  progname = argv[0];

  while ((ch = getopt(argc, argv, "clw")) != -1) {
    switch(ch) {
    case 'c':
      cflag = 1;
      break;
    case 'l':
      lflag = 1;
      break;
    case 'w':
      wflag = 1;
      break;
    case '?':
    default:
      usage();
    }
  }

  if (!cflag && !lflag && !wflag) {
    cflag = 1;
    lflag = 1;
    wflag = 1;
  }

  if (argc == optind) {
    usage();
  }

  for (int i = optind; i < argc; i++) {
    wc(argv[i]);
  }

  return 0;
}
