#include "tzdump.h"

#define DEFAULT_PATH "/usr/share/zoneinfo/"

bool usage()
{
  printf("\n"
         "usage: tz_parser [path | -p <absolute path>] \n"
         "  -h               : help\n"
         "  -p [abs path]    : absolute path.\n"
         "Example:\n"
         "  tz_parser Asia/Taipei\n"
         "  tz_parser -p /usr/share/zoneinfo/Asia/Taipei\n");
  return false;
}

int main(int argc, char *argv[])
{
  int ch;
  char *filepath = NULL;
  while ((ch = getopt(argc, argv, "hp:")) != -1)
  {
    switch (ch)
    {
    case 'h':
      usage();
      goto main_exit;
    case 'p':
      filepath = malloc(256 * sizeof(char));
      strcpy(filepath, optarg);
      break;
    default:
      usage();
      goto main_exit;
    }
  }

  if (filepath == NULL)
  {
    if (argc == 2)
    {
      filepath = calloc(256, sizeof(char));
      strcpy(filepath, DEFAULT_PATH);
      strcat(filepath, argv[1]);
    }
    else
    {
      usage();
      goto main_exit;
    }
  }

  struct tm *dst_start; // DST start time
  struct tm *dst_end;   // DST end time
  char *result = ZyTimeZone(filepath, &dst_start, &dst_end);

  if (result != NULL && strcmp(result, "") != 0)
    printf("%s\n", result);

main_exit:
  return 0;
}