typedef struct StoredMsg_                                      
{                    
   unsigned char From[36];                                                                       
   unsigned char To[36];                                                                          
   unsigned char Subject[72];                                                                    
   unsigned char DateTime[20];                                                                         
   unsigned short TimesRead;                                                                     
   unsigned short DestNode;                                                                           
   unsigned short OrigNode;                                                                         
   unsigned short Cost;                                                                            
   unsigned short OrigNet;                                                                           
   unsigned short DestNet;                                                                          
   unsigned short DestZone;                                                                          
   unsigned short OrigZone;                                                                          
   unsigned short DestPoint;                                                                          
   unsigned short OrigPoint;                                                                         
   unsigned short ReplyTo;                                                                         
   unsigned short Attr;                                                                             
   unsigned short NextReply;                                                                          
   /* Text */                                                                                  
} StoredMsg;  
