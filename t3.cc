// c++ test: excercise malloc

#include <stdio.h>
#include <stdlib.h>
#include <iostream.h>
#include <unistd.h>
#include <string.h>

typedef struct _listitem listitem;
struct _listitem {
  char     *text;
  listitem *next;
};

class filecontents {
 private:
  struct listitem *contents;
  unsigned int wcount;
  void deleteContents();

 public:
  filecontents() { contents = NULL; wcount = 0; };
  ~filecontents() { deleteContents(); };
  filecontents(FILE *fp);

  friend ostream &operator<<(ostream &, filecontents *);
};

filecontents::filecontents(FILE *fp)
{
  char word[1024];
  contents = NULL;
  wcount = 0;

  listitem *c = contents;
  while(fscanf(fp, "%s", word) != EOF) {
    wcount++;

    int l = strlen(word);
    listitem *n = new listitem;
    n->text = NULL;
    n->next = NULL;
    if(l > 0) {
      n->text = new char[l+1];
      for(char *x = n->text ; (x - (n->text) < l) ; x++) {
	*x = 0;
      }
      memset(n->text, 0, l+1);
      strncpy(n->text, word, l);
    }
    if(!c) { 
      contents = c = n;
    } else {
      c->next = n;
      c = n;
    }
  }
}

void
filecontents::deleteContents()
{
  for(listitem *c = contents ; c ; ) {
    listitem *cn = c->next;
    delete[] c->text;
    delete c;
    c = cn;
  }
}

ostream &
operator<<(ostream &stream, filecontents *that)
{
  int level = 0;
  for(listitem *n = that->contents ; n ; n = n->next) {
    if(n->text) {
      int l   = strlen(n->text);
      bool nl = false;

      switch(*(n->text + l - 1)) {
      case '{':
	level++;
	nl = true;
	break;
      case '}':
	level--;
	nl = true;
	break;
      case ';':
	if(n->text[l-2] == '}') level--;
      case ':':
	nl = true;
	break;
      case '<':
      case '>':
	if(*(n->text) == '#')
	  nl = true;
	else 
	  nl = false;
	break;
      default:
	nl = false;
      }

      stream << n->text << " ";

      if(nl == true) {
	stream << "\n";
	for(int i = 0 ; i < level ; i++)
	  cout << "\t";
      }

    } else
      stream << endl;
  }
  return stream;
}

int
main ()
{
  char f[1024];
  cout << "enter a filename: ";
  cin >> f;

  cout << "\nopening \"" << f << "\"" << endl;
  FILE *fp = fopen(f, "r");
  if(!fp) {
    perror("fopen");
    exit(-1);
  }

  filecontents *fc = new filecontents(fp);
  fclose(fp);

  cout << fc << endl;

  delete fc;

  cout << "fc deleted.." << endl;

  return(0);
}
