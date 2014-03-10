typedef struct ArchList_ {
	char Filename[64];
	FidoAddr Source;
	FidoAddr Dest;
	Packer* Packer;
	Flav Flavour;
} ArchList;

typedef struct ftscprod_
{
	unsigned int adr;
	char * name;
} ftscprod;

extern struct ftscprod_ products[];

