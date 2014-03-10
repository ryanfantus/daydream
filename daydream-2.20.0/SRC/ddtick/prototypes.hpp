/* announce.c++ */
void announce();

/* funcs.c++ */
unsigned int HexStringToInt(string s);
string IntToHexString(unsigned int a);
unsigned int StringToInt(string s);
string IntToString(unsigned int a);
void SplitLine(string s, vector<string>& l, char delim = ' ', int times = -1);
string LowerStr(string s);
void RemoveCrLf(string& s);
string Center(string s);

/* patmat.c++ */
int patmat( const char *string, const char *pattern );
int patimat( char *string, char *pattern );

/* toss.c++ */
void hatch(string file, string area);
void toss();

/* request.c++ */
void process_request(FidoAddr aka, string request, string filelist, string reply, bool srif = false);
