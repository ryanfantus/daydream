/* arc.c */
int Arc(void);
char FlavToChar(int flo, Flav Flavour);
char *GetBaseName(FidoAddr *dest);
char *GetDirName(FidoAddr *dest);
void GetPktName(char *pkt, FidoAddr *dest, Flav Flavour, int netmail);
void GetFloName(char *buf, FidoAddr *dest, Flav Flavour);
void AppendToFlo(char *floname, char *file);
/* areafix.c */
int AreafixHelp(bsList *text);
int AreafixList(FidoAddr *addr, bsList *text);
int AreafixQuery(FidoAddr *addr, bsList *text);
int AreafixUnsub(FidoAddr *addr, char *sarea, bsList *text);
int AreafixSub(FidoAddr *addr, char *sarea, bsList *text);
int ProcessAreafixText(MEM_MSG *mem, bsList *text);
void UpdateCfg(void);
int createArea(ForwardList* fl, char* sarea);
int forwardRequest(FidoAddr* Aka, FidoAddr* ourAka, char* area); 
void addHeaders(MEM_MSG *mem, bsList *text);
void SendAreafixResponse(MEM_MSG *mem, MEM_MSG *result_mem, bsList *text);
int ProcessAreafix(MEM_MSG *mem, MEM_MSG *result_mem);
int FindInForwardList(char* area, ForwardList* fl);
/* bsList.c */
void bsList_add(bsList *list, void *item);
void bsList_init(bsList *list);
void bsList_free(bsList *list);
void bsList_freeFromOffset(bsList *list, bsListItem *item);
void bsList_delete(bsList *list, bsListItem *item);
void bsList_insertAfter(bsList *list, bsListItem *item, void *val);
void bsList_insertBefore(bsList *list, bsListItem *item, void *val);
/* cfg.c */
void StripCrLf(char *text);
void FreeCfg(MainCfg *cfg);
FidoAddr *FindMyAka(FidoAddr *my_aka, FidoAddr *dest);
Export *GetExport(Area *area, FidoAddr *aka);
Area *GetArea(char *area);
Area *FindAreaByType(Area_Type Type, FidoAddr *aka);
Password *GetPassword(FidoAddr *aka);
Packer *GetPacker(FidoAddr *dest);
int DestIsHere(FidoAddr *aka);
MainCfg *ReadCfg(MainCfg *cfg, char *file);
int ParseCfgLine(MainCfg *cfg, char *buf);
/* date.c */
struct tm *StrToTm(char *buf, struct tm *st);
char *TmToStr(struct tm *st, char *buf);
/* dupe.c */
bool CheckDupe(MEM_MSG *mem);
int AddDupe(MEM_MSG *mem);
/* export.c */
void RandomFilename(char *buf, char *ext);
void WritePkt(MEM_MSG *mem, FidoAddr *dest, Flav Flavour);
void AddVia(MEM_MSG *mem);
void AddTid(MEM_MSG *mem);
void GateRoute(MEM_MSG *mem);
int RouteMail(MEM_MSG *mem);
int ExportMsg(MEM_MSG *mem);
/* fidoaddr.c */
FidoAddr *text_to_fidoaddr(char *text, FidoAddr *aka);
char *fidoaddr_to_text(FidoAddr *aka, char *text);
int fidoaddr_compare(FidoAddr *aka1, FidoAddr *aka2);
int fidoaddr_empty(FidoAddr *aka);
/* ftscprod.c */
/* log.c */
void BeginLog(char *file);
void EndLog(void);
void logit(bool print, char *format, ...);
/* mb_dd.c */
int lock_file(FILE *fh, int flags);
int unlock_file(FILE *fh);
int dd_GetMsgPtrs(DD_Area *a);
int dd_SetMsgPtrs(DD_Area *a);
DD_Area *dd_OpenArea(char *area, char *path, int basenum);
void dd_CloseArea(DD_Area *a);
int dd_SeekMsg(DD_Area *a, int num);
int dd_RealReadMsg(DD_Area *a, int num, MEM_MSG *mem);
int dd_RealWriteMsg(DD_Area *a, int num, MEM_MSG *mem, int write_msg);
/* mb_msg.c */
void strtolower(char *str);
int msg_HighMsgNum(char *netdir);
void msg_WriteMsg(FILE *fh, MEM_MSG *mem);
/* patmat.c */
int str_tolower(char *str);
int patmat(const char *string, const char *pattern);
int patimat(char *string, char *pattern);
/* pkt.c */
int ReadPktHeader(FILE *fh, MEM_MSG *mem);
int WritePktHeader(FILE *fh, MEM_MSG *mem);
void DumpMemMsg(MEM_MSG *mem);
void CopyMemMsg(MEM_MSG *source, MEM_MSG *dest);
void FreeMemMsg(MEM_MSG *mem);
int NullRead(FILE *fh, char *buf, int size);
int CRRead(FILE *fh, char *buf, int size);
int AttrToFlags(int ftn_attr);
int FlagsToAttr(int flags);
int ReadPktMsg(FILE *fh, MEM_MSG *mem);
int WritePktMsg(FILE *fh, MEM_MSG *mem);
/* seenby.c */
void GetSeenbyList(MEM_MSG* mem, bsList* seenbys);
void SortSeenbyList(bsList* seenbys);
void Node2dList(bsList *list, char *text);
void Node2dText(bsList *text, bsList *list, char *prefix);
int Node2dExists(bsList *list, FidoAddr *addr);
void AddSeenbyPath(MEM_MSG *mem, bsList *links);
/* stats.c */
void AddToStat(char *area);
void PrintStats(void);
/* tosser.c */
char *FindProduct(unsigned int prod);
int CheckPktHeader(MEM_MSG *mem);
Area_Type CheckPktMsg(MEM_MSG *mem, int type);
void WriteNewHeaders(MEM_MSG *mem);
void PrepareMsg(MEM_MSG *mem);
int toss_file(char *fn, int type);
int afix_scan(void);
int scan(int echo);
int toss_from_bad(void);
void SetEnv(char *indir);
int toss_dir(char *indir, int type);
int toss(void);
int main(int argc, char *argv[]);
/* crc.c */
unsigned int StringCRC32(char *str);

