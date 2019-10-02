#include <stdio.h>

void read()
{
  double val;
  char fname[] = "dbls.bin";
  FILE *file = fopen(fname, "rb");
  fread(&val, sizeof(val), 1, file);
  printf("Read %lf from file %s.\n", val, fname);
  fclose(file);
}

void write()
{
  double val;
  char fname[] = "dbls.bin";
  FILE *file = fopen(fname, "wb");
  scanf("%lf", &val);
  printf("Writing %f to %s\n", val, fname);
  fwrite(&val, sizeof(val), 1, file);
  fclose(file);
}

int main(int argc, char *argv[])
{
  read();
  write();
  return 0;
}
