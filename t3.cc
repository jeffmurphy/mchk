// c++ test: excercise malloc

#include <stdio.h>
#include <stdlib.h>
#include <iostream.h>

typedef struct _listitem listitem;
struct _listitem {
  char     *text;
  listitem *next;
};

class filecontents {
 private:
  struct listitem *contents;
 public:
  filecontents() { contents = NULL };
  ~filecontents();
  filecontents(FILE *fp);

  friend ostream &operator<<(ostream &, filecontents &);
};

filecontents::filecontents(FILE *fp)
{
  char word[1024];
  listitem *c = contents;
  if(c) deleteContents();
  while(fscanf(fp, "%s", word) != EOF) {
    
  }
}

void
filecontents::deletecontents()
{
}

ostream &
operator<<(ostream &stream, filecontents &that)
{
  for(listitem *n = that.contents ; n ; n = n->next) {
    if(n->text) 
      stream << n->text;
    else
      stream << endl;
  }
}

void
readfile(FILE *fp)
{
}

int
main ()
{
  char f[1024];
  cout << "enter a filename: ";
  f << cin;

  FILE *fp = fopen(f, "r");
  filecontents fc(fp);
  fclose(fp);

  cout << fc << endl;

  delete fc;
}
